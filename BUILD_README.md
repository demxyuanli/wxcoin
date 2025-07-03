# wxCoin 3D Application - 模块化构建指南

## 项目概述

本项目采用模块化架构重构，将原本的单体结构分解为7个独立的模块：

- **Core** - 核心功能模块
- **Rendering** - 渲染模块  
- **Geometry** - 几何模块
- **Input** - 输入处理模块
- **Navigation** - 导航模块
- **Commands** - 命令系统模块
- **UI** - 用户界面模块

## 项目结构

```
wxcoin/
├── cmake/                     # CMake工具函数
│   └── ModuleUtils.cmake      # 模块构建工具
├── modules/                   # 模块目录
│   ├── core/                  # 核心模块
│   │   ├── include/           # 头文件
│   │   ├── src/               # 源文件
│   │   └── CMakeLists.txt     # 模块构建脚本
│   ├── rendering/             # 渲染模块
│   ├── geometry/              # 几何模块
│   ├── input/                 # 输入模块
│   ├── navigation/            # 导航模块
│   ├── commands/              # 命令模块
│   └── ui/                    # UI模块
├── main.cpp                   # 应用程序入口
├── CMakeLists.txt             # 主构建脚本
└── BUILD_README.md            # 本文件
```

## 模块依赖关系

```
Core (基础模块)
├── Rendering (依赖 Core)
├── Geometry (依赖 Core, Rendering)
├── Input (依赖 Core, Rendering)
├── Navigation (依赖 Core, Rendering, Input)
├── Commands (依赖 Core, Geometry)
└── UI (依赖所有模块)
```

## 构建要求

### 必需依赖
- CMake 3.20+
- Visual Studio 2022 (MSVC)
- wxWidgets (通过vcpkg安装)
- Coin3D (通过vcpkg安装)
- C++17标准

### Vcpkg依赖安装
```bash
# 安装wxWidgets
vcpkg install wxwidgets:x64-windows

# 安装Coin3D
vcpkg install coin:x64-windows
```

## 构建步骤

1. **配置构建环境**
   ```bash
   mkdir build
   cd build
   ```

2. **运行CMake配置**
   ```bash
   cmake .. -G "Visual Studio 17 2022" -A x64 \
        -DCMAKE_TOOLCHAIN_FILE="D:/repos/vcpkg/scripts/buildsystems/vcpkg.cmake" \
        -DwxWidgets_ROOT_DIR="D:/repos/vcpkg/installed/x64-windows"
   ```

3. **构建项目**
   ```bash
   cmake --build . --config Release
   ```

## 构建选项

- `ENABLE_HIGH_DPI`: 启用高DPI支持 (默认: ON)
- `ENABLE_DEBUG_LOGS`: 启用调试日志 (默认: ON)

## 模块说明

### Core模块
包含应用程序基础架构：
- `Application` - 应用程序主类
- `Logger` - 日志系统
- `Globals` - 全局配置

### Rendering模块  
包含3D渲染功能：
- `Canvas` - OpenGL渲染画布
- `SceneManager` - 场景管理器
- `CoordinateSystemRenderer` - 坐标系渲染器
- `DPIManager` - DPI管理器
- `PickingAidManager` - 拾取辅助管理器

### Geometry模块
包含几何对象管理：
- `GeometryFactory` - 几何对象工厂
- `GeometryObject` - 几何对象基类

### Input模块
包含输入设备处理：
- `InputManager` - 输入管理器
- `MouseHandler` - 鼠标事件处理器

### Navigation模块
包含3D导航功能：
- `NavigationController` - 导航控制器
- `NavigationCube` - 导航立方体
- `NavigationStyle` - 导航样式
- `NavigationCubeConfigDialog` - 配置对话框

### Commands模块
包含命令模式实现：
- `Command` - 命令基类
- `CreateCommand` - 创建命令

### UI模块
包含用户界面：
- `MainFrame` - 主窗口
- `ObjectTreePanel` - 对象树面板
- `PropertyPanel` - 属性面板
- `PositionDialog` - 位置对话框

## 故障排除

### 常见问题

1. **Vcpkg路径错误**
   - 确保CMake命令中的vcpkg路径正确
   - 检查环境变量VCPKG_ROOT是否设置

2. **依赖未找到**
   - 重新安装vcpkg包
   - 确认包架构匹配(x64-windows)

3. **编译错误**
   - 检查C++17支持
   - 确认所有头文件路径正确

### 构建验证

构建成功后，CMake会显示：
- 模块依赖验证结果
- 构建配置摘要
- 依赖关系图

## 开发指南

### 添加新模块
1. 在`modules/`下创建新目录
2. 添加`include/`和`src/`子目录
3. 创建`CMakeLists.txt`
4. 更新主`CMakeLists.txt`中的依赖关系
5. 在`ModuleUtils.cmake`中添加验证

### 修改现有模块
- 只需修改对应模块目录中的文件
- CMake会自动处理依赖关系
- 使用`cmake --build . --target <module_name>`单独构建模块 