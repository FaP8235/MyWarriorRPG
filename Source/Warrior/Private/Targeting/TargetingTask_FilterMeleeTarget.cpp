// FaP All Rights Reserve

#include "Targeting/TargetingTask_FilterMeleeTarget.h"

#include "Components/Combat/MeleeTargetingComponent.h"
#include "Types/TargetingSystemTypes.h"

bool UTargetingTask_FilterMeleeTarget::ShouldFilterTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	const UMeleeTargetingComponent* TargetingComponent =
		SourceContext ? Cast<UMeleeTargetingComponent>(SourceContext->SourceObject) : nullptr;
	AActor* Candidate = TargetData.HitResult.GetActor();

	return !TargetingComponent || !TargetingComponent->IsTargetValid(Candidate);
}

