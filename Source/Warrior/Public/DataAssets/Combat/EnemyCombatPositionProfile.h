// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyCombatPositionProfile.generated.h"

/**
 * Shared, designer-authored position rules for one enemy role.
 *
 * Create assets with the DA_ prefix, for example:
 * DA_EnemyCombatPosition_Guardian.
 */
UCLASS(BlueprintType, meta = (DisplayName = "敌人战斗站位配置"))
class WARRIOR_API UEnemyCombatPositionProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Enemies with an attack token use this camera-facing zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front Zone", meta = (DisplayName = "前方区域最近距离", ClampMin = "0.0", Units = "cm"))
	float FrontMinDistance = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front Zone", meta = (DisplayName = "前方区域最远距离", ClampMin = "0.0", Units = "cm"))
	float FrontMaxDistance = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front Zone", meta = (DisplayName = "前方区域半角", ClampMin = "1.0", ClampMax = "179.0", Units = "deg"))
	float FrontHalfAngle = 65.f;

	/** These are guide positions for EQS, not exact final standing points. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Front Zone", meta = (DisplayName = "前方引导位置数量", ClampMin = "1", ClampMax = "16"))
	int32 FrontPositionCount = 4;

	/** Enemies without an attack token use this full ring. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Idle Ring", meta = (DisplayName = "待机圆环最近距离", ClampMin = "0.0", Units = "cm"))
	float IdleMinDistance = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Idle Ring", meta = (DisplayName = "待机圆环最远距离", ClampMin = "0.0", Units = "cm"))
	float IdleMaxDistance = 550.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Idle Ring", meta = (DisplayName = "待机引导位置数量", ClampMin = "1", ClampMax = "32"))
	int32 IdlePositionCount = 12;

	/** Keep an unseen enemy in the same world-space quadrant until seen again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offscreen Zone", meta = (DisplayName = "保持屏幕外世界象限"))
	bool bKeepOffscreenWorldQuadrant = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offscreen Zone", meta = (DisplayName = "屏幕外区域最近距离", ClampMin = "0.0", Units = "cm"))
	float OffscreenMinDistance = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offscreen Zone", meta = (DisplayName = "屏幕外区域最远距离", ClampMin = "0.0", Units = "cm"))
	float OffscreenMaxDistance = 550.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Offscreen Zone", meta = (DisplayName = "屏幕外引导位置数量", ClampMin = "4", ClampMax = "32"))
	int32 OffscreenPositionCount = 12;

	/** Prevent rapid state changes when an enemy is close to a screen edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Screen Check", meta = (DisplayName = "屏幕边缘留白", ClampMin = "0.0", ClampMax = "0.45"))
	float ScreenEdgePadding = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Screen Check", meta = (DisplayName = "离屏确认时间", ClampMin = "0.0", Units = "s"))
	float OffscreenConfirmTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Separation", meta = (DisplayName = "敌人最小间距", ClampMin = "0.0", Units = "cm"))
	float MinimumEnemySpacing = 140.f;

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
};
