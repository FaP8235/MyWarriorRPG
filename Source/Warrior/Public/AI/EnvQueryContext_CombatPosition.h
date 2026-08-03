// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_CombatPosition.generated.h"

/** Exposes the querier's current combat guide position to EQS. */
UCLASS(meta = (DisplayName = "Combat Position"))
class WARRIOR_API UEnvQueryContext_CombatPosition : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(
		FEnvQueryInstance& QueryInstance,
		FEnvQueryContextData& ContextData) const override;
};

