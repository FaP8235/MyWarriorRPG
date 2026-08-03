// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_CombatSlot.generated.h"

/**
 * Exposes the querier's currently reserved logical combat slot to EQS.
 * The context is deliberately read-only: Behavior Tree tasks/services own
 * slot reservation, while EQS only uses the location for filtering/scoring.
 */
UCLASS(meta = (DisplayName = "Combat Slot"))
class WARRIOR_API UEnvQueryContext_CombatSlot : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(
		FEnvQueryInstance& QueryInstance,
		FEnvQueryContextData& ContextData) const override;
};
