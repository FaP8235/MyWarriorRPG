// FaP All Rights Reserve


#include "AnimInstances/WarriorCharacterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Characters/WarriorBaseCharacter.h"
#include "KismetAnimationLibrary.h"

void UWarriorCharacterAnimInstance::NativeInitializeAnimation()
{
	OwningCharacter = Cast<AWarriorBaseCharacter>(TryGetPawnOwner());

	if (OwningCharacter)
	{
		OwningMovementComponent = OwningCharacter->GetCharacterMovement();
	}
}

void UWarriorCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (!OwningCharacter || !OwningMovementComponent)
	{
		return;
	}

	const FVector CharacterVelocity = OwningCharacter->GetVelocity();
	GroundSpeed = CharacterVelocity.Size2D();

	bHasAcceleration = OwningMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;

	const bool bIsMoving = GroundSpeed > LocomotionDirectionMinSpeed;
	if (bIsMoving)
	{
		TargetLocomotionDirection = UKismetAnimationLibrary::CalculateDirection(
			CharacterVelocity,
			OwningCharacter->GetActorRotation()
		);

		if (!bWasMovingLastFrame)
		{
			// 起步或反向经过零速后直接采用正确方向，避免从错误方向绕行
			LocomotionDirection = TargetLocomotionDirection;
		}
		else
		{
			// 使用最短角度差做指数平滑，正确跨越-180/180边界
			const float DirectionDelta = FMath::FindDeltaAngleDegrees(
				LocomotionDirection,
				TargetLocomotionDirection
			);
			const float InterpAlpha = 1.f - FMath::Exp(-LocomotionDirectionInterpSpeed * DeltaSeconds);
			LocomotionDirection = FMath::UnwindDegrees(
				LocomotionDirection + DirectionDelta * InterpAlpha
			);
		}
	}

	bWasMovingLastFrame = bIsMoving;
}
