// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WarriorThreatRingWidget.generated.h"

/**
 * 自包含威胁圆环：全部逻辑在 NativePaint 里（每帧必跑、不依赖 tick）。
 * 算威胁数据 + 世界交点（英雄→敌人直线与圆环交点）+ 逆变换 + 画环 + 画箭头。
 */
UCLASS()
class WARRIOR_API UWarriorThreatRingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "4"))
	int32 RingSegments = 48;

	/** 逻辑圆半径（箭头放置的圆的大小）。配套调 WidgetComponent 的 DrawSize。 */
	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "1.0", DisplayName = "圆环半径"))
	float RingRadius = 100.f;

	/** 箭头大小。 */
	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "1.0", DisplayName = "箭头大小"))
	float ArrowSize = 40.f;

	/** 每个箭头的圆弧半角（度），总弧宽 = 2 × 此值。 */
	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "1.0", ClampMax = "90.0", DisplayName = "圆弧半角度"))
	float ArcHalfAngle = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "0.5"))
	float LineThickness = 2.f;
};
