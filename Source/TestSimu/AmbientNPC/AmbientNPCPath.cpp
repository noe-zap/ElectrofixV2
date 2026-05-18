#include "AmbientNPCPath.h"
#include "AmbientNPCCharacter.h"
#include "AmbientNPCController.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAmbientNPC, Log, All);

namespace
{
	void ScreenMsg(int32 Key, float Duration, const FColor& Color, const FString& Text)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(Key, Duration, Color, Text);
		}
	}
}

AAmbientNPCPath::AAmbientNPCPath()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	RootComponent = Spline;
}

void AAmbientNPCPath::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	BuildWaypoints();

	UE_LOG(LogAmbientNPC, Log, TEXT("Path %s ready | classes=%d waypoints=%d interval=[%.1f, %.1f] maxAlive=%d"),
		*GetName(), NPCClasses.Num(), CachedWaypoints.Num(), MinSpawnInterval, MaxSpawnInterval, MaxAlive);

	ScreenMsg(GetUniqueID(), 15.f, FColor::Cyan, FString::Printf(
		TEXT("[AmbientNPC] %s ready | classes=%d waypoints=%d maxAlive=%d"),
		*GetName(), NPCClasses.Num(), CachedWaypoints.Num(), MaxAlive));

	ScheduleNextSpawn();
}

void AAmbientNPCPath::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AAmbientNPCPath::SetSpawnPaused(bool bPaused)
{
	bSpawnPaused = bPaused;
}

void AAmbientNPCPath::BuildWaypoints()
{
	CachedWaypoints.Reset();
	if (Spline == nullptr)
	{
		return;
	}

	const float Length = Spline->GetSplineLength();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Step = FMath::Max(50.f, WaypointSpacing);
	for (float Dist = 0.f; Dist < Length; Dist += Step)
	{
		CachedWaypoints.Add(Spline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World));
	}
	// Always include the exact end point.
	CachedWaypoints.Add(Spline->GetLocationAtDistanceAlongSpline(Length, ESplineCoordinateSpace::World));
}

void AAmbientNPCPath::ScheduleNextSpawn()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float Min = FMath::Max(0.5f, MinSpawnInterval);
	const float Max = FMath::Max(Min, MaxSpawnInterval);
	const float Delay = FMath::FRandRange(Min, Max);

	World->GetTimerManager().ClearTimer(SpawnTimer);
	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AAmbientNPCPath::TrySpawn);
	World->GetTimerManager().SetTimer(SpawnTimer, Delegate, Delay, false);
}

void AAmbientNPCPath::PurgeStale()
{
	Alive.RemoveAll([](const TWeakObjectPtr<AAmbientNPCCharacter>& Weak)
	{
		return !Weak.IsValid();
	});
}

void AAmbientNPCPath::TrySpawn()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	PurgeStale();

	if (bSpawnPaused)
	{
		ScheduleNextSpawn();
		return;
	}
	if (NPCClasses.Num() == 0)
	{
		UE_LOG(LogAmbientNPC, Warning, TEXT("Path %s skip: NPCClasses is empty"), *GetName());
		ScreenMsg(GetUniqueID() + 1, 6.f, FColor::Red, FString::Printf(TEXT("[AmbientNPC] %s skip: NPCClasses empty"), *GetName()));
		ScheduleNextSpawn();
		return;
	}
	if (CachedWaypoints.Num() < 2)
	{
		UE_LOG(LogAmbientNPC, Warning, TEXT("Path %s skip: spline has no length"), *GetName());
		ScreenMsg(GetUniqueID() + 1, 6.f, FColor::Red, FString::Printf(TEXT("[AmbientNPC] %s skip: spline empty"), *GetName()));
		ScheduleNextSpawn();
		return;
	}
	if (Alive.Num() >= MaxAlive)
	{
		ScheduleNextSpawn();
		return;
	}

	TArray<TSubclassOf<AAmbientNPCCharacter>> ValidClasses;
	ValidClasses.Reserve(NPCClasses.Num());
	for (const TSubclassOf<AAmbientNPCCharacter>& Class : NPCClasses)
	{
		if (Class != nullptr)
		{
			ValidClasses.Add(Class);
		}
	}
	if (ValidClasses.Num() == 0)
	{
		ScheduleNextSpawn();
		return;
	}

	TSubclassOf<AAmbientNPCCharacter> PickedClass = ValidClasses[FMath::RandRange(0, ValidClasses.Num() - 1)];

	const FVector StartLocation = CachedWaypoints[0];
	const FVector NextLocation = CachedWaypoints.Num() > 1 ? CachedWaypoints[1] : StartLocation;
	const FRotator StartRotation = (NextLocation - StartLocation).GetSafeNormal2D().Rotation();
	const FTransform StartXform(StartRotation, StartLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AAmbientNPCCharacter* Spawned = World->SpawnActor<AAmbientNPCCharacter>(PickedClass, StartXform, SpawnParams);
	if (Spawned == nullptr)
	{
		UE_LOG(LogAmbientNPC, Warning, TEXT("Path %s: SpawnActor returned null"), *GetName());
		ScreenMsg(GetUniqueID() + 1, 6.f, FColor::Red, FString::Printf(TEXT("[AmbientNPC] %s: spawn failed"), *GetName()));
		ScheduleNextSpawn();
		return;
	}

	Alive.Add(Spawned);

	UE_LOG(LogAmbientNPC, Log, TEXT("Path %s spawned %s (alive=%d/%d)"), *GetName(), *GetNameSafe(Spawned), Alive.Num(), MaxAlive);
	ScreenMsg(-1, 4.f, FColor::Green, FString::Printf(TEXT("[AmbientNPC] %s spawned (alive=%d/%d)"), *GetName(), Alive.Num(), MaxAlive));

	// Hand the waypoint queue to the controller. AutoPossess may need a frame to run.
	TArray<FVector> WaypointsCopy = CachedWaypoints;
	if (AAmbientNPCController* AICtrl = Cast<AAmbientNPCController>(Spawned->GetController()))
	{
		AICtrl->AssignPath(WaypointsCopy);
	}
	else
	{
		TWeakObjectPtr<AAmbientNPCCharacter> WeakSpawned = Spawned;
		World->GetTimerManager().SetTimerForNextTick([WeakSpawned, WaypointsCopy]()
		{
			AAmbientNPCCharacter* P = WeakSpawned.Get();
			if (P == nullptr)
			{
				return;
			}
			if (AAmbientNPCController* C = Cast<AAmbientNPCController>(P->GetController()))
			{
				C->AssignPath(WaypointsCopy);
			}
		});
	}

	ScheduleNextSpawn();
}
