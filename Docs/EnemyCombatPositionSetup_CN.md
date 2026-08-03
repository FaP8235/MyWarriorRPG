# 敌人战斗站位系统接入说明

## 命名约定

本系统新增内容使用以下名称：

- C++ 数据资产类型：`UEnemyCombatPositionProfile`
- 数据资产：`DA_EnemyCombatPosition_<EnemyType>`
- C++ 行为树服务：`UBTService_UpdateCombatPosition`（编辑器显示 `更新敌人战斗站位`）
- C++ EQS 上下文：`UEnvQueryContext_CombatPosition`（编辑器显示 `Combat Position`）
- C++ EQS 测试：`UEnvQueryTest_CombatPosition`（编辑器显示 `Combat Position Rules`）
- Blackboard Key：`CombatPositionLocation`

旧的 `CombatSlot` 接口仅用于兼容已有行为树。新资产不要继续使用
`CombatSlotLocation`、`Update Combat Slot Location` 或 `Combat Slot`。

如果以后为这些原生节点创建蓝图子类，资产分别使用 `BTS_`、`EQC_`、`EQT_`
前缀；当前原生节点不需要额外创建同名资产。

## 推荐目录

```text
/Game/EnemyCharacter/Data/Combat/Position/
    DA_EnemyCombatPosition_Guardian
    DA_EnemyCombatPosition_Glacer
```

## Guardian 推荐配置

`DA_EnemyCombatPosition_Guardian`：

| 属性 | 建议值 |
|---|---:|
| 前方区域最近距离 | 180 cm |
| 前方区域最远距离 | 320 cm |
| 前方区域半角 | 65° |
| 前方引导位置数量 | 4 |
| 待机圆环最近距离 | 350 cm |
| 待机圆环最远距离 | 520 cm |
| 待机引导位置数量 | 12 |
| 保持屏幕外世界象限 | true |
| 屏幕外区域最近距离 | 380 cm |
| 屏幕外区域最远距离 | 560 cm |
| 屏幕外引导位置数量 | 12 |
| 屏幕边缘留白 | 0.04 |
| 离屏确认时间 | 0.15 s |
| 敌人最小间距 | 160 cm |

将该资产设置到 `BP_Gruntling_Guardian` 的
`EnemyCombatAgentComponent -> Combat Position -> 战斗站位配置`。

## Glacer 推荐配置

`DA_EnemyCombatPosition_Glacer`：

| 属性 | 建议值 |
|---|---:|
| 前方区域最近距离 | 700 cm |
| 前方区域最远距离 | 1000 cm |
| 前方区域半角 | 70° |
| 前方引导位置数量 | 4 |
| 待机圆环最近距离 | 800 cm |
| 待机圆环最远距离 | 1200 cm |
| 待机引导位置数量 | 12 |
| 保持屏幕外世界象限 | true |
| 屏幕外区域最近距离 | 800 cm |
| 屏幕外区域最远距离 | 1200 cm |
| 屏幕外引导位置数量 | 12 |
| 屏幕边缘留白 | 0.04 |
| 离屏确认时间 | 0.15 s |
| 敌人最小间距 | 180 cm |

Glacer 的紧急近战分支仍直接使用近战距离判断；这套远程配置主要约束其
射击和等待站位，不要求近战与远程单位使用同一个距离圆环。

## Blackboard 修改

在 `BB_Enemy_Base` 新增：

```text
CombatPositionLocation : Vector
```

保留旧 `CombatSlotLocation`，直到所有行为树和 EQS 都完成替换并保存成功。

## Guardian 行为树修改

1. 在 `BT_Guardian` 的根 `Selector` 上添加 `Update Combat Position` Service。
2. `Target Actor Key` 选择 `TargetActor`。
3. `Combat Position Location Key` 选择 `CombatPositionLocation`。
4. 删除 `Strafe` Sequence 上旧的 `Update Combat Slot Location` Service。
5. 保留现有的 `Request Attack Token` 节点；获得 Token 后系统会自动把站位区域从
   `IdleRing` 切换为 `FrontZone`。

## Guardian EQS 修改

在 `EQS_FindStrafingLocation` 中：

1. 添加 `Combat Position Rules` Test。
2. `Test Purpose` 设置为 `Filter Only`。
3. Bool Match 设置为 `true`。
4. 保留或新增 Distance Test：
   - Distance To：`Combat Position`
   - Test Purpose：`Score Only`
   - Distance Mode：`Distance 2D`
   - Scoring Equation：`Inverse Linear`
   - Scoring Factor：`2.0`

`Combat Position Rules` 负责硬性过滤距离、前方夹角、屏幕外世界象限和敌人间距；
Distance Test 只负责在合法位置中优先选择靠近引导位置的点。

## Glacer 行为树与 EQS 修改

1. 在 `BT_Glacer` 根 `Selector` 上添加同一个 `Update Combat Position` Service。
2. 使用 `TargetActor` 和 `CombatPositionLocation`。
3. 在 `EQS_FindShootProjectileLocation` 添加 `Combat Position Rules`。
4. 保留原有的射线、路径、可达性和射击距离测试。
5. 可以额外添加到 `Combat Position` 的 Distance Score，但不要删除原射击安全距离逻辑。

## 调试

控制台输入：

```text
warrior.Combat.Debug.Director 1
```

颜色含义：

- 红色：镜头前方进攻区
- 蓝色：外围待机圆环
- 紫色：屏幕外保持区
- 粗线和大球：已占用的位置

敌人头顶文字会显示：

- Combat State
- Aggression Score
- Token 状态和消耗
- 当前 Position Zone
- Position Index
- 是否在屏幕内
