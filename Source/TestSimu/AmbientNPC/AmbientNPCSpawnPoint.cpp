#include "AmbientNPCSpawnPoint.h"
#include "Components/SceneComponent.h"

#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#endif

AAmbientNPCSpawnPoint::AAmbientNPCSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> TargetIcon(TEXT("/Engine/EditorResources/S_TargetPoint"));
		if (TargetIcon.Succeeded())
		{
			Sprite->SetSprite(TargetIcon.Object);
		}
		Sprite->bIsScreenSizeScaled = true;
		Sprite->SetupAttachment(RootComponent);
	}

	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->ArrowColor = FColor::Green;
		Arrow->ArrowSize = 1.f;
		Arrow->SetupAttachment(RootComponent);
	}
#endif
}
