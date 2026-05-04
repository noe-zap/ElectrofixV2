#include "CleanableSpawnerComponent.h"
#include "CleanableActor.h"
#include "CleanableSpawnPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCleaning, Log, All);

UCleanableSpawnerComponent::UCleanableSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UCleanableSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogCleaning, Verbose, TEXT("[Spawner] BeginPlay on non-authority — skipping spawn loop."));
		return;
	}

	// Pick up any spawn points that began play before us (common when the spawner lives on a
	// streamed/replicated actor like the Store rather than the GameMode).
	if (UWorld* World = GetWorld())
	{
		int32 SweptIn = 0;
		for (TActorIterator<ACleanableSpawnPoint> It(World); It; ++It)
		{
			if (ACleanableSpawnPoint* Point = *It)
			{
				if (!Anchors.Contains(Point))
				{
					Anchors.Add(Point);
					++SweptIn;
				}
			}
		}
		if (SweptIn > 0)
		{
			UE_LOG(LogCleaning, Log, TEXT("[Spawner] Adopted %d existing spawn points on BeginPlay."), SweptIn);
		}
	}

	UE_LOG(LogCleaning, Log, TEXT("[Spawner] BeginPlay on %s (authority). Classes=%d, Anchors=%d, MaxAlive=%d, Interval=[%.1f..%.1f]"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("?"),
		CleanableClasses.Num(), Anchors.Num(), MaxAlive, MinSpawnInterval, MaxSpawnInterval);

	// Detect duplicate spawner components running in the same world — would cause N× spawns.
	if (UWorld* World = GetWorld())
	{
		int32 OtherSpawners = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TArray<UCleanableSpawnerComponent*> Comps;
			It->GetComponents<UCleanableSpawnerComponent>(Comps);
			for (UCleanableSpawnerComponent* C : Comps)
			{
				if (C != nullptr && C != this && C->GetOwnerRole() == ROLE_Authority)
				{
					++OtherSpawners;
					UE_LOG(LogCleaning, Error, TEXT("[Spawner] DUPLICATE: another authority spawner exists on %s. Each one runs its own timer — that is why stains appear faster than your interval."),
						*It->GetName());
				}
			}
		}
		if (OtherSpawners > 0)
		{
			UE_LOG(LogCleaning, Error, TEXT("[Spawner] Total duplicate authority spawners besides me: %d. Remove all but one."),
				OtherSpawners);
		}
	}

	ScheduleNextSpawn();
}

void UCleanableSpawnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void UCleanableSpawnerComponent::RegisterSpawnPoint(ACleanableSpawnPoint* Point)
{
	if (Point == nullptr)
	{
		return;
	}
	Anchors.AddUnique(Point);
	UE_LOG(LogCleaning, Log, TEXT("[Spawner] RegisterSpawnPoint %s — total anchors=%d"),
		*Point->GetName(), Anchors.Num());
}

void UCleanableSpawnerComponent::UnregisterSpawnPoint(ACleanableSpawnPoint* Point)
{
	if (Point == nullptr)
	{
		return;
	}
	Anchors.RemoveAll([Point](const TWeakObjectPtr<ACleanableSpawnPoint>& Weak)
	{
		return !Weak.IsValid() || Weak.Get() == Point;
	});
	UE_LOG(LogCleaning, Log, TEXT("[Spawner] UnregisterSpawnPoint %s — remaining anchors=%d"),
		*Point->GetName(), Anchors.Num());
}

void UCleanableSpawnerComponent::ScheduleNextSpawn()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float Min = FMath::Max(1.f, MinSpawnInterval);
	const float Max = FMath::Max(Min, MaxSpawnInterval);
	const float Delay = FMath::FRandRange(Min, Max);

	UE_LOG(LogCleaning, Log, TEXT("[Spawner] Next spawn in %.2fs (range=[%.1f..%.1f]) on %s"),
		Delay, Min, Max, GetOwner() ? *GetOwner()->GetName() : TEXT("?"));

	// Always clear before re-setting, in case anything else scheduled this handle.
	World->GetTimerManager().ClearTimer(SpawnTimer);

	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UCleanableSpawnerComponent::TrySpawn);
	World->GetTimerManager().SetTimer(SpawnTimer, Delegate, Delay, false);
}

void UCleanableSpawnerComponent::PurgeStale()
{
	Alive.RemoveAll([](const TWeakObjectPtr<ACleanableActor>& Weak)
	{
		return !Weak.IsValid();
	});
	Anchors.RemoveAll([](const TWeakObjectPtr<ACleanableSpawnPoint>& Weak)
	{
		return !Weak.IsValid();
	});
}

bool UCleanableSpawnerComponent::IsAnchorOccupied(const ACleanableSpawnPoint* Anchor) const
{
	return Anchor == nullptr || Anchor->IsOccupied();
}

void UCleanableSpawnerComponent::TrySpawn()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	PurgeStale();

	UE_LOG(LogCleaning, Log, TEXT("[Spawner] TrySpawn — anchors=%d, classes=%d, alive=%d/%d"),
		Anchors.Num(), CleanableClasses.Num(), Alive.Num(), MaxAlive);

	if (CleanableClasses.Num() == 0 || Anchors.Num() == 0)
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Spawner] Skipping: no classes (%d) or no anchors (%d)."),
			CleanableClasses.Num(), Anchors.Num());
		ScheduleNextSpawn();
		return;
	}

	if (Alive.Num() >= MaxAlive)
	{
		UE_LOG(LogCleaning, Verbose, TEXT("[Spawner] Skipping: at MaxAlive (%d)."), MaxAlive);
		ScheduleNextSpawn();
		return;
	}

	// Step 1: collect free anchors.
	TArray<ACleanableSpawnPoint*> FreeAnchors;
	for (const TWeakObjectPtr<ACleanableSpawnPoint>& Weak : Anchors)
	{
		ACleanableSpawnPoint* Anchor = Weak.Get();
		if (Anchor && !Anchor->IsOccupied())
		{
			FreeAnchors.Add(Anchor);
		}
	}

	if (FreeAnchors.Num() == 0)
	{
		UE_LOG(LogCleaning, Verbose, TEXT("[Spawner] Skipping: all anchors occupied."));
		ScheduleNextSpawn();
		return;
	}

	// Step 2: pick a random free anchor.
	ACleanableSpawnPoint* PickedAnchor = FreeAnchors[FMath::RandRange(0, FreeAnchors.Num() - 1)];

	// Step 3: collect classes this anchor accepts (tag-compatible) and pick one at random.
	TArray<TSubclassOf<ACleanableActor>> Compatible;
	for (const TSubclassOf<ACleanableActor>& Class : CleanableClasses)
	{
		if (Class == nullptr)
		{
			continue;
		}
		const ACleanableActor* CDO = Class->GetDefaultObject<ACleanableActor>();
		if (CDO == nullptr)
		{
			continue;
		}
		const bool bAnchorAcceptsAny = PickedAnchor->CleaningTag.IsNone();
		const bool bTagsMatch = PickedAnchor->CleaningTag == CDO->CleaningTag;
		if (bAnchorAcceptsAny || bTagsMatch)
		{
			Compatible.Add(Class);
		}
	}

	if (Compatible.Num() == 0)
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Spawner] Anchor %s (tag='%s') accepts none of the configured classes."),
			*PickedAnchor->GetName(), *PickedAnchor->CleaningTag.ToString());

		static bool bLoggedTagDump = false;
		if (!bLoggedTagDump)
		{
			bLoggedTagDump = true;
			UE_LOG(LogCleaning, Warning, TEXT("[Spawner] --- Tag dump (one-time) ---"));
			for (const TWeakObjectPtr<ACleanableSpawnPoint>& Weak : Anchors)
			{
				if (ACleanableSpawnPoint* A = Weak.Get())
				{
					UE_LOG(LogCleaning, Warning, TEXT("[Spawner]   Anchor %s -> CleaningTag='%s'"),
						*A->GetName(), *A->CleaningTag.ToString());
				}
			}
			for (const TSubclassOf<ACleanableActor>& Class : CleanableClasses)
			{
				if (Class)
				{
					if (const ACleanableActor* CDO = Class->GetDefaultObject<ACleanableActor>())
					{
						UE_LOG(LogCleaning, Warning, TEXT("[Spawner]   Class %s -> CleaningTag='%s'"),
							*Class->GetName(), *CDO->CleaningTag.ToString());
					}
				}
			}
		}

		ScheduleNextSpawn();
		return;
	}

	TSubclassOf<ACleanableActor> PickedClass = Compatible[FMath::RandRange(0, Compatible.Num() - 1)];

	// Step 4: spawn at the anchor (with the class's per-CDO offset).
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FTransform Xform = PickedAnchor->GetActorTransform();
	if (const ACleanableActor* PickedCDO = PickedClass->GetDefaultObject<ACleanableActor>())
	{
		Xform.AddToTranslation(Xform.TransformVector(PickedCDO->SpawnOffset));
	}

	ACleanableActor* Spawned = World->SpawnActor<ACleanableActor>(PickedClass, Xform, SpawnParams);
	if (Spawned)
	{
		Alive.Add(Spawned);
		PickedAnchor->CurrentCleanable = Spawned;
		UE_LOG(LogCleaning, Log, TEXT("[Spawner] SPAWNED %s at %s (anchor %s) — alive=%d"),
			*Spawned->GetName(), *Xform.GetLocation().ToCompactString(), *PickedAnchor->GetName(), Alive.Num());
	}
	else
	{
		UE_LOG(LogCleaning, Error, TEXT("[Spawner] SpawnActor returned null for class %s at %s"),
			*PickedClass->GetName(), *Xform.GetLocation().ToCompactString());
	}

	ScheduleNextSpawn();
}
