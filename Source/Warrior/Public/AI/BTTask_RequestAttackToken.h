// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_RequestAttackToken.generated.h"

class UEnemyCombatAgentComponent;

/**
 * Queues an attack request and remains latent until the Combat Director grants
 * a token or the configurable timeout is reached.
 */
UCLASS()
class WARRIOR_API UBTTask_RequestAttackToken : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RequestAttackToken();

	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Token", meta = (ClampMin = "1"))
	int32 TokenCost = 1;

	UPROPERTY(EditAnywhere, Category = "Token", meta = (ClampMin = "0.1", Units = "s"))
	float LeaseDuration = 4.f;

	UPROPERTY(EditAnywhere, Category = "Token", meta = (ClampMin = "0.0", Units = "s"))
	float WaitTimeout = 3.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEnemyCombatAgentComponent> WaitingAgent = nullptr;

	double RequestStartTime = 0.0;
};

