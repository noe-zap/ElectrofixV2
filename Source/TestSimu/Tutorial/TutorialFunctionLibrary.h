#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TutorialFunctionLibrary.generated.h"

class UTutorialManagerComponent;

UCLASS()
class TESTSIMU_API UTutorialFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Call from any Blueprint on any machine when a tutorial-relevant gameplay action happens.
	// The manager lives on the GameState; this helper finds it and routes through its ReportEvent
	// (which server-RPCs as needed). Safe to call when no tutorial is active (it's a no-op).
	UFUNCTION(BlueprintCallable, Category = "Tutorial", meta = (WorldContext = "WorldContextObject"))
	static void ReportTutorialEvent(const UObject* WorldContextObject, FName EventId, AActor* Source);

	// Server-only: rewind the current step so the task with this EventId (and all later tasks in
	// the step) are uncompleted again. Intended for "bail-out" paths — e.g. the player opened the
	// workshop but left without finishing the repair. Call this from a server RPC you've already
	// routed through a player-owned actor.
	UFUNCTION(BlueprintCallable, Category = "Tutorial", meta = (WorldContext = "WorldContextObject"))
	static void RewindTutorialToTask(const UObject* WorldContextObject, FName EventId);

	// Accessor for widgets that need to bind to the delegates on the manager.
	UFUNCTION(BlueprintPure, Category = "Tutorial", meta = (WorldContext = "WorldContextObject"))
	static UTutorialManagerComponent* GetTutorialManager(const UObject* WorldContextObject);
};
