// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "Combat/WarriorCombatTypes.h"
#include "AttackAssistComponent.generated.h"

class UDataAsset_CombatAttackProfile;
class UMotionWarpingComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAttackAssistPreparedDelegate,
	const FWarriorCombatAttackContext&,
	AttackContext,
	FTransform,
	WarpTargetTransform);

/**
 * A small, replaceable facade over UE Motion Warping.
 * It does not move the character every frame; it only computes and publishes
 * the named warp target consumed by montage Motion Warping notify windows.
 */
UCLASS(ClassGroup = (Warrior), meta = (BlueprintSpawnableComponent))
class WARRIOR_API UAttackAssistComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	UAttackAssistComponent();

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat|Attack Assist")
	bool PrepareAttackAssist(
		const FWarriorCombatAttackContext& AttackContext,
		FTransform& OutWarpTargetTransform);

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat|Attack Assist")
	bool RefreshAttackAssistTarget(FTransform& OutWarpTargetTransform);

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat|Attack Assist")
	void ClearAttackAssist();

	UFUNCTION(BlueprintPure, Category = "Warrior|Combat|Attack Assist")
	FWarriorCombatAttackContext GetActiveAttackContext() const;

	UPROPERTY(BlueprintAssignable, Category = "Warrior|Combat|Attack Assist")
	FOnAttackAssistPreparedDelegate OnAttackAssistPrepared;

protected:
	virtual void BeginPlay() override;

private:
	bool BuildWarpTargetTransform(
		const FWarriorCombatAttackContext& AttackContext,
		const UDataAsset_CombatAttackProfile* AttackProfile,
		FTransform& OutTransform) const;

	void DrawAttackAssistDebug(
		const FWarriorCombatAttackContext& AttackContext,
		const UDataAsset_CombatAttackProfile* AttackProfile,
		const FTransform& WarpTargetTransform,
		bool bAccepted) const;

	UPROPERTY(Transient)
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent = nullptr;

	UPROPERTY(Transient)
	FWarriorCombatAttackContext ActiveAttackContext;

	UPROPERTY(Transient)
	FName ActiveWarpTargetName = NAME_None;
};
