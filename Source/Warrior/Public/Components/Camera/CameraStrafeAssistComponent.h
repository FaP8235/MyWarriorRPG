// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/WarriorCombatTypes.h"
#include "CameraStrafeAssistComponent.generated.h"

class USpringArmComponent;
class UAttackAssistComponent;

/**
 * Camera strafe assist / combat framing (参考 God of War GDC 2019 Camera Strafe Assist)。
 *
 * - ① 横移居中：把屏幕内、与玩家交战的敌人的加权群体中心保持在画面里（带角度死区 + 最小修正 + 低通）。
 * - ② 攻击转向：起手攻击时短暂转向当前攻击目标。
 * - ③ 动作在右：交战时给弹簧臂 SocketOffset.Y 叠一个横向偏移。
 * - 与 TargetLock 互斥：TargetLock 激活时本组件的 yaw 修正让位（横向构图仍生效）。
 */
UCLASS(ClassGroup = (Warrior), meta = (BlueprintSpawnableComponent))
class WARRIOR_API UCameraStrafeAssistComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraStrafeAssistComponent();

	/** 由英雄 Input_Look 调用，用于在手动瞄准期间让位 ①。 */
	void NotifyManualLook();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleAttackAssistPrepared(const FWarriorCombatAttackContext& AttackContext, FTransform WarpTargetTransform);

	bool ComputeGroupFocusYaw(const FVector& OwnerLocation, float& OutYaw) const;
	void ApplyYawCorrection(float FocusYaw, float DeltaTime, bool bAttackRecenter);
	void UpdateFramingOffset(float DeltaTime, bool bEngaged);
	void DrawDebugState(float FocusYaw, bool bHasFocus, bool bAttackRecenter) const;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist", meta = (DisplayName = "死区半角", ClampMin = "0.0", Units = "deg"))
	float DeadzoneAngle = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist", meta = (DisplayName = "修正插值速度", ClampMin = "0.1"))
	float CorrectionInterpSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist", meta = (DisplayName = "聚焦低通速度", ClampMin = "0.1"))
	float FocusSmoothSpeed = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist", meta = (DisplayName = "横向构图偏移", ClampMin = "0.0", Units = "cm"))
	float FramingOffsetY = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist", meta = (DisplayName = "横向构图插值速度", ClampMin = "0.1"))
	float FramingInterpSpeed = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist", meta = (DisplayName = "手动瞄准让位时间", ClampMin = "0.0", Units = "s"))
	float ManualLookDebounce = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist", meta = (DisplayName = "距离权重分母", ClampMin = "1.0", Units = "cm"))
	float ProximityDenominator = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist|Attack Recenter", meta = (DisplayName = "攻击转向持续", ClampMin = "0.0", Units = "s"))
	float AttackRecenterDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist|Attack Recenter", meta = (DisplayName = "攻击转向插值速度", ClampMin = "0.1"))
	float AttackRecenterInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Strafe Assist|Attack Recenter", meta = (DisplayName = "攻击转向最大视野角", ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float AttackRecenterMaxViewAngle = 45.f;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CachedSpringArm = nullptr;

	float SmoothedFocusYaw = 0.f;
	bool bHasSmoothedFocus = false;
	float BaseSocketOffsetY = 55.f;
	double LastManualLookTime = -10.0;

	TWeakObjectPtr<AActor> AttackRecenterTarget;
	float AttackRecenterTimer = 0.f;
};
