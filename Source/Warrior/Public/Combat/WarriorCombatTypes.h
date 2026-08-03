// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "WarriorCombatTypes.generated.h"

class AActor;
class UDataAsset_CombatAttackProfile;

UENUM(BlueprintType)
enum class EWarriorAttackAssistMode : uint8
{
	None UMETA(DisplayName = "关闭"),
	RotationOnly UMETA(DisplayName = "仅旋转"),
	MotionWarp UMETA(DisplayName = "位移与旋转（Motion Warp）")
};

UENUM(BlueprintType)
enum class EWarriorEnemyCombatState : uint8
{
	NonAggressive UMETA(DisplayName = "非进攻状态"),
	Positioning UMETA(DisplayName = "调整站位"),
	Queued UMETA(DisplayName = "等待进攻许可"),
	Aggressive UMETA(DisplayName = "允许进攻"),
	Attacking UMETA(DisplayName = "正在进攻"),
	Recovering UMETA(DisplayName = "攻击后恢复"),
	HitReact UMETA(DisplayName = "受击反应")
};

/** The simple, designer-facing position zones used by enemy combat AI. */
UENUM(BlueprintType)
enum class EWarriorEnemyPositionZone : uint8
{
	None UMETA(DisplayName = "无站位区域"),
	IdleRing UMETA(DisplayName = "外围待机圆环"),
	FrontZone UMETA(DisplayName = "镜头前方进攻区"),
	OffscreenZone UMETA(DisplayName = "屏幕外保持区")
};

/** World-space quadrants are intentionally independent of camera rotation. */
UENUM(BlueprintType)
enum class EWarriorWorldQuadrant : uint8
{
	PositiveXPositiveY UMETA(DisplayName = "世界 +X / +Y"),
	NegativeXPositiveY UMETA(DisplayName = "世界 -X / +Y"),
	NegativeXNegativeY UMETA(DisplayName = "世界 -X / -Y"),
	PositiveXNegativeY UMETA(DisplayName = "世界 +X / -Y")
};

UENUM(BlueprintType)
enum class EWarriorThreatIndicatorType : uint8
{
	NearbyIdle,
	MeleeWindup,
	RangedWindup,
	Projectile
};

USTRUCT(BlueprintType, meta = (DisplayName = "近战目标评分权重"))
struct FWarriorMeleeTargetingWeights
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "目标选择", meta = (DisplayName = "镜头朝向权重", ClampMin = "0.0"))
	float CameraAlignment = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "目标选择", meta = (DisplayName = "屏幕中心权重", ClampMin = "0.0"))
	float ScreenCenter = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "目标选择", meta = (DisplayName = "距离权重", ClampMin = "0.0"))
	float Distance = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "目标选择", meta = (DisplayName = "输入方向权重", ClampMin = "0.0"))
	float InputIntent = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "目标选择", meta = (DisplayName = "目标黏性权重", ClampMin = "0.0"))
	float TargetStickiness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "目标选择", meta = (DisplayName = "屏幕内奖励权重", ClampMin = "0.0"))
	float OnScreenBonus = 0.5f;
};

USTRUCT(BlueprintType)
struct FWarriorMeleeTargetScoreBreakdown
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	float CameraAlignment = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	float ScreenCenter = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	float Distance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	float InputIntent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	float TargetStickiness = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	float OnScreenBonus = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Targeting")
	float Total = 0.f;
};

USTRUCT(BlueprintType)
struct FWarriorCombatAttackContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UDataAsset_CombatAttackProfile> AttackProfile = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float TargetScore = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bHasTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bUsedExplicitTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bUsedCounterAttackTarget = false;

	void Reset()
	{
		*this = FWarriorCombatAttackContext();
	}
};

USTRUCT(BlueprintType)
struct FWarriorAttackTokenHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 Id = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 Cost = 0;

	bool IsValid() const
	{
		return Id != INDEX_NONE;
	}

	void Reset()
	{
		Id = INDEX_NONE;
		Cost = 0;
	}
};

USTRUCT(BlueprintType)
struct FWarriorCombatSlotHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 SlotIndex = INDEX_NONE;

	bool IsValid() const
	{
		return SlotIndex != INDEX_NONE;
	}

	void Reset()
	{
		SlotIndex = INDEX_NONE;
	}
};

/** Reservation handle for a logical guide position inside a combat zone. */
USTRUCT(BlueprintType)
struct FWarriorCombatPositionHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Position")
	EWarriorEnemyPositionZone Zone = EWarriorEnemyPositionZone::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Position")
	int32 PositionIndex = INDEX_NONE;

	bool IsValid() const
	{
		return Zone != EWarriorEnemyPositionZone::None
			&& PositionIndex != INDEX_NONE;
	}

	void Reset()
	{
		Zone = EWarriorEnemyPositionZone::None;
		PositionIndex = INDEX_NONE;
	}
};

USTRUCT(BlueprintType)
struct FWarriorThreatIndicatorData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Threat Indicator")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Threat Indicator")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Threat Indicator")
	float RotationDegrees = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Threat Indicator")
	float Distance = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Threat Indicator")
	float Priority = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Threat Indicator")
	EWarriorThreatIndicatorType Type = EWarriorThreatIndicatorType::NearbyIdle;

	UPROPERTY(BlueprintReadOnly, Category = "Threat Indicator")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Threat Indicator")
	bool bOffScreen = true;
};
