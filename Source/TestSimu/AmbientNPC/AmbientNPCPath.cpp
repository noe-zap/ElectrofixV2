#include "AmbientNPCPath.h"
#include "AmbientNPCCharacter.h"
#include "AmbientNPCController.h"
#include "AmbientNPCSpawnPoint.h"
#include "AmbientNPCExitPoint.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAmbientNPC, Log, All);

AAmbientNPCPath::AAmbientNPCPath()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AAmbientNPCPath::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	RefreshPoints();

	UE_LOG(LogAmbientNPC, Log, TEXT("Director %s ready | classes=%d spawnPts=%d exitPts=%d interval=%.1fs perWave=%d maxAlive=%d"),
		*GetName(), NPCClasses.Num(), SpawnPoints.Num(), ExitPoints.Num(), SpawnInterval, NPCsPerWave, MaxAlive);

	ScheduleNextWave();
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

void AAmbientNPCPath::RefreshPoints()
{
	SpawnPoints.Reset();
	ExitPoints.Reset();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(this, AAmbientNPCSpawnPoint::StaticClass(), Found);
	SpawnPoints.Reserve(Found.Num());
	for (AActor* A : Found)
	{
		if (AAmbientNPCSpawnPoint* SP = Cast<AAmbientNPCSpawnPoint>(A))
		{
			SpawnPoints.Add(SP);
		}
	}

	Found.Reset();
	UGameplayStatics::GetAllActorsOfClass(this, AAmbientNPCExitPoint::StaticClass(), Found);
	ExitPoints.Reserve(Found.Num());
	for (AActor* A : Found)
	{
		if (AAmbientNPCExitPoint* EP = Cast<AAmbientNPCExitPoint>(A))
		{
			ExitPoints.Add(EP);
		}
	}
}

void AAmbientNPCPath::ScheduleNextWave()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const float Delay = FMath::Max(0.1f, SpawnInterval);
	World->GetTimerManager().ClearTimer(SpawnTimer);
	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AAmbientNPCPath::SpawnWave);
	World->GetTimerManager().SetTimer(SpawnTimer, Delegate, Delay, false);
}

void AAmbientNPCPath::PurgeStale()
{
	Alive.RemoveAll([](const TWeakObjectPtr<AAmbientNPCCharacter>& Weak)
	{
		return !Weak.IsValid();
	});
}

void AAmbientNPCPath::SpawnWave()
{
	PurgeStale();

	if (bSpawnPaused)
	{
		ScheduleNextWave();
		return;
	}

	if (NPCClasses.Num() == 0 || SpawnPoints.Num() == 0 || ExitPoints.Num() == 0)
	{
		UE_LOG(LogAmbientNPC, Warning, TEXT("Director %s skip: classes=%d spawn=%d exit=%d"),
			*GetName(), NPCClasses.Num(), SpawnPoints.Num(), ExitPoints.Num());
		ScheduleNextWave();
		return;
	}

	const int32 Slots = FMath::Max(0, MaxAlive - Alive.Num());
	const int32 ToSpawn = FMath::Min(NPCsPerWave, Slots);
	for (int32 i = 0; i < ToSpawn; ++i)
	{
		SpawnOne();
	}

	ScheduleNextWave();
}

void AAmbientNPCPath::SpawnOne()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// Resolve a valid spawn/exit/class from the cached lists.
	AAmbientNPCSpawnPoint* SpawnPt = nullptr;
	for (int32 Tries = 0; Tries < 4 && SpawnPt == nullptr && SpawnPoints.Num() > 0; ++Tries)
	{
		const int32 Idx = FMath::RandRange(0, SpawnPoints.Num() - 1);
		SpawnPt = SpawnPoints[Idx].Get();
	}
	if (SpawnPt == nullptr)
	{
		return;
	}

	AAmbientNPCExitPoint* ExitPt = nullptr;
	for (int32 Tries = 0; Tries < 4 && ExitPt == nullptr && ExitPoints.Num() > 0; ++Tries)
	{
		const int32 Idx = FMath::RandRange(0, ExitPoints.Num() - 1);
		ExitPt = ExitPoints[Idx].Get();
	}
	if (ExitPt == nullptr)
	{
		return;
	}

	TSubclassOf<AAmbientNPCCharacter> PickedClass = nullptr;
	for (int32 Tries = 0; Tries < 4 && PickedClass == nullptr && NPCClasses.Num() > 0; ++Tries)
	{
		const int32 Idx = FMath::RandRange(0, NPCClasses.Num() - 1);
		PickedClass = NPCClasses[Idx];
	}
	if (PickedClass == nullptr)
	{
		return;
	}

	const FVector StartLocation = SpawnPt->GetActorLocation();
	const FVector ExitLocation = ExitPt->GetActorLocation();
	const FRotator StartRotation = (ExitLocation - StartLocation).GetSafeNormal2D().Rotation();
	const FTransform StartXform(StartRotation, StartLocation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AAmbientNPCCharacter* Spawned = World->SpawnActor<AAmbientNPCCharacter>(PickedClass, StartXform, SpawnParams);
	if (Spawned == nullptr)
	{
		UE_LOG(LogAmbientNPC, Warning, TEXT("Director %s: SpawnActor returned null"), *GetName());
		return;
	}

	Alive.Add(Spawned);
	UE_LOG(LogAmbientNPC, Log, TEXT("Director %s spawned %s @ %s -> exit %s (alive=%d/%d)"),
		*GetName(), *GetNameSafe(Spawned), *GetNameSafe(SpawnPt), *GetNameSafe(ExitPt), Alive.Num(), MaxAlive);

	// AutoPossess may need a frame to assign the controller.
	if (AAmbientNPCController* AICtrl = Cast<AAmbientNPCController>(Spawned->GetController()))
	{
		AICtrl->AssignDestination(ExitLocation);
	}
	else
	{
		TWeakObjectPtr<AAmbientNPCCharacter> WeakSpawned = Spawned;
		World->GetTimerManager().SetTimerForNextTick([WeakSpawned, ExitLocation]()
		{
			AAmbientNPCCharacter* P = WeakSpawned.Get();
			if (P == nullptr)
			{
				return;
			}
			if (AAmbientNPCController* C = Cast<AAmbientNPCController>(P->GetController()))
			{
				C->AssignDestination(ExitLocation);
			}
		});
	}
}
