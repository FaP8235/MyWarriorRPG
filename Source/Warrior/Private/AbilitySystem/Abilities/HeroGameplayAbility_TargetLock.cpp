// FaP All Rights Reserve


#include "AbilitySystem/Abilities/HeroGameplayAbility_TargetLock.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Characters/WarriorHeroCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/WarriorWidgetBase.h"
#include "Controllers/WarriorHeroController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "WarriorFunctionLibrary.h"
#include "WarriorGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

#include "WarriorDebugHelper.h"

UHeroGameplayAbility_TargetLock::UHeroGameplayAbility_TargetLock()
{
	// Ŀ�������ᱣ�浱ǰĿ�ꡢ��ѡ�б���UI�;�ͷ����״̬������ӵ��ÿ����ɫ����������ʵ��
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// ��������
void UHeroGameplayAbility_TargetLock::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// ��������Ŀ��
	TryLockOnTarget();
	if (!AvailableActorsToLock.IsEmpty())
	{
		// ���ƶ��ٶ��л�Ϊ����Ŀ�����ƶ��ٶ�
		InitTargetLockMovement();
		// ��Ŀ����������ӳ��������
		InitTargetLockMappingContext();
	}
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

// ��������
void UHeroGameplayAbility_TargetLock::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// �����������
	ResetTargetLockMovement();
	// ���Ŀ����������ӳ��������
	ResetTargetLockMappingContext();
	// �������Ŀ����صĻ�������
	CleanUp();

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled
	);
}


// ��֡����Ŀ��
void UHeroGameplayAbility_TargetLock::OnTargetLockTick(float DeltaTime)
{
	AWarriorHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	AWarriorHeroController* HeroController = GetHeroControllerFromActorInfo();

	// ���������߻򱾵ؿ�����ʧЧʱȡ������
	if (!IsValid(HeroCharacter)
		|| !IsValid(HeroController)
		|| DoesActorHaveGameplayTag(HeroCharacter, WarriorGameplayTags::Shared_Status_Dead))
	{
		CancelTargetLockAbility();
		return;
	}

	// ���������١��뿪��Χ���뿪��Ұ��������ʧЧ��ֱ�ӳ����л����������ЧĿ��
	if (!IsValidTargetToLock(CurrentLockedActor, false))
	{
		AActor* PreviousLockedActor = CurrentLockedActor;
		if (!TrySelectNewTarget(PreviousLockedActor))
		{
			CancelTargetLockAbility();
			return;
		}
	}
	// ���������˻򳡾��ڵ�ʱ������ޣ�����ʱ��������л�������Ŀɼ�Ŀ��
	else if (!HasLineOfSightToTarget(CurrentLockedActor))
	{
		CurrentTargetOccludedTime += DeltaTime;
		if (CurrentTargetOccludedTime >= TargetOcclusionGraceTime)
		{
			AActor* OccludedActor = CurrentLockedActor;
			if (!TrySelectNewTarget(OccludedActor))
			{
				CancelTargetLockAbility();
				return;
			}
		}
	}
	else
	{
		CurrentTargetOccludedTime = 0.f;
	}

	// ��Tickˢ��Ŀ������ָʾ��
	SetTargetLockWidgetPosition();

	// ȷ��Ŀ���Ƿ��ڷ������
	const bool bShouldOverrideRotation =
		!DoesActorHaveGameplayTag(HeroCharacter, WarriorGameplayTags::Player_Status_Rolling)
		&&
		!DoesActorHaveGameplayTag(HeroCharacter, WarriorGameplayTags::Player_Status_Blocking);

	// ���û�У�����ɫ���߳���������Ŀ������
	if (bShouldOverrideRotation)
	{
		FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(
			HeroCharacter->GetActorLocation(),
			CurrentLockedActor->GetActorLocation()
		);

		LookAtRot -= FRotator(TargetLockCameraOffsetDistance, 0.f, 0.f);

		const FRotator CurrentControlRot = HeroController->GetControlRotation();

		// 死区：目标在当前镜头方向 TargetLockDeadzoneAngle 内时，yaw 保持不动（最小修正，防抖）
		const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentControlRot.Yaw, LookAtRot.Yaw);
		if (FMath::Abs(DeltaYaw) > TargetLockDeadzoneAngle)
		{
			const float MoveToward = FMath::Abs(DeltaYaw) - TargetLockDeadzoneAngle;
			LookAtRot.Yaw = CurrentControlRot.Yaw + (DeltaYaw > 0.f ? MoveToward : -MoveToward);
		}
		else
		{
			LookAtRot.Yaw = CurrentControlRot.Yaw;
		}

		const float RotationInterpSpeed = bIsSmoothingTargetSwitch
			? TargetSwitchRotationInterpSpeed
			: TargetLockRotationInterpSpeed;
		const FRotator TargetRot = FMath::RInterpTo(CurrentControlRot, LookAtRot, DeltaTime, RotationInterpSpeed);

		HeroController->SetControlRotation(FRotator(TargetRot.Pitch, TargetRot.Yaw, 0.f));
		HeroCharacter->SetActorRotation(FRotator(0.f, TargetRot.Yaw, 0.f));

		if (bIsSmoothingTargetSwitch
			&& FMath::Abs(FMath::FindDeltaAngleDegrees(TargetRot.Yaw, LookAtRot.Yaw)) <= TargetSwitchCompletionTolerance)
		{
			bIsSmoothingTargetSwitch = false;
		}
	}
}

void UHeroGameplayAbility_TargetLock::SwitchTarget(const FGameplayTag& InSwitchDirectionTag)
{
	// ��ȡ�ɱ�������Ŀ�꼯�ϣ���������Ϊ��ɫ�����Ҳ��Ŀ�꼯��
	GetAvailableActorsToLock();

	TArray<AActor*> ActorsOnLeft;
	TArray<AActor*> ActorsOnRight;
	AActor* NewTargetToLock = nullptr;

	GetAvailableActorsAroundTarget(ActorsOnLeft, ActorsOnRight);

	// ���������Χ�����л���ѡ����༯���о��������Ŀ������������ѡ���Ҳ༯���о������Ŀ������
	if (InSwitchDirectionTag == WarriorGameplayTags::Player_Event_SwtichTarget_Left)
	{
		NewTargetToLock = GetNearestTargetFromAvailableActors(ActorsOnLeft);
	}
	else
	{
		NewTargetToLock = GetNearestTargetFromAvailableActors(ActorsOnRight);
	}
	// ��Ԥ����Ŀ���л�Ϊ��ǰ����Ŀ��
	if (NewTargetToLock)
	{
		SetCurrentLockedActor(NewTargetToLock);
	}
	// ��ǰĿ���Ѿ����ڸ÷������Ұ��Եʱ�����κβ������������ֵ�ǰ����
}

void UHeroGameplayAbility_TargetLock::TryLockOnTarget()
{
	// ��ȡ������Ŀ�꼯��
	GetAvailableActorsToLock();

	// û��Ŀ���������ȡ����������
	if (AvailableActorsToLock.IsEmpty())
	{
		CancelTargetLockAbility();
		return;
	}

	// ��ȡ���������Ŀ��
	SetCurrentLockedActor(GetNearestTargetFromAvailableActors(AvailableActorsToLock));

	// �Ƿ�ɹ���ȡ
	if (CurrentLockedActor)
	{
		/*Debug::Print(CurrentLockedActor->GetActorNameOrLabel());*/
		// ����Ļ�ϻ���Ŀ��ָʾ��
		DrawTargetLockWidget();
		// ��ָʾ��λ���ƶ���Ŀ������
		SetTargetLockWidgetPosition();
	}
	else
	{
		// ȡ����������
		CancelTargetLockAbility();
	}
}

void UHeroGameplayAbility_TargetLock::GetAvailableActorsToLock()
{
	// ��տ�������Ŀ�꼯��
	AvailableActorsToLock.Empty();

	AWarriorHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	if (!IsValid(HeroCharacter))
	{
		return;
	}

	// ���ڽ�ɫ��Χ�����ξ����ɸ�������ɫ���������������һ��ʱ©��
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(HeroCharacter);

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::SphereOverlapActors(
		HeroCharacter,
		HeroCharacter->GetActorLocation(),
		BoxTraceDistance,
		BoxTraceChannel,
		APawn::StaticClass(),
		ActorsToIgnore,
		OverlappedActors
	);

	// ���μ��ֻ����ɸ�����պ�ѡ������������Ӫ������Ұ����Ļ���ڵ�����
	for (AActor* OverlappedActor : OverlappedActors)
	{
		if (IsValidTargetToLock(OverlappedActor))
		{
			AvailableActorsToLock.AddUnique(OverlappedActor);
		}
	}
}

bool UHeroGameplayAbility_TargetLock::IsValidTargetToLock(AActor* TargetActor, bool bRequireLineOfSight)
{
	AWarriorHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	AWarriorHeroController* HeroController = GetHeroControllerFromActorInfo();
	APawn* TargetPawn = Cast<APawn>(TargetActor);

	if (!IsValid(HeroCharacter)
		|| !IsValid(HeroController)
		|| !IsValid(TargetPawn)
		|| TargetPawn == HeroCharacter)
	{
		return false;
	}

	// ֻ��������ӵ��ASC������ҵжԵ�Pawn
	if (!UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetPawn)
		|| DoesActorHaveGameplayTag(TargetPawn, WarriorGameplayTags::Shared_Status_Dead)
		|| !UWarriorFunctionLibrary::IsTargetPawnHostile(HeroCharacter, TargetPawn))
	{
		return false;
	}

	const FVector HeroLocation = HeroCharacter->GetActorLocation();
	const FVector TargetLocation = TargetPawn->GetActorLocation();
	if (FVector::DistSquared(HeroLocation, TargetLocation) > FMath::Square(BoxTraceDistance))
	{
		return false;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	HeroController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	// Ŀ����������������Ŀ�����������
	const FVector CameraToTargetDirection = (TargetLocation - CameraLocation).GetSafeNormal();
	const float ViewConeDotThreshold = FMath::Cos(FMath::DegreesToRadians(TargetLockViewConeHalfAngle));
	if (FVector::DotProduct(CameraRotation.Vector(), CameraToTargetDirection) < ViewConeDotThreshold)
	{
		return false;
	}

	// ���˴�������������ڣ�������ʵ��Ͷ���ڵ�ǰ��Ļ��Χ��
	FVector2D ScreenPosition;
	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	HeroController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0
		|| ViewportSizeY <= 0
		|| !HeroController->ProjectWorldLocationToScreen(TargetLocation, ScreenPosition, true)
		|| ScreenPosition.X < 0.f
		|| ScreenPosition.X > static_cast<float>(ViewportSizeX)
		|| ScreenPosition.Y < 0.f
		|| ScreenPosition.Y > static_cast<float>(ViewportSizeY))
	{
		return false;
	}

	return !bRequireLineOfSight || HasLineOfSightToTarget(TargetActor);
}

bool UHeroGameplayAbility_TargetLock::HasLineOfSightToTarget(AActor* TargetActor)
{
	AWarriorHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	AWarriorHeroController* HeroController = GetHeroControllerFromActorInfo();
	UWorld* World = GetWorld();
	if (!IsValid(HeroCharacter)
		|| !IsValid(HeroController)
		|| !IsValid(TargetActor)
		|| !World)
	{
		return false;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	HeroController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	// ���������Ŀ�����ɼ��Լ�⣬���ˡ�ǽ��������赲�ﶼ������ڵ�
	FHitResult VisibilityHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TargetLockVisibility), true, HeroCharacter);
	QueryParams.AddIgnoredActor(HeroCharacter);
	const bool bHitBlockingObject = World->LineTraceSingleByChannel(
		VisibilityHit,
		CameraLocation,
		TargetActor->GetActorLocation(),
		ECC_Visibility,
		QueryParams
	);

	AActor* HitActor = VisibilityHit.GetActor();
	return !bHitBlockingObject
		|| HitActor == TargetActor
		|| (IsValid(HitActor) && HitActor->IsOwnedBy(TargetActor));
}

bool UHeroGameplayAbility_TargetLock::DoesActorHaveGameplayTag(AActor* TargetActor, const FGameplayTag& TagToCheck) const
{
	if (!IsValid(TargetActor) || !TagToCheck.IsValid())
	{
		return false;
	}

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		return TargetASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool UHeroGameplayAbility_TargetLock::TrySelectNewTarget(AActor* TargetToExclude)
{
	GetAvailableActorsToLock();

	if (TargetToExclude)
	{
		AvailableActorsToLock.Remove(TargetToExclude);
	}

	SetCurrentLockedActor(GetNearestTargetFromAvailableActors(AvailableActorsToLock));
	return IsValid(CurrentLockedActor);
}

void UHeroGameplayAbility_TargetLock::SetCurrentLockedActor(AActor* NewLockedActor)
{
	bIsSmoothingTargetSwitch = false;
	CurrentTargetOccludedTime = 0.f;

	AWarriorHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo();
	if (IsValid(HeroCharacter) && IsValid(CurrentLockedActor) && IsValid(NewLockedActor))
	{
		const FVector HeroLocation = HeroCharacter->GetActorLocation();
		const FVector CurrentTargetDirection = (CurrentLockedActor->GetActorLocation() - HeroLocation).GetSafeNormal();
		const FVector NewTargetDirection = (NewLockedActor->GetActorLocation() - HeroLocation).GetSafeNormal();
		const float DirectionDot = FMath::Clamp(
			FVector::DotProduct(CurrentTargetDirection, NewTargetDirection),
			-1.f,
			1.f
		);
		const float TargetSwitchAngle = FMath::RadiansToDegrees(FMath::Acos(DirectionDot));
		bIsSmoothingTargetSwitch = TargetSwitchAngle >= TargetSwitchSmoothAngleThreshold;
	}

	CurrentLockedActor = NewLockedActor;
}

// ��ȡ���������Ŀ��
AActor* UHeroGameplayAbility_TargetLock::GetNearestTargetFromAvailableActors(const TArray<AActor*>& InAvailableActors)
{
	float ClosestDistance = 0.f;
	return UGameplayStatics::FindNearestActor(GetHeroCharacterFromActorInfo()->GetActorLocation(), InAvailableActors, ClosestDistance);
}

void UHeroGameplayAbility_TargetLock::GetAvailableActorsAroundTarget(TArray<AActor*>& OutActorsOnLeft, TArray<AActor*>& OutActorsOnRight)
{
	// ��ȫ�Լ�飬û�������κ�Ŀ�꣬��û�п�����Ŀ��ʱ��Ӧ�����ú���
	if (!CurrentLockedActor || AvailableActorsToLock.IsEmpty())
	{
		CancelTargetLockAbility();
		return;
	}

	// ��ȡ��ҽ�ɫ��λ������
	const FVector PlayerLocation = GetHeroCharacterFromActorInfo()->GetActorLocation();
	// ���㵱ǰ����Ŀ���λ�õ����λ�õķ�������������һ��
	const FVector PlayerToCurrentNormalized = (CurrentLockedActor->GetActorLocation() - PlayerLocation).GetSafeNormal();

	// ����������Ŀ�꼯��
	for (AActor* AvailableActor : AvailableActorsToLock)
	{
		// �����ѡĿ�겻���ڣ���Ϊ��ǰ������Ŀ�꣬��������
		if (!AvailableActor || AvailableActor == CurrentLockedActor) continue;
		
		// �����ѡĿ���λ�õ����λ�õķ�������������һ��
		const FVector PlayerToAvailableNormalized = (AvailableActor->GetActorLocation() - PlayerLocation).GetSafeNormal();

		// �������������Ĳ�ˣ����ݲ���������򣨼�Z���������жϺ�ѡĿ��λ�ڵ�ǰ����Ŀ����໹���Ҳ�
		const FVector CrossResult = FVector::CrossProduct(PlayerToCurrentNormalized, PlayerToAvailableNormalized);

		// ����ڵ�ǰ����Ŀ����࣬������������Ҳ�ĺ�ѡĿ�꼯
		if (CrossResult.Z > 0.f)
		{
			OutActorsOnRight.AddUnique(AvailableActor);
		}
		// ��֮����������������ĺ�ѡĿ�꼯
		else
		{
			OutActorsOnLeft.AddUnique(AvailableActor);
		}
	}
}

// ����Ŀ��ָʾ��
void UHeroGameplayAbility_TargetLock::DrawTargetLockWidget()
{
	if (!DrawnTargetLockWidget)
	{
		// ����Ƿ�ָ��ָʾ��UI
		checkf(TargetLockWidgetClass, TEXT("Forgot to assign a valid widget class in Blueprint"));

		// ����ָ�����ʹ���UI�ؼ�
		DrawnTargetLockWidget = CreateWidget<UWarriorWidgetBase>(GetHeroControllerFromActorInfo(), TargetLockWidgetClass);

		// ����Ƿ�ɹ�����UI�ؼ�
		check(DrawnTargetLockWidget);

		// ��UI�ؼ����ӵ���Ļ��
		DrawnTargetLockWidget->AddToViewport();
	}
	
}

// ����ָʾ��λ��
void UHeroGameplayAbility_TargetLock::SetTargetLockWidgetPosition()
{
	// û�п�����Ŀ���û�гɹ�����ָʾ��ʱȡ������
	if (!DrawnTargetLockWidget || !CurrentLockedActor)
	{
		CancelTargetLockAbility();
		return;
	}

	// ��Ŀ���ɫ���ڵ�������άλ��Ͷ�䵽��Ļ���γɶ�ά��λ
	FVector2D ScreenPosition;
	UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		GetHeroControllerFromActorInfo(),
		CurrentLockedActor->GetActorLocation(),
		ScreenPosition,
		true
	);

	// ���ָʾ����ʼ�ߴ�Ϊ0�����ɼ�����������ߴ�Ϊ�ɼ��ߴ�
	if (TargetLockWidgetSize == FVector2D::ZeroVector)
	{
		DrawnTargetLockWidget->WidgetTree->ForEachWidget(
			[this](UWidget* FoundWidget)
			{
				if (USizeBox* FoundSizeBox = Cast<USizeBox>(FoundWidget))
				{
					TargetLockWidgetSize.X = FoundSizeBox->GetWidthOverride();
					TargetLockWidgetSize.Y = FoundSizeBox->GetHeightOverride();
				}
			}
		);
	}

	// ��ָʾ������Ļ�л��Ƶ�λ���������õ�UI�ؼ��������
	ScreenPosition -= (TargetLockWidgetSize / 2.f);

	DrawnTargetLockWidget->SetPositionInViewport(ScreenPosition, false);
}

// ����������ΪĿ������ʱ����
void UHeroGameplayAbility_TargetLock::InitTargetLockMovement()
{
	UCharacterMovementComponent* MovementComponent = GetHeroCharacterFromActorInfo()->GetCharacterMovement();

	// �����������٣����ڽ���ʱ�ָ�
	CachedDefaultMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
	bCachedOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
	bCachedUseControllerDesiredRotation = MovementComponent->bUseControllerDesiredRotation;
	bHasCachedTargetLockMovementSettings = true;

	// ����״̬��GAͳһ���ƽ�ɫ���򣬱���CharacterMovementͬʱ���Գ��ƶ�������ת
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = false;
	MovementComponent->MaxWalkSpeed = TargetLockMaxWalkSpeed;
}

// ������ӳ��������
void UHeroGameplayAbility_TargetLock::InitTargetLockMappingContext()
{
	// ��ȡ��ɫ�󶨵���ǿ������ϵͳ
	const ULocalPlayer* LocalPlayer = GetHeroControllerFromActorInfo()->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem);

	// ��������ӳ��������
	Subsystem->AddMappingContext(TargetLockMappingContext, 3);
}

// ȡ����������
void UHeroGameplayAbility_TargetLock::CancelTargetLockAbility()
{
	CancelAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true);
}

// �����������
void UHeroGameplayAbility_TargetLock::CleanUp()
{
	// ��տ�����Ŀ�꼯��
	AvailableActorsToLock.Empty();

	// ��ǰ����Ŀ���ÿ�
	CurrentLockedActor = nullptr;
	bIsSmoothingTargetSwitch = false;
	CurrentTargetOccludedTime = 0.f;

	// �Ƴ�ָʾ��UI���
	if (DrawnTargetLockWidget)
	{
		DrawnTargetLockWidget->RemoveFromParent();
	}

	// ָʾ��ָ���ÿ�
	DrawnTargetLockWidget = nullptr;

	// ָʾ���ߴ�ָ�Ϊ0
	TargetLockWidgetSize = FVector2D::ZeroVector;

	// �������ٻָ�Ϊ0
	CachedDefaultMaxWalkSpeed = 0.f;
}

// ������ָ���������
void UHeroGameplayAbility_TargetLock::ResetTargetLockMovement()
{
	if (!bHasCachedTargetLockMovementSettings)
	{
		return;
	}

	if (AWarriorHeroCharacter* HeroCharacter = GetHeroCharacterFromActorInfo())
	{
		UCharacterMovementComponent* MovementComponent = HeroCharacter->GetCharacterMovement();
		MovementComponent->MaxWalkSpeed = CachedDefaultMaxWalkSpeed;
		MovementComponent->bOrientRotationToMovement = bCachedOrientRotationToMovement;
		MovementComponent->bUseControllerDesiredRotation = bCachedUseControllerDesiredRotation;
	}

	bHasCachedTargetLockMovementSettings = false;
}

// �Ƴ�����Ŀ����ص�����ӳ��������
void UHeroGameplayAbility_TargetLock::ResetTargetLockMappingContext()
{
	if (!GetHeroControllerFromActorInfo())
	{
		return;
	}
	const ULocalPlayer* LocalPlayer = GetHeroControllerFromActorInfo()->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem);

	Subsystem->RemoveMappingContext(TargetLockMappingContext);
}
