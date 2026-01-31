# 《WarriorRPG》战斗系统开发手册

**【注意】：开发已完毕，在进行项目框架与具体技术实现的整理**

## 一、现阶段完成情况

### 1 Character开发

在角色基类`WarriorBaseCharacter`中接入：

- GAS、战斗、UI和运动扭曲组件
- 角色属性集和角色初始数据

#### 1.1 PlayerCharacter

- 在构造方法中实例化并绑定弹簧臂`USpringArmComponent`、摄像头`UCameraComponent`、战斗、UI组件

- 配置角色运动组件的速度、朝向、转向等变量

- 绑定角色基本能力和输入回调函数，包括：

  - 基础移动`Input_Move`

  - 视角移动`Input_Look`

  - 切换锁定目标`Input_SwitchTargetTriggered` 和 `Input_SwitchTargetCompleted`

  - 拾取石头物品`Input_PickUpStonesStarted`

  - 基于Gameplay Tag的GAS能力`Input_AbilityInputPressed` 和 `Input_AbilityInputReleased`

- 在`PossessedBy`中基于不同游戏模式的难度设置配置角色不同属性的初始值

#### 1.2 EnemyCharacter

- 在构造方法中实例化并绑定弹簧臂`USpringArmComponent`、摄像头`UCameraComponent`、战斗、UI组件
- 配置角色运动组件的速度、朝向、转向等变量
- 在角色左右手绑定碰撞盒、配置左右手攻击能力（供Boss Enemy激活）
- 在`InitEnemyStartUpData`中基于不同游戏模式的难度设置配置角色不同属性的初始值



### 2 Controller

在控制器中配置TeamID以便敌人区别队友与攻击对象

#### 2.1 AIController

- 为AI控制器配置感知视野与感知机，基于TeamID识别敌人，并将敌人在BlackBoard中设置为TargetActor
- 基于Detour避障算法配置角色避障措施



### 3 GameMode

#### 3.1 SurvivalGameMode

- 从存储中读取游戏难度，以供角色加载配置数据
- 配置游戏玩法与游戏循环，进行游戏状态的切换
- 读取敌人波次生成配置表，根据游戏状态，在各波次开始前预加载敌人数据
- 按波次生成敌人



### 4 GameplayCues



### 5 Items



### 6 Maps



### 7 GameplayEffect



### 8 AnimNotify



### 9 AnimNotifyState



## 二、后续开发规划

### 2.1 修复目标锁定Bug

1. （待修复）在场角色为空时依然会启用目标锁定，且无法取消
2. （待修复）多目标存在时击杀单个目标后不会自动切换锁定目标



### 2.2 基于萨罗斯通用对象池方案设计敌人波次生成器

1. 实现简易版萨罗斯通用对象池
2. 基于对象池实现敌人波次生成
3. 进行多数量敌人生成测试



### 2.3 角色模型更替及UI美化

1. 挑选更为美化的玩家、敌人角色及武器网格体模型，对玩家动画进行重构
2. 使用更美观的血条、怒气和游戏菜单UI方案，优化视觉体验



## 三、开发环境

- 操作系统：Windows11（64位）
- UE版本：5.6
- 主要编程语言：C++



## 四、参考资料

- 个人开发文档记录（逐步补充中）：[我的个人博客](https://fap8235.github.io)
- Udemy课程参考：[Unreal Engine 5 C++: Advanced Action RPG](https://www.udemy.com/course/unreal-engine-5-advanced-action-rpg/)
- 原作者（Vince Petrelli）[GitHub仓库](https://github.com/vinceright3/WarriorRPG)