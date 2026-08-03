// FaP All Rights Reserve

#include "UI/WarriorThreatRingWidget.h"
#include "Components/UI/ThreatIndicatorComponent.h"
#include "Combat/WarriorCombatTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SlateCore.h"

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
	const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;

	// ── Screen-space 投影：用相机的 Right/Up 轴 ──
	TArray<FVector2D> ArrowPos;
	TArray<FLinearColor> ArrowCol;

	APlayerController* PC = GetOwningPlayer();
	APawn* HeroPawn = PC ? PC->GetPawn() : nullptr;
	if (HeroPawn)
	{
		UThreatIndicatorComponent* ThreatComp = HeroPawn->FindComponentByClass<UThreatIndicatorComponent>();
		if (ThreatComp)
		{
			FVector CamLoc; FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			const FVector CamForward = FRotationMatrix(CamRot).GetScaledAxis(EAxis::X);
			const FVector CamRight = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Y);
			const FVector HeroLoc = HeroPawn->GetActorLocation();

			TArray<FWarriorThreatIndicatorData> Indicators = ThreatComp->GetCurrentIndicators();
			Indicators.Sort([](const FWarriorThreatIndicatorData& A, const FWarriorThreatIndicatorData& B)
			{
				return A.Distance < B.Distance;
			});

			const int32 Count = FMath::Min(3, Indicators.Num());
			for (int32 i = 0; i < Count; ++i)
			{
				if (const AActor* Enemy = Indicators[i].SourceActor.Get())
				{
					// 只取水平方向（忽略高度差），投影到相机 Forward/Right
					// → 罗盘方位（前方=环顶）+ 俯仰角自动透视压缩（俯视时环变扁）
					const FVector ToEnemy = Enemy->GetActorLocation() - HeroLoc;
					const FVector DirHoriz(ToEnemy.X, ToEnemy.Y, 0.f);
					const FVector DirN = DirHoriz.GetSafeNormal();

					const float FwdComp = FVector::DotProduct(DirN, CamForward);
					const float RightComp = FVector::DotProduct(DirN, CamRight);
					const float U = RightComp * RingRadius;
					const float V = -FwdComp * RingRadius; // 前方=环顶（UMG Y 向下取负）
					ArrowPos.Add(FVector2D(Center.X + U, Center.Y + V));
					ArrowCol.Add(Indicators[i].Color);
				}
			}
		}
	}

	// ── 画箭头 ──
	for (int32 i = 0; i < ArrowPos.Num() && i < ArrowCol.Num(); ++i)
	{
		const FVector2D Dir2D = (ArrowPos[i] - Center).GetSafeNormal();
		if (Dir2D.IsNearlyZero()) continue;

		// 三角箭头：起点在逻辑圆上，向外延伸
		const FVector2f Tip(
			static_cast<float>(ArrowPos[i].X + Dir2D.X * ArrowSize),
			static_cast<float>(ArrowPos[i].Y + Dir2D.Y * ArrowSize));
		const FVector2f Perp(static_cast<float>(-Dir2D.Y), static_cast<float>(Dir2D.X));
		const FVector2f Base(
			static_cast<float>(ArrowPos[i].X),
			static_cast<float>(ArrowPos[i].Y));
		const FVector2f Bl = Base + Perp * (ArrowSize * 0.5f);
		const FVector2f Br = Base - Perp * (ArrowSize * 0.5f);
		TArray<FVector2f> Tri = { Tip, Bl, Br, Tip };
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(),
			Tri, ESlateDrawEffect::None, ArrowCol[i], true, LineThickness + 2.f);
	}

	return Layer;
}
