// FaP All Rights Reserve


#include "Items/Weapons/WarriorWeaponBase.h"
#include "Components/BoxComponent.h"
#include "WarriorFunctionLibrary.h"

#include "WarriorDebugHelper.h"

// Sets default values
AWarriorWeaponBase::AWarriorWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollisionBox"));
	WeaponCollisionBox->SetupAttachment(GetRootComponent());
	WeaponCollisionBox->SetBoxExtent(FVector(20.f));
	WeaponCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponCollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxBeginOverlap);
	WeaponCollisionBox->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnCollisionBoxEndOverlap);
}

void AWarriorWeaponBase::OnCollisionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* WeaponOwingPawn = GetInstigator<APawn>();

	checkf(WeaponOwingPawn, TEXT("Forgot to assign an instigator as the owing pawn for the weapon: %s"), *GetName());

	if (APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		if (UWarriorFunctionLibrary::IsTargetPawnHostile(WeaponOwingPawn, HitPawn))
		{
			OnWeaponHitTarget.ExecuteIfBound(OtherActor);
		}
	}

	//if (APawn* HitPawn = Cast<APawn>(OtherActor))
	//{
	//	if (WeaponOwingPawn != HitPawn)
	//	{
	//		/*Debug::Print(GetName() + TEXT(" begin overlap with") + HitPawn->GetName(), FColor::Green);*/
	//		OnWeaponHitTarget.ExecuteIfBound(OtherActor);
	//	}

	//	// TODO: Implement hit check for enemy characters
	//}
}

void AWarriorWeaponBase::OnCollisionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* WeaponOwingPawn = GetInstigator<APawn>();

	checkf(WeaponOwingPawn, TEXT("Forgot to assign an instigator as the owing pawn for the weapon: %s"), *GetName());

	if (APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		if (UWarriorFunctionLibrary::IsTargetPawnHostile(WeaponOwingPawn, HitPawn))
		{
			OnWeaponPulledFromTarget.ExecuteIfBound(OtherActor);
		}
	}
	

	//if (APawn* HitPawn = Cast<APawn>(OtherActor))
	//{
	//	if (WeaponOwingPawn != HitPawn)
	//	{
	//		/*Debug::Print(GetName() + TEXT(" end overlap with") + HitPawn->GetName(), FColor::Red);*/
	//		OnWeaponPulledFromTarget.ExecuteIfBound(OtherActor);
	//	}

	//	// TODO: Implement hit check for enemy characters
	//}
}

