// FaP All Rights Reserve

#include "AI/BTService_UpdateCombatPosition.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"

UBTService_UpdateCombatPosition::UBTService_UpdateCombatPosition()
{
	NodeName = TEXT("Update Combat Position");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bRestartTimerOnEachActivation = true;

	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey),
		AActor::StaticClass());
	CombatPositionLocationKey.AddVectorFilter(
		this,
		GET_MEMBER_NAME_CHECKED(ThisClass, CombatPositionLocationKey));

	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UBTService_UpdateCombatPosition::OnSearchStart(
	FBehaviorTreeSearchData& SearchData)
{
	Super::OnSearchStart(SearchData);
	UpdateCombatPosition(SearchData.OwnerComp);
}

void UBTService_UpdateCombatPosition::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	const float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateCombatPosition(OwnerComp);
}

FString UBTService_UpdateCombatPosition::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Update position around %s\nWrite location to %s"),
		*TargetActorKey.SelectedKeyName.ToString(),
		*CombatPositionLocationKey.SelectedKeyName.ToString());
}

void UBTService_UpdateCombatPosition::UpdateCombatPosition(
	UBehaviorTreeComponent& OwnerComp) const
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UEnemyCombatAgentComponent* Agent = ControlledPawn
		? ControlledPawn->FindComponentByClass<UEnemyCombatAgentComponent>()
		: nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Agent || !Blackboard)
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(
		Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	Agent->SetCombatTarget(TargetActor);

	FVector PositionLocation;
	if (IsValid(TargetActor)
		&& Agent->ReserveCombatPosition(PositionLocation))
	{
		Blackboard->SetValueAsVector(
			CombatPositionLocationKey.SelectedKeyName,
			PositionLocation);
	}
	else
	{
		Blackboard->ClearValue(CombatPositionLocationKey.SelectedKeyName);
	}
}

