#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/ToolsWheelTypes.h"
#include "Tool.generated.h"

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API ATool : public AActor
{
	GENERATED_BODY()

public:
	ATool();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	UStaticMeshComponent* ToolMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	UStaticMeshComponent* FirstPersonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Tool")
	EToolType ToolType = EToolType::None;

	UFUNCTION(Client, Reliable)
	void Client_AttachFirstPersonMesh();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tool|FP Animation", meta = (ClampMin = "0.0"))
	float CleanAnimSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tool|FP Animation", meta = (ClampMin = "0.0"))
	float CleanAnimAmplitude = 5.f;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintNativeEvent, Category = "Tool")
	void OnEquipped(AActor* NewOwner);

	UFUNCTION(BlueprintNativeEvent, Category = "Tool")
	void OnUnequipped();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Tool")
	void UseStart();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Tool")
	void UseStop();

private:
	FTransform FirstPersonMeshDefaultRelativeTransform;
	float CleanAnimTime = 0.f;
	bool bIsPlayingCleanAnim = false;
};
