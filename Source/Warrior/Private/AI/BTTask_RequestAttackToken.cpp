// FaP All Rights Reserve

#include "AI/BTTask_RequestAttackToken.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"

UBTTask_RequestAttackToken::UBTTask_RequestAttackToken()
{
	NodeName = TEXT("Request Attack Token");
	bNotifyTick = true;
	bCreateNodeInstance = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey),
		AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_RequestAttackToken::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	WaitingAgent = ControlledPawn
		? ControlledPawn->FindComponentByClass<UEnemyCombatAgentComponent>()
		: nullptr;
	if (!WaitingAgent)
	{
		return EBTNodeResult::Failed;
	}

	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		WaitingAgent->SetCombatTarget(
			Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName)));
	}

	if (!IsValid(WaitingAgent->GetCombatTarget()))
	{
		WaitingAgent = nullptr;
		return EBTNodeResult::Failed;
	}

	RequestStartTime = OwnerComp.GetWorld()
		? OwnerComp.GetWorld()->GetTimeSeconds()
		: 0.0;
	WaitingAgent->RequestAttackToken(TokenCost, LeaseDuration);
	return WaitingAgent->HasAttackToken()
		? EBTNodeResult::Succeeded
		: EBTNodeResult::InProgress;
}

void UBTTask_RequestAttackToken::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	const float DeltaSeconds)
{
	if (!IsValid(WaitingAgent))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (WaitingAgent->HasAttackToken())
	{
		WaitingAgent = nullptr;
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const double CurrentTime = OwnerComp.GetWorld()
		? OwnerComp.GetWorld()->GetTimeSeconds()
		: RequestStartTime;
	if (WaitTimeout > 0.f && CurrentTime - RequestStartTime >= WaitTimeout)
	{
		WaitingAgent->CancelAttackTokenRequest();
		WaitingAgent = nullptr;
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

EBTNodeResult::Type UBTTask_RequestAttackToken::AbortTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	if (IsValid(WaitingAgent))
	{
		if (WaitingAgent->HasAttackToken())
		{
			WaitingAgent->ReleaseAttackToken();
		}
		else
		{
			WaitingAgent->CancelAttackTokenRequest();
		}
	}
	WaitingAgent = nullptr;
	return EBTNodeResult::Aborted;
}

