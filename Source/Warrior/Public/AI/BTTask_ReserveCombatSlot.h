// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ReserveCombatSlot.generated.h"

/**
 * Reserves a logical slot from the Combat Director and writes its current
 * world position to Blackboard. Follow it with UE's built-in Move To or use
 * the result as an EQS context/parameter.
 */
UCLASS()
class WARRIOR_API UBTTask_ReserveCombatSlot : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ReserveCombatSlot();

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector SlotLocationKey;
};

