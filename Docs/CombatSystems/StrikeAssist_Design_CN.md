# Strike Assist（命中拉回屏内）需求设计

> 状态：需求已确认，待评审 → 评审通过后据此开发。
> 参考：God of War GDC 2019《Evolving Combat in 'God of War' for a New Perspective》(Mihir Sheth) 的 Strike Assist（讲座称为"让新战斗成立的最关键一环"）。
> 范围：**单个模块** = Strike Assist（命中瞬间把被击敌人的被击退轨迹偏向「镜头朝向 × 击中方向」的混合，使其留在/回到屏幕内）。

## 1. 背景与目标

近景镜头下，连击会把敌人打飞出画面——玩家看不到自己刚在打的谁，被迫中断进攻。GOW 的解法 Strike Assist：命中瞬间修正受害者朝向 + 让其被击退轨迹偏向"镜头朝向"，把挨打的敌人**拉回/留在屏幕里**；连击会把目标越拉越靠近画面中心，玩家甚至能"用镜头瞄准"把敌人撞向某处。

**目标**：连击时被打的敌人不飞出画面，玩家能持续进攻、始终看得见目标。位移克制（每次小挪动）。

## 2. 现状（已有基础）

- 命中链路完整且事件化：武器命中 → `UWarriorGameplayAbility::ApplyGameplayEffectSpecHandleToHitResults` 应用伤害 GE → 成功则给受害者发 `Shared.Event.HitReact`。**这就是插入 Strike Assist 的天然位置**（攻击方 C++，命中那一帧）。
- `UMotionWarpingComponent` 在 `AWarriorBaseCharacter` 上每个角色都有、插件已启用。
- 攻击方已有 `UAttackAssistComponent`：`PrepareAttackAssist` 用 `AddOrUpdateWarpTargetFromTransform(WarpTargetName, Transform)` 把攻击方吸附到目标——**Strike Assist 是它在受害方的镜像**（把受害者拉到屏内）。
- 攻击方 `AttackAssistComponent->GetActiveAttackContext().AttackProfile` 在命中时有效 → **攻击方 C++ 能直接拿到本招的 `UDataAsset_CombatAttackProfile`，无需经事件 payload 传给受害者**。
- 敌人 HitReact 蒙太奇：`AM_ExoGame_Gruntling_React_Light_Front`（Guardian/Glacer 共用），地面、前向。

## 3. 核心设计（一句话）

> 命中瞬间（攻击方 C++），按「击中方向 × 镜头朝向」按本招 Strength 混合，算一个 warp 目标点，写进**受害者**的 `MotionWarpingComponent`（warp 名 `StrikeAssistTarget`）；受害者的 HitReact 蒙太奇里有一个 `AnimNotifyState_MotionWarp` 窗口消费它，于是硬直期间受害者被挪向那个点（= 被拉回屏内）。受害者 GA 完全不改。

## 4. warp 目标模型（2D，XY 平面、忽略 Z）

- `hitDir` = (受害者位置 − 攻击者位置) 在 XY 归一化 —— 自然被击退方向。
- `camDir` = **玩家 Control Rotation 前向**（yaw）在 XY 归一化 —— "镜头前方/屏幕内"。
- `targetDir = lerp(hitDir, camDir, Strength)` —— Strength 越大越往镜头方向拉。`hitDir≈camDir`（正前方挨打）时几乎不动；偏侧挨打时往镜头拽（"偏得越多、修得越多"，天然贴合 GDC）。
- **warp 目标点 = 受害者位置 + targetDir × WarpDistance**。
- warp transform 的旋转 = 受害者当前旋转（**只挪位移、不强制改朝向**，硬直动画自己管朝向）。
- 约束：每次位移克制（WarpDistance 默认 50cm），不夸张；连击才逐次把目标往画面中心带。

## 5. 改动清单

| # | 位置 | 类型 | 内容 |
|---|---|---|---|
| 1 | `Source/.../DataAssets/Combat/DataAsset_CombatAttackProfile.h` | C++ 加 | 两个 UPROPERTY：`StrikeAssistStrength=0.6`（0–1，ClampMin0 ClampMax1）、`StrikeAssistWarpDistance=50`（cm，ClampMin0）。 |
| 2 | `Source/.../WarriorFunctionLibrary.h/.cpp` | C++ 加 | 静态 `ApplyStrikeAssist(AActor* Victim, AActor* Attacker, const UDataAsset_CombatAttackProfile* Profile)`：guard 受害者有效且有 MotionWarpingComponent；按 §4 算 targetDir 与目标点；`AddOrUpdateWarpTargetFromTransform("StrikeAssistTarget", FTransform(当前旋转, 目标点))`。Profile 为空时回退默认 Strength 0.6、Distance 50。 |
| 3 | `Source/.../AbilitySystem/Abilities/WarriorGameplayAbility.cpp` `ApplyGameplayEffectSpecHandleToHitResults` | C++ 改 | 在 `WasSuccessfullyApplied()` 块内、发 HitReact 事件之后：若受害者是 `AWarriorEnemyCharacter`，取攻击方（avatar）的 `UAttackAssistComponent`→`GetActiveAttackContext().AttackProfile`，调 `UWarriorFunctionLibrary::ApplyStrikeAssist(Victim, Attacker, Profile)`。敌人打英雄时不触发（受害者非敌人）。 |
| 4 | 调试（可选） | C++ | 复用 `warrior.Combat.Debug` 体系加 `warrior.Combat.Debug.StrikeAssist`：画 hitDir/camDir/targetDir 与目标点。 |

> **不改**：受害者 HitReact GA（`GA_Enemy_HitReact_Base` 不动）、事件 payload（`Shared.Event.HitReact` 不动）、`ComputeHitReactDirectionTag`（不动）。

## 6. 编辑器步骤（你做，一次性）

给敌人 HitReact 蒙太奇 **`AM_ExoGame_Gruntling_React_Light_Front`**（Guardian/Glacer 共用）加：
- **`AnimNotifyState_MotionWarp`** 窗口，覆盖**硬直前段**（命中后最早的那段位移期）。
- **Warp Target Name = `StrikeAssistTarget`**（必须和 C++ 写入的 warp 名一致）。
- Root Motion Modifier：`Skew Warp`；按 AttackAssist 既有约定可设 Rotation Type=Facing、IgnoreZAxis=true（本项目只挪位移，旋转用受害者当前值，影响不大）。

> FrostGiant 用的是 `Troll_React_Death`，无独立 HitReact 蒙太奇——暂不覆盖（要给 Boss 加 Strike Assist 再单独配窗口）。

## 7. 不在本模块范围

- 空中命中反应 / 挑飞 / juggling（项目无空中系统；源素材里有 `Knockback_Front` 但不接）。
- 投射物的 Strike Assist（投射物走 `WarriorProjectileBase`，不经 `ApplyGameplayEffectSpecHandleToHitResults`，故不触发；近战命中才生效）。
- Camera Strafe Assist、Power-play（已完成模块）。

## 8. 验证方法

**编译**：`D:/Program Files/Epic Games/UE_5.6/Engine/Build/BatchFiles/Build.bat WarriorEditor Win64 Development -Project="...Warrior.uproject" -WaitMutex`。

**前置（编辑器）**：完成 §6 的蒙太奇窗口。

**玩测**（`CombatTestMap`/`SurvivalGameModeMap`，面对 1–2 个 Guardian/Glacer）：
1. **正前方**连击一个敌人 → 位移很小（hitDir≈camDir），敌人基本不乱飞。
2. **偏侧**打一个敌人（敌人不在屏幕正中）→ 每次命中被**小幅往镜头方向/屏幕内**拽；连击把目标**越拉越靠近画面中心**。
3. 全程敌人**不被打出画面**、不穿模贴脸英雄（WarpDistance 50cm 克制）。
4. （可选）`warrior.Combat.Debug.StrikeAssist 1`（若实现 #4）看 hitDir/camDir/targetDir 与目标点对齐。
5. 调参：在 `DA_Attack_Melee_Default`（或各招 profile）上调 `StrikeAssistStrength`/`StrikeAssistWarpDistance`，免重编。

## 9. 开放项 / 风险

- **蒙太奇窗口时机**：warp 目标在命中帧（C++）写入，受害者在随后激活的 HitReact 蒙太奇到达窗口时消费——只要窗口在硬直前段、且 GA 激活在命中之后（同帧/下一帧），目标已就绪。若玩测发现有时不生效，检查窗口是否覆盖到位移期、warp 名是否一致。
- **位移与近战间距**：50cm 克制；若仍把敌人推出近战范围或贴脸英雄，调小 `StrikeAssistWarpDistance`，或后续加"距英雄最小距离 clamp"。
- **只覆盖 Light_Front**：Guardian/Glacer 用它，生效；其它方向/敌人的反应蒙太奇无窗口则不生效（目前够用）。
