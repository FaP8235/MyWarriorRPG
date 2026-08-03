# Power-play（token 暂留）需求设计

> 状态：需求已确认，待评审 → 评审通过后据此开发。
> 参考：God of War GDC 2019《Evolving Combat in 'God of War' for a New Perspective》(Mihir Sheth)。
> 范围：**单个模块** = Power-play（含其必需的 HitReact 状态信号接线）。

## 1. 背景与目标

讲座原意：当一个**正在进攻**的激进敌人被玩家断招（进入命中反应 / HitReact）时，**暂时不归还它的进攻 token**——既挡住其他敌人抢 token、自己又不能攻击——给玩家一段进攻奖励窗口。

**当前问题（根因在 token 系统）**：敌人攻击 GA 被 HitReact 打断取消时，`UWarriorEnemyGameplayAbility::EndAbility` 里 `bReleaseAttackTokenOnEnd` 照样归还 token（取消与正常结束都触发，无 `bWasCancelled` 守卫）→ token 立刻回池 → 下一个敌人马上抢走 → **没有奖励窗口**。

**目标**：让 token 系统在"因硬直被打断"时**不归还 token**，使 token 贯穿整条"连晕链"持续持有，直到敌人真正还手成功、或连晕达上限、或租约超时。

## 2. 已有基础（无需重造）

- 进攻 token 系统：`UEnemyCombatDirectorSubsystem`（预算/租约/评分）+ `UEnemyCombatAgentComponent`（每敌人适配器，状态机）。
- `EWarriorEnemyCombatState` 枚举**已含 `HitReact` 值**，但 C++ 从不设置它（`GA_Enemy_HitReact_Base` 也未设）。
- 两个已写好、在等调用的守卫：
  - `CanBecomeAggressive()`（`EnemyCombatAgentComponent.cpp:408-413`）：状态==HitReact 时返回 false（硬直中敌人不会被授予新 token）。
  - `HandleAttackTokenRevoked()`（`:652-653`）：状态==HitReact 时跳过转 Recovering。
- **`GA_Enemy_HitReact_Base` 既有配置（关键，省一半活）**：
  - Asset Tag：`Shared.Ability.HitReact`
  - **Cancel Abilities with Tag：`Enemy.Ability`**
  - **Block Abilities with Tag：`Enemy.Ability`**
  - → HitReact 激活时自动 Cancel 在跑的近战 GA、并 Block 期间的新激活。**"HitReact 期间攻击空转 / BT 死循环"问题已被此配置解决，无需额外 C++ 激活守卫。**
- token 预算按难度：Easy=1 / Normal=2 / Hard=3 / VeryHard=4（`ConfigureTokenBudget`，BP 端调）。
- 租约默认 4s（`BTTask_RequestAttackToken` 默认 `LeaseDuration=4`），到点 `CleanupInvalidState` 自动回收。

## 3. 核心设计（一句话）

> **Power-play 是 token 系统的改动**：给 token 的"释放"路径加一条守卫——**当近战 GA 是被硬直取消时，跳过归还**。token 因此贯穿连晕链一直持有。HitReact 侧的唯一改动是给 owner 挂一个"硬直中"身份 Tag，供 token 系统读取。

## 4. token 暂留生命周期（详细）

| 事件 | token | 受击计数 | 状态 |
|---|---|---|---|
| 敌人获得 token、近战 GA 执行中 | 持有 | 0 | Aggressive |
| 被打 → HitReact（Tag 增） | **保留**（取消时不归还） | +1 | HitReact |
| 硬直中（近战 GA 被 Block，无法激活） | 保留 | 不变 | HitReact |
| 硬直结束（Tag 删） → **仍持有**，回到 Aggressive | 保留 | 不变 | Aggressive |
| 敌人**真正打出一次攻击**（近战 GA 正常结束，非取消） | 归还（正常） | **清零** | — |
| 同一持有期内被连续打断 **> 4 次**没还手 | **强制归还** | 清零 | HitReact→… |
| 4s 租约超时 / 死亡 / 脱战 | 归还（兜底） | 清零 | Recovering/NonAggressive |

**关键不变量**：token 只在三种情况释放——①敌人还手成功（近战 GA 正常结束）、②连晕达上限、③租约超时/死亡/脱战。硬直本身（即使结束）**不释放** token，这样连晕链才能累加计数、硬顶才有效。

> 注：早期讨论里"硬直结束即释放"的写法已废弃——那样会让计数器每次归零、硬顶永远触发不了。以本表为准。

## 5. 受击计数器规则

- **计数单位**：每次进入 HitReact（`Shared.Ability.HitReact` Tag 被**添加**）且当前 `HasAttackToken` 为真 → +1。非 token 持有者挨打不计数（无 token 可暂留）。
- **硬顶阈值**：计数 **> 4** 时强制 `ReleaseAttackToken`（换人上）。全难度统一，**不按难度缩放**——难度由 token 预算承载（已实现）。
- **清零时机**：token 被释放时（在 `HandleAttackTokenRevoked` 统一清零）。覆盖正常还手、硬顶强释、超时、死亡所有出口。
- **存储**：`EnemyCombatAgentComponent` 上 `UPROPERTY(EditDefaultsOnly) int32 HitReactRetentionCap = 4` + 运行时计数 `int32 ConsecutiveHitReactCount = 0`。

## 6. 改动清单

| # | 位置（文件） | 类型 | 内容 |
|---|---|---|---|
| 1 | `GA_Enemy_HitReact_Base`（蓝图，1 节点） | 配置 | 把 `Shared.Ability.HitReact` 加入 **Activation Owned Tags**（让 owner 在硬直期间可靠携带此 Tag）。**不碰动画/蒙太奇逻辑。** |
| 2 | `Source/.../WarriorEnemyGameplayAbility.cpp` `EndAbility`（约 :10-32） | C++ 改 | `bReleaseAttackTokenOnEnd` 分支加守卫：**`bWasCancelled==true` 时跳过** `ReleaseAttackToken`（攻击被取消=被打断，暂留 token）。正常结束（非取消）照常归还。用 `bWasCancelled` 而非查 Tag，规避时序问题（见 §9）。 |
| 3 | `Source/.../EnemyCombatAgentComponent` | C++ 加 | 监听本角色 ASC 上 `Shared.Ability.HitReact` Tag 的增/删：<br>• **增**：`SetCombatState(HitReact)`；若 `HasAttackToken` → `ConsecutiveHitReactCount++`；若 `> HitReactRetentionCap` → `ReleaseAttackToken()`。<br>• **删**：若 `HasAttackToken` → `SetCombatState(Aggressive)`；否则 `SetCombatState(Recovering)`。**不释放 token。** |
| 4 | `Source/.../EnemyCombatAgentComponent` `HandleAttackTokenRevoked`（约 :640-658） | C++ 改 | 末尾追加 `ConsecutiveHitReactCount = 0;`（统一清零点）。 |
| 5 | （可选）调试 | C++/配置 | 扩展现有 `warrior.Combat.Debug.Director`：被暂留 token 的敌人头顶显示 `PP`/计数，便于玩测。 |
| 6 | `BT_Guardian`（编辑器，**你手动**） | 蓝图清理 | 移除"受击→反击"分支。power-play 不依赖它（HitReact GA 的 Block 已压住反击激活）；属设计对齐到 GOW"挨打就硬直"模型，时机不限。 |
| 7 | `GE_Enemy_UnderAttack`（编辑器，**待定**） | 蓝图清理 | 反击分支移除后，若该 GE（挂 `Enemy.Status.UnderAttack` 6s）无别处引用则成孤儿，记一笔待清理。**先核实有无其他用途，不盲删。** |

> HitReact GA 的 Cancel/Block `Enemy.Ability` 配置**保持不动**——它已经免费解决了"硬直期间攻击空转"。

## 7. 不在本模块范围

- **空中命中反应 / 挑飞 / juggling**：项目无空中系统（`LaunchCharacter` 等全 0 命中），讲座里"空中延长窗口"部分跳过。
- **窗口/计数按难度缩放**：已决定全难度统一（预算承载难度）。
- **Strike Assist、Camera Strafe Assist**：后续模块，本文档不涉及。

## 8. 验证方法

**编译**：`D:/Program Files/Epic Games/UE_5.6/Engine/Build/BatchFiles/Build.bat WarriorEditor -Development -WaitMutex`（增量）。

**玩测**（`CombatTestMap` 或 `SurvivalGameModeMap`，Normal 难度预算=2）：
1. 让一个**正在攻击**的敌人挨打 → 确认它进入硬直、且**另一个敌人没有立刻补位攻击**（token 被暂留）。
2. 硬直播完 → 确认 token 仍被它持有、它恢复后能正常还手；还手后 token 归还、计数清零。
3. 连续打断同一个敌人，不让其还手 → 第 5 次进入硬直时确认 **token 被强制释放、换另一个敌人上**。
4. 全程确认：硬直期间敌人**不会**反复激活/取消近战 GA（无 ASC 抖动、无控制台报错）——由既有 Block 配置保证。
5. （可选）开 `warrior.Combat.Debug.Director 1`，观察暂留标记与计数。

## 9. 开放项 / 风险（开发时验证）

- **Tag 时序（已规避）**：守卫 #2 最终采用 `bWasCancelled==true` 判断"被打断"（而非查 `Shared.Ability.HitReact` Tag），彻底规避了"HitReact GA 的 Activation Owned Tags 是否在取消前已授予"的时序不确定性。代价：非硬直的取消（如 BT 中止）也会暂留 token，但有 4s 租约兜底。若日后玩测发现非硬直取消暂留过久，可在守卫上额外加 `&& ASC->HasMatchingGameplayTag(Shared.Ability.HitReact)` 收紧。
- **计数/状态/硬顶仍依赖 Tag**：#3 的计数、状态切换、4 次硬顶靠监听 `Shared.Ability.HitReact` Tag，故 GA 的 Activation Owned Tags 配置（#1）**仍是必需**——否则只有 token 暂留生效，计数与硬顶不会触发（退化为"有租约兜底"的安全模式）。
- **Guardian 反击分支移除**为你手动编辑器操作，不阻塞 C++ 开发；移除前 power-play 也能正常工作（Block 已压住反击激活）。
