// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WarriorThreatRingWidget.generated.h"

/**
 * 英雄腰部威胁圆环（快速验证版）：用 Slate 画圆环 + 三角箭头（无贴图），
 * 指向屏外（身后）最近的若干敌人，按威胁类型颜色上色。
 * 数据复用 UThreatIndicatorComponent（屏外威胁 + 颜色）。
 */
UCLASS()
class WARRIOR_API UWarriorThreatRingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "1.0"))
	float RingRadius = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "4"))
	int32 RingSegments = 48;

	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "1.0"))
	float ArrowSize = 12.f;

	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "0.5"))
	float LineThickness = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Threat Ring", meta = (ClampMin = "1"))
	int32 MaxArrows = 3;

private:
	struct FArrowDraw
	{
		FVector2D Direction;
		FLinearColor Color;
	};

	TArray<FArrowDraw> ArrowsToDraw;
};
