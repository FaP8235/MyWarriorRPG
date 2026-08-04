// FaP All Rights Reserve

#include "Components/Camera/CameraStrafeAssistComponent.h"

#include "AI/EnemyCombatDirectorSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/Combat/AttackAssistComponent.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"
#include "Combat/WarriorCombatDebug.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "WarriorGameplayTags.h"

UCameraStrafeAssistComponent::UCameraStrafeAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCameraStrafeAssistComponent::NotifyManualLook()
{
	LastManualLookTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void UCameraStrafeAssistComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		CachedSpringArm = Owner->FindComponentByClass<USpringArmComponent>();
		if (CachedSpringArm)
		{
			BaseSocketOffsetY = CachedSpringArm->SocketOffset.Y;
		}

		if (UAttackAssistComponent* AttackAssist = Owner->FindComponentByClass<UAttackAssistComponent>())
		{
			AttackAssist->OnAttackAssistPrepared.AddDynamic(
				this,
				&UCameraStrafeAssistComponent::HandleAttackAssistPrepared);
		}
	}
}

void UCameraStrafeAssistComponent::HandleAttackAssistPrepared(
	const FWarriorCombatAttackContext& AttackContext,
	FTransform WarpTargetTransform)
{
	AttackRecenterTarget = AttackContext.Target;
	AttackRecenterTimer = AttackRecenterDuration;
}

void UCameraStrafeAssistComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	APawn* OwnerPawn = Cast<APawn>(Owner);
	APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!World || !PlayerController || !CachedSpringArm)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}

	const bool bTargetLocked = ASC->HasMatchingGameplayTag(WarriorGameplayTags::Player_Status_TargetLock);

	// ② 攻击转向倒计时
	const bool bAttackRecenter = AttackRecenterTimer > 0.f && AttackRecenterTarget.IsValid();
	if (AttackRecenterTimer > 0.f)
	{
		AttackRecenterTimer -= DeltaTime;
	}

	// 计算群体聚焦 yaw（不锁定、非攻击转向时）。结果同时用于判断"是否交战"。
	float GroupFocusYaw = 0.f;
	bool bGroupFocus = false;
	if (!bAttackRecenter && !bTargetLocked)
	{
		bGroupFocus = ComputeGroupFocusYaw(Owner->GetActorLocation(), GroupFocusYaw);
	}

	// ③ 横向构图：交战（攻击转向 或 屏内有敌人）时渐入偏移
	const bool bEngaged = bAttackRecenter || bGroupFocus;
	UpdateFramingOffset(DeltaTime, bEngaged);

	// ①/② yaw 修正（让位 TargetLock 与手动瞄准：玩家在转镜头时不抢）
	float FocusYaw = 0.f;
	bool bHasFocus = false;
	const bool bManualAim = (World->GetTimeSeconds() - LastManualLookTime) < ManualLookDebounce;
	if (!bManualAim)
	{
		if (bAttackRecenter)
		{
			if (AActor* Target = AttackRecenterTarget.Get())
			{
				FocusYaw = (Target->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D().Rotation().Yaw;
				bHasFocus = true;
			}
		}
		else if (bGroupFocus)
		{
			FocusYaw = GroupFocusYaw;
			bHasFocus = true;
		}
	}

	if (!bTargetLocked && bHasFocus)
	{
		ApplyYawCorrection(FocusYaw, DeltaTime, bAttackRecenter);
	}

	DebugDrawAccumulator += DeltaTime;
	if (DebugDrawAccumulator >= 0.1f)
	{
		DebugDrawAccumulator = 0.f;
		DrawDebugState(FocusYaw, bHasFocus, bAttackRecenter);
	}
}

bool UCameraStrafeAssistComponent::ComputeGroupFocusYaw(const FVector& OwnerLocation, float& OutYaw) const
{
	UWorld* World = GetWorld();
	UEnemyCombatDirectorSubsystem* Director = World
		? World->GetSubsystem<UEnemyCombatDirectorSubsystem>()
		: nullptr;
	if (!Director)
	{
		return false;
	}

	TArray<UEnemyCombatAgentComponent*> Agents;
	Director->GetOnScreenAgentsTargeting(GetOwner(), Agents);
	if (Agents.Num() == 0)
	{
		return false;
	}

	FVector2D AccumDir = FVector2D::ZeroVector;
	for (const UEnemyCombatAgentComponent* Agent : Agents)
	{
		const AActor* Enemy = Agent ? Agent->GetOwner() : nullptr;
		if (!Enemy)
		{
			continue;
		}

		const FVector ToEnemy = Enemy->GetActorLocation() - OwnerLocation;
		const FVector2D Dir2D = FVector2D(ToEnemy.X, ToEnemy.Y).GetSafeNormal();
		if (Dir2D.IsNearlyZero())
		{
			continue;
		}

		const float Dist = FVector::Dist2D(Enemy->GetActorLocation(), OwnerLocation);
		const float Weight = 1.f / (1.f + Dist / FMath::Max(1.f, ProximityDenominator));
		AccumDir += Dir2D * Weight;
	}

	if (AccumDir.IsNearlyZero())
	{
		return false;
	}

	OutYaw = FMath::RadiansToDegrees(FMath::Atan2(AccumDir.Y, AccumDir.X));
	return true;
}

void UCameraStrafeAssistComponent::ApplyYawCorrection(const float FocusYaw, const float DeltaTime, const bool bAttackRecenter)
{
	AActor* Owner = GetOwner();
	APawn* OwnerPawn = Cast<APawn>(Owner);
	APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PlayerController)
	{
		return;
	}

	// 聚焦 yaw 低通（攻击转向直接用目标，不过低通）
	if (bAttackRecenter || !bHasSmoothedFocus)
	{
		SmoothedFocusYaw = FocusYaw;
		bHasSmoothedFocus = true;
	}
	else
	{
		const FRotator Current(0.f, SmoothedFocusYaw, 0.f);
		const FRotator Target(0.f, FocusYaw, 0.f);
		SmoothedFocusYaw = FMath::RInterpTo(Current, Target, DeltaTime, FocusSmoothSpeed).Yaw;
	}

	const float CamYaw = PlayerController->GetControlRotation().Yaw;
	const float Delta = FMath::FindDeltaAngleDegrees(CamYaw, SmoothedFocusYaw);

	// 死区：框内不动
	if (!bAttackRecenter && FMath::Abs(Delta) <= DeadzoneAngle)
	{
		return;
	}

	// 攻击转向：目标偏离当前镜头视野超过阈值时别硬拽（避免大幅乱转到屏外/身后目标）
	if (bAttackRecenter && FMath::Abs(Delta) > AttackRecenterMaxViewAngle)
	{
		return;
	}

	// 最小修正：只把聚焦点拉回死区边缘（攻击转向则拉到中心）
	float TargetYaw;
	if (bAttackRecenter)
	{
		TargetYaw = SmoothedFocusYaw;
	}
	else
	{
		const float MoveToward = FMath::Abs(Delta) - DeadzoneAngle;
		TargetYaw = CamYaw + (Delta > 0.f ? MoveToward : -MoveToward);
	}

	const float InterpSpeed = bAttackRecenter ? AttackRecenterInterpSpeed : CorrectionInterpSpeed;
	const float NewYaw = FMath::FInterpTo(CamYaw, TargetYaw, DeltaTime, InterpSpeed);

	FRotator NewRot = PlayerController->GetControlRotation();
	NewRot.Yaw = NewYaw;
	PlayerController->SetControlRotation(NewRot);
}

void UCameraStrafeAssistComponent::UpdateFramingOffset(const float DeltaTime, const bool bEngaged)
{
	if (!CachedSpringArm)
	{
		return;
	}

	const float TargetY = BaseSocketOffsetY + (bEngaged ? FramingOffsetY : 0.f);
	FVector Offset = CachedSpringArm->SocketOffset;
	Offset.Y = FMath::FInterpTo(Offset.Y, TargetY, DeltaTime, FramingInterpSpeed);
	CachedSpringArm->SocketOffset = Offset;
}

void UCameraStrafeAssistComponent::DrawDebugState(const float FocusYaw, const bool bHasFocus, const bool bAttackRecenter) const
{
#if !UE_BUILD_SHIPPING
	if (!WarriorCombatDebug::IsCameraEnabled() || !GetWorld())
	{
		return;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FVector OwnerLocation = Owner->GetActorLocation();
	const FVector Head = OwnerLocation + FVector(0.f, 0.f, 120.f);

	if (bHasFocus)
	{
		// 聚焦方向：绿
		const FQuat FocusQuat(FVector::UpVector, FMath::DegreesToRadians(FocusYaw));
		const FVector FocusDir = FocusQuat.RotateVector(FVector::ForwardVector) * 150.f;
		DrawDebugLine(GetWorld(), Head, Head + FocusDir, FColor::Green, false, 0.15f, 0, 2.f);
		DrawDebugString(
			GetWorld(),
			Head + FVector(0.f, 0.f, 30.f),
			FString::Printf(TEXT("%s\nFocus: %.1f"),
				bAttackRecenter ? TEXT("ATTACK RECENTER") : TEXT("Strafe"),
				FocusYaw),
			nullptr,
			bAttackRecenter ? FColor::Orange : FColor::Green,
			0.15f,
			true,
			WarriorCombatDebug::GetTextScale());
	}

	if (CachedSpringArm)
	{
		DrawDebugString(
			GetWorld(),
			OwnerLocation + FVector(0.f, 0.f, 150.f),
			FString::Printf(TEXT("SocketOffset.Y: %.1f"), CachedSpringArm->SocketOffset.Y),
			nullptr,
			FColor::Cyan,
			0.15f,
			true,
			WarriorCombatDebug::GetTextScale());
	}
#endif
}
