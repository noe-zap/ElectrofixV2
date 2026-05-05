#pragma once

#include "CoreMinimal.h"
#include "Tools/Tool.h"
#include "CleaningTool.generated.h"

class UAnimMontage;
class ACleanableActor;

UCLASS(Blueprintable, BlueprintType)
class TESTSIMU_API ACleaningTool : public ATool
{
	GENERATED_BODY()

public:
	ACleaningTool();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning")
	FName CleaningTag = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning")
	UAnimMontage* CleaningMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning")
	float CleanAmountPerClick = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning")
	float CleanRange = 400.f;

	// Trace radius for aim tolerance. 0 = exact line. ~10cm is forgiving without being silly.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning", meta = (ClampMin = "0.0"))
	float CleanTraceRadius = 6.f;

	// Seconds between clean ticks while the use input is held.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cleaning", meta = (ClampMin = "0.01"))
	float CleanInterval = 0.1f;

	virtual void UseStart_Implementation() override;
	virtual void UseStop_Implementation() override;

private:
	UFUNCTION(Server, Reliable)
	void ServerCleanAt(FVector_NetQuantize Start, FVector_NetQuantize End);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCleaningMontage();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopCleaningMontage();

	bool ComputeAimRay(FVector& OutStart, FVector& OutEnd) const;
	void DoCleanAt(const FVector& Start, const FVector& End);
	void CleanTick();

	FTimerHandle CleanHoldTimer;
};
