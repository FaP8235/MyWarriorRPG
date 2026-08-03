// FaP All Rights Reserve

#include "Components/Combat/MeleeTargetingComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/WarriorCombatDebug.h"
#include "DataAssets/Combat/DataAsset_CombatAttackProfile.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "WarriorFunctionLibrary.h"
#include "WarriorGameplayTags.h"

UMeleeTargetingComponent::UMeleeTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UMeleeTargetingComponent::SelectMeleeTarget(
	UDataAsset_CombatAttackProfile* AttackProfile,
	const FVector2D InputIntent,
	FWarriorCombatAttackContext& OutAttackContext)
{
	OutAttackContext.Reset();

	APawn* OwningPawn = GetOwningPawn();
	UDataAsset_CombatAttackProfile* ResolvedProfile = AttackProfile ? AttackProfile : DefaultAttackProfile.Get();
	if (!IsValid(OwningPawn) || !IsValid(ResolvedProfile))
	{
		return false;
	}

	ActiveAttackProfile = ResolvedProfile;
	ActiveInputIntent = InputIntent.GetClampedToMaxSize(1.f);

	AActor* SelectedTarget = nullptr;
	float SelectedScore = 0.f;
	bool bUsedExplicitTarget = false;
	bool bUsedCounterAttackTarget = false;
	TArray<AActor*> Candidates;

	if (ExplicitTarget.IsValid() && IsTargetValid(ExplicitTarget.Get()))
	{
		SelectedTarget = ExplicitTarget.Get();
		SelectedScore = CalculateTargetScore(SelectedTarget);
		bUsedExplicitTarget = true;
		Candidates.Add(SelectedTarget);
	}
	else
	{
		if (ExplicitTarget.IsValid())
		{
			ExplicitTarget.Reset();
		}

		if (CounterAttackTarget.IsValid()
			&& GetWorld()
			&& GetWorld()->GetTimeSeconds() <= CounterAttackTargetExpireTime
			&& IsCounterAttackTargetValid(CounterAttackTarget.Get()))
		{
			SelectedTarget = CounterAttackTarget.Get();
			SelectedScore = CalculateTargetScore(SelectedTarget);
			bUsedCounterAttackTarget = true;
			Candidates.Add(SelectedTarget);
			ClearCounterAttackTarget();
		}
		else
		{
			ClearCounterAttackTarget();

			if (!GatherPresetCandidates(ResolvedProfile, Candidates))
			{
				GatherFallbackCandidates(ResolvedProfile, Candidates);
			}

			for (AActor* Candidate : Candidates)
			{
				if (!IsTargetValid(Candidate))
				{
					continue;
				}

				const float CandidateScore = CalculateTargetScore(Candidate);
				if (!SelectedTarget || CandidateScore > SelectedScore)
				{
					SelectedTarget = Candidate;
					SelectedScore = CandidateScore;
				}
			}
		}
	}

	CurrentTarget = SelectedTarget;
	OutAttackContext.Instigator = OwningPawn;
	OutAttackContext.Target = SelectedTarget;
	OutAttackContext.AttackProfile = ResolvedProfile;
	OutAttackContext.TargetScore = SelectedScore;
	OutAttackContext.bHasTarget = IsValid(SelectedTarget);
	OutAttackContext.bUsedExplicitTarget = bUsedExplicitTarget;
	OutAttackContext.bUsedCounterAttackTarget = bUsedCounterAttackTarget;

	if (SelectedTarget)
	{
		OutAttackContext.TargetLocation = SelectedTarget->GetActorLocation();
	}

	OnMeleeTargetChanged.Broadcast(SelectedTarget, SelectedScore);
	DrawTargetingDebug(Candidates, SelectedTarget, bUsedCounterAttackTarget);

	ActiveAttackProfile = nullptr;
	ActiveInputIntent = FVector2D::ZeroVector;

	return OutAttackContext.bHasTarget || ResolvedProfile->bAllowAttackWithoutTarget;
}

void UMeleeTargetingComponent::SetExplicitTarget(AActor* NewExplicitTarget)
{
	ExplicitTarget = NewExplicitTarget;
	if (IsValid(NewExplicitTarget))
	{
		CurrentTarget = NewExplicitTarget;
	}
}

void UMeleeTargetingComponent::ClearExplicitTarget()
{
	ExplicitTarget.Reset();
}

void UMeleeTargetingComponent::SetCounterAttackTarget(AActor* NewCounterAttackTarget)
{
	AActor* ResolvedTarget = NewCounterAttackTarget;
	if (IsValid(ResolvedTarget) && !ResolvedTarget->IsA<APawn>())
	{
		ResolvedTarget = ResolvedTarget->GetInstigator();
		if (!IsValid(ResolvedTarget))
		{
			ResolvedTarget = Cast<APawn>(NewCounterAttackTarget->GetOwner());
		}
	}

	if (!IsValid(ResolvedTarget) || !GetWorld())
	{
		ClearCounterAttackTarget();
		return;
	}

	CounterAttackTarget = ResolvedTarget;
	CounterAttackTargetExpireTime =
		GetWorld()->GetTimeSeconds() + FMath::Max(CounterAttackTargetDuration, 0.f);
	CurrentTarget = ResolvedTarget;
}

void UMeleeTargetingComponent::ClearCounterAttackTarget()
{
	CounterAttackTarget.Reset();
	CounterAttackTargetExpireTime = 0.f;
}

AActor* UMeleeTargetingComponent::GetCurrentTarget() const
{
	return CurrentTarget.Get();
}

AActor* UMeleeTargetingComponent::GetExplicitTarget() const
{
	return ExplicitTarget.Get();
}

AActor* UMeleeTargetingComponent::GetCounterAttackTarget() const
{
	if (!CounterAttackTarget.IsValid()
		|| !GetWorld()
		|| GetWorld()->GetTimeSeconds() > CounterAttackTargetExpireTime)
	{
		return nullptr;
	}

	return CounterAttackTarget.Get();
}

bool UMeleeTargetingComponent::IsBasicTargetValid(AActor* CandidateTarget) const
{
	const APawn* OwningPawn = GetOwningPawn();
	APawn* TargetPawn = Cast<APawn>(CandidateTarget);
	if (!IsValid(OwningPawn)
		|| !IsValid(TargetPawn)
		|| TargetPawn == OwningPawn
		|| !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn)
		|| !UWarriorFunctionLibrary::IsTargetPawnHostile(
			const_cast<APawn*>(OwningPawn),
			TargetPawn))
	{
		return false;
	}

	if (UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn))
	{
		return !TargetASC->HasMatchingGameplayTag(WarriorGameplayTags::Shared_Status_Dead);
	}

	return false;
}

bool UMeleeTargetingComponent::IsCounterAttackTargetValid(AActor* CandidateTarget) const
{
	if (!IsBasicTargetValid(CandidateTarget))
	{
		return false;
	}

	const APawn* OwningPawn = GetOwningPawn();
	const UDataAsset_CombatAttackProfile* Profile = GetActiveAttackProfile();
	if (!IsValid(OwningPawn) || !IsValid(Profile))
	{
		return false;
	}

	const FVector ToTarget = CandidateTarget->GetActorLocation() - OwningPawn->GetActorLocation();
	if (ToTarget.SizeSquared() > FMath::Square(Profile->MaxTargetDistance))
	{
		return false;
	}

	return !Profile->bRequireLineOfSight || HasLineOfSightToTarget(CandidateTarget);
}

bool UMeleeTargetingComponent::IsTargetValid(AActor* CandidateTarget) const
{
	const APawn* OwningPawn = GetOwningPawn();
	const UDataAsset_CombatAttackProfile* Profile = GetActiveAttackProfile();

	if (!IsValid(OwningPawn) || !IsBasicTargetValid(CandidateTarget))
	{
		return false;
	}

	if (Profile)
	{
		const FVector OwnerLocation = OwningPawn->GetActorLocation();
		const FVector ToTarget = CandidateTarget->GetActorLocation() - OwnerLocation;
		if (ToTarget.SizeSquared() > FMath::Square(Profile->MaxTargetDistance))
		{
			return false;
		}

		FVector ViewForward = OwningPawn->GetActorForwardVector();
		if (const APlayerController* PlayerController = GetOwningPlayerController())
		{
			FVector ViewLocation;
			FRotator ViewRotation;
			PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
			ViewForward = ViewRotation.Vector();
		}

		const float ViewDot = FVector::DotProduct(
			ViewForward.GetSafeNormal2D(),
			ToTarget.GetSafeNormal2D());
		const float MinViewDot = FMath::Cos(FMath::DegreesToRadians(Profile->MaxTargetAngleDegrees));
		if (ViewDot < MinViewDot)
		{
			return false;
		}

		if (Profile->bRequireLineOfSight && !HasLineOfSightToTarget(CandidateTarget))
		{
			return false;
		}
	}

	return true;
}

float UMeleeTargetingComponent::CalculateTargetScore(AActor* CandidateTarget) const
{
	return CalculateTargetScoreBreakdown(CandidateTarget).Total;
}

FWarriorMeleeTargetScoreBreakdown UMeleeTargetingComponent::CalculateTargetScoreBreakdown(
	AActor* CandidateTarget) const
{
	FWarriorMeleeTargetScoreBreakdown Breakdown;
	const APawn* OwningPawn = GetOwningPawn();
	const UDataAsset_CombatAttackProfile* Profile = GetActiveAttackProfile();
	if (!IsValid(OwningPawn) || !IsValid(CandidateTarget) || !IsValid(Profile))
	{
		return Breakdown;
	}

	const FVector OwnerLocation = OwningPawn->GetActorLocation();
	const FVector TargetLocation = CandidateTarget->GetActorLocation();
	const FVector ToTarget = TargetLocation - OwnerLocation;
	const float DistanceAlpha = 1.f - FMath::Clamp(
		ToTarget.Size() / FMath::Max(Profile->MaxTargetDistance, 1.f),
		0.f,
		1.f);

	FVector ViewLocation = OwnerLocation;
	FRotator ViewRotation = OwningPawn->GetActorRotation();
	APlayerController* PlayerController = GetOwningPlayerController();
	if (PlayerController)
	{
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const FVector ViewToTarget = (TargetLocation - ViewLocation).GetSafeNormal();
	const float CameraAlpha = FMath::Clamp(
		(FVector::DotProduct(ViewRotation.Vector(), ViewToTarget) + 1.f) * 0.5f,
		0.f,
		1.f);

	float ScreenCenterAlpha = CameraAlpha;
	float OnScreenAlpha = 0.f;
	if (PlayerController)
	{
		int32 ViewportX = 0;
		int32 ViewportY = 0;
		FVector2D ScreenPosition;
		PlayerController->GetViewportSize(ViewportX, ViewportY);
		if (ViewportX > 0
			&& ViewportY > 0
			&& PlayerController->ProjectWorldLocationToScreen(TargetLocation, ScreenPosition, true))
		{
			const FVector2D HalfViewport(ViewportX * 0.5f, ViewportY * 0.5f);
			const FVector2D NormalizedOffset(
				(ScreenPosition.X - HalfViewport.X) / FMath::Max(HalfViewport.X, 1.f),
				(ScreenPosition.Y - HalfViewport.Y) / FMath::Max(HalfViewport.Y, 1.f));
			ScreenCenterAlpha = 1.f - FMath::Clamp(NormalizedOffset.Size(), 0.f, 1.f);
			OnScreenAlpha =
				ScreenPosition.X >= 0.f && ScreenPosition.X <= ViewportX
				&& ScreenPosition.Y >= 0.f && ScreenPosition.Y <= ViewportY
				? 1.f
				: 0.f;
		}
	}

	float InputIntentAlpha = 0.f;
	if (!ActiveInputIntent.IsNearlyZero())
	{
		const FRotator YawRotation(0.f, ViewRotation.Yaw, 0.f);
		const FVector WorldIntent =
			YawRotation.RotateVector(FVector::ForwardVector) * ActiveInputIntent.Y
			+ YawRotation.RotateVector(FVector::RightVector) * ActiveInputIntent.X;
		InputIntentAlpha = FMath::Clamp(
			(FVector::DotProduct(WorldIntent.GetSafeNormal2D(), ToTarget.GetSafeNormal2D()) + 1.f) * 0.5f,
			0.f,
			1.f);
	}

	const float StickinessAlpha = CandidateTarget == CurrentTarget.Get() ? 1.f : 0.f;
	const FWarriorMeleeTargetingWeights& Weights = Profile->TargetingWeights;

	Breakdown.CameraAlignment = CameraAlpha * Weights.CameraAlignment;
	Breakdown.ScreenCenter = ScreenCenterAlpha * Weights.ScreenCenter;
	Breakdown.Distance = DistanceAlpha * Weights.Distance;
	Breakdown.InputIntent = InputIntentAlpha * Weights.InputIntent;
	Breakdown.TargetStickiness = StickinessAlpha * Weights.TargetStickiness;
	Breakdown.OnScreenBonus = OnScreenAlpha * Weights.OnScreenBonus;
	Breakdown.Total =
		Breakdown.CameraAlignment
		+ Breakdown.ScreenCenter
		+ Breakdown.Distance
		+ Breakdown.InputIntent
		+ Breakdown.TargetStickiness
		+ Breakdown.OnScreenBonus;
	return Breakdown;
}

const UDataAsset_CombatAttackProfile* UMeleeTargetingComponent::GetActiveAttackProfile() const
{
	return ActiveAttackProfile ? ActiveAttackProfile.Get() : DefaultAttackProfile.Get();
}

bool UMeleeTargetingComponent::GatherPresetCandidates(
	const UDataAsset_CombatAttackProfile* AttackProfile,
	TArray<AActor*>& OutCandidates) const
{
	if (!AttackProfile || !AttackProfile->TargetingPreset)
	{
		return false;
	}

	UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(GetWorld());
	if (!TargetingSubsystem)
	{
		return false;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = GetOwner();
	SourceContext.InstigatorActor = GetOwner();
	SourceContext.SourceLocation = GetOwner()->GetActorLocation();
	SourceContext.SourceObject = const_cast<UMeleeTargetingComponent*>(this);

	FTargetingRequestHandle RequestHandle =
		UTargetingSubsystem::MakeTargetRequestHandle(AttackProfile->TargetingPreset, SourceContext);
	if (!RequestHandle.IsValid())
	{
		return false;
	}

	TargetingSubsystem->ExecuteTargetingRequestWithHandle(RequestHandle);
	TargetingSubsystem->GetTargetingResultsActors(RequestHandle, OutCandidates);
	UTargetingSubsystem::ReleaseTargetRequestHandle(RequestHandle);
	return true;
}

void UMeleeTargetingComponent::GatherFallbackCandidates(
	const UDataAsset_CombatAttackProfile* AttackProfile,
	TArray<AActor*>& OutCandidates) const
{
	if (!AttackProfile || !GetWorld())
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WarriorMeleeTargeting), false, GetOwner());
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		GetOwner()->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(AttackProfile->MaxTargetDistance),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* Actor = Overlap.GetActor())
		{
			OutCandidates.AddUnique(Actor);
		}
	}
}

bool UMeleeTargetingComponent::HasLineOfSightToTarget(AActor* CandidateTarget) const
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !GetWorld() || !IsValid(CandidateTarget))
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WarriorMeleeTargetingLOS), true, GetOwner());
	QueryParams.AddIgnoredActor(GetOwner());

	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		Hit,
		ViewLocation,
		CandidateTarget->GetActorLocation(),
		LineOfSightChannel,
		QueryParams);

	AActor* HitActor = Hit.GetActor();
	return !bBlocked
		|| HitActor == CandidateTarget
		|| (IsValid(HitActor) && HitActor->IsOwnedBy(CandidateTarget));
}

APlayerController* UMeleeTargetingComponent::GetOwningPlayerController() const
{
	const APawn* OwningPawn = GetOwningPawn();
	return OwningPawn ? Cast<APlayerController>(OwningPawn->GetController()) : nullptr;
}

void UMeleeTargetingComponent::DrawTargetingDebug(
	const TArray<AActor*>& Candidates,
	AActor* SelectedTarget,
	const bool bUsedCounterAttackTarget) const
{
	if (!WarriorCombatDebug::IsTargetingEnabled()
		|| !GetWorld()
		|| !IsValid(GetOwner()))
	{
		return;
	}

	const UDataAsset_CombatAttackProfile* Profile = GetActiveAttackProfile();
	const float Duration = WarriorCombatDebug::GetDrawDuration();
	const float TextScale = WarriorCombatDebug::GetTextScale();
	const FVector OwnerLocation = GetOwner()->GetActorLocation();

	if (Profile)
	{
		DrawDebugSphere(
			GetWorld(),
			OwnerLocation,
			Profile->MaxTargetDistance,
			40,
			FColor(40, 180, 255),
			false,
			Duration,
			0,
			1.f);
	}

	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		const bool bValidTarget = IsTargetValid(Candidate);
		const bool bSelected = Candidate == SelectedTarget;
		const FColor DebugColor = bSelected
			? FColor::Green
			: (bValidTarget ? FColor::Yellow : FColor::Red);
		const FVector CandidateLocation = Candidate->GetActorLocation();

		if (const ACharacter* Character = Cast<ACharacter>(Candidate))
		{
			const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
			DrawDebugCapsule(
				GetWorld(),
				Capsule->GetComponentLocation(),
				Capsule->GetScaledCapsuleHalfHeight(),
				Capsule->GetScaledCapsuleRadius(),
				Capsule->GetComponentQuat(),
				DebugColor,
				false,
				Duration,
				0,
				bSelected ? 5.f : 2.f);
		}
		else
		{
			DrawDebugSphere(
				GetWorld(),
				CandidateLocation,
				45.f,
				16,
				DebugColor,
				false,
				Duration,
				0,
				bSelected ? 5.f : 2.f);
		}

		DrawDebugLine(
			GetWorld(),
			OwnerLocation,
			CandidateLocation,
			DebugColor,
			false,
			Duration,
			0,
			bSelected ? 4.f : 1.f);

		const FWarriorMeleeTargetScoreBreakdown Score =
			CalculateTargetScoreBreakdown(Candidate);
		const FString DebugText = FString::Printf(
			TEXT("%s%s\nTotal %.2f\nCamera %.2f | Screen %.2f\nDistance %.2f | Input %.2f\nSticky %.2f | Visible %.2f"),
			bSelected
				? (bUsedCounterAttackTarget ? TEXT("[COUNTER SELECTED] ") : TEXT("[SELECTED] "))
				: TEXT(""),
			*GetNameSafe(Candidate),
			Score.Total,
			Score.CameraAlignment,
			Score.ScreenCenter,
			Score.Distance,
			Score.InputIntent,
			Score.TargetStickiness,
			Score.OnScreenBonus);
		DrawDebugString(
			GetWorld(),
			CandidateLocation + FVector(0.f, 0.f, 140.f),
			DebugText,
			nullptr,
			DebugColor,
			Duration,
			true,
			TextScale);
	}

	if (!SelectedTarget)
	{
		DrawDebugString(
			GetWorld(),
			OwnerLocation + FVector(0.f, 0.f, 120.f),
			TEXT("Melee Target: NONE"),
			nullptr,
			FColor::Red,
			Duration,
			true,
			TextScale);
	}
}
