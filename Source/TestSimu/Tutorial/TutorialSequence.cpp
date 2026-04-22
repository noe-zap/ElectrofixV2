#include "TutorialSequence.h"

FPrimaryAssetId UTutorialSequence::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("TutorialSequence"), GetFName());
}
