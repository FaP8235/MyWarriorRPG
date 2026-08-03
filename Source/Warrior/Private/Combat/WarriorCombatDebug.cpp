// FaP All Rights Reserve

#include "Combat/WarriorCombatDebug.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarWarriorCombatDebugAll(
		TEXT("warrior.Combat.Debug.All"),
		0,
		TEXT("Enables every Warrior combat debug visualization."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarWarriorCombatDebugTargeting(
		TEXT("warrior.Combat.Debug.Targeting"),
		0,
		TEXT("Draws melee targeting candidates, scores and the selected target."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarWarriorCombatDebugAttackAssist(
		TEXT("warrior.Combat.Debug.AttackAssist"),
		0,
		TEXT("Draws Motion Warping target, assist cone and ideal attack distance."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarWarriorCombatDebugDirector(
		TEXT("warrior.Combat.Debug.Director"),
		0,
		TEXT("Draws attack tokens, aggression scores and combat slots."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarWarriorCombatDebugThreatIndicator(
		TEXT("warrior.Combat.Debug.ThreatIndicator"),
		0,
		TEXT("Draws threat source lines and screen-space projection diagnostics."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarWarriorCombatDebugCamera(
		TEXT("warrior.Combat.Debug.Camera"),
		0,
		TEXT("Draws camera strafe assist focus yaw, deadzone and framing offset."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarWarriorCombatDebugStrikeAssist(
		TEXT("warrior.Combat.Debug.StrikeAssist"),
		0,
		TEXT("Draws strike assist pull direction (hit/cam/target arrows)."),
		ECVF_Cheat);

	TAutoConsoleVariable<float> CVarWarriorCombatDebugDuration(
		TEXT("warrior.Combat.Debug.Duration"),
		1.5f,
		TEXT("Lifetime in seconds for event-based combat debug shapes."),
		ECVF_Cheat);

	TAutoConsoleVariable<float> CVarWarriorCombatDebugTextScale(
		TEXT("warrior.Combat.Debug.TextScale"),
		1.f,
		TEXT("World and screen debug text scale."),
		ECVF_Cheat);

	bool IsAllEnabled()
	{
		return CVarWarriorCombatDebugAll.GetValueOnGameThread() != 0;
	}
}

bool WarriorCombatDebug::IsTargetingEnabled()
{
	return IsAllEnabled()
		|| CVarWarriorCombatDebugTargeting.GetValueOnGameThread() != 0;
}

bool WarriorCombatDebug::IsAttackAssistEnabled()
{
	return IsAllEnabled()
		|| CVarWarriorCombatDebugAttackAssist.GetValueOnGameThread() != 0;
}

bool WarriorCombatDebug::IsDirectorEnabled()
{
	return IsAllEnabled()
		|| CVarWarriorCombatDebugDirector.GetValueOnGameThread() != 0;
}

bool WarriorCombatDebug::IsThreatIndicatorEnabled()
{
	return IsAllEnabled()
		|| CVarWarriorCombatDebugThreatIndicator.GetValueOnGameThread() != 0;
}

bool WarriorCombatDebug::IsCameraEnabled()
{
	return IsAllEnabled()
		|| CVarWarriorCombatDebugCamera.GetValueOnGameThread() != 0;
}

bool WarriorCombatDebug::IsStrikeAssistEnabled()
{
	return IsAllEnabled()
		|| CVarWarriorCombatDebugStrikeAssist.GetValueOnGameThread() != 0;
}

float WarriorCombatDebug::GetDrawDuration()
{
	return FMath::Max(0.f, CVarWarriorCombatDebugDuration.GetValueOnGameThread());
}

float WarriorCombatDebug::GetTextScale()
{
	return FMath::Max(0.25f, CVarWarriorCombatDebugTextScale.GetValueOnGameThread());
}

