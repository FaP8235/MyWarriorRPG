// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_UpdateCombatSlotLocation.generated.h"

/**
 * Reserves a logical combat slot when its branch becomes relevant and keeps
 * the Blackboard location synchronized as the combat target moves.
 */
UCLASS()
class WARRIOR_API UBTService_UpdateCombatSlotLocation : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateCombatSlotLocation();

	virtual void OnSearchStart(FBehaviorTreeSearchData& SearchData) override;

	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector SlotLocationKey;

private:
	void UpdateSlotLocation(UBehaviorTreeComponent& OwnerComp) const;
};
