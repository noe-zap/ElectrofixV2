#include "CleanableSpawnerComponent.h"
#include "CleanableActor.h"
#include "CleanableSpawnPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

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
		return;
	}

	// Pick up any spawn points that began play before us (common when the spawner lives on a
	// streamed/replicated actor like the Store rather than the GameMode).
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ACleanableSpawnPoint> It(World); It; ++It)
		{
			if (ACleanableSpawnPoint* Point = *It)
			{
				if (!Anchors.Contains(Point))
				{
					Anchors.Add(Point);
				}
			}
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

	if (CleanableClasses.Num() == 0 || Anchors.Num() == 0)
	{
		ScheduleNextSpawn();
		return;
	}

	if (Alive.Num() >= MaxAlive)
	{
		ScheduleNextSpawn();
		return;
	}

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
		ScheduleNextSpawn();
		return;
	}

	ACleanableSpawnPoint* PickedAnchor = FreeAnchors[FMath::RandRange(0, FreeAnchors.Num() - 1)];

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
		ScheduleNextSpawn();
		return;
	}

	TSubclassOf<ACleanableActor> PickedClass = Compatible[FMath::RandRange(0, Compatible.Num() - 1)];

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
	}

	ScheduleNextSpawn();
}
