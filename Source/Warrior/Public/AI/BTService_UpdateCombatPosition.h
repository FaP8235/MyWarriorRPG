// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_UpdateCombatPosition.generated.h"

/** Keeps the enemy's shared combat position and Blackboard location current. */
UCLASS(meta = (DisplayName = "更新敌人战斗站位"))
class WARRIOR_API UBTService_UpdateCombatPosition : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateCombatPosition();

	virtual void OnSearchStart(FBehaviorTreeSearchData& SearchData) override;
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "目标角色"))
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "战斗站位位置"))
	FBlackboardKeySelector CombatPositionLocationKey;

private:
	void UpdateCombatPosition(UBehaviorTreeComponent& OwnerComp) const;
};

