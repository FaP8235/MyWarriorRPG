// FaP All Rights Reserve

#include "AI/EnvQueryContext_CombatSlot.h"

#include "AIController.h"
#include "Components/ActorComponent.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Pawn.h"

namespace
{
	APawn* ResolveQuerierPawn(UObject* QueryOwner)
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

void UEnvQueryContext_CombatSlot::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData) const
{
	APawn* QuerierPawn = ResolveQuerierPawn(QueryInstance.Owner.Get());
	const UEnemyCombatAgentComponent* Agent = QuerierPawn
		? QuerierPawn->FindComponentByClass<UEnemyCombatAgentComponent>()
		: nullptr;
	if (!Agent)
	{
		return;
	}

	FVector SlotLocation;
	if (Agent->GetReservedCombatSlotLocation(SlotLocation)
		&& !SlotLocation.ContainsNaN())
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, SlotLocation);
	}
}
