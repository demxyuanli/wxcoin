# Dock Layout Configuration Integration

## 概述

已成功通过 CommandListener 系统集成了 Dock Layout Configuration 功能到 FlatFrameDocking 中。

## 实现内容

### 1. 创建了 DockLayoutConfigListener
- **文件**: `src/commands/DockLayoutConfigListener.cpp` 和 `.h`
- **功能**: 处理 `DOCK_LAYOUT_CONFIG` 命令，打开配置对话框
- **特点**: 
  - 获取当前布局配置
  - 显示配置对话框
  - 应用更改到 DockManager 和 DockContainerWidget

### 2. 添加了命令类型
- **文件**: `include/CommandType.h`
- **新增**: `CommandType::DockLayoutConfig`
- **映射**: `"DOCK_LAYOUT_CONFIG"`

### 3. 在 UI 中添加了按钮
- **位置**: Tools 页面的工具栏
- **文件**: `src/ui/FlatFrameInit.cpp`
- **按钮**: "Dock Layout" 带有布局图标
- **提示**: "Configure dock panel sizes and layout"

### 4. 注册了事件处理
- **文件**: `src/ui/FlatFrameEvents.cpp`
- **映射**: `ID_DOCK_LAYOUT_CONFIG` → `CommandType::DockLayoutConfig`

### 5. 在 FlatFrameDocking 中注册 Listener
- **文件**: `src/ui/FlatFrameDocking.cpp`
- **方法**: `RegisterDockLayoutConfigListener()`
- **时机**: 在构造函数中调用

## 使用方法

1. 运行 FlatFrameDocking 应用程序
2. 点击 ribbon 界面中的 "Tools" 标签页
3. 在工具栏中找到 "Dock Layout" 按钮（在 "Edge Settings" 旁边）
4. 点击按钮打开配置对话框
5. 在对话框中：
   - 选择使用像素或百分比模式
   - 调整各个区域的大小
   - 使用预设按钮快速设置布局
   - 预览面板实时显示布局效果
6. 点击 OK 或 Apply 应用更改

## 特性

- **百分比模式**: 默认启用，左侧占 20%，中心占 80%
- **预设布局**: 
  - 20/80 布局
  - 3列布局（20%-60%-20%）
  - IDE 布局（完整的开发环境布局）
- **实时预览**: 配置时可以看到布局效果
- **即时应用**: 更改立即生效，无需重启

## 文件更改列表

1. `src/commands/DockLayoutConfigListener.cpp` - 新增
2. `src/commands/DockLayoutConfigListener.h` - 新增
3. `src/commands/CMakeLists.txt` - 添加新文件
4. `include/CommandType.h` - 添加新命令类型
5. `include/FlatFrame.h` - 添加 ID_DOCK_LAYOUT_CONFIG
6. `src/ui/FlatFrameInit.cpp` - 添加按钮
7. `src/ui/FlatFrameEvents.cpp` - 添加命令映射
8. `src/ui/FlatFrameDocking.cpp` - 注册 listener
9. `include/FlatFrameDocking.h` - 添加方法声明

## 架构优势

使用 CommandListener 模式的好处：
- 解耦了 UI 和功能实现
- 遵循了现有的框架模式
- 易于维护和扩展
- 统一的命令处理机制