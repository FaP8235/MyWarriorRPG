// FaP All Rights Reserve

#include "AI/BTService_EnemySeparation.h"

#include "AIController.h"
#include "AI/EnemyCombatDirectorSubsystem.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

UBTService_EnemySeparation::UBTService_EnemySeparation()
{
	NodeName = TEXT("Enemy Separation");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
	bRestartTimerOnEachActivation = true;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UBTService_EnemySeparation::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	const float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UEnemyCombatAgentComponent* Agent = ControlledPawn
		? ControlledPawn->FindComponentByClass<UEnemyCombatAgentComponent>()
		: nullptr;
	if (!Agent || !ControlledPawn || !ControlledPawn->GetWorld())
	{
		return;
	}

	// 正在 MoveTo/寻路时不强制滑开：交给 Detour Crowd Avoidance 做"方向叠加避让"，
	// 避免与 MoveTo 打架造成抽搐。分离只在 idle/站定时生效。
	if (AIController)
	{
		if (const UPathFollowingComponent* PathFollowing = AIController->GetPathFollowingComponent())
		{
			if (PathFollowing->GetStatus() == EPathFollowingStatus::Moving)
			{
				return;
			}
		}
	}

	UEnemyCombatDirectorSubsystem* Director = ControlledPawn->GetWorld()->GetSubsystem<UEnemyCombatDirectorSubsystem>();
	if (!Director)
	{
		return;
	}

	FVector Delta;
	if (Director->GetSeparationDelta(Agent, Delta))
	{
		// 不直接推开（会和移动打架/抽搐）。改为作废当前站位，
		// 让行为树的 UpdateCombatPosition 重选一个"和别人分开"的点位、自己走过去（走，不是滑）。
		Agent->ReleaseCombatPosition();
	}
}

FString UBTService_EnemySeparation::GetStaticDescription() const
{
	return TEXT("近身敌人互相斥力滑开 + 重选站位");
}
