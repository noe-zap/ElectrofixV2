#include "EquippableBase.h"
#include "Net/UnrealNetwork.h"

AEquippableBase::AEquippableBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
}

void AEquippableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	static FProperty* AttachmentReplicationProp = GetReplicatedProperty(
		AActor::StaticClass(),
		AActor::StaticClass(),
		FName(TEXT("AttachmentReplication")));

	if (AttachmentReplicationProp)
	{
		const uint16 RepIndex = AttachmentReplicationProp->RepIndex;
		OutLifetimeProps.RemoveAll([RepIndex](const FLifetimeProperty& Prop)
		{
			return Prop.RepIndex == RepIndex;
		});
	}
}
