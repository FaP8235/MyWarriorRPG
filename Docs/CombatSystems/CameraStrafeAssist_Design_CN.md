# Camera Strafe Assist（战斗镜头取景）需求设计

> 状态：需求已确认，待评审 → 评审通过后据此开发。
> 参考：God of War GDC 2019《Evolving Combat in 'God of War' for a New Perspective》(Mihir Sheth) 的 Camera Strafe Assist。
> 范围：**单个模块** = Camera Strafe Assist（横移居中 + 攻击转向 + "动作在右"横向构图 + 死区，含改 TargetLock）。

## 1. 背景与目标

近景肩越镜头下，玩家左右横移时容易把敌人甩出画面、丢失威胁方位。GOW 的解法是 Camera Strafe Assist：镜头自动微调，把相关威胁保持在画面里；起手攻击时短暂转向目标；并用"动作在右"构图让画面可读。

**目标**：让玩家在横移/多敌战斗中不必频繁手动转镜头也能看清威胁，且不与手动瞄准打架、不抖。

## 2. 现状（已有基础，无需重造）

- 镜头是**纯弹簧臂**：`CameraBoom`（`USpringArmComponent`，`TargetArmLength=200`、`SocketOffset=(0,55,65)`、`bUsePawnControlRotation=true`），无 CameraLag、无 Probe。整个 Source 无自定义 `PlayerCameraManager`/`ViewTarget`。
- **镜头唯一通道 = Control Rotation**。`Input_Look` 加 yaw/pitch 输入；`HeroGameplayAbility_TargetLock::OnTargetLockTick` 每帧 `SetControlRotation`（`FindLookAtRotation` 减一个 pitch 偏移 `TargetLockCameraOffsetDistance=20`，是角度非距离），`RInterpTo` 平滑，Roll/Block 时让位。
- `bOrientRotationToMovement=true`（非锁定角色朝移动方向）。
- 数据源齐全：`UEnemyCombatDirectorSubsystem`（`RegisteredAgents`，每 agent 有 `IsOnPlayerScreen()`/`GetCombatTarget()`/`HasAttackToken()`/`GetAggressionScore()`）；`UMeleeTargetingComponent::GetCurrentTarget()` + `OnMeleeTargetChanged`；`UAttackAssistComponent::OnAttackAssistPrepared`。

## 3. 核心设计（三个子行为 + 一个跨模式死区）

| 子行为 | 作用 | 触发 |
|---|---|---|
| ① 横移居中 | yaw 微调，把屏内相关敌人的加权中心保持在画面里 | 交战中、非手动瞄准时 |
| ② 攻击转向 | 起手攻击时柔和转向当前单一近战目标 | 攻击起手（覆盖①约一拍） |
| ③ 动作在右 | `SocketOffset.Y` 向左偏，英雄偏左、动作区偏右 | 交战时（锁定/非锁定都套） |
| 死区 | 聚焦点在容忍角内不动镜头，出区只修到边缘 | 跨①②与锁定，全程 |

**核心矛盾的处理**：辅助要自动调 yaw，但玩家又能手动瞄准。解法 = 死区（框内不干预）+ 手动瞄准让位（用 `IA_Look` 时暂停①）。

## 4. 子行为详细

### ① 横移居中（非锁定）
- **聚焦点（focus yaw）= 屏内交战敌人的加权群体中心**：
  - 集合 = `CombatTarget==玩家` 且 `IsOnPlayerScreen()` 且未死的敌人（屏幕外的不参与，避免把镜头拽离玩家意图；它们由 ThreatIndicator 箭头负责）。
  - 权重 `w = 1/(1+距离/500)`（越近越重）。
  - 算法 = 每个敌人相对玩家的**单位方向向量**按 `w` 加权求和，取合成向量的 Yaw。
  - 防抖：focus yaw 再过一层 `FInterpTo`（低通，偏慢）。
- **修正（叠加在 Control Rotation 的 yaw 上）**：见 §5 死区机制。
- **手动瞄准让位**：`IA_Look` 去抖 ~0.5s 内有输入 → 暂停①的 yaw 修正（②③仍生效）；停手后恢复。

### ② 攻击转向
- 订阅 `UAttackAssistComponent::OnAttackAssistPrepared`（或 `UMeleeTargetingComponent::OnMeleeTargetChanged`）。
- 攻击起手时，把 focus 临时设为**当前单一近战目标**的方位，持续约一个攻击窗口（~0.3–0.5s），用比①更"用力"的插值转向；结束后回到①的群体中心。
- 优先级高于①；与手动瞄准让位的关系：攻击是强意图，②期间即便玩家在手动瞄准也允许转向（可玩测后收紧）。

### ③ "动作在右"横向构图
- 交战时给 `CameraBoom->SocketOffset.Y` 叠一个向左偏移（推荐 **+40**，叠在现有静态 55 之上），`FInterpTo` 渐入；脱战/无目标时渐出回基准。
- 锁定/非锁定都套用（构图规则）。TargetLock 继续管它的 pitch 偏移，**横向偏移归新组件**。

### pitch
- **只辅助 yaw，不碰 pitch**。俯仰完全留给玩家（TargetLock 现有 pitch 行为保留）。

## 5. 死区（跨模式，核心防抖）

- **度量**：角度法。`delta = 角度差(当前 camYaw, focusYaw)` ∈ [−180,180]。
- **框内不动**：`|delta| <= DeadzoneAngle`（推荐 **10°**，可调）→ 不改 yaw。
- **最小修正**：出区时只把镜头转到"focus 落到死区边缘"，不拉到正中：`目标yaw = camYaw + sign(delta)*(|delta| − DeadzoneAngle)`，再 `RInterpTo` 平滑过去。
- **锁定模式同样套用**：改 `HeroGameplayAbility_TargetLock::OnTargetLockTick`——计算到目标的 look-at yaw 后，先过死区（最小修正）再 `SetControlRotation`，pitch 路径不变。这样锁定时目标小幅移动不会让镜头逐帧微抖。
- **互斥**：`Player.Status.TargetLock` tag 存在时，新组件的①yaw 修正**让位**（TargetLock 接管 yaw，且它自带死区）；③横向构图仍生效。

## 6. 改动清单

| # | 位置 | 类型 | 内容 |
|---|---|---|---|
| 1 | `Source/.../Components/` 新建 `UCameraStrafeAssistComponent`（.h/.cpp） | C++ 新增 | 挂 `AWarriorHeroCharacter`。Tick：算 focus yaw（①）→ 死区+最小修正+低通 → 叠到 Control Rotation yaw（`AddControllerYawInput` 或 `SetControlRotation`）；`Player.Status.TargetLock` 在则跳过①。管③横向 `SocketOffset.Y` 渐入渐出。订阅②事件做攻击转向覆盖。可调 UPROPERTY：`DeadzoneAngle=10`、`CorrectionInterpSpeed`、`FramingOffsetY=40`、`ManualLookDebounce=0.5`、`ProximityDenominator=500`、`AttackRecenterDuration=0.4`。 |
| 2 | `Source/.../Characters/WarriorHeroCharacter` | C++ 改 | 构造里 `CreateDefaultSubobject<UCameraStrafeAssistComponent>`；`Input_Look` 里调 `StrafeAssist->NotifyManualLook()` 记录手动瞄准时间（供⑤让位）。 |
| 3 | `Source/.../AbilitySystem/Abilities/HeroGameplayAbility_TargetLock.cpp` `OnTargetLockTick` | C++ 改 | 计算 look-at yaw 后，套死区（最小修正，§5）再 `SetControlRotation`；新增 `DeadzoneAngle` UPROPERTY（默认 10）。pitch 路径不变。 |
| 4 | `Source/.../AI/EnemyCombatDirectorSubsystem` | C++ 加 | public 查询 `GetOnScreenAgentsTargeting(AActor* Player, TArray<UEnemyCombatAgentComponent*>& Out) const`：遍历 `RegisteredAgents`，筛 `GetCombatTarget()==Player && IsOnPlayerScreen()`。供①算 focus。 |
| 5 | 调试 | C++ | 复用 `warrior.Combat.Debug` 体系加一个开关（如 `warrior.Combat.Debug.Camera`）：画 focus yaw 方向、死区角、当前 camYaw、攻击转向窗口、SocketOffset 偏移量，便于调参。 |

## 7. 不在本模块范围

- pitch 自动辅助（仅 yaw）。
- 设置菜单里的开关 UI（先用 CVar/UPROPERTY，GDC 的 settings toggle 日后再加）。
- Strike Assist（下一模块）。

## 8. 验证方法

**编译**：`D:/Program Files/Epic Games/UE_5.6/Engine/Build/BatchFiles/Build.bat WarriorEditor Win64 Development -Project="...Warrior.uproject" -WaitMutex`。

**玩测**（`CombatTestMap`/`SurvivalGameModeMap`，多敌）：
1. **非锁定横移**：面对 2–3 个敌人，左右横移 → 镜头应把屏内敌人保持在画面里、不甩出；停下/不横移时镜头不乱动（死区）。
2. **手动瞄准让位**：横移中用鼠标/右摇杆主动转镜头 → 辅助不抢；松手后柔和回到群体中心。
3. **攻击转向**：起手轻击 → 镜头短暂转向当前目标，打完回到群体中心。
4. **动作在右**：交战时英雄应稳定偏画面左侧；脱战回到居中。
5. **锁定死区**：锁定一个目标，目标小幅横移 → 镜头不逐帧微抖（死区生效）；大幅移动才追。
6. （可选）`warrior.Combat.Debug.Camera 1` 看 focus/死区可视化，确认对齐。

## 9. 开放项 / 风险

- **抖动主战场**：①的加权中心天然会随敌人移动漂移；靠"低通 + 死区 + 最小修正"三层压制。若玩测仍抖，优先调大 `DeadzoneAngle`、调慢 focus 低通，或临时退化为单一主导威胁（§问题3 的方案 A）。
- **与 TargetLock 的 yaw 边界**：TargetLock 激活/取消瞬间，yaw 控制权在新组件与 TargetLock 间切换，可能有一帧跳变；用 tag 检测 + 平滑过渡缓解，玩测确认。
- **"动作在右"与 TargetLock pitch 偏移共存**：两者改不同属性（SocketOffset.Y vs ControlRotation pitch），理论上不冲突；玩测确认锁定时构图不违和。
