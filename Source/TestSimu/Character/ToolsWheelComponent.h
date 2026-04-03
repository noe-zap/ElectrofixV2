#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/ToolsWheelTypes.h"
#include "ToolsWheelComponent.generated.h"

class UToolsWheelWidget;
class ATool;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolEquipped, ATool*, NewTool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWheelOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWheelClosed);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESTSIMU_API UToolsWheelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UToolsWheelComponent();

	// --- Configuration ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel")
	TArray<FToolSlotInfo> ToolSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|UI")
	TSubclassOf<UToolsWheelWidget> WheelWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel|Attachment")
	FName AttachSocketName = FName("hand_r");

	// Text shown for the default empty-hands slot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tools Wheel")
	FText EmptyHandsName = FText::FromString(TEXT("Empty Hands"));

	// --- State ---

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentTool, Category = "Tools Wheel")
	ATool* CurrentTool;

	UPROPERTY(BlueprintReadOnly, Category = "Tools Wheel")
	bool bIsWheelOpen;

	UPROPERTY(BlueprintReadOnly, Category = "Tools Wheel")
	int32 SelectedSlotIndex;

	UPROPERTY(BlueprintReadOnly, Category = "Tools Wheel")
	bool bHasToolEquipped;

	// --- Delegates ---

	UPROPERTY(BlueprintAssignable, Category = "Tools Wheel|Events")
	FOnToolEquipped OnToolEquipped;

	UPROPERTY(BlueprintAssignable, Category = "Tools Wheel|Events")
	FOnWheelOpened OnWheelOpened;

	UPROPERTY(BlueprintAssignable, Category = "Tools Wheel|Events")
	FOnWheelClosed OnWheelClosed;

	// --- Functions ---

	UFUNCTION(BlueprintCallable, Category = "Tools Wheel")
	void ToggleWheel(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Tools Wheel")
	void EquipToolAtIndex(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Tools Wheel")
	void UnequipCurrentTool();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TObjectPtr<UToolsWheelWidget> WheelWidget;

	UFUNCTION(Server, Reliable)
	void ServerEquipTool(int32 SlotIndex);

	UFUNCTION()
	void OnRep_CurrentTool();

	void CreateWheelWidget();
	void DestroyWheelWidget();

	APlayerController* GetOwnerPlayerController() const;
};
