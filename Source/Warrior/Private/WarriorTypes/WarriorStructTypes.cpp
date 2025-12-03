// FaP All Rights Reserve


#include "WarriorTypes/WarriorStructTypes.h"
#include "AbilitySystem/Abilities/WarriorHeroGameplayAbility.h"

bool FWarriorHeroAbilitySet::IsValid() const
{
	return InputTag.IsValid() && AbilityToGrant;
}