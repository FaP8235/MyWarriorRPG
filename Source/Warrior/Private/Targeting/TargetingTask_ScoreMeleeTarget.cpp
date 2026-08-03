// FaP All Rights Reserve

#include "Targeting/TargetingTask_ScoreMeleeTarget.h"

#include "Components/Combat/MeleeTargetingComponent.h"
#include "Types/TargetingSystemTypes.h"

UTargetingTask_ScoreMeleeTarget::UTargetingTask_ScoreMeleeTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAscending = false;
	bStableSort = true;
}

float UTargetingTask_ScoreMeleeTarget::GetScoreForTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	const UMeleeTargetingComponent* TargetingComponent =
		SourceContext ? Cast<UMeleeTargetingComponent>(SourceContext->SourceObject) : nullptr;

	return TargetingComponent
		? TargetingComponent->CalculateTargetScore(TargetData.HitResult.GetActor())
		: 0.f;
}

