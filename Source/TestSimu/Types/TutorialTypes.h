#pragma once

#include "CoreMinimal.h"
#include "TutorialTypes.generated.h"

USTRUCT(BlueprintType)
struct FTutorialTaskData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Task")
	FText TaskName;

	// Event ID that task listens for. Interactables call ReportTutorialEvent with a matching name to complete the task.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Task")
	FName EventId = NAME_None;

	// Number of matching events required before this task is considered complete. 1 = single action.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Task", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;

	// Optional direct reference to an actor placed in the level — the arrow will follow this actor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Task")
	TSoftObjectPtr<AActor> TargetActor;

	// Fallback if TargetActor is not set: find the first actor in the world with this tag.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Task")
	FName TargetActorTag = NAME_None;

	// World-space offset above the target where the arrow will float.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Task")
	FVector ArrowOffset = FVector(0.f, 0.f, 120.f);
};

USTRUCT(BlueprintType)
struct FTutorialStepData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Step")
	FText StepName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Step", meta = (MultiLine = "true"))
	FText StepDescription;

	// Up to 32 tasks per step (internal replication uses an int32 bitmask).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Step")
	TArray<FTutorialTaskData> Tasks;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnTutorialStepChanged, const FTutorialStepData&, NewStep, int32, StepIndex, int32, CurrentTaskIndex, int32, TotalTaskCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTutorialTaskProgress, int32, CompletedCount, int32, TotalCount, const FText&, CurrentTaskName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialCompleted);
