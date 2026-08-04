// FaP All Rights Reserve

#include "UI/GMPanelWidget.h"
#include "UI/GMCommandButton.h"
#include "WarriorFunctionLibrary.h"
#include "Components/UniformGridPanel.h"
#include "Blueprint/UserWidget.h"
#include "HAL/IConsoleManager.h"

const TArray<UGMPanelWidget::FGMCommand>& UGMPanelWidget::GetCommands()
{
	static const TArray<FGMCommand> Commands =
	{
		{TEXT("刷 Guardian"),       false, TEXT("")},
		{TEXT("刷 Glacer"),         false, TEXT("")},
		{TEXT("刷 FrostGiant"),     false, TEXT("")},
		{TEXT("满血"),               false, TEXT("")},
		{TEXT("满怒气"),             false, TEXT("")},
		{TEXT("维持满怒"),           true,  TEXT("")},
		{TEXT("刷治疗石"),           false, TEXT("")},
		{TEXT("刷怒气石"),           false, TEXT("")},
		{TEXT("锁血"),               true,  TEXT("")},
		{TEXT("攻9999"),             true,  TEXT("")},
		{TEXT("清空敌人"),           false, TEXT("")},
		{TEXT("Debug全开"),          true,  TEXT("warrior.Combat.Debug.All")},
		{TEXT("调试无锁定目标"),      true,  TEXT("warrior.Combat.Debug.Targeting")},
		{TEXT("调试攻击辅助"),        true,  TEXT("warrior.Combat.Debug.AttackAssist")},
		{TEXT("调试站位系统"),        true,  TEXT("warrior.Combat.Debug.Director")},
		{TEXT("调试敌人指示器"),      true,  TEXT("warrior.Combat.Debug.ThreatIndicator")},
		{TEXT("调试镜头辅助"),        true,  TEXT("warrior.Combat.Debug.Camera")},
		{TEXT("调试命中拉回"),        true,  TEXT("warrior.Combat.Debug.StrikeAssist")},
	};
	return Commands;
}

static void ToggleCVar(const FString& Name)
{
	if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Name))
	{
		CVar->Set(CVar->GetBool() ? 0 : 1, EConsoleVariableFlags::ECVF_SetByCode);
	}
}

static bool IsCVarOn(const FString& Name)
{
	if (const IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*Name))
	{
		return CVar->GetBool();
	}
	return false;
}

void UGMPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	PopulateButtons();
}

void UGMPanelWidget::PopulateButtons()
{
	if (!CommandGrid || !CommandButtonClass)
	{
		return;
	}

	CommandGrid->ClearChildren();
	const auto& Commands = GetCommands();

	for (int32 i = 0; i < Commands.Num(); ++i)
	{
		UGMCommandButton* Btn = CreateWidget<UGMCommandButton>(this, CommandButtonClass);
		if (!Btn)
		{
			continue;
		}

		Btn->CommandIndex = i;
		Btn->OwnerPanel = this;
		Btn->SetLabelText(Commands[i].Name);

		const int32 Row = i / 4;
		const int32 Col = i % 4;
		CommandGrid->AddChildToUniformGrid(Btn, Row, Col);
	}

	RefreshToggleStates();
}

void UGMPanelWidget::ExecuteCommand(int32 CommandIndex)
{
	const UObject* WorldCtx = GetOwningPlayer();
	const auto& Commands = GetCommands();

	if (CommandIndex < 0 || CommandIndex >= Commands.Num())
	{
		return;
	}

	// 有 CVarName 的 → 直接 toggle CVar
	if (!Commands[CommandIndex].CVarName.IsEmpty())
	{
		ToggleCVar(Commands[CommandIndex].CVarName);
		RefreshToggleStates();
		return;
	}

	// 无 CVarName 的 → switch-case 调对应函数
	switch (CommandIndex)
	{
	case 0:  UWarriorFunctionLibrary::GM_SpawnEnemy(WorldCtx, GuardianClass);     break;
	case 1:  UWarriorFunctionLibrary::GM_SpawnEnemy(WorldCtx, GlacerClass);       break;
	case 2:  UWarriorFunctionLibrary::GM_SpawnEnemy(WorldCtx, FrostGiantClass);   break;
	case 3:  UWarriorFunctionLibrary::GM_FullHealth(WorldCtx);                     break;
	case 4:  UWarriorFunctionLibrary::GM_FullRage(WorldCtx);                       break;
	case 5:  UWarriorFunctionLibrary::GM_ToggleMaxRage();                          break;
	case 6:  UWarriorFunctionLibrary::GM_SpawnPickup(WorldCtx, HealStoneClass);    break;
	case 7:  UWarriorFunctionLibrary::GM_SpawnPickup(WorldCtx, RageStoneClass);    break;
	case 8:  UWarriorFunctionLibrary::GM_ToggleGodMode();                          break;
	case 9:  UWarriorFunctionLibrary::GM_ToggleMaxAttack(WorldCtx);                break;
	case 10: UWarriorFunctionLibrary::GM_ClearAllEnemies(WorldCtx);                break;
	default: break;
	}

	RefreshToggleStates();
}

void UGMPanelWidget::RefreshToggleStates()
{
	if (!CommandGrid)
	{
		return;
	}

	const auto& Commands = GetCommands();
	for (int32 i = 0; i < Commands.Num() && i < CommandGrid->GetChildrenCount(); ++i)
	{
		if (!Commands[i].bToggle)
		{
			continue;
		}

		UWidget* Child = CommandGrid->GetChildAt(i);
		if (UGMCommandButton* Btn = Cast<UGMCommandButton>(Child))
		{
			bool bOn = false;
			if (!Commands[i].CVarName.IsEmpty())
			{
				bOn = IsCVarOn(Commands[i].CVarName);
			}
			else
			{
				switch (i)
				{
				case 5:  bOn = UWarriorFunctionLibrary::GM_IsMaxRageOn();   break;
				case 8:  bOn = UWarriorFunctionLibrary::GM_IsGodModeOn();   break;
				case 9:  bOn = UWarriorFunctionLibrary::GM_IsMaxAttackOn(); break;
				default: break;
				}
			}
			Btn->SetLabelText(Commands[i].Name + (bOn ? TEXT(" (ON)") : TEXT(" (OFF)")));
		}
	}
}
