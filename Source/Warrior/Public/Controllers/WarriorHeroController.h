// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"
#include "WarriorHeroController.generated.h"

/**
 * 
 */
UCLASS()
class WARRIOR_API AWarriorHeroController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	AWarriorHeroController();

	//~ Begin IGenericTeamAgent Interface.
	virtual FGenericTeamId GetGenericTeamId() const override;
	//~ End IGenericTeamAgent Interface.
private:
	FGenericTeamId HeroTeamID;
};
