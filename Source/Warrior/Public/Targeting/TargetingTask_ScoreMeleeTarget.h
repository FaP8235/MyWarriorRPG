// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingSortTask_Base.h"
#include "TargetingTask_ScoreMeleeTarget.generated.h"

/** Adds Warrior camera/input/distance/stickiness score and sorts best first. */
UCLASS(EditInlineNew, BlueprintType)
class WARRIOR_API UTargetingTask_ScoreMeleeTarget : public UTargetingSortTask_Base
{
	GENERATED_BODY()

public:
	UTargetingTask_ScoreMeleeTarget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual float GetScoreForTarget(
		const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData) const override;
};

