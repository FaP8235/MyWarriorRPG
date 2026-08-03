// FaP All Rights Reserve

#include "AI/EnemyCombatDirectorSubsystem.h"

#include "Algo/Count.h"
#include "Combat/WarriorCombatDebug.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"
#include "DataAssets/Combat/EnemyCombatPositionProfile.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr int32 PositionIndexBits = 16;
	constexpr int32 PositionIndexMask = (1 << PositionIndexBits) - 1;

	int32 MakePositionKey(
		const EWarriorEnemyPositionZone Zone,
		const int32 PositionIndex)
	{
		return (static_cast<int32>(Zone) << PositionIndexBits)
			| (PositionIndex & PositionIndexMask);
	}

	EWarriorEnemyPositionZone GetZoneFromPositionKey(const int32 PositionKey)
	{
		return static_cast<EWarriorEnemyPositionZone>(
			PositionKey >> PositionIndexBits);
	}

	int32 GetIndexFromPositionKey(const int32 PositionKey)
	{
		return PositionKey & PositionIndexMask;
	}

	float GetTargetViewYaw(const AActor* TargetActor)
	{
		if (!IsValid(TargetActor))
		{
			return 0.f;
		}

		float ViewYaw = TargetActor->GetActorRotation().Yaw;
		if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
		{
			if (const APlayerController* PlayerController =
				Cast<APlayerController>(TargetPawn->GetController()))
			{
				ViewYaw = PlayerController->GetControlRotation().Yaw;
			}
		}
		return ViewYaw;
	}

	bool IsDirectionInWorldQuadrant(
		const FVector& Direction,
		const EWarriorWorldQuadrant Quadrant)
	{
		switch (Quadrant)
		{
		case EWarriorWorldQuadrant::PositiveXPositiveY:
			return Direction.X >= 0.f && Direction.Y >= 0.f;
		case EWarriorWorldQuadrant::NegativeXPositiveY:
			return Direction.X < 0.f && Direction.Y >= 0.f;
		case EWarriorWorldQuadrant::NegativeXNegativeY:
			return Direction.X < 0.f && Direction.Y < 0.f;
		case EWarriorWorldQuadrant::PositiveXNegativeY:
			return Direction.X >= 0.f && Direction.Y < 0.f;
		default:
			return true;
		}
	}
}

void UEnemyCombatDirectorSubsystem::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GetWorld())
	{
		return;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& AgentPtr : RegisteredAgents)
	{
		if (UEnemyCombatAgentComponent* Agent = AgentPtr.Get())
		{
			Agent->RefreshScreenState();
		}
	}
	CleanupInvalidState(CurrentTime);
	GrantQueuedAttackTokens(CurrentTime);
	DrawDebugState();
}

TStatId UEnemyCombatDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(
		UEnemyCombatDirectorSubsystem,
		STATGROUP_Tickables);
}

void UEnemyCombatDirectorSubsystem::ConfigureTokenBudget(const int32 NewMaxTokenBudget)
{
	MaxTokenBudget = FMath::Max(0, NewMaxTokenBudget);
}

int32 UEnemyCombatDirectorSubsystem::GetMaxTokenBudget() const
{
	return MaxTokenBudget;
}

bool UEnemyCombatDirectorSubsystem::GetSeparationDelta(
	const UEnemyCombatAgentComponent* Agent,
	FVector& OutDelta) const
{
	OutDelta = FVector::ZeroVector;
	if (!IsValid(Agent) || !IsValid(Agent->GetOwner()))
	{
		return false;
	}

	constexpr float MaxSeparationStep = 50.f;

	const FVector MyLoc = Agent->GetOwner()->GetActorLocation();
	const float MyRadius = Agent->GetSeparationRadius();
	bool bAnyOverlap = false;

	for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& OtherPtr : RegisteredAgents)
	{
		const UEnemyCombatAgentComponent* Other = OtherPtr.Get();
		if (!Other || Other == Agent || !IsValid(Other->GetOwner()))
		{
			continue;
		}

		const FVector OtherLoc = Other->GetOwner()->GetActorLocation();
		const float CombinedRadius = MyRadius + Other->GetSeparationRadius();
		const FVector2D ToOther(OtherLoc.X - MyLoc.X, OtherLoc.Y - MyLoc.Y);
		const float DistSq = ToOther.SizeSquared();
		if (DistSq >= FMath::Square(CombinedRadius) || DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Dist = FMath::Sqrt(DistSq);
		const float Overlap = CombinedRadius - Dist;
		const float Step = FMath::Min(Overlap * 0.5f, MaxSeparationStep);
		// 远离对方：从对方指向自己的方向
		OutDelta.X += (MyLoc.X - OtherLoc.X) / Dist * Step;
		OutDelta.Y += (MyLoc.Y - OtherLoc.Y) / Dist * Step;
		bAnyOverlap = true;
	}

	return bAnyOverlap;
}

int32 UEnemyCombatDirectorSubsystem::GetUsedTokenBudgetForTarget(AActor* TargetActor) const
{
	const FTargetCombatPool* Pool = TargetPools.Find(TargetActor);
	return Pool ? GetUsedTokenBudget(*Pool) : 0;
}

void UEnemyCombatDirectorSubsystem::GetOnScreenAgentsTargeting(
	AActor* TargetActor,
	TArray<UEnemyCombatAgentComponent*>& OutAgents) const
{
	OutAgents.Reset();
	if (!IsValid(TargetActor))
	{
		return;
	}

	for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& AgentPtr : RegisteredAgents)
	{
		if (UEnemyCombatAgentComponent* Agent = AgentPtr.Get())
		{
			if (IsValid(Agent)
				&& Agent->GetCombatTarget() == TargetActor
				&& Agent->IsOnPlayerScreen())
			{
				OutAgents.Add(Agent);
			}
		}
	}
}

void UEnemyCombatDirectorSubsystem::RegisterAgent(UEnemyCombatAgentComponent* Agent)
{
	if (IsValid(Agent))
	{
		RegisteredAgents.Add(Agent);
	}
}

void UEnemyCombatDirectorSubsystem::UnregisterAgent(UEnemyCombatAgentComponent* Agent)
{
	if (!Agent)
	{
		return;
	}

	CancelAttackRequest(Agent);
	ReleaseAttackToken(Agent);
	ReleaseCombatSlot(Agent);
	RegisteredAgents.Remove(Agent);
}

void UEnemyCombatDirectorSubsystem::QueueAttackRequest(
	UEnemyCombatAgentComponent* Agent,
	const int32 TokenCost,
	const float LeaseDuration)
{
	if (!IsValid(Agent) || !IsValid(Agent->GetCombatTarget()))
	{
		return;
	}

	RegisterAgent(Agent);
	Agent->HandleAttackRequestQueued(
		FMath::Max(1, TokenCost),
		FMath::Max(LeaseDuration, 0.1f));
}

void UEnemyCombatDirectorSubsystem::CancelAttackRequest(UEnemyCombatAgentComponent* Agent)
{
	if (IsValid(Agent))
	{
		Agent->HandleAttackRequestCancelled();
	}
}

void UEnemyCombatDirectorSubsystem::ReleaseAttackToken(UEnemyCombatAgentComponent* Agent)
{
	if (!Agent)
	{
		return;
	}

	for (TPair<TWeakObjectPtr<AActor>, FTargetCombatPool>& PoolPair : TargetPools)
	{
		TArray<int32> LeaseIdsToRemove;
		for (const TPair<int32, FAttackLease>& LeasePair : PoolPair.Value.AttackLeases)
		{
			if (LeasePair.Value.Agent.Get() == Agent)
			{
				LeaseIdsToRemove.Add(LeasePair.Key);
			}
		}

		for (const int32 LeaseId : LeaseIdsToRemove)
		{
			PoolPair.Value.AttackLeases.Remove(LeaseId);
		}
	}

	Agent->HandleAttackTokenRevoked();
}

bool UEnemyCombatDirectorSubsystem::ReserveCombatPosition(
	UEnemyCombatAgentComponent* Agent,
	FWarriorCombatPositionHandle& OutHandle,
	FVector& OutWorldLocation)
{
	OutHandle.Reset();
	OutWorldLocation = FVector::ZeroVector;

	if (!IsValid(Agent) || !IsValid(Agent->GetCombatTarget()))
	{
		return false;
	}

	AActor* TargetActor = Agent->GetCombatTarget();
	FTargetCombatPool& Pool = FindOrAddPool(TargetActor);
	const EWarriorEnemyPositionZone DesiredZone = Agent->GetCombatPositionZone();
	if (DesiredZone == EWarriorEnemyPositionZone::None)
	{
		return false;
	}

	TArray<int32> OldPositionKeys;
	for (const TPair<int32, TWeakObjectPtr<UEnemyCombatAgentComponent>>& PositionPair :
		Pool.PositionOccupants)
	{
		if (PositionPair.Value.Get() == Agent)
		{
			const EWarriorEnemyPositionZone ReservedZone =
				GetZoneFromPositionKey(PositionPair.Key);
			const int32 ReservedIndex = GetIndexFromPositionKey(PositionPair.Key);
			if (ReservedZone == DesiredZone)
			{
				OutHandle.Zone = ReservedZone;
				OutHandle.PositionIndex = ReservedIndex;
				OutWorldLocation = CalculateCombatPositionLocation(
					Agent,
					ReservedZone,
					ReservedIndex);
				return true;
			}

			OldPositionKeys.Add(PositionPair.Key);
		}
	}
	for (const int32 OldPositionKey : OldPositionKeys)
	{
		Pool.PositionOccupants.Remove(OldPositionKey);
	}

	const int32 PositionCount = GetPositionCount(Agent, DesiredZone);
	const FVector TargetToAgent =
		(Agent->GetOwner()->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal2D();
	const float AgentWorldYaw = TargetToAgent.IsNearlyZero()
		? 0.f
		: TargetToAgent.Rotation().Yaw;

	int32 BestPositionIndex = INDEX_NONE;
	float BestAngularDifference = TNumericLimits<float>::Max();
	for (int32 PositionIndex = 0; PositionIndex < PositionCount; ++PositionIndex)
	{
		const int32 PositionKey = MakePositionKey(DesiredZone, PositionIndex);
		const TWeakObjectPtr<UEnemyCombatAgentComponent>* Occupant =
			Pool.PositionOccupants.Find(PositionKey);
		if (Occupant && Occupant->IsValid())
		{
			continue;
		}

		const FVector CandidateLocation = CalculateCombatPositionLocation(
			Agent,
			DesiredZone,
			PositionIndex);
		const UEnemyCombatPositionProfile* Profile =
			Agent->GetCombatPositionProfile();
		const float MinimumSpacing = Profile
			? Profile->MinimumEnemySpacing
			: 140.f;
		if (!IsLocationSeparatedFromOtherAgents(
			Agent,
			CandidateLocation,
			MinimumSpacing))
		{
			continue;
		}
		const FVector CandidateDirection =
			(CandidateLocation - TargetActor->GetActorLocation()).GetSafeNormal2D();
		if (DesiredZone == EWarriorEnemyPositionZone::OffscreenZone
			&& !IsDirectionInWorldQuadrant(
				CandidateDirection,
				Agent->GetOffscreenWorldQuadrant()))
		{
			continue;
		}

		const float CandidateWorldYaw = CandidateDirection.Rotation().Yaw;
		const float AngularDifference = FMath::Abs(
			FMath::FindDeltaAngleDegrees(AgentWorldYaw, CandidateWorldYaw));
		if (AngularDifference < BestAngularDifference)
		{
			BestAngularDifference = AngularDifference;
			BestPositionIndex = PositionIndex;
		}
	}

	if (BestPositionIndex == INDEX_NONE)
	{
		return false;
	}

	Pool.PositionOccupants.Add(
		MakePositionKey(DesiredZone, BestPositionIndex),
		Agent);
	OutHandle.Zone = DesiredZone;
	OutHandle.PositionIndex = BestPositionIndex;
	OutWorldLocation = CalculateCombatPositionLocation(
		Agent,
		DesiredZone,
		BestPositionIndex);
	return true;
}

void UEnemyCombatDirectorSubsystem::ReleaseCombatPosition(
	UEnemyCombatAgentComponent* Agent)
{
	if (!Agent)
	{
		return;
	}

	for (TPair<TWeakObjectPtr<AActor>, FTargetCombatPool>& PoolPair : TargetPools)
	{
		TArray<int32> PositionsToRemove;
		for (const TPair<int32, TWeakObjectPtr<UEnemyCombatAgentComponent>>& PositionPair :
			PoolPair.Value.PositionOccupants)
		{
			if (PositionPair.Value.Get() == Agent)
			{
				PositionsToRemove.Add(PositionPair.Key);
			}
		}

		for (const int32 PositionKey : PositionsToRemove)
		{
			PoolPair.Value.PositionOccupants.Remove(PositionKey);
		}
	}

	Agent->HandleCombatSlotReleased();
}

bool UEnemyCombatDirectorSubsystem::GetReservedCombatPositionLocation(
	const UEnemyCombatAgentComponent* Agent,
	FVector& OutWorldLocation) const
{
	OutWorldLocation = FVector::ZeroVector;
	if (!IsValid(Agent) || !IsValid(Agent->GetCombatTarget()))
	{
		return false;
	}

	const FTargetCombatPool* Pool = TargetPools.Find(Agent->GetCombatTarget());
	if (!Pool)
	{
		return false;
	}

	for (const TPair<int32, TWeakObjectPtr<UEnemyCombatAgentComponent>>& PositionPair :
		Pool->PositionOccupants)
	{
		if (PositionPair.Value.Get() == Agent)
		{
			OutWorldLocation = CalculateCombatPositionLocation(
				Agent,
				GetZoneFromPositionKey(PositionPair.Key),
				GetIndexFromPositionKey(PositionPair.Key));
			return true;
		}
	}

	return false;
}

bool UEnemyCombatDirectorSubsystem::IsLocationSeparatedFromOtherAgents(
	const UEnemyCombatAgentComponent* Agent,
	const FVector& Location,
	const float MinimumSpacing) const
{
	if (!IsValid(Agent) || MinimumSpacing <= 0.f)
	{
		return true;
	}

	const float MinimumSpacingSquared = FMath::Square(MinimumSpacing);
	for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& OtherAgentPtr :
		RegisteredAgents)
	{
		const UEnemyCombatAgentComponent* OtherAgent = OtherAgentPtr.Get();
		if (!IsValid(OtherAgent)
			|| OtherAgent == Agent
			|| OtherAgent->GetCombatTarget() != Agent->GetCombatTarget()
			|| !IsValid(OtherAgent->GetOwner()))
		{
			continue;
		}

		if (FVector::DistSquared2D(
			Location,
			OtherAgent->GetOwner()->GetActorLocation()) < MinimumSpacingSquared)
		{
			return false;
		}
	}

	return true;
}

bool UEnemyCombatDirectorSubsystem::ReserveCombatSlot(
	UEnemyCombatAgentComponent* Agent,
	const int32 SlotCount,
	const float SlotRadius,
	FWarriorCombatSlotHandle& OutHandle,
	FVector& OutWorldLocation)
{
	FWarriorCombatPositionHandle PositionHandle;
	const bool bReserved = ReserveCombatPosition(
		Agent,
		PositionHandle,
		OutWorldLocation);
	OutHandle.SlotIndex = bReserved ? PositionHandle.PositionIndex : INDEX_NONE;
	return bReserved;
}

void UEnemyCombatDirectorSubsystem::ReleaseCombatSlot(
	UEnemyCombatAgentComponent* Agent)
{
	ReleaseCombatPosition(Agent);
}

bool UEnemyCombatDirectorSubsystem::GetReservedCombatSlotLocation(
	const UEnemyCombatAgentComponent* Agent,
	const float SlotRadius,
	FVector& OutWorldLocation) const
{
	return GetReservedCombatPositionLocation(Agent, OutWorldLocation);
}

void UEnemyCombatDirectorSubsystem::CleanupInvalidState(const double CurrentTime)
{
	TArray<TWeakObjectPtr<UEnemyCombatAgentComponent>> AgentsToRemove;
	for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& Agent : RegisteredAgents)
	{
		if (!Agent.IsValid())
		{
			AgentsToRemove.Add(Agent);
		}
	}
	for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& Agent : AgentsToRemove)
	{
		RegisteredAgents.Remove(Agent);
	}

	TArray<TWeakObjectPtr<AActor>> PoolsToRemove;
	for (TPair<TWeakObjectPtr<AActor>, FTargetCombatPool>& PoolPair : TargetPools)
	{
		FTargetCombatPool& Pool = PoolPair.Value;
		if (!Pool.TargetActor.IsValid())
		{
			PoolsToRemove.Add(PoolPair.Key);
			continue;
		}

		TArray<int32> LeaseIdsToRemove;
		for (const TPair<int32, FAttackLease>& LeasePair : Pool.AttackLeases)
		{
			if (!LeasePair.Value.Agent.IsValid()
				|| LeasePair.Value.ExpirationTime <= CurrentTime)
			{
				if (UEnemyCombatAgentComponent* Agent = LeasePair.Value.Agent.Get())
				{
					Agent->HandleAttackTokenRevoked();
				}
				LeaseIdsToRemove.Add(LeasePair.Key);
			}
		}
		for (const int32 LeaseId : LeaseIdsToRemove)
		{
			Pool.AttackLeases.Remove(LeaseId);
		}

		TArray<int32> PositionKeysToRemove;
		for (const TPair<int32, TWeakObjectPtr<UEnemyCombatAgentComponent>>& PositionPair :
			Pool.PositionOccupants)
		{
			if (!PositionPair.Value.IsValid())
			{
				PositionKeysToRemove.Add(PositionPair.Key);
			}
		}
		for (const int32 PositionKey : PositionKeysToRemove)
		{
			Pool.PositionOccupants.Remove(PositionKey);
		}
	}

	for (const TWeakObjectPtr<AActor>& PoolKey : PoolsToRemove)
	{
		TargetPools.Remove(PoolKey);
	}
}

void UEnemyCombatDirectorSubsystem::GrantQueuedAttackTokens(const double CurrentTime)
{
	TArray<UEnemyCombatAgentComponent*> QueuedAgents;
	for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& AgentPtr : RegisteredAgents)
	{
		UEnemyCombatAgentComponent* Agent = AgentPtr.Get();
		if (IsValid(Agent)
			&& Agent->IsAttackRequestQueued()
			&& !Agent->HasAttackToken()
			&& Agent->CanBecomeAggressive()
			&& IsValid(Agent->GetCombatTarget()))
		{
			Agent->UpdateAggressionScore();
			QueuedAgents.Add(Agent);
		}
	}

	QueuedAgents.StableSort(
		[](const UEnemyCombatAgentComponent& Left, const UEnemyCombatAgentComponent& Right)
		{
			if (Left.GetEffectiveAggressionPriority()
				!= Right.GetEffectiveAggressionPriority())
			{
				return Left.GetEffectiveAggressionPriority()
					> Right.GetEffectiveAggressionPriority();
			}
			if (Left.IsTargetedByPlayer() != Right.IsTargetedByPlayer())
			{
				return Left.IsTargetedByPlayer();
			}
			if (!FMath::IsNearlyEqual(
				Left.GetAggressionScore(),
				Right.GetAggressionScore()))
			{
				return Left.GetAggressionScore() > Right.GetAggressionScore();
			}
			return Left.GetAttackRequestWaitingTime()
				> Right.GetAttackRequestWaitingTime();
		});

	for (UEnemyCombatAgentComponent* Agent : QueuedAgents)
	{
		FTargetCombatPool& Pool = FindOrAddPool(Agent->GetCombatTarget());
		const int32 TokenCost = Agent->GetQueuedTokenCost();
		if (GetUsedTokenBudget(Pool) + TokenCost > MaxTokenBudget)
		{
			continue;
		}

		FWarriorAttackTokenHandle Handle;
		Handle.Id = NextTokenId++;
		Handle.Cost = TokenCost;

		FAttackLease Lease;
		Lease.Agent = Agent;
		Lease.Handle = Handle;
		Lease.ExpirationTime = CurrentTime + Agent->GetQueuedLeaseDuration();
		Pool.AttackLeases.Add(Handle.Id, Lease);
		Agent->HandleAttackTokenGranted(Handle);
	}
}

void UEnemyCombatDirectorSubsystem::DrawDebugState()
{
	if (!WarriorCombatDebug::IsDirectorEnabled() || !GetWorld())
	{
		return;
	}

	const float TextScale = WarriorCombatDebug::GetTextScale();

	// 画一段弧（XY 平面，按 Yaw 采样、折线连接），用于区域轮廓。
	auto DrawArc = [this](
		const FVector& Center,
		const float Radius,
		const float StartYawDeg,
		const float EndYawDeg,
		const int32 NumSegments,
		const FColor& Color,
		const float Thickness)
	{
		const int32 SegmentCount = FMath::Max(1, NumSegments);
		auto PointAt = [&Center, Radius](const float YawDeg)
		{
			const float Rad = FMath::DegreesToRadians(YawDeg);
			return Center + FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.f) * Radius;
		};

		FVector Prev = PointAt(StartYawDeg);
		for (int32 i = 1; i <= SegmentCount; ++i)
		{
			const float Alpha = StartYawDeg
				+ (EndYawDeg - StartYawDeg) * static_cast<float>(i) / static_cast<float>(SegmentCount);
			const FVector Curr = PointAt(Alpha);
			DrawDebugLine(GetWorld(), Prev, Curr, Color, false, 0.f, 0, Thickness);
			Prev = Curr;
		}
	};

	for (const TPair<TWeakObjectPtr<AActor>, FTargetCombatPool>& PoolPair : TargetPools)
	{
		AActor* TargetActor = PoolPair.Value.TargetActor.Get();
		if (!IsValid(TargetActor))
		{
			continue;
		}

		const UEnemyCombatAgentComponent* ReferenceAgent = nullptr;
		for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& AgentPtr : RegisteredAgents)
		{
			if (const UEnemyCombatAgentComponent* Agent = AgentPtr.Get())
			{
				if (Agent->GetCombatTarget() == TargetActor)
				{
					ReferenceAgent = Agent;
					break;
				}
			}
		}

		if (ReferenceAgent)
		{
			// 区域轮廓：前方扇环（月牙，红）+ 远处双同心圆（蓝），朝玩家镜头 Yaw。
			const UEnemyCombatPositionProfile* Profile = ReferenceAgent->GetCombatPositionProfile();
			const float FrontMin = Profile ? Profile->FrontMinDistance : 220.f;
			const float FrontMax = Profile ? Profile->FrontMaxDistance : 350.f;
			const float FrontHalf = Profile ? Profile->FrontHalfAngle : 65.f;
			const float IdleMin = Profile ? Profile->IdleMinDistance : 350.f;
			const float IdleMax = Profile ? Profile->IdleMaxDistance : 550.f;

			float ViewYaw = TargetActor->GetActorRotation().Yaw;
			if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
			{
				if (const APlayerController* PlayerController = Cast<APlayerController>(TargetPawn->GetController()))
				{
					ViewYaw = PlayerController->GetControlRotation().Yaw;
				}
			}

			const FVector Center = TargetActor->GetActorLocation();
			const FColor FrontColor(255, 60, 60);
			const FColor IdleColor(80, 180, 255);

			// 前方扇环：外弧 + 内弧 + 两条径向直边。
			const float StartYaw = ViewYaw - FrontHalf;
			const float EndYaw = ViewYaw + FrontHalf;
			DrawArc(Center, FrontMax, StartYaw, EndYaw, 24, FrontColor, 2.f);
			DrawArc(Center, FrontMin, StartYaw, EndYaw, 24, FrontColor, 2.f);
			DrawDebugLine(GetWorld(),
				Center + FVector(FMath::Cos(FMath::DegreesToRadians(StartYaw)), FMath::Sin(FMath::DegreesToRadians(StartYaw)), 0.f) * FrontMin,
				Center + FVector(FMath::Cos(FMath::DegreesToRadians(StartYaw)), FMath::Sin(FMath::DegreesToRadians(StartYaw)), 0.f) * FrontMax,
				FrontColor, false, 0.f, 0, 2.f);
			DrawDebugLine(GetWorld(),
				Center + FVector(FMath::Cos(FMath::DegreesToRadians(EndYaw)), FMath::Sin(FMath::DegreesToRadians(EndYaw)), 0.f) * FrontMin,
				Center + FVector(FMath::Cos(FMath::DegreesToRadians(EndYaw)), FMath::Sin(FMath::DegreesToRadians(EndYaw)), 0.f) * FrontMax,
				FrontColor, false, 0.f, 0, 2.f);

			// 远处双同心圆。
			DrawArc(Center, IdleMin, 0.f, 360.f, 48, IdleColor, 2.f);
			DrawArc(Center, IdleMax, 0.f, 360.f, 48, IdleColor, 2.f);

			const EWarriorEnemyPositionZone Zones[] =
			{
				EWarriorEnemyPositionZone::FrontZone,
				EWarriorEnemyPositionZone::IdleRing,
				EWarriorEnemyPositionZone::OffscreenZone
			};
			for (const EWarriorEnemyPositionZone Zone : Zones)
			{
				const FColor ZoneColor = Zone == EWarriorEnemyPositionZone::FrontZone
					? FColor::Red
					: (Zone == EWarriorEnemyPositionZone::IdleRing
						? FColor(80, 180, 255)
						: FColor::Purple);
				const int32 PositionCount = GetPositionCount(ReferenceAgent, Zone);
				for (int32 PositionIndex = 0; PositionIndex < PositionCount; ++PositionIndex)
				{
					const int32 PositionKey = MakePositionKey(Zone, PositionIndex);
					const TWeakObjectPtr<UEnemyCombatAgentComponent>* Occupant =
						PoolPair.Value.PositionOccupants.Find(PositionKey);
					const bool bOccupied = Occupant && Occupant->IsValid();
					const FVector PositionLocation = CalculateCombatPositionLocation(
						ReferenceAgent,
						Zone,
						PositionIndex);

					DrawDebugSphere(
						GetWorld(),
						PositionLocation,
						bOccupied ? 24.f : 12.f,
						10,
						ZoneColor,
						false,
						0.f,
						0,
						bOccupied ? 4.f : 1.5f);

					if (bOccupied)
					{
						DrawDebugString(
							GetWorld(),
							PositionLocation + FVector(0.f, 0.f, 35.f),
							FString::Printf(TEXT("Position %d"), PositionIndex),
							nullptr,
							ZoneColor,
							0.f,
							true,
							TextScale * 0.75f);

						if (const UEnemyCombatAgentComponent* OccupyingAgent = Occupant->Get())
						{
					DrawDebugLine(
						GetWorld(),
						OccupyingAgent->GetOwner()->GetActorLocation(),
						PositionLocation,
						ZoneColor,
						false,
						0.f,
						0,
						2.f);
						}
					}
				}
			}
		}

		DrawDebugString(
			GetWorld(),
			TargetActor->GetActorLocation() + FVector(0.f, 0.f, 210.f),
			FString::Printf(
				TEXT("COMBAT DIRECTOR\nTokens: %d / %d\nQueued: %d"),
				GetUsedTokenBudget(PoolPair.Value),
				MaxTokenBudget,
				Algo::CountIf(
					RegisteredAgents,
					[TargetActor](const TWeakObjectPtr<UEnemyCombatAgentComponent>& AgentPtr)
					{
						const UEnemyCombatAgentComponent* Agent = AgentPtr.Get();
						return Agent
							&& Agent->GetCombatTarget() == TargetActor
							&& Agent->IsAttackRequestQueued();
					})),
			nullptr,
			FColor::Cyan,
			0.f,
			true,
			TextScale);
	}

	for (const TWeakObjectPtr<UEnemyCombatAgentComponent>& AgentPtr : RegisteredAgents)
	{
		UEnemyCombatAgentComponent* Agent = AgentPtr.Get();
		if (!IsValid(Agent) || !IsValid(Agent->GetOwner()))
		{
			continue;
		}

		// 分离检测器（白圆）
		DrawDebugCircle(
			GetWorld(),
			Agent->GetOwner()->GetActorLocation(),
			Agent->GetSeparationRadius(),
			24,
			FColor::White,
			false,
			0.f,
			0,
			2.f,
			FVector(1.f, 0.f, 0.f),
			FVector(0.f, 1.f, 0.f),
			false);

		Agent->UpdateAggressionScore();
		const bool bHasToken = Agent->HasAttackToken();
		const bool bQueued = Agent->IsAttackRequestQueued();
		const FColor StateColor = bHasToken
			? FColor::Green
			: (bQueued ? FColor::Orange : FColor::Cyan);
		const UEnum* StateEnum = StaticEnum<EWarriorEnemyCombatState>();
		const UEnum* ZoneEnum = StaticEnum<EWarriorEnemyPositionZone>();
		const FString StateName = StateEnum
			? StateEnum->GetNameStringByValue(
				static_cast<int64>(Agent->GetCombatState()))
			: TEXT("Unknown");
		const FString TokenText = bHasToken
			? FString::Printf(TEXT("Granted (%d)"), Agent->GetAttackTokenCost())
			: (bQueued
				? FString::Printf(TEXT("Waiting (%d)"), Agent->GetQueuedTokenCost())
				: TEXT("None"));
		const FString ZoneName = ZoneEnum
			? ZoneEnum->GetDisplayNameTextByValue(
				static_cast<int64>(Agent->GetCombatPositionZone())).ToString()
			: TEXT("Unknown");

		DrawDebugString(
			GetWorld(),
			Agent->GetOwner()->GetActorLocation() + FVector(0.f, 0.f, 170.f),
			FString::Printf(
				TEXT("%s\nState: %s\nScore: %.2f\nToken: %s\nZone: %s\nPosition: %d\nOn Screen: %s"),
				*GetNameSafe(Agent->GetOwner()),
				*StateName,
				Agent->GetAggressionScore(),
				*TokenText,
				*ZoneName,
				Agent->GetReservedCombatSlotIndex(),
				Agent->IsOnPlayerScreen() ? TEXT("Yes") : TEXT("No")),
			nullptr,
			StateColor,
			0.f,
			true,
			TextScale);

		if (AActor* CombatTarget = Agent->GetCombatTarget())
		{
			DrawDebugLine(
				GetWorld(),
				Agent->GetOwner()->GetActorLocation(),
				CombatTarget->GetActorLocation(),
				StateColor,
				false,
				0.f,
				0,
				bHasToken ? 4.f : 1.f);
		}
	}
}

int32 UEnemyCombatDirectorSubsystem::GetUsedTokenBudget(const FTargetCombatPool& Pool) const
{
	int32 UsedBudget = 0;
	for (const TPair<int32, FAttackLease>& LeasePair : Pool.AttackLeases)
	{
		UsedBudget += LeasePair.Value.Handle.Cost;
	}
	return UsedBudget;
}

UEnemyCombatDirectorSubsystem::FTargetCombatPool&
UEnemyCombatDirectorSubsystem::FindOrAddPool(AActor* TargetActor)
{
	FTargetCombatPool& Pool = TargetPools.FindOrAdd(TargetActor);
	Pool.TargetActor = TargetActor;
	return Pool;
}

FVector UEnemyCombatDirectorSubsystem::CalculateCombatPositionLocation(
	const UEnemyCombatAgentComponent* Agent,
	const EWarriorEnemyPositionZone Zone,
	const int32 PositionIndex) const
{
	if (!IsValid(Agent) || !IsValid(Agent->GetCombatTarget()))
	{
		return FVector::ZeroVector;
	}

	const UEnemyCombatPositionProfile* Profile = Agent->GetCombatPositionProfile();
	const int32 PositionCount = GetPositionCount(Agent, Zone);
	float MinDistance = Agent->GetCombatSlotRadius();
	float MaxDistance = Agent->GetCombatSlotRadius();
	float WorldYaw = 0.f;

	switch (Zone)
	{
	case EWarriorEnemyPositionZone::FrontZone:
	{
		MinDistance = Profile ? Profile->FrontMinDistance : 220.f;
		MaxDistance = Profile ? Profile->FrontMaxDistance : 350.f;
		const float HalfAngle = Profile ? Profile->FrontHalfAngle : 65.f;
		const float PositionAlpha =
			(static_cast<float>(PositionIndex) + 0.5f)
			/ FMath::Max(1, PositionCount);
		const float LocalYaw = FMath::Lerp(-HalfAngle, HalfAngle, PositionAlpha);
		WorldYaw = GetTargetViewYaw(Agent->GetCombatTarget()) + LocalYaw;
		break;
	}
	case EWarriorEnemyPositionZone::IdleRing:
		MinDistance = Profile ? Profile->IdleMinDistance : Agent->GetCombatSlotRadius();
		MaxDistance = Profile ? Profile->IdleMaxDistance : Agent->GetCombatSlotRadius();
		WorldYaw = 360.f * (static_cast<float>(PositionIndex) + 0.5f)
			/ FMath::Max(1, PositionCount);
		break;
	case EWarriorEnemyPositionZone::OffscreenZone:
		MinDistance = Profile ? Profile->OffscreenMinDistance : Agent->GetCombatSlotRadius();
		MaxDistance = Profile ? Profile->OffscreenMaxDistance : Agent->GetCombatSlotRadius();
		WorldYaw = 360.f * (static_cast<float>(PositionIndex) + 0.5f)
			/ FMath::Max(1, PositionCount);
		break;
	default:
		return FVector::ZeroVector;
	}

	const float GuideDistance = FMath::Max(0.f, (MinDistance + MaxDistance) * 0.5f);
	const FVector Direction = FRotator(0.f, WorldYaw, 0.f).Vector();
	return Agent->GetCombatTarget()->GetActorLocation() + Direction * GuideDistance;
}

int32 UEnemyCombatDirectorSubsystem::GetPositionCount(
	const UEnemyCombatAgentComponent* Agent,
	const EWarriorEnemyPositionZone Zone) const
{
	if (!IsValid(Agent))
	{
		return 1;
	}

	const UEnemyCombatPositionProfile* Profile = Agent->GetCombatPositionProfile();
	switch (Zone)
	{
	case EWarriorEnemyPositionZone::FrontZone:
		return FMath::Max(1, Profile
			? Profile->FrontPositionCount
			: FMath::Min(4, Agent->GetCombatSlotCount()));
	case EWarriorEnemyPositionZone::IdleRing:
		return FMath::Max(1, Profile
			? Profile->IdlePositionCount
			: Agent->GetCombatSlotCount());
	case EWarriorEnemyPositionZone::OffscreenZone:
		return FMath::Max(4, Profile
			? Profile->OffscreenPositionCount
			: FMath::Max(8, Agent->GetCombatSlotCount()));
	default:
		return 1;
	}
}

FVector UEnemyCombatDirectorSubsystem::CalculateSlotLocation(
	AActor* TargetActor,
	const int32 SlotIndex,
	const int32 SlotCount,
	const float SlotRadius) const
{
	if (!IsValid(TargetActor))
	{
		return FVector::ZeroVector;
	}

	FRotator BasisRotation = TargetActor->GetActorRotation();
	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		if (const APlayerController* PlayerController =
			Cast<APlayerController>(TargetPawn->GetController()))
		{
			BasisRotation.Yaw = PlayerController->GetControlRotation().Yaw;
		}
	}

	const float SlotAngle = 360.f * static_cast<float>(SlotIndex)
		/ FMath::Max(1, SlotCount);
	const FVector SlotDirection =
		FRotator(0.f, BasisRotation.Yaw + SlotAngle, 0.f).Vector();
	return TargetActor->GetActorLocation() + SlotDirection * SlotRadius;
}
