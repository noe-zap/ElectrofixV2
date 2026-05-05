#include "CleanableSpawnPoint.h"
#include "CleanableSpawnerComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	UCleanableSpawnerComponent* FindSpawnerInWorld(UWorld* World)
	{
		if (World == nullptr)
		{
			return nullptr;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (UCleanableSpawnerComponent* Spawner = It->FindComponentByClass<UCleanableSpawnerComponent>())
			{
				return Spawner;
			}
		}
		return nullptr;
	}
}

ACleanableSpawnPoint::ACleanableSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = Root;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->SetupAttachment(Root);
		SpriteComponent->bIsScreenSizeScaled = true;
	}

	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (ArrowComponent)
	{
		ArrowComponent->SetupAttachment(Root);
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
	}
#endif
}

void ACleanableSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	if (UCleanableSpawnerComponent* Spawner = FindSpawnerInWorld(GetWorld()))
	{
		Spawner->RegisterSpawnPoint(this);
	}
}

void ACleanableSpawnPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (UCleanableSpawnerComponent* Spawner = FindSpawnerInWorld(GetWorld()))
		{
			Spawner->UnregisterSpawnPoint(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}
