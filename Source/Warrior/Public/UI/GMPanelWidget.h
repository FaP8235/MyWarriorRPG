// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GMPanelWidget.generated.h"

class UUniformGridPanel;
class AWarriorEnemyCharacter;
class UGMCommandButton;

/**
 * GM 指令面板（C++ 驱动）：定义指令列表、动态创建按钮、路由点击。
 * BP 只管视觉（Border 背景 + Grid 容器 + asset 引用）。
 */
UCLASS(Abstract)
class WARRIOR_API UGMPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	/** 由 UGMCommandButton 点击时调用。 */
	void ExecuteCommand(int32 CommandIndex);

protected:
	/** 指令按钮的 Widget 模板（BP 设为 WBP_GMCommandButton）。 */
	UPROPERTY(EditDefaultsOnly, Category = "GM Panel")
	TSubclassOf<UGMCommandButton> CommandButtonClass;

	// 刷怪/刷物品的 class（BP 设）
	UPROPERTY(EditDefaultsOnly, Category = "GM Panel|Assets") TSubclassOf<AWarriorEnemyCharacter> GuardianClass;
	UPROPERTY(EditDefaultsOnly, Category = "GM Panel|Assets") TSubclassOf<AWarriorEnemyCharacter> GlacerClass;
	UPROPERTY(EditDefaultsOnly, Category = "GM Panel|Assets") TSubclassOf<AWarriorEnemyCharacter> FrostGiantClass;
	UPROPERTY(EditDefaultsOnly, Category = "GM Panel|Assets") TSubclassOf<AActor> HealStoneClass;
	UPROPERTY(EditDefaultsOnly, Category = "GM Panel|Assets") TSubclassOf<AActor> RageStoneClass;

	/** Grid 容器（BP 里 BindWidget 绑名为 CommandGrid 的 UniformGridPanel）。 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> CommandGrid;

private:
	struct FGMCommand
	{
		FString Name;
		bool bToggle;
		FString CVarName;
	};

	static const TArray<FGMCommand>& GetCommands();
	void PopulateButtons();
	void RefreshToggleStates();
};
