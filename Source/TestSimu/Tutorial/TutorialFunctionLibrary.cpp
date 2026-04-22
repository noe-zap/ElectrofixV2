#include "TutorialFunctionLibrary.h"
#include "TutorialManagerComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"

UTutorialManagerComponent* UTutorialFunctionLibrary::GetTutorialManager(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Common location: GameState.
	if (AGameStateBase* GS = World->GetGameState())
	{
		if (UTutorialManagerComponent* FromGS = GS->FindComponentByClass<UTutorialManagerComponent>())
		{
			return FromGS;
		}
	}

	// Fallback: any replicated actor in the world carrying the component
	// (e.g. a project-specific store-manager actor).
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (UTutorialManagerComponent* Found = It->FindComponentByClass<UTutorialManagerComponent>())
		{
			return Found;
		}
	}

	return nullptr;
}

void UTutorialFunctionLibrary::ReportTutorialEvent(const UObject* WorldContextObject, FName EventId, AActor* Source)
{
	if (UTutorialManagerComponent* Mgr = GetTutorialManager(WorldContextObject))
	{
		Mgr->ReportEvent(EventId, Source);
	}
}

void UTutorialFunctionLibrary::RewindTutorialToTask(const UObject* WorldContextObject, FName EventId)
{
	if (UTutorialManagerComponent* Mgr = GetTutorialManager(WorldContextObject))
	{
		Mgr->RewindCurrentStepToTask(EventId);
	}
}
