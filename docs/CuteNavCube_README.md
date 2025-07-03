# CuteNavCube - 可爱的导航立方体

## 概述

CuteNavCube 是一个基于 FreeCAD 导航立方体（Navigation Cube）功能迁移而来的可爱版本导航立方体。它显示在 Canvas 的左下角，提供直观的 3D 视图导航功能。

## 功能特性

### 1. 基本功能
- **3D 立方体显示**：在左下角显示一个可交互的 3D 立方体
- **面点击导航**：点击立方体的不同面可以切换到对应的视图（前、后、左、右、上、下）
- **拖拽旋转**：可以拖拽立方体来旋转视角
- **摄像机同步**：与主场景摄像机保持同步

### 2. 视觉设计
- **可爱配色**：使用浅蓝色材质和蓝色文字，比原版更加可爱
- **较小尺寸**：默认尺寸为 150x150 像素，比原版导航立方体更小巧
- **高DPI支持**：支持高分辨率显示器，自动缩放

### 3. 交互功能
- **鼠标左键点击**：点击立方体面切换视图
- **鼠标左键拖拽**：拖拽旋转立方体视角
- **自动布局**：自动定位在左下角，支持窗口大小变化

## 技术实现

### 文件结构
```
include/
├── CuteNavCube.h          # CuteNavCube 类头文件
└── Canvas.h               # 修改后的 Canvas 头文件

src/
├── CuteNavCube.cpp        # CuteNavCube 类实现
└── Canvas.cpp             # 修改后的 Canvas 实现
```

### 核心类

#### CuteNavCube 类
```cpp
class CuteNavCube {
public:
    // 构造函数
    CuteNavCube(std::function<void(const std::string&)> viewChangeCallback, 
                float dpiScale, int windowWidth, int windowHeight);
    
    // 基本方法
    void initialize();
    void render(int x, int y, const wxSize& size);
    void handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize);
    
    // 状态控制
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // 位置和大小
    void setPosition(int x, int y);
    void setSize(int size);
    
    // 摄像机控制
    void setCameraPosition(const SbVec3f& position);
    void setCameraOrientation(const SbRotation& orientation);
};
```

#### Canvas 集成
```cpp
class Canvas {
public:
    // CuteNavCube 控制方法
    void setCuteNavCubeEnabled(bool enabled);
    bool isCuteNavCubeEnabled() const;
    void SetCuteNavCubeRect(int x, int y, int size);
    void SyncCuteNavCubeCamera();
};
```

### 渲染技术
- **Open Inventor**：使用 Coin3D 库进行 3D 渲染
- **OpenGL**：底层图形渲染
- **wxWidgets**：GUI 框架集成
- **纹理生成**：动态生成立方体面纹理

## 使用方法

### 1. 基本使用
CuteNavCube 默认启用，会在 Canvas 左下角自动显示。

### 2. 启用/禁用
```cpp
// 启用 CuteNavCube
canvas->setCuteNavCubeEnabled(true);

// 禁用 CuteNavCube
canvas->setCuteNavCubeEnabled(false);

// 检查是否启用
bool enabled = canvas->isCuteNavCubeEnabled();
```

### 3. 位置和大小设置
```cpp
// 设置位置和大小
canvas->SetCuteNavCubeRect(x, y, size);
```

### 4. 视图切换
点击立方体的不同面可以切换到对应的视图：
- **F**：前视图 (Front)
- **B**：后视图 (Back)
- **L**：左视图 (Left)
- **R**：右视图 (Right)
- **T**：上视图 (Top)
- **D**：下视图 (Bottom)

## 与原有 NavigationCube 的区别

| 特性 | NavigationCube | CuteNavCube |
|------|----------------|-------------|
| 位置 | 右上角 | 左下角 |
| 尺寸 | 200x200 | 150x150 |
| 颜色 | 白色背景，红色文字 | 浅蓝色材质，蓝色文字 |
| 字体大小 | 16pt | 14pt |
| 材质 | 标准材质 | 可爱风格材质 |

## 编译和运行

### 编译
```bash
cd /d/source/repos/wxcoin
cmake --build build --config Release
```

### 运行
```bash
./build/Release/wxcoin.exe
```

### 测试程序
```bash
# 编译测试程序
g++ -o test_cute_navcube test_cute_navcube.cpp -lwx_baseu-3.1 -lwx_gl-3.1 -lInventor -lInventorXt

# 运行测试程序
./test_cute_navcube
```

## 技术细节

### 纹理生成
- 使用 wxWidgets 的 wxMemoryDC 生成纹理
- 支持 DPI 缩放
- 自动缓存纹理以提高性能

### 鼠标事件处理
- 坐标转换：将 Canvas 坐标转换为立方体局部坐标
- 拾取检测：使用 Open Inventor 的 SoRayPickAction 进行精确拾取
- 拖拽处理：支持鼠标拖拽旋转

### 摄像机同步
- 主摄像机位置同步到 CuteNavCube
- 支持视角切换时的平滑过渡
- 保持距离和方向的一致性

## 扩展功能

### 可能的改进
1. **动画效果**：添加旋转和切换动画
2. **自定义皮肤**：支持用户自定义外观
3. **更多交互**：支持右键菜单、滚轮缩放等
4. **配置界面**：添加设置对话框
5. **主题系统**：支持多种主题风格

### 性能优化
1. **纹理缓存**：优化纹理生成和缓存机制
2. **渲染优化**：减少不必要的重绘
3. **内存管理**：优化内存使用

## 故障排除

### 常见问题
1. **不显示**：检查是否启用，确认位置设置
2. **交互无响应**：检查鼠标事件处理
3. **渲染异常**：检查 OpenGL 上下文
4. **DPI 问题**：确认 DPI 缩放设置

### 调试信息
程序会输出详细的日志信息，包括：
- 初始化状态
- 渲染信息
- 鼠标事件
- 错误信息

## 许可证

本项目基于原有项目的许可证，遵循相同的开源协议。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进 CuteNavCube 的功能。 