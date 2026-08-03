// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "Combat/WarriorCombatTypes.h"
#include "GameplayTagContainer.h"
#include "EnemyCombatAgentComponent.generated.h"

class UEnemyCombatDirectorSubsystem;
class UEnemyCombatPositionProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnEnemyCombatStateChangedDelegate,
	EWarriorEnemyCombatState,
	PreviousState,
	EWarriorEnemyCombatState,
	NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAttackTokenChangedDelegate,
	bool,
	bHasToken,
	FWarriorAttackTokenHandle,
	TokenHandle);

/**
 * Per-enemy adapter used by Behavior Trees and Gameplay Abilities.
 * Group-wide decisions are delegated to UEnemyCombatDirectorSubsystem.
 */
UCLASS(ClassGroup = (Warrior), meta = (BlueprintSpawnableComponent))
class WARRIOR_API UEnemyCombatAgentComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	UEnemyCombatAgentComponent();

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void SetCombatTarget(AActor* NewCombatTarget);

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat")
	AActor* GetCombatTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	bool RequestAttackToken(int32 TokenCost = 1, float LeaseDuration = 4.f);

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void CancelAttackTokenRequest();

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void ReleaseAttackToken();

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat")
	bool HasAttackToken() const;

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat")
	bool IsAttackRequestQueued() const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat Position")
	bool ReserveCombatPosition(FVector& OutPositionLocation);

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat Position")
	void ReleaseCombatPosition();

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat Position")
	bool GetReservedCombatPositionLocation(FVector& OutPositionLocation) const;

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat Position")
	EWarriorEnemyPositionZone GetCombatPositionZone() const;

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat Position")
	bool IsOnPlayerScreen() const;

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat Position")
	bool IsCombatPositionLocationAllowed(FVector Location) const;

	/** Releases all shared combat resources without waiting for actor destruction. */
	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void LeaveCombat();

	/** Legacy Blueprint entry points. Use the CombatPosition versions in new assets. */
	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	bool ReserveCombatSlot(FVector& OutSlotLocation);

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void ReleaseCombatSlot();

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat")
	bool GetReservedCombatSlotLocation(FVector& OutSlotLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void SetCombatState(EWarriorEnemyCombatState NewState);

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat")
	EWarriorEnemyCombatState GetCombatState() const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void UpdateAggressionScore();

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat")
	float GetAggressionScore() const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Aggression")
	void SetCanBecomeAggressive(bool bCanBecomeAggressive);

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Aggression")
	bool CanBecomeAggressive() const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Aggression")
	void SetTargetedByPlayer(bool bNewTargetedByPlayer);

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Aggression")
	bool IsTargetedByPlayer() const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void SetThreatIndicatorState(
		EWarriorThreatIndicatorType ThreatType,
		float Priority = 0.f,
		float Duration = -1.f);

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat")
	void ClearThreatIndicator();

	int32 GetQueuedTokenCost() const;
	float GetQueuedLeaseDuration() const;
	int32 GetCombatSlotCount() const;
	float GetCombatSlotRadius() const;
	float GetSeparationRadius() const { return SeparationRadius; }
	int32 GetReservedCombatSlotIndex() const;
	int32 GetAttackTokenCost() const;
	int32 GetEffectiveAggressionPriority() const;
	double GetAttackRequestWaitingTime() const;
	const UEnemyCombatPositionProfile* GetCombatPositionProfile() const;
	EWarriorWorldQuadrant GetOffscreenWorldQuadrant() const;
	void RefreshScreenState();

	void HandleAttackRequestQueued(int32 TokenCost, float LeaseDuration);
	void HandleAttackRequestCancelled();
	void HandleAttackTokenGranted(const FWarriorAttackTokenHandle& NewHandle);
	void HandleAttackTokenRevoked();
	void HandleCombatSlotReleased();

	UPROPERTY(BlueprintAssignable, Category = "Warrior|AI|Combat")
	FOnEnemyCombatStateChangedDelegate OnCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Warrior|AI|Combat")
	FOnAttackTokenChangedDelegate OnAttackTokenChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aggression", meta = (ClampMin = "0.0"))
	float BaseAggression = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aggression", meta = (DisplayName = "进攻优先级", ClampMin = "0"))
	int32 AggressionPriority = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aggression", meta = (DisplayName = "优先级生效距离", ClampMin = "0.0", Units = "cm"))
	float AggressionPriorityDistance = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aggression", meta = (ClampMin = "0.0"))
	float DistanceWeight = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aggression", meta = (ClampMin = "0.0"))
	float WaitingTimeWeight = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aggression", meta = (ClampMin = "0.0"))
	float FacingWeight = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aggression", meta = (DisplayName = "屏幕内权重", ClampMin = "0.0"))
	float OnScreenWeight = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aggression", meta = (ClampMin = "1.0", Units = "cm"))
	float PreferredAttackDistance = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Compatibility", AdvancedDisplay, meta = (DisplayName = "旧版站位数量（无配置时使用）", ClampMin = "1"))
	int32 CombatSlotCount = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Compatibility", AdvancedDisplay, meta = (DisplayName = "旧版站位半径（无配置时使用）", ClampMin = "0.0", Units = "cm"))
	float CombatSlotRadius = 350.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Position", meta = (DisplayName = "战斗站位配置"))
	TObjectPtr<UEnemyCombatPositionProfile> CombatPositionProfile = nullptr;

	/** power-play：同一 token 持有者被连续打断超过此次数则强制归还 token（换人上）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Director", meta = (DisplayName = "连晕暂留上限", ClampMin = "1"))
	int32 HitReactRetentionCap = 4;

	/** 分离斥力检测半径（按体型）；与其它敌人 2D 距离 < 双方半径之和时互相斥开。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Separation", meta = (DisplayName = "分离半径", ClampMin = "0.0", Units = "cm"))
	float SeparationRadius = 100.f;

private:
	UEnemyCombatDirectorSubsystem* GetCombatDirector() const;
	void RegisterIdleThreat();
	void BindDeadStateEvent();
	void HandleDeadStateChanged(const FGameplayTag ChangedTag, int32 NewCount);
	void BindHitReactTagEvent();
	void HandleHitReactTagChanged(const FGameplayTag ChangedTag, int32 NewCount);
	EWarriorWorldQuadrant CalculateCurrentWorldQuadrant() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CombatTarget;

	UPROPERTY(Transient)
	EWarriorEnemyCombatState CombatState = EWarriorEnemyCombatState::NonAggressive;

	UPROPERTY(Transient)
	FWarriorAttackTokenHandle AttackTokenHandle;

	UPROPERTY(Transient)
	FWarriorCombatPositionHandle CombatPositionHandle;

	float AggressionScore = 0.f;
	double AttackRequestStartTime = 0.0;
	double FirstOffscreenTime = -1.0;
	int32 QueuedTokenCost = 1;
	float QueuedLeaseDuration = 4.f;
	bool bAttackRequestQueued = false;
	bool bCanBecomeAggressive = true;
	bool bTargetedByPlayer = false;
	bool bIsOnPlayerScreen = true;
	bool bHasScreenState = false;
	EWarriorWorldQuadrant OffscreenWorldQuadrant =
		EWarriorWorldQuadrant::PositiveXPositiveY;
	FDelegateHandle DeadStateEventHandle;
	FDelegateHandle HitReactTagEventHandle;
	int32 ConsecutiveHitReactCount = 0;
};
