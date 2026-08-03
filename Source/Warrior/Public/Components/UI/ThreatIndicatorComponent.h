// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnExtensionComponentBase.h"
#include "Combat/WarriorCombatTypes.h"
#include "ThreatIndicatorComponent.generated.h"

class UCanvas;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnThreatIndicatorsUpdatedDelegate,
	const TArray<FWarriorThreatIndicatorData>&,
	Indicators);

/**
 * Owns threat source data and converts world positions to screen-edge data.
 * Widget creation/pooling remains in a single HUD layer bound to
 * OnThreatIndicatorsUpdated.
 */
UCLASS(ClassGroup = (Warrior), meta = (BlueprintSpawnableComponent))
class WARRIOR_API UThreatIndicatorComponent : public UPawnExtensionComponentBase
{
	GENERATED_BODY()

public:
	UThreatIndicatorComponent();

	UFUNCTION(BlueprintCallable, Category = "Warrior|UI|Threat Indicator")
	void RegisterOrUpdateThreat(
		AActor* SourceActor,
		EWarriorThreatIndicatorType Type,
		float Priority = 0.f,
		float Duration = -1.f);

	UFUNCTION(BlueprintCallable, Category = "Warrior|UI|Threat Indicator")
	void RemoveThreat(AActor* SourceActor);

	UFUNCTION(BlueprintCallable, Category = "Warrior|UI|Threat Indicator")
	void ClearAllThreats();

	UFUNCTION(BlueprintCallable, Category = "Warrior|UI|Threat Indicator")
	void RefreshThreatIndicators();

	UFUNCTION(BlueprintPure, Category = "Warrior|UI|Threat Indicator")
	TArray<FWarriorThreatIndicatorData> GetCurrentIndicators() const;

	UPROPERTY(BlueprintAssignable, Category = "Warrior|UI|Threat Indicator")
	FOnThreatIndicatorsUpdatedDelegate OnThreatIndicatorsUpdated;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat Indicator", meta = (ClampMin = "0.01", Units = "s"))
	float RefreshInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat Indicator", meta = (ClampMin = "0.0"))
	float EdgeMargin = 56.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat Indicator")
	bool bIncludeOnScreenThreats = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat Indicator|Colors")
	FLinearColor NearbyIdleColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat Indicator|Colors")
	FLinearColor MeleeWindupColor = FLinearColor::Red;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat Indicator|Colors")
	FLinearColor RangedWindupColor = FLinearColor(1.f, 0.8f, 0.15f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Threat Indicator|Colors")
	FLinearColor ProjectileColor = FLinearColor(0.75f, 0.25f, 1.f);

private:
	struct FThreatRegistration
	{
		TWeakObjectPtr<AActor> SourceActor;
		EWarriorThreatIndicatorType Type = EWarriorThreatIndicatorType::NearbyIdle;
		float Priority = 0.f;
		double ExpirationTime = -1.0;
	};

	bool BuildIndicatorData(
		APlayerController* PlayerController,
		const FVector2D& ViewportSize,
		const FThreatRegistration& Registration,
		FWarriorThreatIndicatorData& OutData) const;

	FLinearColor GetColorForType(EWarriorThreatIndicatorType Type) const;
	void DrawThreatDebug(UCanvas* Canvas, APlayerController* PlayerController);

	TArray<FThreatRegistration> ThreatRegistrations;

	UPROPERTY(Transient)
	TArray<FWarriorThreatIndicatorData> CurrentIndicators;

	FTimerHandle RefreshTimerHandle;
	FDelegateHandle DebugDrawDelegateHandle;
};
