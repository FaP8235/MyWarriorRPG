// FaP All Rights Reserve


#include "AbilitySystem/Abilities/WarriorEnemyGameplayAbility.h"
#include "Characters/WarriorEnemyCharacter.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "WarriorGameplayTags.h"
#include "Components/Combat/EnemyCombatAgentComponent.h"

void UWarriorEnemyGameplayAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const bool bReplicateEndAbility,
    const bool bWasCancelled)
{
    if (bReleaseAttackTokenOnEnd)
    {
        if (UEnemyCombatAgentComponent* Agent = GetEnemyCombatAgentComponentFromActorInfo())
        {
            Agent->SetThreatIndicatorState(EWarriorThreatIndicatorType::NearbyIdle);

            // Power-play：攻击被取消（典型是被硬直打断）时不归还 token，让 token 贯穿整条连晕链。
            // 用 bWasCancelled 而非查 HitReact Tag，规避 GAS 提交时序的不确定性（见 PowerPlay_Design_CN.md §9）。
            // 若玩测发现非硬直的取消频繁导致 token 暂留过久，再收紧为额外检查 Shared.Ability.HitReact Tag。
            if (!bWasCancelled)
            {
                Agent->ReleaseAttackToken();
            }
        }
    }

    Super::EndAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        bReplicateEndAbility,
        bWasCancelled);
}

AWarriorEnemyCharacter* UWarriorEnemyGameplayAbility::GetEnemyCharacterFromActorInfo()
{
    if (!CachedWarriorEnemyCharacter.IsValid())
    {
        CachedWarriorEnemyCharacter = Cast<AWarriorEnemyCharacter>(CurrentActorInfo->AvatarActor);
    }
    return CachedWarriorEnemyCharacter.IsValid() ? CachedWarriorEnemyCharacter.Get() : nullptr;
}

UEnemyCombatComponent* UWarriorEnemyGameplayAbility::GetEnemyCombatComponentFromActorInfo()
{
    return GetEnemyCharacterFromActorInfo()->GetEnemyCombatComponent();
}

UEnemyCombatAgentComponent* UWarriorEnemyGameplayAbility::GetEnemyCombatAgentComponentFromActorInfo()
{
    if (AWarriorEnemyCharacter* EnemyCharacter = GetEnemyCharacterFromActorInfo())
    {
        return EnemyCharacter->GetEnemyCombatAgentComponent();
    }
    return nullptr;
}

void UWarriorEnemyGameplayAbility::SetAttackThreatIndicator(
    const EWarriorThreatIndicatorType ThreatType,
    const float Priority,
    const float Duration)
{
    if (UEnemyCombatAgentComponent* Agent = GetEnemyCombatAgentComponentFromActorInfo())
    {
        Agent->SetThreatIndicatorState(ThreatType, Priority, Duration);
    }
}

FGameplayEffectSpecHandle UWarriorEnemyGameplayAbility::MakeEnemyDamageEffectSpecHandle(TSubclassOf<UGameplayEffect> EffectClass, const FScalableFloat& InDamageScalableFloat)
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
        InDamageScalableFloat.GetValueAtLevel(GetAbilityLevel())
    );

    return EffectSpecHandle;
}
