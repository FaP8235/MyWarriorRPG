// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "Combat/WarriorCombatTypes.h"
#include "MeleeTargetingComponent.generated.h"

class UDataAsset_CombatAttackProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMeleeTargetChangedDelegate, AActor*, NewTarget, float, TargetScore);

/**
 * Thin gameplay wrapper around UE's Targeting System.
 *
 * The configured TargetingPreset owns candidate selection/filter/sort tasks.
 * This component supplies Warrior-specific validation and scoring, freezes the
 * selected target into an attack context, and provides a collision-query
 * fallback while a preset is not assigned.
 */
UCLASS(ClassGroup = (Warrior), meta = (BlueprintSpawnableComponent))
class WARRIOR_API UMeleeTargetingComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	UMeleeTargetingComponent();

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat|Targeting")
	bool SelectMeleeTarget(
		UDataAsset_CombatAttackProfile* AttackProfile,
		FVector2D InputIntent,
		FWarriorCombatAttackContext& OutAttackContext);

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat|Targeting")
	void SetExplicitTarget(AActor* NewExplicitTarget);

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat|Targeting")
	void ClearExplicitTarget();

	/** Gives the next unlocked melee attack a short-lived priority target. */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat|Targeting", meta = (DisplayName = "设置弹反目标"))
	void SetCounterAttackTarget(AActor* NewCounterAttackTarget);

	UFUNCTION(BlueprintCallable, Category = "Warrior|Combat|Targeting", meta = (DisplayName = "清除弹反目标"))
	void ClearCounterAttackTarget();

	UFUNCTION(BlueprintPure, Category = "Warrior|Combat|Targeting")
	AActor* GetCurrentTarget() const;

	UFUNCTION(BlueprintPure, Category = "Warrior|Combat|Targeting")
	AActor* GetExplicitTarget() const;

	UFUNCTION(BlueprintPure, Category = "Warrior|Combat|Targeting", meta = (DisplayName = "获取弹反目标"))
	AActor* GetCounterAttackTarget() const;

	UFUNCTION(BlueprintPure, Category = "Warrior|Combat|Targeting")
	bool IsTargetValid(AActor* CandidateTarget) const;

	float CalculateTargetScore(AActor* CandidateTarget) const;

	FWarriorMeleeTargetScoreBreakdown CalculateTargetScoreBreakdown(
		AActor* CandidateTarget) const;

	const UDataAsset_CombatAttackProfile* GetActiveAttackProfile() const;

	UPROPERTY(BlueprintAssignable, Category = "Warrior|Combat|Targeting")
	FOnMeleeTargetChangedDelegate OnMeleeTargetChanged;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	TObjectPtr<UDataAsset_CombatAttackProfile> DefaultAttackProfile = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting")
	TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting|Counter Attack",
		meta = (DisplayName = "弹反目标有效时间", ClampMin = "0.0", Units = "s"))
	float CounterAttackTargetDuration = 1.25f;

private:
	bool IsBasicTargetValid(AActor* CandidateTarget) const;
	bool IsCounterAttackTargetValid(AActor* CandidateTarget) const;

	bool GatherPresetCandidates(
		const UDataAsset_CombatAttackProfile* AttackProfile,
		TArray<AActor*>& OutCandidates) const;

	void GatherFallbackCandidates(
		const UDataAsset_CombatAttackProfile* AttackProfile,
		TArray<AActor*>& OutCandidates) const;

	bool HasLineOfSightToTarget(AActor* CandidateTarget) const;
	APlayerController* GetOwningPlayerController() const;
	void DrawTargetingDebug(
		const TArray<AActor*>& Candidates,
		AActor* SelectedTarget,
		bool bUsedCounterAttackTarget) const;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ExplicitTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CounterAttackTarget;

	float CounterAttackTargetExpireTime = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UDataAsset_CombatAttackProfile> ActiveAttackProfile = nullptr;

	FVector2D ActiveInputIntent = FVector2D::ZeroVector;
};
