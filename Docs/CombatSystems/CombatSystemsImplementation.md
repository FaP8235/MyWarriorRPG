# Warrior Combat Systems - UE 5.6 接入说明

本次实现提供四套系统的 C++ 运行时基础，并把玩家、敌人、GAS 和 AI Controller 的入口接好。动画、Targeting Preset、行为树、EQS 与 Widget 仍然使用 Unreal Editor 资产配置。

## 1. 玩家无锁定近战目标

### 已实现

- `UMeleeTargetingComponent`
- `UDataAsset_CombatAttackProfile`
- `UTargetingTask_FilterMeleeTarget`
- `UTargetingTask_ScoreMeleeTarget`
- `FWarriorCombatAttackContext`
- 弹反目标优先规则：硬锁定目标 > 短时弹反目标 > 普通无锁定评分
- `MeleeTargetingComponent.弹反目标有效时间` 默认 1.25 秒；下一次攻击使用后立即清除
- 开启目标选择 Debug 后，弹反目标会显示为 `[COUNTER SELECTED]`
- 在 `GA_Hero_Block` 的 `PerfectBlock` 分支调用 `Set Counter Attack Target`，目标使用成功格挡事件数据中的 `Instigator`
- 未配置 Targeting Preset 时的 Pawn 球形查询兜底

一次攻击只调用一次 `SelectMeleeTarget`，结果被冻结到 `FWarriorCombatAttackContext`。Motion Warping 不会在攻击中途重新选择其他敌人。

### 编辑器配置

1. 创建 `DA_Attack_Melee_Default`，父类选择 `DataAsset_CombatAttackProfile`。
2. 创建 Targeting Preset，例如 `TP_Melee_Default`。
3. Preset 中按以下顺序添加任务：
   - `TargetingSelectionTask_AOE`
   - `TargetingTask_FilterMeleeTarget`
   - `TargetingTask_ScoreMeleeTarget`
4. AOE 推荐配置：
   - Shape Type：Sphere
   - Collision Object Types：Pawn
   - Ignore Source Actor：true
5. 将 `TP_Melee_Default` 配到 `DA_Attack_Melee_Default.TargetingPreset`。
6. 在 `BP_HeroCharacter` 的 `MeleeTargetingComponent` 上配置 Default Attack Profile。

如果暂时不创建 Targeting Preset，组件会使用 `MaxTargetDistance` 做 Pawn 球形查询，并使用同一套验证与评分规则。

## 2. 攻击吸附

### 已实现

- `UAttackAssistComponent`
- `PrepareMeleeAttack`
- `FinishMeleeAttack`
- Rotation Only / Motion Warp / None 三种模式

`PrepareMeleeAttack` 是近战 Gameplay Ability 的统一入口：

1. 选择目标。
2. 创建攻击上下文。
3. 计算 Warp Target Transform。
4. 将命名 Warp Target 写入现有 `UMotionWarpingComponent`。

### Gameplay Ability 接线

在以下蓝图 Ability 的播放 Montage 之前调用 `Prepare Melee Attack`：

- `GA_Hero_LightAttackMaster`
- `GA_Hero_HeavyAttackMaster`
- 需要吸附的特殊攻击

返回 false 时终止攻击；返回 true 时继续播放 Montage。Montage 完成、打断或取消时调用 `Finish Melee Attack`。

### Montage 配置

在需要吸附的攻击 Montage 中添加 UE 自带的 Motion Warping Notify State：

- Warp Target Name：必须和 Attack Profile 的 `WarpTargetName` 一致，默认 `AttackTarget`
- Root Motion Modifier：Skew Warp
- Rotation Type：Facing
- Ignore ZAxis：true

吸附窗口只覆盖攻击接近阶段，不要覆盖命中后的完整收招阶段。

## 3. 敌人进攻系统

### 已实现

- `UEnemyCombatDirectorSubsystem`
- `UEnemyCombatAgentComponent`
- 按玩家目标划分的 Token Pool
- Token Cost、Lease、超时回收
- 等待时间、距离、朝向组成的进攻性评分
- 相机朝向空间中的动态环形槽位
- `BTTask_RequestAttackToken`
- `BTTask_ReleaseAttackToken`
- `BTTask_ReserveCombatSlot`

敌人通过 AI Perception 获得玩家时，会同步更新 Agent 的 Combat Target。

### 推荐行为树结构

```text
Selector
├── Death / HitReact
├── Has Attack Token
│   └── Execute Attack Ability
└── Combat
    ├── Reserve Combat Slot
    ├── Run EQS Query（验证可导航、遮挡、与其他敌人的距离）
    ├── Move To
    └── Request Attack Token
```

`Reserve Combat Slot` 输出逻辑槽位到 Blackboard。目标会移动，因此需要在战斗循环中周期性重新执行，或者把槽位作为 EQS Context/参数继续验证。

在敌人攻击 Ability 上：

- 将 `bReleaseAttackTokenOnEnd` 打开。
- 起手阶段调用 `Set Attack Threat Indicator`。
- 近战使用 `MeleeWindup`。
- 远程使用 `RangedWindup`。

无论 Ability 正常结束还是被取消，开启该选项后都会归还 Token。

可以在 GameMode BeginPlay 中获得 `EnemyCombatDirectorSubsystem`，调用 `ConfigureTokenBudget` 设置难度对应的总预算：

- Easy：1
- Normal：2
- Hard：3
- Very Hard：4

## 4. 屏幕外威胁指示

### 已实现

- `UThreatIndicatorComponent`
- 世界坐标到 Widget 坐标投影
- DPI Scale 修正
- 屏幕内外判断
- 相机背后目标处理
- 椭圆边缘约束
- 指示器方向角
- 威胁颜色、优先级、限时注册

### Widget 接线

在 `WBP_HeroOverlay` 中创建统一的指示器 Layer，并绑定 Hero 的 `ThreatIndicatorComponent.OnThreatIndicatorsUpdated`：

1. 按 `SourceActor` 复用或创建箭头 Widget。
2. 使用 `ScreenPosition` 设置 Canvas Slot Position。
3. 使用 `RotationDegrees` 设置箭头 Render Transform Angle。
4. 使用 `Color` 设置颜色。
5. 本次数据中不存在的 SourceActor 对应箭头归还对象池。

不要在每个敌人上创建屏幕空间 Widget Component；敌人只注册威胁数据，箭头统一由 Hero HUD 管理。

## 当前边界

- C++ 已通过 UE 5.6 Development Editor 编译。
- 没有自动修改现有二进制 Blueprint、Montage、Behavior Tree、EQS 和 Widget 资产。
- 现有硬锁定逻辑没有被替换；`UMeleeTargetingComponent.SetExplicitTarget` 已提供硬锁定覆盖入口，可在下一步接入目标锁定 Ability 的目标变更位置。

## 5. 运行时 Debug 与录屏

所有自定义显示默认关闭，PIE 或 Standalone 中打开控制台执行：

```text
warrior.Combat.Debug.All 1
```

也可以独立启用：

```text
warrior.Combat.Debug.Targeting 1
warrior.Combat.Debug.AttackAssist 1
warrior.Combat.Debug.Director 1
warrior.Combat.Debug.ThreatIndicator 1
```

显示时间和文字缩放：

```text
warrior.Combat.Debug.Duration 2.5
warrior.Combat.Debug.TextScale 1.0
```

关闭全部：

```text
warrior.Combat.Debug.All 0
warrior.Combat.Debug.Targeting 0
warrior.Combat.Debug.AttackAssist 0
warrior.Combat.Debug.Director 0
warrior.Combat.Debug.ThreatIndicator 0
```

### Targeting 图例

- 青色球：最大搜索范围
- 黄色胶囊：有效候选
- 红色胶囊：无效候选
- 绿色粗胶囊和连线：最终选择目标
- 头顶文字：Camera、Screen、Distance、Input、Sticky、Visible 的加权分数

### Attack Assist 图例

- 蓝色点：角色当前位置
- 红色点：目标位置
- 绿色点：Motion Warp 目标位置
- 绿色圆环：理想攻击距离
- 蓝色扇形：允许吸附的角度和距离
- 绿色/红色文字：本次吸附 Accepted 或 Rejected

### Combat Director 图例

- 玩家周围蓝色圆环：逻辑战斗槽位半径
- 绿色槽位：空闲
- 红色槽位：已占用
- 敌人头顶：State、Aggression Score、Token、Slot
- 玩家头顶：Token 使用量、总预算、排队数量
- 粗绿色连线：持有进攻 Token 的敌人

### Threat Indicator 图例

- 世界空间彩色线：玩家到威胁来源
- 屏幕中心到边缘的彩色线：威胁方向
- 彩色十字：修正后的屏幕边缘位置
- 白色叉号：UE 原始投影位置
- 标签：威胁类型、优先级、距离和屏幕坐标

### UE 原生调试配合

```text
ShowDebug TargetingSystem
```

按 `'` 打开 Gameplay Debugger，可继续查看 Behavior Tree、EQS 和 AI Perception。录屏时建议自定义 Debug 与 UE 原生 Debug 分段展示，避免画面信息过密。
