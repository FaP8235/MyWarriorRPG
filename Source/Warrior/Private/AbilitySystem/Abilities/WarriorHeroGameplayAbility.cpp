// FaP All Rights Reserve


#include "AbilitySystem/Abilities/WarriorHeroGameplayAbility.h"
#include "Characters/WarriorHeroCharacter.h"
#include "Controllers/WarriorHeroController.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "Components/Combat/MeleeTargetingComponent.h"
#include "Components/Combat/AttackAssistComponent.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "WarriorGameplayTags.h"

AWarriorHeroCharacter* UWarriorHeroGameplayAbility::GetHeroCharacterFromActorInfo()
{
    if (!CachedWarriorHeroCharacter.IsValid())
    {
        CachedWarriorHeroCharacter = Cast<AWarriorHeroCharacter>(CurrentActorInfo->AvatarActor);
    }

    return CachedWarriorHeroCharacter.IsValid() ? CachedWarriorHeroCharacter.Get() : nullptr;
}

AWarriorHeroController* UWarriorHeroGameplayAbility::GetHeroControllerFromActorInfo()
{
    if (!CachedWarriorHeroController.IsValid())
    {
        CachedWarriorHeroController = Cast<AWarriorHeroController>(CurrentActorInfo->PlayerController);
    }
    return CachedWarriorHeroController.IsValid() ? CachedWarriorHeroController.Get() : nullptr;
}

UHeroCombatComponent* UWarriorHeroGameplayAbility::GetHeroCombatComponentFromActorInfo()
{
    return GetHeroCharacterFromActorInfo()->GetHeroCombatComponent();
}

UHeroUIComponent* UWarriorHeroGameplayAbility::GetHeroUIComponentFromActorInfo()
{
    return GetHeroCharacterFromActorInfo()->GetHeroUIComponent();
}

UMeleeTargetingComponent* UWarriorHeroGameplayAbility::GetMeleeTargetingComponentFromActorInfo()
{
    if (AWarriorHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo())
    {
        return HeroCharacter->GetMeleeTargetingComponent();
    }
    return nullptr;
}

UAttackAssistComponent* UWarriorHeroGameplayAbility::GetAttackAssistComponentFromActorInfo()
{
    if (AWarriorHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo())
    {
        return HeroCharacter->GetAttackAssistComponent();
    }
    return nullptr;
}

bool UWarriorHeroGameplayAbility::PrepareMeleeAttack(
    UDataAsset_CombatAttackProfile* AttackProfile,
    const FVector2D InputIntent,
    FWarriorCombatAttackContext& OutAttackContext)
{
    UMeleeTargetingComponent* TargetingComponent = GetMeleeTargetingComponentFromActorInfo();
    UAttackAssistComponent* AssistComponent = GetAttackAssistComponentFromActorInfo();
    if (!TargetingComponent
        || !TargetingComponent->SelectMeleeTarget(AttackProfile, InputIntent, OutAttackContext))
    {
        return false;
    }

    if (AssistComponent && OutAttackContext.bHasTarget)
    {
        FTransform WarpTargetTransform;
        AssistComponent->PrepareAttackAssist(OutAttackContext, WarpTargetTransform);
    }
    return true;
}

void UWarriorHeroGameplayAbility::FinishMeleeAttack()
{
    if (UAttackAssistComponent* AssistComponent = GetAttackAssistComponentFromActorInfo())
    {
        AssistComponent->ClearAttackAssist();
    }
}

FGameplayEffectSpecHandle UWarriorHeroGameplayAbility::MakeHeroDamageEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass, float InWeaponBaseDamage, FGameplayTag InCurrentAttackTypeTag, int32 InUsedComboCount)
{
    check(EffectClass);

    FGameplayEffectContextHandle ContextHandle = GetWarriorAbilitySystemComponentFromActorInfo()->MakeEffectContext();
    ContextHandle.SetAbility(this);
    ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
    ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

    FGameplayEffectSpecHandle EffectSpecHandle = GetWarriorAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(
        EffectClass,
        GetAbilityLevel(),
        ContextHandle
    );

    EffectSpecHandle.Data->SetSetByCallerMagnitude(
        WarriorGameplayTags::Shared_SetByCaller_BaseDamage,
        InWeaponBaseDamage
    );

    if (InCurrentAttackTypeTag.IsValid())
    {
        EffectSpecHandle.Data->SetSetByCallerMagnitude(InCurrentAttackTypeTag, InUsedComboCount);
    }
    return EffectSpecHandle;
}

bool UWarriorHeroGameplayAbility::GetAbilityRemainingCooldownByTag(FGameplayTag InCooldownTag, float& TotalCooldownTime, float& RemainingCooldownTime)
{
    check(InCooldownTag.IsValid());

    FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(InCooldownTag.GetSingleTagContainer());

    TArray< TPair <float, float> > TimeRemainingAndDuration = GetAbilitySystemComponentFromActorInfo()->GetActiveEffectsTimeRemainingAndDuration(CooldownQuery);

    if (!TimeRemainingAndDuration.IsEmpty())
    {
        RemainingCooldownTime = TimeRemainingAndDuration[0].Key;
        TotalCooldownTime = TimeRemainingAndDuration[0].Value;
    }

    return RemainingCooldownTime > 0.f;
}
