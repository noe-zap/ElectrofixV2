#include "TutorialManagerComponent.h"
#include "TutorialSequence.h"
#include "TutorialArrow.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

UTutorialManagerComponent::UTutorialManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTutorialManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority && bAutoStart && !bTutorialComplete && CurrentStepIndex < 0)
	{
		Server_StartTutorial();
	}
}

void UTutorialManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTutorialManagerComponent, CurrentStepIndex);
	DOREPLIFETIME(UTutorialManagerComponent, CurrentStepTaskCompletionMask);
	DOREPLIFETIME(UTutorialManagerComponent, bTutorialComplete);
}

// ---- Public API ----

void UTutorialManagerComponent::ReportEvent(FName EventId, AActor* Source)
{
	if (EventId.IsNone() || bTutorialComplete)
	{
		return;
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerReportEvent_Implementation(EventId, Source);
	}
	else
	{
		ServerReportEvent(EventId, Source);
	}
}

void UTutorialManagerComponent::RewindCurrentStepToTask(FName EventId)
{
	if (GetOwnerRole() != ROLE_Authority || EventId.IsNone() || !IsTutorialActive())
	{
		return;
	}

	const FTutorialStepData& Step = TutorialSequence->Steps[CurrentStepIndex];
	int32 TargetIdx = INDEX_NONE;
	for (int32 i = 0; i < Step.Tasks.Num() && i < 32; ++i)
	{
		if (Step.Tasks[i].EventId == EventId)
		{
			TargetIdx = i;
			break;
		}
	}

	if (TargetIdx == INDEX_NONE)
	{
		return;
	}

	// Keep bits strictly below TargetIdx, clear the rest.
	const int32 KeepMask = (TargetIdx == 0) ? 0 : ((1 << TargetIdx) - 1);
	const int32 NewMask = CurrentStepTaskCompletionMask & KeepMask;

	if (NewMask == CurrentStepTaskCompletionMask)
	{
		return;
	}

	CurrentStepTaskCompletionMask = NewMask;

	// Server-side notify (OnRep_ only fires on clients).
	OnTutorialTaskProgress.Broadcast(GetCompletedTaskCount(), GetTotalTaskCount(), GetCurrentTaskName());
	UpdateArrowForCurrentTask();
}

void UTutorialManagerComponent::SkipTutorial()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerSkip_Implementation();
	}
	else
	{
		ServerSkip();
	}
}

void UTutorialManagerComponent::RestartTutorial()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerRestart_Implementation();
	}
	else
	{
		ServerRestart();
	}
}

FTutorialStepData UTutorialManagerComponent::GetCurrentStep() const
{
	if (TutorialSequence && TutorialSequence->Steps.IsValidIndex(CurrentStepIndex))
	{
		return TutorialSequence->Steps[CurrentStepIndex];
	}
	return FTutorialStepData();
}

int32 UTutorialManagerComponent::GetCompletedTaskCount() const
{
	return FMath::CountBits(static_cast<uint32>(CurrentStepTaskCompletionMask));
}

int32 UTutorialManagerComponent::GetTotalTaskCount() const
{
	if (TutorialSequence && TutorialSequence->Steps.IsValidIndex(CurrentStepIndex))
	{
		return TutorialSequence->Steps[CurrentStepIndex].Tasks.Num();
	}
	return 0;
}

TArray<bool> UTutorialManagerComponent::GetTaskCompletionStates() const
{
	TArray<bool> Result;
	const int32 Total = GetTotalTaskCount();
	Result.Reserve(Total);
	for (int32 i = 0; i < Total; ++i)
	{
		Result.Add((CurrentStepTaskCompletionMask & (1 << i)) != 0);
	}
	return Result;
}

bool UTutorialManagerComponent::IsTutorialActive() const
{
	return !bTutorialComplete
		&& CurrentStepIndex >= 0
		&& TutorialSequence
		&& TutorialSequence->Steps.IsValidIndex(CurrentStepIndex);
}

// ---- Server RPCs ----

void UTutorialManagerComponent::ServerReportEvent_Implementation(FName EventId, AActor* Source)
{
	if (bTutorialComplete)
	{
		return;
	}

	bool bMatched = false;

	if (IsTutorialActive())
	{
		const FTutorialStepData& Step = TutorialSequence->Steps[CurrentStepIndex];
		for (int32 i = 0; i < Step.Tasks.Num() && i < 32; ++i)
		{
			const FTutorialTaskData& Task = Step.Tasks[i];
			const int32 Bit = 1 << i;

			if ((CurrentStepTaskCompletionMask & Bit) != 0)
			{
				continue;
			}

			if (Task.EventId == EventId)
			{
				CurrentStepTaskCompletionMask |= Bit;
				bMatched = true;
				break;
			}
		}
	}

	if (bMatched)
	{
		OnTutorialTaskProgress.Broadcast(GetCompletedTaskCount(), GetTotalTaskCount(), GetCurrentTaskName());
		UpdateArrowForCurrentTask();
		Server_AdvanceIfStepComplete();
	}
	else
	{
		// Event didn't land on the current step's mask. Queue it — a future step may have a
		// task with this EventId, and doing it early should credit the player when that step
		// becomes active. Drained by ApplyUnconsumedEvents on step start/advance.
		UnconsumedEvents.Add(EventId);
	}
}

void UTutorialManagerComponent::ServerSkip_Implementation()
{
	if (!TutorialSequence)
	{
		return;
	}

	CurrentStepIndex = TutorialSequence->Steps.Num();
	CurrentStepTaskCompletionMask = 0;
	bTutorialComplete = true;
	UnconsumedEvents.Reset();

	// Server-side notify.
	OnTutorialCompleted.Broadcast();

	if (ArrowInstance)
	{
		ArrowInstance->Destroy();
		ArrowInstance = nullptr;
	}
}

void UTutorialManagerComponent::ServerRestart_Implementation()
{
	bTutorialComplete = false;
	CurrentStepTaskCompletionMask = 0;
	CurrentStepIndex = -1;
	UnconsumedEvents.Reset();
	Server_StartTutorial();
}

// ---- OnReps ----

void UTutorialManagerComponent::OnRep_CurrentStepIndex(int32 OldIndex)
{
	if (bTutorialComplete || !TutorialSequence || !TutorialSequence->Steps.IsValidIndex(CurrentStepIndex))
	{
		OnTutorialCompleted.Broadcast();
		return;
	}

	OnTutorialStepChanged.Broadcast(TutorialSequence->Steps[CurrentStepIndex], CurrentStepIndex, GetCurrentTaskIndex(), GetTotalTaskCount());
	OnTutorialTaskProgress.Broadcast(GetCompletedTaskCount(), GetTotalTaskCount(), GetCurrentTaskName());
	UpdateArrowForCurrentTask();
}

void UTutorialManagerComponent::OnRep_TaskMask(int32 OldMask)
{
	OnTutorialTaskProgress.Broadcast(GetCompletedTaskCount(), GetTotalTaskCount(), GetCurrentTaskName());
	UpdateArrowForCurrentTask();
}

// ---- Server helpers ----

void UTutorialManagerComponent::Server_StartTutorial()
{
	if (!TutorialSequence || TutorialSequence->Steps.Num() == 0)
	{
		return;
	}

	CurrentStepIndex = 0;
	CurrentStepTaskCompletionMask = 0;
	bTutorialComplete = false;

	// Drain any events reported before the tutorial started.
	ApplyUnconsumedEvents();

	// Server-side notify (OnRep_ only fires on clients).
	OnTutorialStepChanged.Broadcast(TutorialSequence->Steps[CurrentStepIndex], CurrentStepIndex, GetCurrentTaskIndex(), GetTotalTaskCount());
	OnTutorialTaskProgress.Broadcast(GetCompletedTaskCount(), GetTotalTaskCount(), GetCurrentTaskName());
	UpdateArrowForCurrentTask();

	// If the drain already completed step 0 entirely, cascade-advance.
	Server_AdvanceIfStepComplete();
}

void UTutorialManagerComponent::Server_AdvanceIfStepComplete()
{
	const int32 FullMask = ComputeFullMaskForCurrentStep();
	if (FullMask == 0 || (CurrentStepTaskCompletionMask & FullMask) != FullMask)
	{
		return;
	}

	const int32 NextIndex = CurrentStepIndex + 1;
	CurrentStepTaskCompletionMask = 0;

	if (!TutorialSequence->Steps.IsValidIndex(NextIndex))
	{
		// No more steps — tutorial complete.
		CurrentStepIndex = NextIndex;
		bTutorialComplete = true;
		UnconsumedEvents.Reset();
		OnTutorialCompleted.Broadcast();

		if (ArrowInstance)
		{
			ArrowInstance->Destroy();
			ArrowInstance = nullptr;
		}
		return;
	}

	CurrentStepIndex = NextIndex;

	// Drain queued events into the new step's mask before broadcasting, so clients see the
	// final post-drain state in a single update.
	ApplyUnconsumedEvents();

	// Server-side notify.
	OnTutorialStepChanged.Broadcast(TutorialSequence->Steps[CurrentStepIndex], CurrentStepIndex, GetCurrentTaskIndex(), GetTotalTaskCount());
	OnTutorialTaskProgress.Broadcast(GetCompletedTaskCount(), GetTotalTaskCount(), GetCurrentTaskName());
	UpdateArrowForCurrentTask();

	// If the drain completed the new step entirely (e.g. a future license was already bought),
	// recursively advance. Recursion terminates at end-of-tutorial or an incomplete step.
	Server_AdvanceIfStepComplete();
}

void UTutorialManagerComponent::ApplyUnconsumedEvents()
{
	if (!IsTutorialActive() || UnconsumedEvents.Num() == 0)
	{
		return;
	}

	const FTutorialStepData& Step = TutorialSequence->Steps[CurrentStepIndex];

	// Walk backwards so RemoveAt is safe.
	for (int32 e = UnconsumedEvents.Num() - 1; e >= 0; --e)
	{
		const FName EventId = UnconsumedEvents[e];
		for (int32 i = 0; i < Step.Tasks.Num() && i < 32; ++i)
		{
			const int32 Bit = 1 << i;
			if ((CurrentStepTaskCompletionMask & Bit) != 0)
			{
				continue;
			}
			if (Step.Tasks[i].EventId == EventId)
			{
				CurrentStepTaskCompletionMask |= Bit;
				UnconsumedEvents.RemoveAt(e);
				break;
			}
		}
	}
}

int32 UTutorialManagerComponent::ComputeFullMaskForCurrentStep() const
{
	if (!TutorialSequence || !TutorialSequence->Steps.IsValidIndex(CurrentStepIndex))
	{
		return 0;
	}
	const int32 N = FMath::Min(TutorialSequence->Steps[CurrentStepIndex].Tasks.Num(), 32);
	if (N <= 0)
	{
		return 0;
	}
	// Build (1<<N) - 1 safely for N up to 32.
	return (N >= 32) ? static_cast<int32>(0xFFFFFFFF) : ((1 << N) - 1);
}

AActor* UTutorialManagerComponent::ResolveTaskTarget(const FTutorialTaskData& Task) const
{
	if (AActor* Direct = Task.TargetActor.Get())
	{
		return Direct;
	}

	// Soft ptr may not be loaded; try a synchronous load via LoadSynchronous on server only.
	if (!Task.TargetActor.IsNull() && GetOwnerRole() == ROLE_Authority)
	{
		if (AActor* Loaded = Task.TargetActor.LoadSynchronous())
		{
			return Loaded;
		}
	}

	if (!Task.TargetActorTag.IsNone())
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->Tags.Contains(Task.TargetActorTag))
				{
					return *It;
				}
			}
		}
	}

	return nullptr;
}

int32 UTutorialManagerComponent::GetCurrentTaskIndex() const
{
	if (!IsTutorialActive())
	{
		return -1;
	}
	const int32 Total = GetTotalTaskCount();
	for (int32 i = 0; i < Total; ++i)
	{
		if ((CurrentStepTaskCompletionMask & (1 << i)) == 0)
		{
			return i;
		}
	}
	return -1;
}

FText UTutorialManagerComponent::GetCurrentTaskName() const
{
	const int32 Idx = GetCurrentTaskIndex();
	if (Idx < 0 || !TutorialSequence || !TutorialSequence->Steps.IsValidIndex(CurrentStepIndex))
	{
		return FText::GetEmpty();
	}
	const FTutorialStepData& Step = TutorialSequence->Steps[CurrentStepIndex];
	if (!Step.Tasks.IsValidIndex(Idx))
	{
		return FText::GetEmpty();
	}
	return Step.Tasks[Idx].TaskName;
}

void UTutorialManagerComponent::UpdateArrowForCurrentTask()
{
	// Arrow is spawned and positioned on the server; it replicates to all clients.
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (!IsTutorialActive())
	{
		if (ArrowInstance)
		{
			ArrowInstance->Destroy();
			ArrowInstance = nullptr;
		}
		return;
	}

	const int32 TaskIdx = GetCurrentTaskIndex();
	if (TaskIdx < 0)
	{
		return;
	}

	const FTutorialTaskData& Task = TutorialSequence->Steps[CurrentStepIndex].Tasks[TaskIdx];
	AActor* Target = ResolveTaskTarget(Task);

	if (!Target)
	{
		if (ArrowInstance)
		{
			ArrowInstance->SetActorHiddenInGame(true);
		}
		return;
	}

	if (!ArrowInstance)
	{
		if (!ArrowClass)
		{
			return;
		}
		UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ArrowInstance = World->SpawnActor<ATutorialArrow>(ArrowClass, Target->GetActorLocation() + Task.ArrowOffset, FRotator::ZeroRotator, SpawnParams);
	}

	if (ArrowInstance)
	{
		ArrowInstance->SetActorHiddenInGame(false);
		ArrowInstance->SetTarget(Target, Task.ArrowOffset);
	}
}
