// FaP All Rights Reserve

#include "Components/Combat/EnemyCombatAgentComponent.h"

#include "AI/EnemyCombatDirectorSubsystem.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/UI/ThreatIndicatorComponent.h"
#include "DataAssets/Combat/EnemyCombatPositionProfile.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "WarriorGameplayTags.h"

UEnemyCombatAgentComponent::UEnemyCombatAgentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyCombatAgentComponent::BeginPlay()
{
	Super::BeginPlay();
	if (UEnemyCombatDirectorSubsystem* Director = GetCombatDirector())
	{
		Director->RegisterAgent(this);
	}
	BindDeadStateEvent();
	BindHitReactTagEvent();
	RefreshScreenState();
}

void UEnemyCombatAgentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DeadStateEventHandle.IsValid())
	{
		if (UAbilitySystemComponent* AbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
		{
			AbilitySystem->RegisterGameplayTagEvent(
				WarriorGameplayTags::Shared_Status_Dead,
				EGameplayTagEventType::NewOrRemoved).Remove(DeadStateEventHandle);
		}
		DeadStateEventHandle.Reset();
	}

	if (HitReactTagEventHandle.IsValid())
	{
		if (UAbilitySystemComponent* AbilitySystem =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
		{
			AbilitySystem->RegisterGameplayTagEvent(
				WarriorGameplayTags::Shared_Ability_HitReact,
				EGameplayTagEventType::NewOrRemoved).Remove(HitReactTagEventHandle);
		}
		HitReactTagEventHandle.Reset();
	}

	ClearThreatIndicator();
	if (UEnemyCombatDirectorSubsystem* Director = GetCombatDirector())
	{
		Director->UnregisterAgent(this);
	}
	Super::EndPlay(EndPlayReason);
}

void UEnemyCombatAgentComponent::SetCombatTarget(AActor* NewCombatTarget)
{
	if (CombatTarget.Get() == NewCombatTarget)
	{
		return;
	}

	ClearThreatIndicator();
	CombatTarget.Reset();
	ReleaseAttackToken();
	ReleaseCombatPosition();
	CombatTarget = NewCombatTarget;
	AttackRequestStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	bHasScreenState = false;
	bIsOnPlayerScreen = true;
	FirstOffscreenTime = -1.0;

	if (IsValid(NewCombatTarget))
	{
		RefreshScreenState();
		SetCombatState(EWarriorEnemyCombatState::Positioning);
		RegisterIdleThreat();
	}
	else
	{
		SetCombatState(EWarriorEnemyCombatState::NonAggressive);
	}
}

AActor* UEnemyCombatAgentComponent::GetCombatTarget() const
{
	return CombatTarget.Get();
}

bool UEnemyCombatAgentComponent::RequestAttackToken(
	const int32 TokenCost,
	const float LeaseDuration)
{
	if (HasAttackToken())
	{
		return true;
	}

	if (UEnemyCombatDirectorSubsystem* Director = GetCombatDirector())
	{
		Director->QueueAttackRequest(this, TokenCost, LeaseDuration);
	}
	return HasAttackToken();
}

void UEnemyCombatAgentComponent::CancelAttackTokenRequest()
{
	if (UEnemyCombatDirectorSubsystem* Director = GetCombatDirector())
	{
		Director->CancelAttackRequest(this);
	}
	else
	{
		HandleAttackRequestCancelled();
	}
}

void UEnemyCombatAgentComponent::ReleaseAttackToken()
{
	if (UEnemyCombatDirectorSubsystem* Director = GetCombatDirector())
	{
		Director->ReleaseAttackToken(this);
	}
	else
	{
		HandleAttackTokenRevoked();
	}
}

bool UEnemyCombatAgentComponent::HasAttackToken() const
{
	return AttackTokenHandle.IsValid();
}

bool UEnemyCombatAgentComponent::IsAttackRequestQueued() const
{
	return bAttackRequestQueued;
}

bool UEnemyCombatAgentComponent::ReserveCombatPosition(FVector& OutPositionLocation)
{
	if (UEnemyCombatDirectorSubsystem* Director = GetCombatDirector())
	{
		const bool bReserved = Director->ReserveCombatPosition(
			this,
			CombatPositionHandle,
			OutPositionLocation);
		if (bReserved && !HasAttackToken())
		{
			SetCombatState(EWarriorEnemyCombatState::Positioning);
		}
		return bReserved;
	}

	OutPositionLocation = FVector::ZeroVector;
	return false;
}

void UEnemyCombatAgentComponent::ReleaseCombatPosition()
{
	if (UEnemyCombatDirectorSubsystem* Director = GetCombatDirector())
	{
		Director->ReleaseCombatPosition(this);
	}
	else
	{
		HandleCombatSlotReleased();
	}
}

bool UEnemyCombatAgentComponent::GetReservedCombatPositionLocation(
	FVector& OutPositionLocation) const
{
	if (const UEnemyCombatDirectorSubsystem* Director = GetCombatDirector())
	{
		return Director->GetReservedCombatPositionLocation(
			this,
			OutPositionLocation);
	}

	OutPositionLocation = FVector::ZeroVector;
	return false;
}

EWarriorEnemyPositionZone UEnemyCombatAgentComponent::GetCombatPositionZone() const
{
	if (!CombatTarget.IsValid())
	{
		return EWarriorEnemyPositionZone::None;
	}

	const bool bKeepOffscreenQuadrant = !CombatPositionProfile
		|| CombatPositionProfile->bKeepOffscreenWorldQuadrant;
	if (bKeepOffscreenQuadrant && bHasScreenState && !bIsOnPlayerScreen)
	{
		return EWarriorEnemyPositionZone::OffscreenZone;
	}

	return HasAttackToken()
		? EWarriorEnemyPositionZone::FrontZone
		: EWarriorEnemyPositionZone::IdleRing;
}

bool UEnemyCombatAgentComponent::IsOnPlayerScreen() const
{
	return bIsOnPlayerScreen;
}

bool UEnemyCombatAgentComponent::IsCombatPositionLocationAllowed(
	const FVector Location) const
{
	if (!CombatTarget.IsValid())
	{
		return false;
	}

	const EWarriorEnemyPositionZone Zone = GetCombatPositionZone();
	const FVector TargetLocation = CombatTarget->GetActorLocation();
	const FVector TargetToLocation = Location - TargetLocation;
	const float Distance = TargetToLocation.Size2D();
	float MinDistance = CombatSlotRadius;
	float MaxDistance = CombatSlotRadius;

	if (CombatPositionProfile)
	{
		switch (Zone)
		{
		case EWarriorEnemyPositionZone::FrontZone:
			MinDistance = CombatPositionProfile->FrontMinDistance;
			MaxDistance = CombatPositionProfile->FrontMaxDistance;
			break;
		case EWarriorEnemyPositionZone::IdleRing:
			MinDistance = CombatPositionProfile->IdleMinDistance;
			MaxDistance = CombatPositionProfile->IdleMaxDistance;
			break;
		case EWarriorEnemyPositionZone::OffscreenZone:
			MinDistance = CombatPositionProfile->OffscreenMinDistance;
			MaxDistance = CombatPositionProfile->OffscreenMaxDistance;
			break;
		default:
			return false;
		}
	}
	else if (Zone == EWarriorEnemyPositionZone::FrontZone)
	{
		MinDistance = 220.f;
		MaxDistance = 350.f;
	}

	if (Distance < FMath::Min(MinDistance, MaxDistance)
		|| Distance > FMath::Max(MinDistance, MaxDistance))
	{
		return false;
	}

	const FVector Direction = TargetToLocation.GetSafeNormal2D();
	if (Zone == EWarriorEnemyPositionZone::FrontZone)
	{
		float ViewYaw = CombatTarget->GetActorRotation().Yaw;
		if (const APawn* TargetPawn = Cast<APawn>(CombatTarget.Get()))
		{
			if (const APlayerController* PlayerController =
				Cast<APlayerController>(TargetPawn->GetController()))
			{
				ViewYaw = PlayerController->GetControlRotation().Yaw;
			}
		}

		const float HalfAngle = CombatPositionProfile
			? CombatPositionProfile->FrontHalfAngle
			: 65.f;
		if (FMath::Abs(FMath::FindDeltaAngleDegrees(
			ViewYaw,
			Direction.Rotation().Yaw)) > HalfAngle)
		{
			return false;
		}
	}
	else if (Zone == EWarriorEnemyPositionZone::OffscreenZone)
	{
		const EWarriorWorldQuadrant LocationQuadrant = Direction.X >= 0.f
			? (Direction.Y >= 0.f
				? EWarriorWorldQuadrant::PositiveXPositiveY
				: EWarriorWorldQuadrant::PositiveXNegativeY)
			: (Direction.Y >= 0.f
				? EWarriorWorldQuadrant::NegativeXPositiveY
				: EWarriorWorldQuadrant::NegativeXNegativeY);
		if (LocationQuadrant != OffscreenWorldQuadrant)
		{
			return false;
		}
	}

	const float MinimumSpacing = CombatPositionProfile
		? CombatPositionProfile->MinimumEnemySpacing
		: 140.f;
	return !GetCombatDirector()
		|| GetCombatDirector()->IsLocationSeparatedFromOtherAgents(
			this,
			Location,
			MinimumSpacing);
}

void UEnemyCombatAgentComponent::LeaveCombat()
{
	ClearThreatIndicator();
	CombatTarget.Reset();
	CancelAttackTokenRequest();
	ReleaseAttackToken();
	ReleaseCombatPosition();
	bHasScreenState = false;
	bIsOnPlayerScreen = false;
	FirstOffscreenTime = -1.0;
	SetCombatState(EWarriorEnemyCombatState::NonAggressive);
}

bool UEnemyCombatAgentComponent::ReserveCombatSlot(FVector& OutSlotLocation)
{
	return ReserveCombatPosition(OutSlotLocation);
}

void UEnemyCombatAgentComponent::ReleaseCombatSlot()
{
	ReleaseCombatPosition();
}

bool UEnemyCombatAgentComponent::GetReservedCombatSlotLocation(FVector& OutSlotLocation) const
{
	return GetReservedCombatPositionLocation(OutSlotLocation);
}

void UEnemyCombatAgentComponent::SetCombatState(const EWarriorEnemyCombatState NewState)
{
	if (CombatState == NewState)
	{
		return;
	}

	const EWarriorEnemyCombatState PreviousState = CombatState;
	CombatState = NewState;
	OnCombatStateChanged.Broadcast(PreviousState, CombatState);
}

EWarriorEnemyCombatState UEnemyCombatAgentComponent::GetCombatState() const
{
	return CombatState;
}

void UEnemyCombatAgentComponent::UpdateAggressionScore()
{
	if (!CombatTarget.IsValid() || !GetWorld())
	{
		AggressionScore = 0.f;
		return;
	}

	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const FVector TargetLocation = CombatTarget->GetActorLocation();
	const float Distance = FVector::Distance(OwnerLocation, TargetLocation);
	const float DistanceAlpha = 1.f - FMath::Clamp(
		FMath::Abs(Distance - PreferredAttackDistance)
			/ FMath::Max(PreferredAttackDistance, 1.f),
		0.f,
		1.f);
	const float WaitingSeconds = bAttackRequestQueued
		? FMath::Max(0.0, GetWorld()->GetTimeSeconds() - AttackRequestStartTime)
		: 0.0;
	FVector ViewForward = CombatTarget->GetActorForwardVector().GetSafeNormal2D();
	if (const APawn* TargetPawn = Cast<APawn>(CombatTarget.Get()))
	{
		if (const APlayerController* PlayerController =
			Cast<APlayerController>(TargetPawn->GetController()))
		{
			FVector ViewLocation;
			FRotator ViewRotation;
			PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
			ViewForward = ViewRotation.Vector().GetSafeNormal2D();
		}
	}

	const FVector TargetToEnemy = (OwnerLocation - TargetLocation).GetSafeNormal2D();
	const float FacingAlpha = FMath::Clamp(
		(FVector::DotProduct(ViewForward, TargetToEnemy) + 1.f)
			* 0.5f,
		0.f,
		1.f);

	AggressionScore =
		BaseAggression
		+ DistanceAlpha * DistanceWeight
		+ static_cast<float>(WaitingSeconds) * WaitingTimeWeight
		+ FacingAlpha * FacingWeight
		+ (bIsOnPlayerScreen ? OnScreenWeight : 0.f);
}

float UEnemyCombatAgentComponent::GetAggressionScore() const
{
	return AggressionScore;
}

void UEnemyCombatAgentComponent::SetCanBecomeAggressive(
	const bool bNewCanBecomeAggressive)
{
	bCanBecomeAggressive = bNewCanBecomeAggressive;
	if (!bCanBecomeAggressive)
	{
		CancelAttackTokenRequest();
		ReleaseAttackToken();
	}
}

bool UEnemyCombatAgentComponent::CanBecomeAggressive() const
{
	return bCanBecomeAggressive
		&& CombatTarget.IsValid()
		&& CombatState != EWarriorEnemyCombatState::HitReact;
}

void UEnemyCombatAgentComponent::SetTargetedByPlayer(
	const bool bNewTargetedByPlayer)
{
	bTargetedByPlayer = bNewTargetedByPlayer;
}

bool UEnemyCombatAgentComponent::IsTargetedByPlayer() const
{
	return bTargetedByPlayer;
}

void UEnemyCombatAgentComponent::SetThreatIndicatorState(
	const EWarriorThreatIndicatorType ThreatType,
	const float Priority,
	const float Duration)
{
	if (AActor* Target = CombatTarget.Get())
	{
		if (UThreatIndicatorComponent* IndicatorComponent =
			Target->FindComponentByClass<UThreatIndicatorComponent>())
		{
			IndicatorComponent->RegisterOrUpdateThreat(
				GetOwner(),
				ThreatType,
				Priority,
				Duration);
		}
	}
}

void UEnemyCombatAgentComponent::ClearThreatIndicator()
{
	if (AActor* Target = CombatTarget.Get())
	{
		if (UThreatIndicatorComponent* IndicatorComponent =
			Target->FindComponentByClass<UThreatIndicatorComponent>())
		{
			IndicatorComponent->RemoveThreat(GetOwner());
		}
	}
}

int32 UEnemyCombatAgentComponent::GetQueuedTokenCost() const
{
	return QueuedTokenCost;
}

float UEnemyCombatAgentComponent::GetQueuedLeaseDuration() const
{
	return QueuedLeaseDuration;
}

int32 UEnemyCombatAgentComponent::GetCombatSlotCount() const
{
	return CombatSlotCount;
}

float UEnemyCombatAgentComponent::GetCombatSlotRadius() const
{
	return CombatSlotRadius;
}

int32 UEnemyCombatAgentComponent::GetReservedCombatSlotIndex() const
{
	return CombatPositionHandle.PositionIndex;
}

int32 UEnemyCombatAgentComponent::GetAttackTokenCost() const
{
	return AttackTokenHandle.Cost;
}

int32 UEnemyCombatAgentComponent::GetEffectiveAggressionPriority() const
{
	if (!CombatTarget.IsValid() || !IsValid(GetOwner()))
	{
		return 0;
	}

	const float DistanceSquared = FVector::DistSquared(
		GetOwner()->GetActorLocation(),
		CombatTarget->GetActorLocation());
	return DistanceSquared <= FMath::Square(AggressionPriorityDistance)
		? AggressionPriority
		: 0;
}

double UEnemyCombatAgentComponent::GetAttackRequestWaitingTime() const
{
	return bAttackRequestQueued && GetWorld()
		? FMath::Max(0.0, GetWorld()->GetTimeSeconds() - AttackRequestStartTime)
		: 0.0;
}

const UEnemyCombatPositionProfile*
UEnemyCombatAgentComponent::GetCombatPositionProfile() const
{
	return CombatPositionProfile;
}

EWarriorWorldQuadrant UEnemyCombatAgentComponent::GetOffscreenWorldQuadrant() const
{
	return OffscreenWorldQuadrant;
}

void UEnemyCombatAgentComponent::RefreshScreenState()
{
	if (!CombatTarget.IsValid() || !IsValid(GetOwner()) || !GetWorld())
	{
		return;
	}

	const APawn* TargetPawn = Cast<APawn>(CombatTarget.Get());
	const APlayerController* PlayerController = TargetPawn
		? Cast<APlayerController>(TargetPawn->GetController())
		: nullptr;
	if (!PlayerController)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewToEnemy = GetOwner()->GetActorLocation() - ViewLocation;
	const bool bInFrontOfCamera = FVector::DotProduct(
		ViewRotation.Vector(),
		ViewToEnemy.GetSafeNormal()) > 0.f;

	FVector2D ScreenPosition;
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	const bool bProjected = PlayerController->ProjectWorldLocationToScreen(
		GetOwner()->GetActorLocation(),
		ScreenPosition,
		true);
	const float EdgePadding = CombatPositionProfile
		? CombatPositionProfile->ScreenEdgePadding
		: 0.04f;
	const FVector2D MinimumScreen(
		ViewportWidth * EdgePadding,
		ViewportHeight * EdgePadding);
	const FVector2D MaximumScreen(
		ViewportWidth * (1.f - EdgePadding),
		ViewportHeight * (1.f - EdgePadding));
	const bool bInsideScreen = bProjected
		&& bInFrontOfCamera
		&& ViewportWidth > 0
		&& ViewportHeight > 0
		&& ScreenPosition.X >= MinimumScreen.X
		&& ScreenPosition.Y >= MinimumScreen.Y
		&& ScreenPosition.X <= MaximumScreen.X
		&& ScreenPosition.Y <= MaximumScreen.Y;

	const bool bPreviousOnScreen = bIsOnPlayerScreen;
	const double CurrentTime = GetWorld()->GetTimeSeconds();
	if (bInsideScreen)
	{
		bIsOnPlayerScreen = true;
		bHasScreenState = true;
		FirstOffscreenTime = -1.0;
	}
	else
	{
		if (FirstOffscreenTime < 0.0)
		{
			FirstOffscreenTime = CurrentTime;
		}

		const float ConfirmTime = CombatPositionProfile
			? CombatPositionProfile->OffscreenConfirmTime
			: 0.15f;
		if (!bHasScreenState || CurrentTime - FirstOffscreenTime >= ConfirmTime)
		{
			bIsOnPlayerScreen = false;
			bHasScreenState = true;
		}
	}

	if (bPreviousOnScreen && bHasScreenState && !bIsOnPlayerScreen)
	{
		OffscreenWorldQuadrant = CalculateCurrentWorldQuadrant();
	}

	if (bHasScreenState && bPreviousOnScreen != bIsOnPlayerScreen)
	{
		ReleaseCombatPosition();
	}
}

void UEnemyCombatAgentComponent::HandleAttackRequestQueued(
	const int32 TokenCost,
	const float LeaseDuration)
{
	if (!bAttackRequestQueued)
	{
		AttackRequestStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	}

	QueuedTokenCost = FMath::Max(1, TokenCost);
	QueuedLeaseDuration = FMath::Max(0.1f, LeaseDuration);
	bAttackRequestQueued = true;
	SetCombatState(EWarriorEnemyCombatState::Queued);
}

void UEnemyCombatAgentComponent::HandleAttackRequestCancelled()
{
	bAttackRequestQueued = false;
	if (!HasAttackToken() && CombatTarget.IsValid())
	{
		SetCombatState(EWarriorEnemyCombatState::Positioning);
	}
}

void UEnemyCombatAgentComponent::HandleAttackTokenGranted(
	const FWarriorAttackTokenHandle& NewHandle)
{
	AttackTokenHandle = NewHandle;
	bAttackRequestQueued = false;
	ReleaseCombatPosition();
	SetCombatState(EWarriorEnemyCombatState::Aggressive);
	OnAttackTokenChanged.Broadcast(true, AttackTokenHandle);
}

void UEnemyCombatAgentComponent::HandleAttackTokenRevoked()
{
	const bool bHadToken = AttackTokenHandle.IsValid();
	AttackTokenHandle.Reset();
	bAttackRequestQueued = false;
	ReleaseCombatPosition();
	ConsecutiveHitReactCount = 0;

	if (bHadToken)
	{
		OnAttackTokenChanged.Broadcast(false, AttackTokenHandle);
	}

	if (CombatTarget.IsValid()
		&& CombatState != EWarriorEnemyCombatState::HitReact)
	{
		SetCombatState(EWarriorEnemyCombatState::Recovering);
		RegisterIdleThreat();
	}
}

void UEnemyCombatAgentComponent::HandleCombatSlotReleased()
{
	CombatPositionHandle.Reset();
}

UEnemyCombatDirectorSubsystem* UEnemyCombatAgentComponent::GetCombatDirector() const
{
	return GetWorld()
		? GetWorld()->GetSubsystem<UEnemyCombatDirectorSubsystem>()
		: nullptr;
}

void UEnemyCombatAgentComponent::RegisterIdleThreat()
{
	SetThreatIndicatorState(EWarriorThreatIndicatorType::NearbyIdle);
}

void UEnemyCombatAgentComponent::BindDeadStateEvent()
{
	if (DeadStateEventHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		DeadStateEventHandle = AbilitySystem->RegisterGameplayTagEvent(
			WarriorGameplayTags::Shared_Status_Dead,
			EGameplayTagEventType::NewOrRemoved).AddUObject(
				this,
				&ThisClass::HandleDeadStateChanged);
	}
}

void UEnemyCombatAgentComponent::HandleDeadStateChanged(
	const FGameplayTag ChangedTag,
	const int32 NewCount)
{
	if (ChangedTag == WarriorGameplayTags::Shared_Status_Dead && NewCount > 0)
	{
		LeaveCombat();
	}
}

void UEnemyCombatAgentComponent::BindHitReactTagEvent()
{
	if (HitReactTagEventHandle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		HitReactTagEventHandle = AbilitySystem->RegisterGameplayTagEvent(
			WarriorGameplayTags::Shared_Ability_HitReact,
			EGameplayTagEventType::NewOrRemoved).AddUObject(
				this,
				&ThisClass::HandleHitReactTagChanged);
	}
}

void UEnemyCombatAgentComponent::HandleHitReactTagChanged(
	const FGameplayTag ChangedTag,
	const int32 NewCount)
{
	if (ChangedTag != WarriorGameplayTags::Shared_Ability_HitReact)
	{
		return;
	}

	if (NewCount > 0)
	{
		// 进入硬直：登记状态；持有 token 的敌人启动 power-play（暂留 + 计数 + 硬顶）。
		SetCombatState(EWarriorEnemyCombatState::HitReact);
		if (HasAttackToken())
		{
			++ConsecutiveHitReactCount;
			if (ConsecutiveHitReactCount > HitReactRetentionCap)
			{
				// 连晕超上限：强制归还 token，换人上。
				// ReleaseAttackToken -> HandleAttackTokenRevoked 会清零计数并保持 HitReact 状态。
				ReleaseAttackToken();
			}
		}
	}
	else
	{
		// 离开硬直：token 仍被暂留（贯穿连晕链），只切回进攻就绪状态，不归还 token。
		if (HasAttackToken())
		{
			SetCombatState(EWarriorEnemyCombatState::Aggressive);
		}
		else if (CombatTarget.IsValid())
		{
			SetCombatState(EWarriorEnemyCombatState::Recovering);
			RegisterIdleThreat();
		}
		else
		{
			SetCombatState(EWarriorEnemyCombatState::NonAggressive);
		}
	}
}

EWarriorWorldQuadrant
UEnemyCombatAgentComponent::CalculateCurrentWorldQuadrant() const
{
	if (!CombatTarget.IsValid() || !IsValid(GetOwner()))
	{
		return EWarriorWorldQuadrant::PositiveXPositiveY;
	}

	const FVector Direction =
		GetOwner()->GetActorLocation() - CombatTarget->GetActorLocation();
	if (Direction.X >= 0.f)
	{
		return Direction.Y >= 0.f
			? EWarriorWorldQuadrant::PositiveXPositiveY
			: EWarriorWorldQuadrant::PositiveXNegativeY;
	}

	return Direction.Y >= 0.f
		? EWarriorWorldQuadrant::NegativeXPositiveY
		: EWarriorWorldQuadrant::NegativeXNegativeY;
}
