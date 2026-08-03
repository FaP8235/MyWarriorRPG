// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_CombatPosition.generated.h"

/** Filters EQS points using the enemy's current combat position zone. */
UCLASS(meta = (DisplayName = "Combat Position Rules"))
class WARRIOR_API UEnvQueryTest_CombatPosition : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_CombatPosition(const FObjectInitializer& ObjectInitializer);

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

