// FaP All Rights Reserve

#include "AI/BTTask_ReserveCombatSlot.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"

UBTTask_ReserveCombatSlot::UBTTask_ReserveCombatSlot()
{
	NodeName = TEXT("Reserve Combat Slot");
	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey),
		AActor::StaticClass());
	SlotLocationKey.AddVectorFilter(
		this,
		GET_MEMBER_NAME_CHECKED(ThisClass, SlotLocationKey));
}

EBTNodeResult::Type UBTTask_ReserveCombatSlot::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UEnemyCombatAgentComponent* Agent = ControlledPawn
		? ControlledPawn->FindComponentByClass<UEnemyCombatAgentComponent>()
		: nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Agent || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	Agent->SetCombatTarget(
		Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)));

	FVector SlotLocation;
	if (!Agent->ReserveCombatSlot(SlotLocation))
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsVector(SlotLocationKey.SelectedKeyName, SlotLocation);
	return EBTNodeResult::Succeeded;
}

