// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WarriorTypes/WarriorEnumTypes.h"
#include "WarriorFunctionLibrary.generated.h"

class UWarriorAbilitySystemComponent;
class UPawnCombatComponent;
class UDataAsset_CombatAttackProfile;
class AWarriorEnemyCharacter;
struct FScalableFloat;
class UWarriorGameInstance;
/**
 * 
 */
UCLASS()
class WARRIOR_API UWarriorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	static UWarriorAbilitySystemComponent* NativeGetWarriorASCFromActor(AActor* InActor);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary")
	static void AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary")
	static void RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove);

	static bool NativeDoesActorHaveTag(AActor* InActor, FGameplayTag TagToCheck);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary", meta = (DisplayName = "Does Actor Have Tag", ExpandEnumAsExecs = "OutConfirmType"))
	static void BP_DosActorHaveTag(AActor* InActor, FGameplayTag TagToCheck, EWarriorConfirmType& OutConfirmType);

	static UPawnCombatComponent* NativeGetPawnCombatComponentFromActor(AActor* InActor);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary", meta = (DisplayName = "Get Pawn Combat Component From Actor", ExpandEnumAsExecs = "OutValidType"))
	static UPawnCombatComponent * BP_GetPawnCombatComponentFromActor(AActor * InActor, EWarriorValidType& OutValidType);

	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary")
	static bool IsTargetPawnHostile(APawn* QueryPawn, APawn* TargetPawn);

	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary", meta = (CompactNodeTitle = "Get Value At Level"))
	static float GetScalableFloatValueAtLevel(const FScalableFloat& InScalableFloat, float InLevel = 1.f);

	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary")
	static FGameplayTag ComputeHitReactDirectionTag(AActor* InAttacker, AActor* InVictim, float& OutAngleDifference);

	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary")
	static bool IsValidBlock(AActor* InAttacker, AActor* InDefender);

	/** Strike Assist：命中瞬间把受害者被击退轨迹偏向「镜头朝向 × 击中方向」，拉回屏内。warp 名 StrikeAssistTarget。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary")
	static void ApplyStrikeAssist(AActor* InVictim, AActor* InAttacker, const UDataAsset_CombatAttackProfile* InAttackProfile);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary")
	static bool ApplyGameplayEffectSpecHandleToTargetActor(AActor* InInstigator, AActor* InTargetActor, const FGameplayEffectSpecHandle& InSpecHandle);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary", meta = (Latent, WorldContext = "WorldContextObject", LatentInfo = "LatentInfo", ExpandEnumAsExecs = "CountDownInput|CountDownOutput", Totaltime = "1.0", UpdateInterval = "0.1"))
	static void CountDown(const UObject* WorldContextObject, float TotalTime, float UpdateInterval,
		float& OutRemainingTime, EWarriorCountDownActionInput CountDownInput,
		UPARAM(DisplayName = "Output") EWarriorCountDownActionOutput& CountDownOutput, FLatentActionInfo LatentInfo);

	UFUNCTION(BlueprintPure, Category = "Warrior|FunctionLibrary", meta = (WorldContext = "WorldContextObject"))
	static UWarriorGameInstance* GetWarriorGameInstance(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary", meta = (WorldContext = "WorldContextObject"))
	static void ToggleInputMode(const UObject* WorldContextObject, EWarriorInputMode InInputMode);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary")
	static void SaveCurrentGameDifficulty(EWarriorGameDifficulty InDifficultyToSave);

	UFUNCTION(BlueprintCallable, Category = "Warrior|FunctionLibrary")
	static bool TryLoadSavedGameDifficulty(EWarriorGameDifficulty& OutSavedDifficulty);

	// ── GM 指令 ──

	/** 视线前方刷一个敌人。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM", meta = (WorldContext = "WorldContextObject"))
	static void GM_SpawnEnemy(const UObject* WorldContextObject, TSubclassOf<AWarriorEnemyCharacter> EnemyClass);

	/** 脚下刷一个拾取物。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM", meta = (WorldContext = "WorldContextObject"))
	static void GM_SpawnPickup(const UObject* WorldContextObject, TSubclassOf<AActor> PickupClass);

	/** 满血。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM", meta = (WorldContext = "WorldContextObject"))
	static void GM_FullHealth(const UObject* WorldContextObject);

	/** 满怒气。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM", meta = (WorldContext = "WorldContextObject"))
	static void GM_FullRage(const UObject* WorldContextObject);

	/** 切换锁血。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM")
	static void GM_ToggleGodMode();

	/** 切换持续满怒气。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM")
	static void GM_ToggleMaxRage();

	/** 切换攻击力 9999。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM", meta = (WorldContext = "WorldContextObject"))
	static void GM_ToggleMaxAttack(const UObject* WorldContextObject);

	/** 切换 Debug 全开。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM")
	static void GM_ToggleDebugAll();

	/** 清空所有敌人。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|GM", meta = (WorldContext = "WorldContextObject"))
	static void GM_ClearAllEnemies(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Warrior|GM") static bool GM_IsGodModeOn();
	UFUNCTION(BlueprintPure, Category = "Warrior|GM") static bool GM_IsMaxRageOn();
	UFUNCTION(BlueprintPure, Category = "Warrior|GM") static bool GM_IsMaxAttackOn();
	UFUNCTION(BlueprintPure, Category = "Warrior|GM") static bool GM_IsDebugAllOn();
};
