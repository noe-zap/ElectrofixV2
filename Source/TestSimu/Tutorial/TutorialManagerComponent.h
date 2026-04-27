#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/TutorialTypes.h"
#include "TutorialManagerComponent.generated.h"

class UTutorialSequence;
class ATutorialArrow;

UCLASS(ClassGroup = (Tutorial), meta = (BlueprintSpawnableComponent))
class TESTSIMU_API UTutorialManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTutorialManagerComponent();

	// --- Configuration ---

	// Authoring asset: the ordered step list to play.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TObjectPtr<UTutorialSequence> TutorialSequence;

	// Arrow actor class spawned over the current task's target.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TSubclassOf<ATutorialArrow> ArrowClass;

	// If true, start the tutorial automatically on BeginPlay (server only).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|IDs")
	FName StoreShelfID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|IDs")
	FName AvailableLicenseID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|IDs")
	FName RepairShelfID;

	// --- Replicated state ---

	// -1 = not started. 0..N-1 = active step. N (== Steps.Num()) = finished.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentStepIndex, Category = "Tutorial")
	int32 CurrentStepIndex = -1;

	// Bitmask: bit i = task i of the current step is complete. Reset when step advances.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TaskMask, Category = "Tutorial")
	int32 CurrentStepTaskCompletionMask = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Tutorial")
	bool bTutorialComplete = false;

	// --- Delegates (bind from widgets / Blueprints) ---

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialStepChanged OnTutorialStepChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialTaskProgress OnTutorialTaskProgress;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialCompleted OnTutorialCompleted;

	// --- Public API ---

	// Call from any client when a tutorial-relevant gameplay event happens. Routes to server.
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ReportEvent(FName EventId, AActor* Source);

	// Server-authoritative: within the current step, clear the completion bit of the task whose
	// EventId matches, plus every task after it. No-op if the tutorial isn't active, the EventId
	// isn't found in the current step, or called on a non-authority role.
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void RewindCurrentStepToTask(FName EventId);

	// Jump straight to completed state (server auth; call from debug / save-load).
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SkipTutorial();

	// Reset to the first step (server auth).
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void RestartTutorial();

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	FTutorialStepData GetCurrentStep() const;

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	int32 GetCompletedTaskCount() const;

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	int32 GetTotalTaskCount() const;

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	TArray<bool> GetTaskCompletionStates() const;

	// 0-based index of the task currently being worked on in the current step (the first
	// uncompleted task). Returns -1 when no active task (step finished or tutorial inactive).
	// For a UI like "1/3", display (GetCurrentTaskIndex + 1) / GetTotalTaskCount.
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	int32 GetCurrentTaskIndex() const;

	// Display name of the task currently being worked on (first uncompleted). Empty when none.
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	FText GetCurrentTaskName() const;

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsTutorialActive() const;

	// True if a task with this EventId has already been credited: tutorial finished, the event is
	// queued in UnconsumedEvents (done early), an occurrence sits in a past step, or its bit is
	// set in the current step's mask. False if EventId is None, missing from the sequence, or
	// only present in future/unfinished slots.
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsEventCompleted(FName EventId) const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(Server, Reliable)
	void ServerReportEvent(FName EventId, AActor* Source);

	UFUNCTION(Server, Reliable)
	void ServerSkip();

	UFUNCTION(Server, Reliable)
	void ServerRestart();

	UFUNCTION()
	void OnRep_CurrentStepIndex(int32 OldIndex);

	UFUNCTION()
	void OnRep_TaskMask(int32 OldMask);

	// Server-only: check if all tasks of the current step are done, advance if so.
	void Server_AdvanceIfStepComplete();

	// Server-only: start at step 0 and broadcast/arrow-spawn.
	void Server_StartTutorial();

	// Compute the bitmask of all-tasks-complete for the current step (e.g. 3 tasks -> 0b111).
	int32 ComputeFullMaskForCurrentStep() const;

	// Resolve the target actor for a given task (direct ref first, tag fallback).
	AActor* ResolveTaskTarget(const FTutorialTaskData& Task) const;

	// Spawn the arrow actor on the server (if missing) and re-point it at the current focused task target.
	void UpdateArrowForCurrentTask();

	// Server-only: events that didn't match the current step when reported. Drained into a
	// step's mask when that step becomes active, so doing a task early (including same-ID ones
	// in future steps) credits the player when the matching step arrives.
	void ApplyUnconsumedEvents();

	UPROPERTY()
	TArray<FName> UnconsumedEvents;

	UPROPERTY()
	TObjectPtr<ATutorialArrow> ArrowInstance;
};
