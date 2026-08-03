// FaP All Rights Reserve

#include "AI/BTTask_ReleaseAttackToken.h"

#include "AIController.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"

UBTTask_ReleaseAttackToken::UBTTask_ReleaseAttackToken()
{
	NodeName = TEXT("Release Attack Token");
}

EBTNodeResult::Type UBTTask_ReleaseAttackToken::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UEnemyCombatAgentComponent* Agent = ControlledPawn
		? ControlledPawn->FindComponentByClass<UEnemyCombatAgentComponent>()
		: nullptr;
	if (!Agent)
	{
		return EBTNodeResult::Failed;
	}

	Agent->ReleaseAttackToken();
	if (bAlsoReleaseCombatSlot)
	{
		Agent->ReleaseCombatSlot();
	}
	return EBTNodeResult::Succeeded;
}

