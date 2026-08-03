// FaP All Rights Reserve

#include "Components/Combat/AttackAssistComponent.h"

#include "Combat/WarriorCombatDebug.h"
#include "DataAssets/Combat/DataAsset_CombatAttackProfile.h"
#include "DrawDebugHelpers.h"
#include "MotionWarpingComponent.h"

UAttackAssistComponent::UAttackAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAttackAssistComponent::BeginPlay()
{
	Super::BeginPlay();
	MotionWarpingComponent = GetOwner()->FindComponentByClass<UMotionWarpingComponent>();
}

bool UAttackAssistComponent::PrepareAttackAssist(
	const FWarriorCombatAttackContext& AttackContext,
	FTransform& OutWarpTargetTransform)
{
	ClearAttackAssist();

	const UDataAsset_CombatAttackProfile* AttackProfile = AttackContext.AttackProfile;
	if (!MotionWarpingComponent
		|| !IsValid(AttackProfile)
		|| AttackProfile->AttackAssistMode == EWarriorAttackAssistMode::None
		|| !AttackContext.bHasTarget
		|| !IsValid(AttackContext.Target))
	{
		return false;
	}

	if (!BuildWarpTargetTransform(AttackContext, AttackProfile, OutWarpTargetTransform))
	{
		DrawAttackAssistDebug(
			AttackContext,
			AttackProfile,
			FTransform::Identity,
			false);
		return false;
	}

	ActiveAttackContext = AttackContext;
	ActiveWarpTargetName = AttackProfile->WarpTargetName;
	MotionWarpingComponent->AddOrUpdateWarpTargetFromTransform(
		ActiveWarpTargetName,
		OutWarpTargetTransform);
	DrawAttackAssistDebug(
		AttackContext,
		AttackProfile,
		OutWarpTargetTransform,
		true);
	OnAttackAssistPrepared.Broadcast(ActiveAttackContext, OutWarpTargetTransform);
	return true;
}

bool UAttackAssistComponent::RefreshAttackAssistTarget(FTransform& OutWarpTargetTransform)
{
	const UDataAsset_CombatAttackProfile* AttackProfile = ActiveAttackContext.AttackProfile;
	if (!MotionWarpingComponent
		|| ActiveWarpTargetName.IsNone()
		|| !IsValid(AttackProfile)
		|| !IsValid(ActiveAttackContext.Target)
		|| !BuildWarpTargetTransform(ActiveAttackContext, AttackProfile, OutWarpTargetTransform))
	{
		return false;
	}

	ActiveAttackContext.TargetLocation = ActiveAttackContext.Target->GetActorLocation();
	MotionWarpingComponent->AddOrUpdateWarpTargetFromTransform(
		ActiveWarpTargetName,
		OutWarpTargetTransform);
	DrawAttackAssistDebug(
		ActiveAttackContext,
		AttackProfile,
		OutWarpTargetTransform,
		true);
	return true;
}

void UAttackAssistComponent::ClearAttackAssist()
{
	if (MotionWarpingComponent && !ActiveWarpTargetName.IsNone())
	{
		MotionWarpingComponent->RemoveWarpTarget(ActiveWarpTargetName);
	}

	ActiveWarpTargetName = NAME_None;
	ActiveAttackContext.Reset();
}

FWarriorCombatAttackContext UAttackAssistComponent::GetActiveAttackContext() const
{
	return ActiveAttackContext;
}

bool UAttackAssistComponent::BuildWarpTargetTransform(
	const FWarriorCombatAttackContext& AttackContext,
	const UDataAsset_CombatAttackProfile* AttackProfile,
	FTransform& OutTransform) const
{
	const APawn* OwningPawn = GetOwningPawn();
	AActor* Target = AttackContext.Target;
	if (!IsValid(OwningPawn) || !IsValid(Target) || !IsValid(AttackProfile))
	{
		return false;
	}

	const FVector OwnerLocation = OwningPawn->GetActorLocation();
	const FVector TargetLocation = Target->GetActorLocation();
	FVector OwnerToTarget = TargetLocation - OwnerLocation;
	if (AttackProfile->bIgnoreZAxis)
	{
		OwnerToTarget.Z = 0.f;
	}

	const float Distance = OwnerToTarget.Size();
	if (Distance <= KINDA_SMALL_NUMBER
		|| Distance > AttackProfile->MaxAssistDistance)
	{
		return false;
	}

	// 已在理想攻击距离内时不做位移吸附：MotionWarp 在"warp 目标≈自身位置"时会反复 re-skew
	// 攻击的 root motion，近身单敌时表现为角色闪烁。此时让攻击自然播放即可（仅靠朝向）。
	if (AttackProfile->AttackAssistMode == EWarriorAttackAssistMode::MotionWarp
		&& Distance <= AttackProfile->IdealAttackDistance)
	{
		return false;
	}

	const FVector Direction = OwnerToTarget / Distance;
	const float FacingDot = FVector::DotProduct(
		OwningPawn->GetActorForwardVector().GetSafeNormal2D(),
		Direction.GetSafeNormal2D());
	const float AssistAngle = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(FacingDot, -1.f, 1.f)));
	if (AssistAngle > AttackProfile->MaxAssistAngleDegrees)
	{
		return false;
	}

	FVector WarpLocation = OwnerLocation;
	if (AttackProfile->AttackAssistMode == EWarriorAttackAssistMode::MotionWarp)
	{
		// Never place the warp target behind the attacker when already inside the
		// ideal attack distance. A Facing warp would otherwise rotate the character
		// roughly 180 degrees to face that overshot location. Keep the point a tiny
		// distance ahead instead of exactly at the actor so Facing still has a stable
		// direction when no approach movement is needed.
		constexpr float MinFacingWarpDistance = 1.f;
		const float ForwardWarpDistance = FMath::Max(
			Distance - AttackProfile->IdealAttackDistance,
			MinFacingWarpDistance);
		WarpLocation = OwnerLocation + Direction * ForwardWarpDistance;
		if (AttackProfile->bIgnoreZAxis)
		{
			WarpLocation.Z = OwnerLocation.Z;
		}
		WarpLocation += Direction.Rotation().RotateVector(AttackProfile->TargetOffset);
	}

	const FRotator WarpRotation(0.f, Direction.Rotation().Yaw, 0.f);
	OutTransform = FTransform(WarpRotation, WarpLocation);
	return true;
}

void UAttackAssistComponent::DrawAttackAssistDebug(
	const FWarriorCombatAttackContext& AttackContext,
	const UDataAsset_CombatAttackProfile* AttackProfile,
	const FTransform& WarpTargetTransform,
	const bool bAccepted) const
{
	if (!WarriorCombatDebug::IsAttackAssistEnabled()
		|| !GetWorld()
		|| !IsValid(GetOwner())
		|| !IsValid(AttackProfile)
		|| !IsValid(AttackContext.Target))
	{
		return;
	}

	const float Duration = WarriorCombatDebug::GetDrawDuration();
	const float TextScale = WarriorCombatDebug::GetTextScale();
	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const FVector TargetLocation = AttackContext.Target->GetActorLocation();
	const FVector WarpLocation = bAccepted
		? WarpTargetTransform.GetLocation()
		: OwnerLocation;
	const FColor ResultColor = bAccepted ? FColor::Green : FColor::Red;

	DrawDebugSphere(
		GetWorld(),
		OwnerLocation,
		18.f,
		12,
		FColor::Blue,
		false,
		Duration,
		0,
		3.f);
	DrawDebugSphere(
		GetWorld(),
		TargetLocation,
		22.f,
		12,
		FColor::Red,
		false,
		Duration,
		0,
		3.f);
	DrawDebugSphere(
		GetWorld(),
		WarpLocation,
		25.f,
		16,
		ResultColor,
		false,
		Duration,
		0,
		5.f);
	DrawDebugLine(
		GetWorld(),
		OwnerLocation,
		TargetLocation,
		FColor::Blue,
		false,
		Duration,
		0,
		1.5f);
	DrawDebugLine(
		GetWorld(),
		OwnerLocation,
		WarpLocation,
		ResultColor,
		false,
		Duration,
		0,
		4.f);

	DrawDebugCircle(
		GetWorld(),
		TargetLocation,
		AttackProfile->IdealAttackDistance,
		32,
		FColor(0, 255, 128),
		false,
		Duration,
		0,
		2.f,
		FVector::ForwardVector,
		FVector::RightVector,
		false);

	DrawDebugCone(
		GetWorld(),
		OwnerLocation,
		GetOwner()->GetActorForwardVector(),
		AttackProfile->MaxAssistDistance,
		FMath::DegreesToRadians(AttackProfile->MaxAssistAngleDegrees),
		FMath::DegreesToRadians(5.f),
		24,
		FColor(80, 160, 255),
		false,
		Duration,
		0,
		1.f);

	const float Distance = FVector::Distance(OwnerLocation, TargetLocation);
	const FVector ToTarget = (TargetLocation - OwnerLocation).GetSafeNormal2D();
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
		FVector::DotProduct(GetOwner()->GetActorForwardVector().GetSafeNormal2D(), ToTarget),
		-1.f,
		1.f)));
	const UEnum* AssistModeEnum = StaticEnum<EWarriorAttackAssistMode>();
	const FString ModeName = AssistModeEnum
		? AssistModeEnum->GetNameStringByValue(
			static_cast<int64>(AttackProfile->AttackAssistMode))
		: TEXT("Unknown");
	const FString DebugText = FString::Printf(
		TEXT("Attack Assist: %s\nMode: %s\nTarget: %s\nDistance: %.0f / %.0f\nAngle: %.1f / %.1f\nWarp: %s"),
		bAccepted ? TEXT("ACCEPTED") : TEXT("REJECTED"),
		*ModeName,
		*GetNameSafe(AttackContext.Target),
		Distance,
		AttackProfile->MaxAssistDistance,
		Angle,
		AttackProfile->MaxAssistAngleDegrees,
		*AttackProfile->WarpTargetName.ToString());
	DrawDebugString(
		GetWorld(),
		OwnerLocation + FVector(0.f, 0.f, 160.f),
		DebugText,
		nullptr,
		ResultColor,
		Duration,
		true,
		TextScale);
}
