// FaP All Rights Reserve

#pragma once

#include "CoreMinimal.h"

/**
 * Runtime debug switches shared by the four combat systems.
 *
 * Console variables:
 * warrior.Combat.Debug.All
 * warrior.Combat.Debug.Targeting
 * warrior.Combat.Debug.AttackAssist
 * warrior.Combat.Debug.Director
 * warrior.Combat.Debug.ThreatIndicator
 * warrior.Combat.Debug.Duration
 * warrior.Combat.Debug.TextScale
 */
namespace WarriorCombatDebug
{
	WARRIOR_API bool IsTargetingEnabled();
	WARRIOR_API bool IsAttackAssistEnabled();
	WARRIOR_API bool IsDirectorEnabled();
	WARRIOR_API bool IsThreatIndicatorEnabled();
	WARRIOR_API bool IsCameraEnabled();
	WARRIOR_API bool IsStrikeAssistEnabled();
	WARRIOR_API float GetDrawDuration();
	WARRIOR_API float GetTextScale();
}

