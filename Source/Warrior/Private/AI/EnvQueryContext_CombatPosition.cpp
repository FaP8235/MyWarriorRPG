// FaP All Rights Reserve

#include "AI/EnvQueryContext_CombatPosition.h"

#include "AIController.h"
#include "Components/ActorComponent.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"

namespace
{
	APawn* ResolveCombatPositionQuerier(UObject* QueryOwner)
	{
		if (APawn* Pawn = Cast<APawn>(QueryOwner))
		{
			return Pawn;
		}

		if (const AAIController* AIController = Cast<AAIController>(QueryOwner))
		{
			return AIController->GetPawn();
		}

		if (const UActorComponent* ActorComponent = Cast<UActorComponent>(QueryOwner))
		{
			return Cast<APawn>(ActorComponent->GetOwner());
		}

		return nullptr;
	}
}

void UEnvQueryContext_CombatPosition::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData) const
{
	APawn* QuerierPawn = ResolveCombatPositionQuerier(QueryInstance.Owner.Get());
	const UEnemyCombatAgentComponent* Agent = QuerierPawn
		? QuerierPawn->FindComponentByClass<UEnemyCombatAgentComponent>()
		: nullptr;
	if (!Agent)
	{
		return;
	}

	FVector PositionLocation;
	if (Agent->GetReservedCombatPositionLocation(PositionLocation)
		&& !PositionLocation.ContainsNaN())
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, PositionLocation);
	}
}

