// FaP All Rights Reserve

#include "AI/BTService_UpdateCombatSlotLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"
#include "GameFramework/Pawn.h"

UBTService_UpdateCombatSlotLocation::UBTService_UpdateCombatSlotLocation()
{
	NodeName = TEXT("Update Combat Slot Location");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bRestartTimerOnEachActivation = true;

	TargetActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey),
		AActor::StaticClass());
	SlotLocationKey.AddVectorFilter(
		this,
		GET_MEMBER_NAME_CHECKED(ThisClass, SlotLocationKey));

	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UBTService_UpdateCombatSlotLocation::OnSearchStart(
	FBehaviorTreeSearchData& SearchData)
{
	Super::OnSearchStart(SearchData);
	UpdateSlotLocation(SearchData.OwnerComp);
}

void UBTService_UpdateCombatSlotLocation::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	const float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateSlotLocation(OwnerComp);
}

FString UBTService_UpdateCombatSlotLocation::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("Reserve/refresh slot around %s\nWrite location to %s"),
		*TargetActorKey.SelectedKeyName.ToString(),
		*SlotLocationKey.SelectedKeyName.ToString());
}

void UBTService_UpdateCombatSlotLocation::UpdateSlotLocation(
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

	FVector SlotLocation;
	if (IsValid(TargetActor) && Agent->ReserveCombatSlot(SlotLocation))
	{
		Blackboard->SetValueAsVector(
			SlotLocationKey.SelectedKeyName,
			SlotLocation);
	}
	else
	{
		Blackboard->ClearValue(SlotLocationKey.SelectedKeyName);
	}
}
