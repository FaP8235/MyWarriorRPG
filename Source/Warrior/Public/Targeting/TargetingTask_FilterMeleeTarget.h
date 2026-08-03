// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingFilterTask_BasicFilterTemplate.h"
#include "TargetingTask_FilterMeleeTarget.generated.h"

/** Filters Targeting System results through UMeleeTargetingComponent rules. */
UCLASS(EditInlineNew, BlueprintType)
class WARRIOR_API UTargetingTask_FilterMeleeTarget : public UTargetingFilterTask_BasicFilterTemplate
{
	GENERATED_BODY()

protected:
	virtual bool ShouldFilterTarget(
		const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData) const override;
};

