## 书名
用 AI Coding 驱动的高级 C++/CMake CAD 应用开发实战

## 目标读者
- 高级 C++ 工程师：需要在大型代码库中进行快速增量开发与重构
- 渲染/几何方向工程师：涉及 OpenCASCADE、Coin3D、GPU/边缘渲染
- 桌面应用工程师：wxWidgets UI、Docking 框架集成与复杂交互
- 技术负责人：以 AI 加速研发流程与质量保障

## 全书结构

### 第 I 篇·工程基座与 AI 工作流
- 第1章 AI Coding 工作流在本项目中的角色与边界
  - 方法论：对话驱动、并行检索、可回溯的“编辑-构建-验证”
  - 仓库结构速览：`src/`、`include/`、`docs/`、`config/`
  - 基建要点：根 `CMakeLists.txt`、模块化静态库、`CMakePresets.json`
- 第2章 在大型 C++/CMake 工程中落地 AI 助手
  - 代码搜索策略：多模式并行 `Grep`、`Glob` 精准定位
  - 变更策略：小步编辑、最小可验证增量、可回滚
  - 团队规则内化：Windows 构建、英文注释、禁止自动生成测试/审计/说明
- 第3章 工程构建与运行基线
  - 主要 CMake 目标与依赖图（`CADCore`、`CADRendering`、`CADOCC`、`CADUI` 等）
  - 典型构建命令与验证流程
  - 常见编译故障快速修复模式

### 第 II 篇·核心子系统与模块脉络
- 第4章 几何内核与 OpenCASCADE 集成
  - `CADOCC` 模块与 `OCCGeometry*` 系列
  - 网格转换：`OCCMeshConverter`、质量与 LOD 管理
  - 大文件流式导入设计（STEP/IGES/STL/OBJ）
- 第5章 渲染架构与 Coin3D/So* 节点栈
  - `CADRendering` 与 `rendering_toolkit` 协作
  - 材质/外观/变换：`OCCGeometryAppearance/Transform/Material`
  - 多视口、相机与场景管理接口
- 第6章 边缘渲染体系与 GPU 加速
  - 抽取与渲染管线：`EdgeExtractor`、`EdgeRenderer`、`GPUEdgeRenderer`
  - LOD/重要性管理、轮廓/特征/网格边
  - 典型问题：法线/轮廓不一致与后处理
- 第7章 渲染预览与参数化设置
  - `RenderPreview` 设计与 `RenderingConfig` 应用
  - 预设体系与 `config/*.ini` 驱动
  - 实时性与细节的权衡
- 第8章 UI 与 Docking 框架
  - `CADUI`、`widgets`、`ui` 与 `docking` 模块
  - 现代 Dock 集成、布局管理、无边框窗口逻辑
  - `FlatUI` 框架与工具栏/面板体系
- 第9章 输入与导航、多视口与导航立方体
  - `CADInput`、输入路由模式
  - `NavigationCubeManager` 与 `NavCubeLib`
  - `IMultiViewportManager` 接口契约

### 第 III 篇·用 AI Coding 驱动的特性开发
- 第10章 选中对象渲染增强（增量迭代范式）
  - 需求拆分、接口勾连、数据流校准
  - 渲染参数注入与 UI 联动的最小改动集
  - 构建验证与回归风险控制
- 第11章 几何导入性能优化（面向大模型指导）
  - 流式导入管线改造点位清单
  - 并行/异步化策略与内存占用控制
  - 进度可视化与可取消交互
- 第12章 可配置渲染预设与实时切换
  - `rendering_presets.ini` 设计与默认值策略
  - 与 `RenderingConfig` 的双向绑定
  - 预设持久化与热加载
- 第13章 GPU 加速边缘渲染路径
  - 数据准备、触发条件与降级策略
  - LOD 分层与可视性规则
  - 性能基线对比与指标化

### 第 IV 篇·重构与稳定性工程
- 第14章 面向“编译必过”的可持续重构
  - 构造/成员初始化安全准则（保留全部初始化）
  - 头文件前置声明与依赖收敛
  - 模块边界与公共接口清理
- 第15章 Docking 与 FlatUI 复杂交互问题清单
  - 常见崩溃/卡顿/覆盖层异常
  - 事件路由与生命周期陷阱
  - 高 DPI 与样式一致性
- 第16章 性能剖析与热点定位
  - 边缘、网格、材质三大热点
  - 缓存策略：几何缓存、边缘缓存、图标与 UI 缓存
  - FPS/内存/耗时指标体系

### 第 V 篇·AI Coding 实战剧本
- 第17章 对话模板：需求 → 检索 → 编辑 → 构建 → 验证
  - 并行检索清单与关键正则
  - 变更前后差异对照与回滚策略
- 第18章 典型任务剧本
  - 新增命令增强：如 `CreateBoxListener`
  - UI 面板联动：属性树与渲染参数
  - 导入流程异常修复：法线、轮廓、贴图
- 第19章 分支协作与代码评审
  - 粗粒度变更分组
  - PR 描述与验证步骤模板
  - 评审关注点与走查顺序

## 附录
- A. 模块地图与关键文件速查
- B. CMake 目标与依赖矩阵
- C. 渲染与边缘渲染调参与默认值
- D. 常见编译错误与快捷修复对照表
- E. 性能优化 Checklist（导入/渲染/UI/交互）

## 章节内可落地的 AI 练习卡（示例）
- 几何导入流式化：将 STEP 导入拆分为分块解析 + 进度同步
- 边缘渲染 LOD 接入：在 `EdgeLODManager` 扩展新等级并联动 UI
- 渲染预设热切换：扩展 `RenderingConfig` 与 `*.ini` 的绑定
- Docking 交互修复：解决 `FloatingDockContainer` 生命周期边界问题
- 多视口导航：基于 `IMultiViewportManager` 新增辅助预览视口

## 工程与规则要点（全书统一遵循）
- 构建与验证：统一使用 `cmake --build build --config Release` 进行编译验证
- 代码注释：仅使用英文注释，避免中文与非英文标准字符
- 生成限制：不自动生成测试、说明、审计类脚本（除非明确需求）
- 重构安全：不删除成员变量初始化；对比重构前后完整初始化列表；保留指针初始化
