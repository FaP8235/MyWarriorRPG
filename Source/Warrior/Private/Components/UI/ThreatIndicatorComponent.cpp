// FaP All Rights Reserve

#include "Components/UI/ThreatIndicatorComponent.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Combat/WarriorCombatDebug.h"
#include "Debug/DebugDrawService.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UThreatIndicatorComponent::UThreatIndicatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UThreatIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();
	DebugDrawDelegateHandle = UDebugDrawService::Register(
		TEXT("Game"),
		FDebugDrawDelegate::CreateUObject(this, &ThisClass::DrawThreatDebug));
}

void UThreatIndicatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DebugDrawDelegateHandle.IsValid())
	{
		UDebugDrawService::Unregister(DebugDrawDelegateHandle);
		DebugDrawDelegateHandle.Reset();
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UThreatIndicatorComponent::RegisterOrUpdateThreat(
	AActor* SourceActor,
	const EWarriorThreatIndicatorType Type,
	const float Priority,
	const float Duration)
{
	if (!IsValid(SourceActor) || SourceActor == GetOwner() || !GetWorld())
	{
		return;
	}

	FThreatRegistration* ExistingRegistration = ThreatRegistrations.FindByPredicate(
		[SourceActor](const FThreatRegistration& Registration)
		{
			return Registration.SourceActor.Get() == SourceActor;
		});

	FThreatRegistration& Registration = ExistingRegistration
		? *ExistingRegistration
		: ThreatRegistrations.AddDefaulted_GetRef();
	Registration.SourceActor = SourceActor;
	Registration.Type = Type;
	Registration.Priority = Priority;
	Registration.ExpirationTime = Duration > 0.f
		? GetWorld()->GetTimeSeconds() + Duration
		: -1.0;

	if (!GetWorld()->GetTimerManager().IsTimerActive(RefreshTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			RefreshTimerHandle,
			this,
			&ThisClass::RefreshThreatIndicators,
			RefreshInterval,
			true,
			0.f);
	}
}

void UThreatIndicatorComponent::RemoveThreat(AActor* SourceActor)
{
	ThreatRegistrations.RemoveAll(
		[SourceActor](const FThreatRegistration& Registration)
		{
			return !Registration.SourceActor.IsValid()
				|| Registration.SourceActor.Get() == SourceActor;
		});

	RefreshThreatIndicators();
}

void UThreatIndicatorComponent::ClearAllThreats()
{
	ThreatRegistrations.Reset();
	CurrentIndicators.Reset();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}
	OnThreatIndicatorsUpdated.Broadcast(CurrentIndicators);
}

void UThreatIndicatorComponent::RefreshThreatIndicators()
{
	CurrentIndicators.Reset();

	if (!GetWorld())
	{
		return;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	ThreatRegistrations.RemoveAll(
		[CurrentTime](const FThreatRegistration& Registration)
		{
			return !Registration.SourceActor.IsValid()
				|| (Registration.ExpirationTime > 0.0 && Registration.ExpirationTime <= CurrentTime);
		});

	APawn* OwningPawn = GetOwningPawn();
	APlayerController* PlayerController =
		OwningPawn ? Cast<APlayerController>(OwningPawn->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	const float ViewportScale = FMath::Max(
		UWidgetLayoutLibrary::GetViewportScale(this),
		KINDA_SMALL_NUMBER);
	const FVector2D ViewportSize(
		static_cast<float>(ViewportWidth) / ViewportScale,
		static_cast<float>(ViewportHeight) / ViewportScale);

	if (ViewportSize.X <= 0.f || ViewportSize.Y <= 0.f)
	{
		return;
	}

	for (const FThreatRegistration& Registration : ThreatRegistrations)
	{
		FWarriorThreatIndicatorData IndicatorData;
		if (BuildIndicatorData(PlayerController, ViewportSize, Registration, IndicatorData))
		{
			CurrentIndicators.Add(IndicatorData);

			if (WarriorCombatDebug::IsThreatIndicatorEnabled()
				&& IndicatorData.SourceActor)
			{
				const float DebugLifeTime = FMath::Max(
					RefreshInterval * 1.5f,
					0.05f);
				DrawDebugLine(
					GetWorld(),
					GetOwner()->GetActorLocation(),
					IndicatorData.SourceActor->GetActorLocation(),
					IndicatorData.Color.ToFColor(true),
					false,
					DebugLifeTime,
					0,
					2.f);

				const UEnum* ThreatTypeEnum =
					StaticEnum<EWarriorThreatIndicatorType>();
				const FString TypeName = ThreatTypeEnum
					? ThreatTypeEnum->GetNameStringByValue(
						static_cast<int64>(IndicatorData.Type))
					: TEXT("Unknown");
				DrawDebugString(
					GetWorld(),
					IndicatorData.SourceActor->GetActorLocation()
						+ FVector(0.f, 0.f, 130.f),
					FString::Printf(
						TEXT("Threat: %s\nPriority: %.1f\nDistance: %.0f"),
						*TypeName,
						IndicatorData.Priority,
						IndicatorData.Distance),
					nullptr,
					IndicatorData.Color.ToFColor(true),
					DebugLifeTime,
					true,
					WarriorCombatDebug::GetTextScale());
			}
		}
	}

	CurrentIndicators.Sort(
		[](const FWarriorThreatIndicatorData& Left, const FWarriorThreatIndicatorData& Right)
		{
			return Left.Priority > Right.Priority;
		});

	OnThreatIndicatorsUpdated.Broadcast(CurrentIndicators);

	if (ThreatRegistrations.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}
}

TArray<FWarriorThreatIndicatorData> UThreatIndicatorComponent::GetCurrentIndicators() const
{
	return CurrentIndicators;
}

bool UThreatIndicatorComponent::BuildIndicatorData(
	APlayerController* PlayerController,
	const FVector2D& ViewportSize,
	const FThreatRegistration& Registration,
	FWarriorThreatIndicatorData& OutData) const
{
	AActor* SourceActor = Registration.SourceActor.Get();
	if (!IsValid(PlayerController) || !IsValid(SourceActor))
	{
		return false;
	}

	const FVector SourceLocation = SourceActor->GetActorLocation();
	FVector2D ProjectedPosition;
	const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PlayerController,
		SourceLocation,
		ProjectedPosition,
		true);

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector ToSource = SourceLocation - CameraLocation;
	const float ForwardAmount = FVector::DotProduct(CameraRotation.Vector(), ToSource);

	const bool bInsideViewport =
		bProjected
		&& ForwardAmount > 0.f
		&& ProjectedPosition.X >= EdgeMargin
		&& ProjectedPosition.X <= ViewportSize.X - EdgeMargin
		&& ProjectedPosition.Y >= EdgeMargin
		&& ProjectedPosition.Y <= ViewportSize.Y - EdgeMargin;

	if (bInsideViewport && !bIncludeOnScreenThreats)
	{
		return false;
	}

	const FVector2D Center = ViewportSize * 0.5f;
	FVector2D EdgeDirection = ProjectedPosition - Center;
	if (!bProjected || ForwardAmount <= 0.f)
	{
		const FVector CameraRight = FRotationMatrix(CameraRotation).GetScaledAxis(EAxis::Y);
		EdgeDirection.X = FVector::DotProduct(CameraRight, ToSource);
		EdgeDirection.Y = -ForwardAmount;
	}

	if (EdgeDirection.IsNearlyZero())
	{
		EdgeDirection = FVector2D(0.f, 1.f);
	}

	const FVector2D HalfBounds(
		FMath::Max(Center.X - EdgeMargin, 1.f),
		FMath::Max(Center.Y - EdgeMargin, 1.f));
	const float EllipseScale = 1.f / FMath::Sqrt(
		FMath::Square(EdgeDirection.X / HalfBounds.X)
		+ FMath::Square(EdgeDirection.Y / HalfBounds.Y));
	const FVector2D ClampedPosition = Center + EdgeDirection * EllipseScale;

	OutData.SourceActor = SourceActor;
	OutData.ScreenPosition = bInsideViewport ? ProjectedPosition : ClampedPosition;
	OutData.RotationDegrees = FMath::RadiansToDegrees(
		FMath::Atan2(EdgeDirection.Y, EdgeDirection.X));
	OutData.Distance = FVector::Distance(GetOwner()->GetActorLocation(), SourceLocation);
	OutData.Priority = Registration.Priority;
	OutData.Type = Registration.Type;
	OutData.Color = GetColorForType(Registration.Type);
	OutData.bOffScreen = !bInsideViewport;
	return true;
}

FLinearColor UThreatIndicatorComponent::GetColorForType(
	const EWarriorThreatIndicatorType Type) const
{
	switch (Type)
	{
	case EWarriorThreatIndicatorType::MeleeWindup:
		return MeleeWindupColor;
	case EWarriorThreatIndicatorType::RangedWindup:
		return RangedWindupColor;
	case EWarriorThreatIndicatorType::Projectile:
		return ProjectileColor;
	case EWarriorThreatIndicatorType::NearbyIdle:
	default:
		return NearbyIdleColor;
	}
}

void UThreatIndicatorComponent::DrawThreatDebug(
	UCanvas* Canvas,
	APlayerController* PlayerController)
{
	if (!WarriorCombatDebug::IsThreatIndicatorEnabled()
		|| !IsValid(Canvas)
		|| !IsValid(PlayerController))
	{
		return;
	}

	const APawn* OwningPawn = GetOwningPawn();
	if (!OwningPawn || OwningPawn->GetController() != PlayerController)
	{
		return;
	}

	const float ViewportScale = FMath::Max(
		UWidgetLayoutLibrary::GetViewportScale(this),
		KINDA_SMALL_NUMBER);
	const FVector2D ScreenCenter(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
	const float MarkerSize = 10.f;
	const float TextScale = WarriorCombatDebug::GetTextScale();

	Canvas->K2_DrawText(
		nullptr,
		FString::Printf(
			TEXT("THREAT INDICATORS: %d"),
			CurrentIndicators.Num()),
		FVector2D(32.f, 80.f),
		FVector2D(TextScale),
		FLinearColor::White,
		0.f,
		FLinearColor::Black,
		FVector2D(1.f, 1.f),
		false,
		false,
		true,
		FLinearColor::Black);

	for (const FWarriorThreatIndicatorData& Indicator : CurrentIndicators)
	{
		if (!IsValid(Indicator.SourceActor))
		{
			continue;
		}

		const FVector2D MarkerPosition =
			Indicator.ScreenPosition * ViewportScale;
		const FLinearColor DebugColor = Indicator.Color;

		Canvas->K2_DrawLine(
			ScreenCenter,
			MarkerPosition,
			1.f,
			DebugColor * 0.65f);
		Canvas->K2_DrawLine(
			MarkerPosition + FVector2D(-MarkerSize, 0.f),
			MarkerPosition + FVector2D(MarkerSize, 0.f),
			3.f,
			DebugColor);
		Canvas->K2_DrawLine(
			MarkerPosition + FVector2D(0.f, -MarkerSize),
			MarkerPosition + FVector2D(0.f, MarkerSize),
			3.f,
			DebugColor);

		FVector2D RawProjection;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController,
			Indicator.SourceActor->GetActorLocation(),
			RawProjection,
			true))
		{
			RawProjection *= ViewportScale;
			Canvas->K2_DrawLine(
				RawProjection + FVector2D(-5.f, -5.f),
				RawProjection + FVector2D(5.f, 5.f),
				1.f,
				FLinearColor::White);
			Canvas->K2_DrawLine(
				RawProjection + FVector2D(-5.f, 5.f),
				RawProjection + FVector2D(5.f, -5.f),
				1.f,
				FLinearColor::White);
			Canvas->K2_DrawLine(
				RawProjection,
				MarkerPosition,
				1.f,
				FLinearColor::White);
		}

		const UEnum* ThreatTypeEnum =
			StaticEnum<EWarriorThreatIndicatorType>();
		const FString TypeName = ThreatTypeEnum
			? ThreatTypeEnum->GetNameStringByValue(
				static_cast<int64>(Indicator.Type))
			: TEXT("Unknown");
		const FString Label = FString::Printf(
			TEXT("%s | %s\nP %.1f | D %.0f | (%.0f, %.0f)"),
			*GetNameSafe(Indicator.SourceActor),
			*TypeName,
			Indicator.Priority,
			Indicator.Distance,
			Indicator.ScreenPosition.X,
			Indicator.ScreenPosition.Y);
		Canvas->K2_DrawText(
			nullptr,
			Label,
			MarkerPosition + FVector2D(14.f, -8.f),
			FVector2D(TextScale * 0.8f),
			DebugColor,
			0.f,
			FLinearColor::Black,
			FVector2D(1.f, 1.f),
			false,
			false,
			true,
			FLinearColor::Black);
	}
}
