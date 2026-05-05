#include "CleanableActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

ACleanableActor::ACleanableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(2.f);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void ACleanableActor::BeginPlay()
{
	Super::BeginPlay();

	if (Mesh && Mesh->GetMaterial(0))
	{
		DynMat = Mesh->CreateDynamicMaterialInstance(0);
	}

	if (Mesh && !CleaningTag.IsNone())
	{
		Mesh->ComponentTags.AddUnique(CleaningTag);
	}

	ApplyOpacityToMaterial();
}

void ACleanableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (Mesh && !CleaningTag.IsNone())
	{
		Mesh->ComponentTags.AddUnique(CleaningTag);
	}
}

void ACleanableActor::ApplyClean(float Amount)
{
	if (!HasAuthority() || Amount <= 0.f)
	{
		return;
	}

	DirtAmount = FMath::Max(0.f, DirtAmount - Amount);
	ApplyOpacityToMaterial();

	if (DirtAmount < 0.15f)
	{
		Destroy();
	}
}

void ACleanableActor::OnRep_DirtAmount()
{
	ApplyOpacityToMaterial();
}

void ACleanableActor::ApplyOpacityToMaterial()
{
	if (DynMat)
	{
		DynMat->SetScalarParameterValue(OpacityParameterName, DirtAmount);
	}
}

void ACleanableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACleanableActor, DirtAmount);
}
