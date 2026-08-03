// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_EnemySeparation.generated.h"

/** 敌人间分离斥力：idle 时按间隔检测近身敌人，2D 距离 < 双方半径之和则互相滑开 + 重选站位。 */
UCLASS(meta = (DisplayName = "敌人分离斥力"))
class WARRIOR_API UBTService_EnemySeparation : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_EnemySeparation();

	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	virtual FString GetStaticDescription() const override;
};
