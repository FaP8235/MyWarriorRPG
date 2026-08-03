// FaP All Rights Reserve


#include "WarriorFunctionLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "Interfaces/PawnCombatInterface.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "WarriorGameplayTags.h"
#include "WarriorTypes/WarriorCountDownAction.h"
#include "WarriorGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGame/WarriorSaveGame.h"
#include "DataAssets/Combat/DataAsset_CombatAttackProfile.h"
#include "MotionWarpingComponent.h"
#include "GameFramework/Controller.h"
#include "Combat/WarriorCombatDebug.h"
#include "DrawDebugHelpers.h"

#include "WarriorDebugHelper.h"

UWarriorAbilitySystemComponent* UWarriorFunctionLibrary::NativeGetWarriorASCFromActor(AActor* InActor)
{
    check(InActor);

    return CastChecked<UWarriorAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor));
}

void UWarriorFunctionLibrary::ApplyStrikeAssist(AActor* InVictim, AActor* InAttacker, const UDataAsset_CombatAttackProfile* InAttackProfile)
{
    if (!InVictim || !InAttacker)
    {
        return;
    }

    UMotionWarpingComponent* MotionWarp = InVictim->FindComponentByClass<UMotionWarpingComponent>();
    if (!MotionWarp)
    {
        return;
    }

    const float Strength = InAttackProfile ? FMath::Clamp(InAttackProfile->StrikeAssistStrength, 0.f, 1.f) : 0.6f;
    const float WarpDistance = InAttackProfile ? FMath::Max(0.f, InAttackProfile->StrikeAssistWarpDistance) : 50.f;

    const FVector VictimLoc = InVictim->GetActorLocation();
    const FVector AttackerLoc = InAttacker->GetActorLocation();

    const FVector2D HitDir = FVector2D(VictimLoc.X - AttackerLoc.X, VictimLoc.Y - AttackerLoc.Y).GetSafeNormal();

    FVector2D CamDir = FVector2D::ZeroVector;
    if (const APawn* AttackerPawn = Cast<APawn>(InAttacker))
    {
        if (const AController* Controller = AttackerPawn->GetController())
        {
            const FVector Forward = Controller->GetControlRotation().Vector();
            CamDir = FVector2D(Forward.X, Forward.Y).GetSafeNormal();
        }
    }
    if (CamDir.IsNearlyZero())
    {
        CamDir = HitDir;
    }

    // 拉扯目标 = 攻击者前方 PullDistance 处（近战甜蜜点：可砍到 + 在屏内）。把敌人往这里拽，保持可砍。
    const float PullDistance = InAttackProfile ? FMath::Max(0.f, InAttackProfile->IdealAttackDistance) : 125.f;
    const FVector PullPoint = AttackerLoc + FVector(CamDir.X, CamDir.Y, 0.f) * PullDistance;
    const FVector2D TowardPull = FVector2D(PullPoint.X - VictimLoc.X, PullPoint.Y - VictimLoc.Y).GetSafeNormal();
    if (TowardPull.IsNearlyZero())
    {
        return;
    }

    // 击退方向（远离英雄）与 指向甜蜜点方向（贴近英雄）的混合：Strength 越大越往甜蜜点拉。
    const FVector2D TargetDir = FMath::Lerp(HitDir, TowardPull, Strength).GetSafeNormal();
    if (TargetDir.IsNearlyZero())
    {
        return;
    }

    const FVector WarpPoint = VictimLoc + FVector(TargetDir.X, TargetDir.Y, 0.f) * WarpDistance;

#if !UE_BUILD_SHIPPING
    if (WarriorCombatDebug::IsStrikeAssistEnabled() && InVictim->GetWorld())
    {
        UWorld* W = InVictim->GetWorld();
        const FVector Base = VictimLoc + FVector(0.f, 0.f, 40.f);
        DrawDebugDirectionalArrow(W, Base, Base + FVector(HitDir.X, HitDir.Y, 0.f) * 100.f, 60.f, FColor::Red, false, 0.3f, 0, 2.f);
        DrawDebugDirectionalArrow(W, Base, Base + FVector(CamDir.X, CamDir.Y, 0.f) * 100.f, 60.f, FColor::Cyan, false, 0.3f, 0, 2.f);
        DrawDebugDirectionalArrow(W, Base, WarpPoint + FVector(0.f, 0.f, 40.f), 80.f, FColor::Green, false, 0.3f, 0, 4.f);
        DrawDebugSphere(W, PullPoint + FVector(0.f, 0.f, 40.f), 12.f, 8, FColor::Yellow, false, 0.3f, 0, 2.f);
        DrawDebugString(W, Base + FVector(0.f, 0.f, 30.f), FString::Printf(TEXT("SA s%.1f d%.0f"), Strength, WarpDistance), nullptr, FColor::Green, 0.3f, true, 1.f);
    }
#endif

    MotionWarp->AddOrUpdateWarpTargetFromTransform(FName(TEXT("StrikeAssistTarget")), FTransform(InVictim->GetActorQuat(), WarpPoint));
}

void UWarriorFunctionLibrary::AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd)
{
    UWarriorAbilitySystemComponent* ASC = NativeGetWarriorASCFromActor(InActor);

    if (!ASC->HasMatchingGameplayTag(TagToAdd))
    {
        ASC->AddLooseGameplayTag(TagToAdd);
    }
}

void UWarriorFunctionLibrary::RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove)
{
    UWarriorAbilitySystemComponent* ASC = NativeGetWarriorASCFromActor(InActor);


    if (ASC->HasMatchingGameplayTag(TagToRemove))
    {
        ASC->RemoveLooseGameplayTag(TagToRemove);
    }
}

bool UWarriorFunctionLibrary::NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck)
{
    UWarriorAbilitySystemComponent* ASC = NativeGetWarriorASCFromActor(InActor);

    return ASC->HasMatchingGameplayTag(TagToCheck);
}

void UWarriorFunctionLibrary::BP_DosActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EWarriorConfirmType& OutConfirmType)
{
    OutConfirmType = NativeDoesActorHaveTag(InActor, TagToCheck) ? EWarriorConfirmType::Yes : EWarriorConfirmType::No;
}

UPawnCombatComponent* UWarriorFunctionLibrary::NativeGetPawnCombatComponentFromActor(AActor* InActor)
{
    check(InActor);

    if (IPawnCombatInterface* PawnCombatInterface = Cast<IPawnCombatInterface>(InActor))
    {
        return PawnCombatInterface->GetPawnCombatComponent();
    }

    return nullptr;
}

UPawnCombatComponent* UWarriorFunctionLibrary::BP_GetPawnCombatComponentFromActor(AActor* InActor, EWarriorValidType& OutValidType)
{
    UPawnCombatComponent* CombatComponent = NativeGetPawnCombatComponentFromActor(InActor);

    OutValidType = CombatComponent ? EWarriorValidType::Valid : EWarriorValidType::InValid;

    return CombatComponent;
}

bool UWarriorFunctionLibrary::IsTargetPawnHostile(APawn* QueryPawn, APawn* TargetPawn)
{
    check(QueryPawn && TargetPawn);

    IGenericTeamAgentInterface* QueryTeamAgent = Cast<IGenericTeamAgentInterface>(QueryPawn->GetController());
    IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(TargetPawn->GetController());
    
    if (QueryTeamAgent && TargetTeamAgent)
    {
        return QueryTeamAgent->GetGenericTeamId() != TargetTeamAgent->GetGenericTeamId();
    }
    
    return false;
}

float UWarriorFunctionLibrary::GetScalableFloatValueAtLevel(const FScalableFloat& InScalableFloat, float InLevel)
{
    return InScalableFloat.GetValueAtLevel(InLevel);
}

FGameplayTag UWarriorFunctionLibrary::ComputeHitReactDirectionTag(AActor* InAttacker, AActor* InVictim, float& OutAngleDifference)
{
    check(InAttacker && InVictim);

    const FVector VictimForward = InVictim->GetActorForwardVector();
    const FVector VictimAttackerNormalized = (InAttacker->GetActorLocation() - InVictim->GetActorLocation()).GetSafeNormal();

    const float DotResult = FVector::DotProduct(VictimForward, VictimAttackerNormalized);
    OutAngleDifference = UKismetMathLibrary::DegAcos(DotResult);

    const FVector CrossResult = FVector::CrossProduct(VictimForward, VictimAttackerNormalized);

    if (CrossResult.Z < 0.f)
    {
        OutAngleDifference *= -1.f;
    }

    if (OutAngleDifference >= -45.f && OutAngleDifference <= 45.f)
    {
        return WarriorGameplayTags::Shared_Status_HitReact_Front;
    }
    else if (OutAngleDifference < -45.f && OutAngleDifference >= -135.f)
    {
        return WarriorGameplayTags::Shared_Status_HitReact_Left;
    }
    else if (OutAngleDifference < -135.f || OutAngleDifference > 135.f)
    {
        return WarriorGameplayTags::Shared_Status_HitReact_Back;
    }
    else if (OutAngleDifference > 45.f && OutAngleDifference <= 135.f)
    {
        return WarriorGameplayTags::Shared_Status_HitReact_Right;
    }

    return WarriorGameplayTags::Shared_Status_HitReact_Front;
}

bool UWarriorFunctionLibrary::IsValidBlock(AActor* InAttacker, AActor* InDefender)
{
    check(InAttacker && InDefender);

    const float DotResult = FVector::DotProduct(InAttacker->GetActorForwardVector(), InDefender->GetActorForwardVector());
    
    /*const FString DebugString = FString::Printf(TEXT("Dot Result: %f %s"), DotResult, DotResult < 0.f ? TEXT("Valid Block") : TEXT("Invalid Block"));
    
    Debug::Print(DebugString, DotResult < -0.1f ? FColor::Green : FColor::Red);*/
    return DotResult < -0.1f;
}

bool UWarriorFunctionLibrary::ApplyGameplayEffectSpecHandleToTargetActor(AActor* InInstigator, AActor* InTargetActor, const FGameplayEffectSpecHandle& InSpecHandle)
{
    UWarriorAbilitySystemComponent* SourceASC = NativeGetWarriorASCFromActor(InInstigator);
    UWarriorAbilitySystemComponent* TargetASC = NativeGetWarriorASCFromActor(InTargetActor);

    FActiveGameplayEffectHandle ActiveGameplayEffectHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*InSpecHandle.Data, TargetASC);

    return ActiveGameplayEffectHandle.WasSuccessfullyApplied();
}

void UWarriorFunctionLibrary::CountDown(const UObject* WorldContextObject, float TotalTime, float UpdateInterval, float& OutRemainingTime, EWarriorCountDownActionInput CountDownInput, UPARAM(DisplayName = "Output") EWarriorCountDownActionOutput& CountDownOutput, FLatentActionInfo LatentInfo)
{
    UWorld* World = nullptr;

    if (GEngine)
    {
        World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    }

    if (!World)
    {
        return;
    }

    FLatentActionManager& LatentActionManager = World->GetLatentActionManager();

    FWarriorCountDownAction* FoundAction = LatentActionManager.FindExistingAction<FWarriorCountDownAction>(LatentInfo.CallbackTarget, LatentInfo.UUID);

    if (CountDownInput == EWarriorCountDownActionInput::Start)
    {
        if (!FoundAction)
        {
            LatentActionManager.AddNewAction(
                LatentInfo.CallbackTarget,
                LatentInfo.UUID,
                new FWarriorCountDownAction(TotalTime, UpdateInterval, OutRemainingTime, CountDownOutput, LatentInfo)
            );
        }
    }

    if (CountDownInput == EWarriorCountDownActionInput::Cancel)
    {
        if (FoundAction)
        {
            FoundAction->CancelAction();
        }
    }
}

UWarriorGameInstance* UWarriorFunctionLibrary::GetWarriorGameInstance(const UObject* WorldContextObject)
{
    if (GEngine)
    {
        if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
        {
            return World->GetGameInstance<UWarriorGameInstance>();
        }
    }
    
    return nullptr;
}

void UWarriorFunctionLibrary::ToggleInputMode(const UObject* WorldContextObject, EWarriorInputMode InInputMode)
{
    APlayerController* PlayerController = nullptr;

    if (GEngine)
    {
        if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
        {
            PlayerController = World->GetFirstPlayerController();
        }
    }

    if (!PlayerController)
    {
        return;
    }

    FInputModeGameOnly GameOnlyMode;
    FInputModeUIOnly UIOnlyMode;

    switch (InInputMode)
    {
    case EWarriorInputMode::GameOnly:
        PlayerController->SetInputMode(GameOnlyMode);
        PlayerController->bShowMouseCursor = false;
        break;

    case EWarriorInputMode::UIOnly:

        PlayerController->SetInputMode(UIOnlyMode);
        PlayerController->bShowMouseCursor = true;

        break;

    default:
        break;
    }
}

void UWarriorFunctionLibrary::SaveCurrentGameDifficulty(EWarriorGameDifficulty InDifficultyToSave)
{
    USaveGame* SaveGameObject = UGameplayStatics::CreateSaveGameObject(UWarriorSaveGame::StaticClass());

    if (UWarriorSaveGame* WarriorSaveGameObject = Cast<UWarriorSaveGame>(SaveGameObject))
    {
        WarriorSaveGameObject->SavedCurrentGameDifficulty = InDifficultyToSave;

        const bool bWasSaved = UGameplayStatics::SaveGameToSlot(WarriorSaveGameObject, WarriorGameplayTags::GameData_SaveGame_Slot_1.GetTag().ToString(), 0);

        Debug::Print(bWasSaved ? TEXT("Difficulty Saved") : TEXT("Difficulty Not Saved"));
    }
}

bool UWarriorFunctionLibrary::TryLoadSavedGameDifficulty(EWarriorGameDifficulty& OutSavedDifficulty)
{
    if (UGameplayStatics::DoesSaveGameExist(WarriorGameplayTags::GameData_SaveGame_Slot_1.GetTag().ToString(), 0))
    {
        USaveGame* SaveGameObject = UGameplayStatics::LoadGameFromSlot(WarriorGameplayTags::GameData_SaveGame_Slot_1.GetTag().ToString(), 0);

        if (UWarriorSaveGame* WarriorSaveGameObject = Cast<UWarriorSaveGame>(SaveGameObject))
        {
            OutSavedDifficulty = WarriorSaveGameObject->SavedCurrentGameDifficulty;

            Debug::Print(TEXT("Loading Successful"), FColor::Green);

            return true;
        }
    }

    return false;
}
