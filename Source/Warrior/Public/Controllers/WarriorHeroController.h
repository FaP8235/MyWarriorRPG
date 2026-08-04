// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "WarriorHeroController.generated.h"

class UInputAction;
class UGMPanelWidget;

UCLASS()
class WARRIOR_API AWarriorHeroController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AWarriorHeroController();

	//~ Begin IGenericTeamAgent Interface.
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgent Interface.

protected:
	virtual void SetupInputComponent() override;

private:
	FGenericTeamId HeroTeamID;

	// ── GM 面板 ──
	UPROPERTY(EditDefaultsOnly, Category = "GM Panel")
	TObjectPtr<UInputAction> GMPanelInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "GM Panel")
	TSubclassOf<UGMPanelWidget> GMPanelWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGMPanelWidget> GMPanelInstance;

	void ToggleGMPanel();
};
