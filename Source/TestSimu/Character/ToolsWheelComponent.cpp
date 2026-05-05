#include "ToolsWheelComponent.h"
#include "UI/ToolsWheelWidget.h"
#include "Tools/Tool.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"

UToolsWheelComponent::UToolsWheelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	bIsWheelOpen = false;
	SelectedSlotIndex = -1;
	CurrentTool = nullptr;
	bHasToolEquipped = false;
	WheelWidget = nullptr;
}

void UToolsWheelComponent::BeginPlay()
{
	Super::BeginPlay();

	// Insert "Empty Hands" as the first slot (index 0)
	FToolSlotInfo EmptySlot;
	EmptySlot.DisplayName = EmptyHandsName;
	EmptySlot.ToolClass = nullptr;
	EmptySlot.Icon = nullptr;
	ToolSlots.Insert(EmptySlot, 0);

	// Default selection is empty hands
	SelectedSlotIndex = 0;
}

void UToolsWheelComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsWheelOpen || WheelWidget == nullptr)
	{
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (PC == nullptr)
	{
		return;
	}

	float MouseX, MouseY;
	if (PC->GetMousePosition(MouseX, MouseY))
	{
		int32 ViewportX, ViewportY;
		PC->GetViewportSize(ViewportX, ViewportY);
		const FVector2D ScreenCenter(ViewportX * 0.5f, ViewportY * 0.5f);
		WheelWidget->UpdateSelection(FVector2D(MouseX, MouseY), ScreenCenter);
	}
}

void UToolsWheelComponent::ToggleWheel(bool bOpen)
{
	if (bOpen == bIsWheelOpen)
	{
		return;
	}

	bIsWheelOpen = bOpen;

	if (bOpen)
	{
		CreateWheelWidget();
		OnWheelOpened.Broadcast();
	}
	else
	{
		int32 HoveredIdx = -1;
		if (WheelWidget != nullptr)
		{
			HoveredIdx = WheelWidget->GetHoveredIndex();
		}

		DestroyWheelWidget();
		OnWheelClosed.Broadcast();

		if (HoveredIdx >= 0 && HoveredIdx < ToolSlots.Num())
		{
			EquipToolAtIndex(HoveredIdx);
		}
	}
}

void UToolsWheelComponent::EquipToolAtIndex(int32 SlotIndex)
{
	if (!ToolSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	if (SlotIndex == SelectedSlotIndex)
	{
		return;
	}

	SelectedSlotIndex = SlotIndex;

	if (GetOwnerRole() == ROLE_Authority)
	{
		ServerEquipTool_Implementation(SlotIndex);
	}
	else
	{
		ServerEquipTool(SlotIndex);
	}
}

void UToolsWheelComponent::UnequipCurrentTool()
{
	SelectedSlotIndex = -1;

	if (CurrentTool != nullptr)
	{
		CurrentTool->UseStop();
		CurrentTool->OnUnequipped();
		CurrentTool->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		CurrentTool->Destroy();
		CurrentTool = nullptr;
		bHasToolEquipped = false;
	}
}

void UToolsWheelComponent::ServerEquipTool_Implementation(int32 SlotIndex)
{
	if (!ToolSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	// Destroy current tool
	if (CurrentTool != nullptr)
	{
		CurrentTool->UseStop();
		CurrentTool->OnUnequipped();
		CurrentTool->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		CurrentTool->Destroy();
		CurrentTool = nullptr;
	}
	bHasToolEquipped = false;

	const FToolSlotInfo& Slot = ToolSlots[SlotIndex];
	if (Slot.ToolClass == nullptr)
	{
		// Empty hands — just unequip, no new tool to spawn
		OnToolEquipped.Broadcast(nullptr);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATool* NewTool = GetWorld()->SpawnActor<ATool>(Slot.ToolClass, FTransform::Identity, SpawnParams);
	if (NewTool == nullptr)
	{
		return;
	}

	// Attach to the owner's skeletal mesh
	USkeletalMeshComponent* OwnerMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
	if (OwnerMesh != nullptr)
	{
		NewTool->AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Slot.AttachSocketName);
	}

	NewTool->OnEquipped(GetOwner());
	CurrentTool = NewTool;
	bHasToolEquipped = true;
	OnToolEquipped.Broadcast(CurrentTool);
}

void UToolsWheelComponent::OnRep_CurrentTool()
{
	bHasToolEquipped = (CurrentTool != nullptr);
	OnToolEquipped.Broadcast(CurrentTool);
}

void UToolsWheelComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UToolsWheelComponent, CurrentTool);
}

void UToolsWheelComponent::CreateWheelWidget()
{
	APlayerController* PC = GetOwnerPlayerController();
	if (PC == nullptr || WheelWidgetClass == nullptr)
	{
		return;
	}

	WheelWidget = CreateWidget<UToolsWheelWidget>(PC, WheelWidgetClass);
	if (WheelWidget != nullptr)
	{
		WheelWidget->Slots = ToolSlots;
		WheelWidget->AddToViewport(100);

		// Center mouse on screen
		int32 ViewportX, ViewportY;
		PC->GetViewportSize(ViewportX, ViewportY);
		PC->SetMouseLocation(ViewportX / 2, ViewportY / 2);

		// Show cursor and set Game+UI input mode
		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(InputMode);
	}
}

void UToolsWheelComponent::DestroyWheelWidget()
{
	if (WheelWidget != nullptr)
	{
		WheelWidget->RemoveFromParent();
		WheelWidget = nullptr;
	}

	APlayerController* PC = GetOwnerPlayerController();
	if (PC != nullptr)
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
}

APlayerController* UToolsWheelComponent::GetOwnerPlayerController() const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn != nullptr)
	{
		return Cast<APlayerController>(OwnerPawn->GetController());
	}
	return nullptr;
}
