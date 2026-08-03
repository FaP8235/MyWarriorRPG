# 敌人分离斥力（Separation）需求设计

> 状态：需求已确认，待评审 → 评审通过后据此开发。
> 参考：God of War GDC 2019《Evolving Combat in 'God of War' for a New Perspective》的 Separation Constraint。
> 范围：**单个模块** = 敌人间实时分离斥力（防抱团），检测放行为树、idle 时按间隔判定。

## 1. 背景与目标

近景战斗中敌人容易叠在一起（抱团），既不真实也影响可读性。GOW 给每个敌人一个**按体型的分离检测器**（激进敌人=圆/半径），互相靠太近就**像斥力一样推开**、重选站位。

**目标**：敌人不重叠/不抱团，靠近时互相挤开，并重选分开的站位。

## 2. 已有相关机制（避免重复造）

- **站位级分离**：`MinimumEnemySpacing` + `IsLocationSeparatedFromOtherAgents`——**选站位时**校验和别人够不够远。
- **Detour 群体避障**：`WarriorAIController` 配了 Crowd Avoidance——**寻路移动时**互相避让。

这两层分别管"选点"和"移动"。本模块补**第三层：idle/逼近时的实时斥力滑开**（上面两层管不到的"站定后/靠近时叠在一起"）。

## 3. 核心设计

> 检测器 = **圆**（半径，按体型）。检测放 **BT Service**、只在 idle/站位状态、按 `TickInterval`（≈0.2s）跑——**不每帧、不每 tick**，省性能。重叠则**滑开 + 作废站位重选**。

- **检测**：2D 距离 `(A.半径 + B.半径)` → 重叠。简单距离判断，不复杂。
- **施斥力**：本敌人沿"远离对方"方向**滑开 `重叠量/2`**（`SetActorLocation` sweep，只走 XY、忽略 Z），单次滑移有上限（~50cm）防飞。
- **重选站位**：被推则 `ReleaseCombatPosition()` → BT 的 `UpdateCombatPosition` 下次重选分开点位。
- **对称**：A、B 各滑一半，自然分开，不重复算。

## 4. 改动清单

| # | 位置 | 类型 | 内容 |
|---|---|---|---|
| 1 | `Source/.../AI/EnemyCombatDirectorSubsystem` | C++ 加 | `bool GetSeparationDelta(const UEnemyCombatAgentComponent* Agent, FVector& OutDelta) const`：遍历 `RegisteredAgents`，找 2D 距离 < (Agent.半径 + 对方.半径) 的，把各"重叠量/2 × 远离方向"累加进 OutDelta（单轴上限 ~50cm）。有重叠返回 true。Director 持有全部 agent，由它算最自然。 |
| 2 | `Source/.../Components/Combat/EnemyCombatAgentComponent.h` | C++ 加 | `UPROPERTY(EditDefaultsOnly, ...) float SeparationRadius = 100.f;`（按体型/胶囊可调）。 |
| 3 | `Source/.../AI/BTService_EnemySeparation.h/.cpp`（新） | C++ 新增 | BT Service，`TickInterval≈0.2s`。每间隔：取 owner 的 Agent → `Director->GetSeparationDelta(Agent, Delta)`；若 true：`Owner->SetActorLocation(OwnerLoc + Delta, sweep)`（XY）+ `Agent->ReleaseCombatPosition()`。 |
| 4 | `Source/.../AI/EnemyCombatDirectorSubsystem.cpp` `DrawDebugState` | C++ 加 | 在现有 `warrior.Combat.Debug.Director` 下，每个 agent 画**分离圆**（半径=SeparationRadius）。⚠️ 只是画圆（每帧几个圆、几乎不耗），**与检测逻辑分开**——检测在 BT(0.2s)、画圆是纯可视化。 |

## 5. 编辑器步骤（你做）

把 **`BTService_EnemySeparation`** 加到敌人 BT（`BT_Guardian`/`BT_Glacer`/…）的**待机/调整站位分支**（敌人没在攻击的那段），`TickInterval` 设 ~0.2s。这步是让"检测"真正在 idle 时跑起来。

## 6. 不在本模块范围

- 月牙/前重后轻形状（已定用圆）。
- 空中分离（无空中系统）。
- 跨目标分离（只管同一 Director 注册集内的敌人，足够）。

## 7. 验证方法

**编译**：`D:/Program Files/Epic Games/UE_5.6/Engine/Build/BatchFiles/Build.bat WarriorEditor Win64 Development -Project="...Warrior.uproject" -WaitMutex`。

**前置（编辑器）**：完成 §5（BT 接服务）。

**玩测**（`CombatTestMap`/`SurvivalGameModeMap`，多敌）：
1. 让 2–3 个敌人靠近/叠在一起 → 它们应**互相挤开**，不再重叠。
2. 挤开后它们**重新站到分开的点位**（不又叠回去）。
3. 单敌时不触发（无重叠）。
4. 开 `warrior.Combat.Debug.Director 1` → 每个敌人脚下有**分离圆**，重叠时可见圆相交、随后分开。
5. 调参：`SeparationRadius`（每敌人 BP 的 Agent 上）、滑移上限。

## 8. 开放项 / 风险

- **滑开与 BT MoveTo 共存**：`SetActorLocation` 小幅滑移走 XY、单次有上限，不通过速度逻辑，理论上不与 BT 的 MoveTo 速度抢；玩测确认敌人不抖。
- **作废站位频率**：仅在确实被推（有重叠）时 `ReleaseCombatPosition`，idle 无重叠不释放，避免反复重预留抖动。
- **TickInterval 取舍**：0.2s 省性能但斥力略"顿"；嫌慢可调小（0.1s），仍远省于每帧。
