// FaP All Rights Reserve

#include "AI/EnvQueryTest_CombatPosition.h"

#include "AIController.h"
#include "Components/ActorComponent.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "EnvQueryTestCombatPosition"

namespace
{
	APawn* ResolveCombatPositionTestPawn(UObject* QueryOwner)
	{
		if (APawn* Pawn = Cast<APawn>(QueryOwner))
		{
			return Pawn;
		}

		if (const AAIController* AIController = Cast<AAIController>(QueryOwner))
		{
			return AIController->GetPawn();
		}

		if (const UActorComponent* ActorComponent = Cast<UActorComponent>(QueryOwner))
		{
			return Cast<APawn>(ActorComponent->GetOwner());
		}

		return nullptr;
	}
}

UEnvQueryTest_CombatPosition::UEnvQueryTest_CombatPosition(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Cost = EEnvTestCost::Medium;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	TestPurpose = EEnvTestPurpose::Filter;
	FilterType = EEnvTestFilterType::Match;
	BoolValue.DefaultValue = true;
	SetWorkOnFloatValues(false);
}

void UEnvQueryTest_CombatPosition::RunTest(
	FEnvQueryInstance& QueryInstance) const
{
	const UObject* DataOwner = QueryInstance.Owner.Get();
	BoolValue.BindData(DataOwner, QueryInstance.QueryID);
	const bool bWantsAllowedLocation = BoolValue.GetValue();

	APawn* QuerierPawn = ResolveCombatPositionTestPawn(QueryInstance.Owner.Get());
	const UEnemyCombatAgentComponent* Agent = QuerierPawn
		? QuerierPawn->FindComponentByClass<UEnemyCombatAgentComponent>()
		: nullptr;

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, It.GetIndex());
		const bool bAllowed = Agent
			&& Agent->IsCombatPositionLocationAllowed(ItemLocation);
		It.SetScore(
			TestPurpose,
			FilterType,
			bAllowed,
			bWantsAllowedLocation);
	}
}

FText UEnvQueryTest_CombatPosition::GetDescriptionTitle() const
{
	return FText::Format(
		LOCTEXT("Title", "{0}: Current Enemy Combat Zone"),
		Super::GetDescriptionTitle());
}

FText UEnvQueryTest_CombatPosition::GetDescriptionDetails() const
{
	return LOCTEXT(
		"Details",
		"Filters by distance, front angle, offscreen world quadrant, and enemy spacing.");
}

#undef LOCTEXT_NAMESPACE
