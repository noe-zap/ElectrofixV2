#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tool.generated.h"

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API ATool : public AActor
{
	GENERATED_BODY()

public:
	ATool();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	UStaticMeshComponent* ToolMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tool|Attachment")
	FVector AttachOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Tool|Attachment")
	FRotator AttachRotation = FRotator::ZeroRotator;

	UFUNCTION(BlueprintNativeEvent, Category = "Tool")
	void OnEquipped(AActor* NewOwner);

	UFUNCTION(BlueprintNativeEvent, Category = "Tool")
	void OnUnequipped();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Tool")
	void UseStart();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Tool")
	void UseStop();
};
