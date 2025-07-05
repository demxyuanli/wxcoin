# CMake 模块化结构说明

## 项目结构

```
FreeCADNavigation/
├── CMakeLists.txt                 # 主CMake文件
├── CMakePresets.json             # CMake预设配置
├── CMakeSettings.json            # Visual Studio配置
├── include/                      # 所有头文件
│   ├── Application.h
│   ├── Canvas.h
│   └── ...
└── src/                         # 源代码目录
    ├── CMakeLists.txt           # 源代码总管理文件
    ├── main.cpp                 # 应用程序入口点
    ├── core/                    # 核心模块
    │   ├── CMakeLists.txt
    │   ├── Application.cpp
    │   ├── Logger.cpp
    │   ├── Command.cpp
    │   ├── CreateCommand.cpp
    │   ├── GeometryObject.cpp
    │   ├── GeometryFactory.cpp
    │   ├── DPIManager.cpp
    │   └── DPIAwareRendering.cpp
    ├── rendering/               # 渲染模块
    │   ├── CMakeLists.txt
    │   ├── RenderingEngine.cpp
    │   ├── ViewportManager.cpp
    │   ├── SceneManager.cpp
    │   ├── CoordinateSystemRenderer.cpp
    │   └── PickingAidManager.cpp
    ├── input/                   # 输入模块
    │   ├── CMakeLists.txt
    │   ├── InputManager.cpp
    │   ├── EventCoordinator.cpp
    │   ├── MouseHandler.cpp
    │   ├── DefaultInputState.cpp
    │   └── PickingInputState.cpp
    ├── navigation/              # 导航模块
    │   ├── CMakeLists.txt
    │   ├── NavigationController.cpp
    │   ├── NavigationStyle.cpp
    │   ├── NavigationCube.cpp
    │   ├── NavigationCubeManager.cpp
    │   ├── CuteNavCube.cpp
    │   └── NavigationCubeConfigDialog.cpp
    └── ui/                      # UI模块
        ├── CMakeLists.txt
        ├── MainFrame.cpp
        ├── Canvas.cpp
        ├── ObjectTreePanel.cpp
        ├── PropertyPanel.cpp
        └── PositionDialog.cpp
```

## 模块依赖关系

```
FreeCADNavigation (可执行文件)
└── FreeCADUI (UI模块)
    ├── FreeCADCore (核心模块)
    ├── FreeCADRendering (渲染模块)
    │   └── FreeCADCore
    ├── FreeCADInput (输入模块)
    │   └── FreeCADCore
    └── FreeCADNavigationLib (导航模块)
        ├── FreeCADCore
        └── FreeCADRendering
```

## 模块说明

### 1. FreeCADCore (核心模块)
- **职责**: 基础功能、日志系统、命令系统、几何对象、DPI管理
- **依赖**: 无内部依赖
- **输出**: 静态库 `FreeCADCore`

### 2. FreeCADRendering (渲染模块)
- **职责**: OpenGL渲染、场景管理、视口管理、坐标系渲染、拾取辅助
- **依赖**: FreeCADCore
- **输出**: 静态库 `FreeCADRendering`

### 3. FreeCADInput (输入模块)
- **职责**: 输入管理、事件协调、鼠标处理、输入状态管理
- **依赖**: FreeCADCore
- **输出**: 静态库 `FreeCADInput`

### 4. FreeCADNavigationLib (导航模块)
- **职责**: 导航控制、导航样式、导航立方体管理
- **依赖**: FreeCADCore, FreeCADRendering
- **输出**: 静态库 `FreeCADNavigationLib`

### 5. FreeCADUI (UI模块)
- **职责**: 主窗口、画布、面板、对话框等用户界面组件
- **依赖**: 所有其他模块
- **输出**: 静态库 `FreeCADUI`

## 构建过程

1. **配置阶段**: CMake处理所有CMakeLists.txt文件
2. **模块构建顺序**:
   - FreeCADCore (无依赖，首先构建)
   - FreeCADRendering (依赖Core)
   - FreeCADInput (依赖Core)
   - FreeCADNavigationLib (依赖Core和Rendering)
   - FreeCADUI (依赖所有模块)
3. **最终链接**: 主可执行文件链接FreeCADUI模块

## 使用方法

### 文件重组织
运行 `move_files.bat` 脚本来重新组织现有源文件到模块目录中。

### 构建项目
```bash
# 使用预设构建
cmake --preset windows-vcpkg-x64
cmake --build --preset windows-vcpkg-x64-debug

# 或使用构建脚本
build.bat
```

## 优势

1. **模块化**: 清晰的模块边界和职责分离
2. **可维护性**: 每个模块可以独立开发和测试
3. **重用性**: 模块可以在其他项目中重用
4. **并行构建**: CMake可以并行构建独立的模块
5. **IDE友好**: 在Visual Studio中以文件夹形式组织项目
6. **依赖管理**: 明确的模块间依赖关系

## 扩展

要添加新模块：
1. 在 `src/` 下创建新目录
2. 创建模块的 `CMakeLists.txt`
3. 在 `src/CMakeLists.txt` 中添加 `add_subdirectory(新模块)`
4. 在需要的地方添加模块依赖 