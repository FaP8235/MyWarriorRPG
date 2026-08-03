// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/WarriorCombatTypes.h"
#include "DataAsset_CombatAttackProfile.generated.h"

class UTargetingPreset;

/**
 * Per-attack targeting and assistance tuning. One profile can be shared by
 * several Gameplay Abilities, while special attacks can override it.
 */
UCLASS(BlueprintType, meta = (DisplayName = "战斗攻击配置"))
class WARRIOR_API UDataAsset_CombatAttackProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "目标选择", meta = (DisplayName = "目标选择预设"))
	TObjectPtr<UTargetingPreset> TargetingPreset = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "目标选择", meta = (DisplayName = "最大目标识别距离", ClampMin = "0.0", Units = "cm"))
	float MaxTargetDistance = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "目标选择", meta = (DisplayName = "最大目标识别角度", ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float MaxTargetAngleDegrees = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "目标选择", meta = (DisplayName = "要求视线无遮挡"))
	bool bRequireLineOfSight = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "目标选择", meta = (DisplayName = "允许无目标攻击"))
	bool bAllowAttackWithoutTarget = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "目标选择", meta = (DisplayName = "目标评分权重"))
	FWarriorMeleeTargetingWeights TargetingWeights;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击吸附", meta = (DisplayName = "攻击吸附模式"))
	EWarriorAttackAssistMode AttackAssistMode = EWarriorAttackAssistMode::MotionWarp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击吸附", meta = (DisplayName = "吸附目标名称"))
	FName WarpTargetName = TEXT("AttackTarget");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击吸附", meta = (DisplayName = "理想攻击距离", ClampMin = "0.0", Units = "cm"))
	float IdealAttackDistance = 125.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击吸附", meta = (DisplayName = "最大吸附距离", ClampMin = "0.0", Units = "cm"))
	float MaxAssistDistance = 650.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击吸附", meta = (DisplayName = "最大吸附角度", ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float MaxAssistAngleDegrees = 65.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击吸附", meta = (DisplayName = "忽略Z轴"))
	bool bIgnoreZAxis = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "攻击吸附", meta = (DisplayName = "目标位置偏移"))
	FVector TargetOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Strike Assist", meta = (DisplayName = "命中拉回强度", ClampMin = "0.0", ClampMax = "1.0"))
	float StrikeAssistStrength = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Strike Assist", meta = (DisplayName = "命中拉回距离", ClampMin = "0.0", Units = "cm"))
	float StrikeAssistWarpDistance = 50.f;
};
