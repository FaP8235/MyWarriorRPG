// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/WarriorHeroGameplayAbility.h"
#include "HeroGameplayAbility_TargetLock.generated.h"

class UWarriorWidgetBase;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class WARRIOR_API UHeroGameplayAbility_TargetLock : public UWarriorHeroGameplayAbility
{
	GENERATED_BODY()
public:
	UHeroGameplayAbility_TargetLock();

protected:
	// ~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	// ~ End UGameplayAbility Interface

	// ��֡����Ŀ�꣬����ָʾ��λ��
	UFUNCTION(BlueprintCallable)
	void OnTargetLockTick(float DeltaTime);

	// �л�����Ŀ��
	UFUNCTION(BlueprintCallable)
	void SwitchTarget(const FGameplayTag& InSwitchDirectionTag);

private:
	// ���Խ���Ŀ������
	void TryLockOnTarget();

	// ��ȡ������Ŀ�꼯��
	void GetAvailableActorsToLock();

	// ���Actor�Ƿ�����жԡ������롢��Ұ����������
	bool IsValidTargetToLock(AActor* TargetActor, bool bRequireLineOfSight = true);

	// ����������Ŀ��֮���Ƿ���Visibility�ڵ�
	bool HasLineOfSightToTarget(AActor* TargetActor);

	// ��ȫ���Actor�Ƿ�ӵ��ָ��GameplayTag
	bool DoesActorHaveGameplayTag(AActor* TargetActor, const FGameplayTag& TagToCheck) const;

	// �ӵ�ǰ��Ч��ѡ��ѡ����Ŀ��
	bool TrySelectNewTarget(AActor* TargetToExclude = nullptr);

	// ���µ�ǰ����Ŀ�꣬�������¾�Ŀ��нǾ����Ƿ�ƽ���л���ͷ
	void SetCurrentLockedActor(AActor* NewLockedActor);

	// ��ȡ��������Ŀ�����Ŀ��
	AActor* GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors);

	// ��ȡ������Ŀ����Χ��Ŀ�꣬�����Ϊ��������Ŀ�꣬�Ա��л�����
	void GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft, TArray<AActor*>& OutActorsOnRight);

	// ����Ŀ��ָʾ������ȡ��ָ��
	void DrawTargetLockWidget();

	// ����Ŀ��ָʾ��λ�ã���ʹ����ӻ�
	void SetTargetLockWidgetPosition();

	// ����������ΪĿ������ʱ����
	void InitTargetLockMovement();

	// ������Ŀ����ص�����ӳ�������ģ����л�Ŀ�꣩
	void InitTargetLockMappingContext();

	// ȡ��Ŀ����������
	void CancelTargetLockAbility();

	// ����������������������
	void CleanUp();

	// �ָ�����Ϊ������������
	void ResetTargetLockMovement();

	// �������Ŀ����ص�����ӳ��������
	void ResetTargetLockMappingContext();

	// ������Ŀ���������
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float BoxTraceDistance = 5000.f;

	// ��������������εİ��
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock", meta = (ClampMin = "1.0", ClampMax = "89.0", Units = "Degrees"))
	float TargetLockViewConeHalfAngle = 60.f;

	// ��ǰĿ�걻�ڵ����Ա��������Ŀ���ʱ��
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock", meta = (ClampMin = "0.0", Units = "Seconds"))
	float TargetOcclusionGraceTime = 0.5f;

	// ��Χ���ʹ�õ�Object���ͣ�����ԭ�������Լ���������ͼ���ã�
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TArray< TEnumAsByte < EObjectTypeQuery > > BoxTraceChannel;

	// ָʾ��UI����
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	TSubclassOf<UWarriorWidgetBase> TargetLockWidgetClass;

	// ����ʱ��ͷ�ƶ��Ĳ�ֵ
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockRotationInterpSpeed = 5.f;

	// �¾�Ŀ�������ҵļнǴﵽ��ֵʱ�����ö����ľ�ͷ�л���ֵ
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
	float TargetSwitchSmoothAngleThreshold = 45.f;

	// ��Ƕ��л�Ŀ��ʱRInterpTo�Ĳ�ֵϵ�����ǲ�Խ��ʵ��ת��Խ�죬�ӽ�Ŀ��ʱ����Ȼ����
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock", meta = (ClampMin = "0.1"))
	float TargetSwitchRotationInterpSpeed = 7.5f;

	// ��ͷ����Ŀ���ƫ����С�ڸ�ֵ�󣬽����л���ֵ״̬
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock", meta = (ClampMin = "0.1", Units = "Degrees"))
	float TargetSwitchCompletionTolerance = 2.f;

	// ����ʱ���������
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockMaxWalkSpeed = 150.f;

	// Ŀ��������ص�����ӳ��������
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	UInputMappingContext* TargetLockMappingContext;

	// ��ͷƫ��ֵ��ʹ��ͷƫ���ɫ���󷽣�������ҽ�ɫ�ڵ�����
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock")
	float TargetLockCameraOffsetDistance = 20.f;

	// 死区半角：目标在镜头方向该角度内时，yaw 不再逐帧微调（最小修正，防抖）
	UPROPERTY(EditDefaultsOnly, Category = "Target Lock", meta = (ClampMin = "0.0", Units = "Degrees"))
	float TargetLockDeadzoneAngle = 10.f;

	// ������Ŀ�꼯��
	UPROPERTY()
	TArray<AActor*> AvailableActorsToLock;

	// ��ǰ������Ŀ��
	UPROPERTY()
	AActor* CurrentLockedActor;

	// �ѻ��Ƴ���Ŀ��ָʾ��UI
	UPROPERTY()
	UWarriorWidgetBase* DrawnTargetLockWidget;

	// ָʾ��UI�ߴ磬������UIû����ȷ�Խ�Ŀ��ʱʹUI��ʱ���ɼ�
	UPROPERTY()
	FVector2D TargetLockWidgetSize = FVector2D::ZeroVector;

	// ����������٣������ڽ���Ŀ��ʱ�ָ�����
	UPROPERTY()
	float CachedDefaultMaxWalkSpeed = 0.f;

	// �����������ǰ�Ľ�ɫ�ƶ���ת���ã�����ʱ�����ָ�
	bool bCachedOrientRotationToMovement = true;
	bool bCachedUseControllerDesiredRotation = false;
	bool bHasCachedTargetLockMovementSettings = false;

	// ��ǰ�Ƿ��ڴ�Ƕ�Ŀ���л��ľ�ͷƽ��������
	bool bIsSmoothingTargetSwitch = false;

	// ��ǰĿ���Ѿ��������ڵ���ʱ��
	float CurrentTargetOccludedTime = 0.f;
};
