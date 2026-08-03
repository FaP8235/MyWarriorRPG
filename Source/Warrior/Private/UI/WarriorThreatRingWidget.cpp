// FaP All Rights Reserve

#include "UI/WarriorThreatRingWidget.h"

#include "Components/UI/ThreatIndicatorComponent.h"
#include "Combat/WarriorCombatTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SlateCore.h"

void UWarriorThreatRingWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);
	ArrowsToDraw.Reset();

	APlayerController* PC = GetOwningPlayer();
	APawn* HeroPawn = PC ? PC->GetPawn() : nullptr;
	UThreatIndicatorComponent* ThreatComp = HeroPawn
		? HeroPawn->FindComponentByClass<UThreatIndicatorComponent>()
		: nullptr;
	if (!PC || !HeroPawn || !ThreatComp)
	{
		return;
	}

	TArray<FWarriorThreatIndicatorData> Indicators = ThreatComp->GetCurrentIndicators();
	// 按距离取最近若干个（屏外=身后）
	Indicators.Sort([](const FWarriorThreatIndicatorData& A, const FWarriorThreatIndicatorData& B)
	{
		return A.Distance < B.Distance;
	});

	// 环是世界固定水平面：箭头放在敌人的"世界方位"上（atan2(Y,X)）。
	const FVector HeroLoc = HeroPawn->GetActorLocation();

	const int32 Count = FMath::Min(MaxArrows, Indicators.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		const AActor* Enemy = Indicators[i].SourceActor.Get();
		if (!Enemy)
		{
			continue;
		}

		const FVector To = Enemy->GetActorLocation() - HeroLoc;
		const float Bearing = FMath::Atan2(To.Y, To.X); // 世界方位角
		// 水平 widget(pitch90) 的 2D 映射：U→世界+Y、V→世界+X，故 (sin, cos)
		ArrowsToDraw.Add({ FVector2D(FMath::Sin(Bearing), FMath::Cos(Bearing)), Indicators[i].Color });
	}
}

int32 UWarriorThreatRingWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2D CenterD = AllottedGeometry.GetLocalSize() * 0.5f;
	const FVector2f Center(CenterD.X, CenterD.Y);

	// 圆环（灰）
	TArray<FVector2f> RingPoints;
	RingPoints.SetNumUninitialized(RingSegments + 1);
	for (int32 i = 0; i <= RingSegments; ++i)
	{
		const float A = static_cast<float>(i) / static_cast<float>(RingSegments) * TWO_PI;
		RingPoints[i] = Center + FVector2f(FMath::Cos(A), FMath::Sin(A)) * RingRadius;
	}
	FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), RingPoints, ESlateDrawEffect::None, FLinearColor(0.8f, 0.8f, 0.8f, 0.7f), true, LineThickness);

	// 箭头（三角轮廓，按威胁颜色）
	for (const FArrowDraw& Arrow : ArrowsToDraw)
	{
		const FVector2f Dir(Arrow.Direction.X, Arrow.Direction.Y);
		const FVector2f DirN = Dir.GetSafeNormal();
		const FVector2f Base = Center + DirN * RingRadius;
		const FVector2f Tip = Center + DirN * (RingRadius + ArrowSize);
		const FVector2f Perp(-DirN.Y, DirN.X);
		const FVector2f Bl = Base + Perp * (ArrowSize * 0.5f);
		const FVector2f Br = Base - Perp * (ArrowSize * 0.5f);
		TArray<FVector2f> Tri = { Tip, Bl, Br, Tip };
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Tri, ESlateDrawEffect::None, Arrow.Color, true, LineThickness + 1.f);
	}

	return Layer;
}
