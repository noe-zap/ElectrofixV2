#include "CleanableActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogCleaning, Log, All);

static const TCHAR* RoleToString(ENetRole Role)
{
	switch (Role)
	{
		case ROLE_Authority:		return TEXT("Authority");
		case ROLE_AutonomousProxy:	return TEXT("AutonomousProxy");
		case ROLE_SimulatedProxy:	return TEXT("SimulatedProxy");
		default:					return TEXT("None");
	}
}

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

	const bool bHasMesh = Mesh && Mesh->GetStaticMesh() != nullptr;
	const bool bHasMaterial = Mesh && Mesh->GetMaterial(0) != nullptr;

	if (Mesh && Mesh->GetMaterial(0))
	{
		DynMat = Mesh->CreateDynamicMaterialInstance(0);
	}

	if (Mesh && !CleaningTag.IsNone())
	{
		Mesh->ComponentTags.AddUnique(CleaningTag);
	}

	UE_LOG(LogCleaning, Log, TEXT("[Cleanable] BeginPlay %s role=%s loc=%s mesh=%s mat0=%s dynMat=%s tag=%s dirt=%.2f"),
		*GetName(),
		RoleToString(GetLocalRole()),
		*GetActorLocation().ToCompactString(),
		bHasMesh ? TEXT("yes") : TEXT("NO"),
		bHasMaterial ? TEXT("yes") : TEXT("NO"),
		DynMat ? TEXT("yes") : TEXT("NO"),
		*CleaningTag.ToString(),
		DirtAmount);

	if (!bHasMesh)
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Cleanable] %s has NO StaticMesh assigned — actor exists but is invisible."), *GetName());
	}
	if (!bHasMaterial)
	{
		UE_LOG(LogCleaning, Warning, TEXT("[Cleanable] %s has NO material on slot 0 — opacity parameter will not apply."), *GetName());
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

	const float Old = DirtAmount;
	DirtAmount = FMath::Max(0.f, DirtAmount - Amount);
	ApplyOpacityToMaterial();

	UE_LOG(LogCleaning, Verbose, TEXT("[Cleanable] %s ApplyClean %.2f: %.2f -> %.2f"),
		*GetName(), Amount, Old, DirtAmount);

	if (DirtAmount <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogCleaning, Log, TEXT("[Cleanable] %s fully cleaned — destroying."), *GetName());
		Destroy();
	}
}

void ACleanableActor::OnRep_DirtAmount()
{
	UE_LOG(LogCleaning, Verbose, TEXT("[Cleanable] %s OnRep_DirtAmount=%.2f (client side)"), *GetName(), DirtAmount);
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
