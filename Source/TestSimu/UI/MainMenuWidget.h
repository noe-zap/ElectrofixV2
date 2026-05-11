#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;

UCLASS(BlueprintType, Blueprintable)
class TESTSIMU_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main Menu")
	TSoftObjectPtr<UWorld> LevelToLoad;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* PlayButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* SettingsButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* QuitButton;

	UFUNCTION()
	void OnPlayClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
	void OnSettingsRequested();
};
