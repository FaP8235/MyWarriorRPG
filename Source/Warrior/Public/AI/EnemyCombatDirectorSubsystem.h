// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Combat/WarriorCombatTypes.h"
#include "EnemyCombatDirectorSubsystem.generated.h"

class UEnemyCombatAgentComponent;

/**
 * World-level arbitration for enemies attacking the same target.
 *
 * Behavior Trees still decide what each enemy wants to do. The director only
 * grants scarce attack tokens and reserves logical positions around a target.
 */
UCLASS()
class WARRIOR_API UEnemyCombatDirectorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }

	UFUNCTION(BlueprintCallable, Category = "Warrior|AI|Combat Director")
	void ConfigureTokenBudget(int32 NewMaxTokenBudget);

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat Director")
	int32 GetMaxTokenBudget() const;

	UFUNCTION(BlueprintPure, Category = "Warrior|AI|Combat Director")
	int32 GetUsedTokenBudgetForTarget(AActor* TargetActor) const;

	/** On-screen combat agents currently targeting the given actor (e.g. the player). */
	void GetOnScreenAgentsTargeting(AActor* TargetActor, TArray<UEnemyCombatAgentComponent*>& OutAgents) const;

	/** 计算 Agent 因近身其它敌人而产生的斥力滑移量（XY，半重叠量、有上限）。有重叠返回 true。 */
	bool GetSeparationDelta(const UEnemyCombatAgentComponent* Agent, FVector& OutDelta) const;

	void RegisterAgent(UEnemyCombatAgentComponent* Agent);
	void UnregisterAgent(UEnemyCombatAgentComponent* Agent);
	void QueueAttackRequest(UEnemyCombatAgentComponent* Agent, int32 TokenCost, float LeaseDuration);
	void CancelAttackRequest(UEnemyCombatAgentComponent* Agent);
	void ReleaseAttackToken(UEnemyCombatAgentComponent* Agent);

	bool ReserveCombatPosition(
		UEnemyCombatAgentComponent* Agent,
		FWarriorCombatPositionHandle& OutHandle,
		FVector& OutWorldLocation);

	void ReleaseCombatPosition(UEnemyCombatAgentComponent* Agent);
	bool GetReservedCombatPositionLocation(
		const UEnemyCombatAgentComponent* Agent,
		FVector& OutWorldLocation) const;
	bool IsLocationSeparatedFromOtherAgents(
		const UEnemyCombatAgentComponent* Agent,
		const FVector& Location,
		float MinimumSpacing) const;

	/** Legacy API kept for existing Behavior Tree assets. */
	bool ReserveCombatSlot(
		UEnemyCombatAgentComponent* Agent,
		int32 SlotCount,
		float SlotRadius,
		FWarriorCombatSlotHandle& OutHandle,
		FVector& OutWorldLocation);

	void ReleaseCombatSlot(UEnemyCombatAgentComponent* Agent);
	bool GetReservedCombatSlotLocation(
		const UEnemyCombatAgentComponent* Agent,
		float SlotRadius,
		FVector& OutWorldLocation) const;

private:
	struct FAttackLease
	{
		TWeakObjectPtr<UEnemyCombatAgentComponent> Agent;
		FWarriorAttackTokenHandle Handle;
		double ExpirationTime = 0.0;
	};

	struct FTargetCombatPool
	{
		TWeakObjectPtr<AActor> TargetActor;
		TMap<int32, FAttackLease> AttackLeases;
		TMap<int32, TWeakObjectPtr<UEnemyCombatAgentComponent>> PositionOccupants;
	};

	void CleanupInvalidState(double CurrentTime);
	void GrantQueuedAttackTokens(double CurrentTime);
	void DrawDebugState();
	int32 GetUsedTokenBudget(const FTargetCombatPool& Pool) const;
	FTargetCombatPool& FindOrAddPool(AActor* TargetActor);
	FVector CalculateCombatPositionLocation(
		const UEnemyCombatAgentComponent* Agent,
		EWarriorEnemyPositionZone Zone,
		int32 PositionIndex) const;
	int32 GetPositionCount(
		const UEnemyCombatAgentComponent* Agent,
		EWarriorEnemyPositionZone Zone) const;
	FVector CalculateSlotLocation(
		AActor* TargetActor,
		int32 SlotIndex,
		int32 SlotCount,
		float SlotRadius) const;

	TSet<TWeakObjectPtr<UEnemyCombatAgentComponent>> RegisteredAgents;
	TMap<TWeakObjectPtr<AActor>, FTargetCombatPool> TargetPools;

	int32 MaxTokenBudget = 3;
	int32 NextTokenId = 1;
	float DebugDrawAccumulator = 0.f;
};
