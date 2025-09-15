目标：

这个程序来自于一些AI编程的实验性编码，里面的各种模块，包括GUI框架，几何可视化，几何的一些处理功能最早开始于2025年4月，那时候的主力Model还是Grok2，ChatGPT3，DeepSeek R1，claude3.5等等。
本程序选用了比较小众的GUI框架wxdgets3.2.7，开始时想用QT + Py来着，但在3D渲染引擎这一步就给劝退了，主要因为无法正确的在pyQT中初始化和访问多线程渲染引擎，后来决定从原生基础直接用C++开发，3D引擎选用了Coin3D4.0.3，因为vcpkg的库的兼容性问题，构建工程所需的包只能降低一些版本，几何引擎用来OCCT7.9，本来想使用GCAL开源算法库，不过这个库太基础了，展现效果过于繁琐，无法快速的展现AI的能力就放弃了。
经过几个月的AI编程，Model到了现在的GPT-5，Claude 4.1 Grok4，程序也基本成型，虽然里面有很多bug和细节功能还未调整修复，不过整体样貌基本能够呈现。

懒得写Readme，就让AI写一个吧

先放效果

GUI布局，支持Fluen Ribbon风格

<img width="1195" height="797" alt="image" src="https://github.com/user-attachments/assets/ec5384b3-ccff-4242-b480-2bc7734ff49d" />

导入几何，可对几何按照Shape->Solid->shell->face分级分解和配色

<img width="1198" height="799" alt="image" src="https://github.com/user-attachments/assets/16d7b5c2-1a5a-4abd-8a54-02f6d4143ba8" />

导入的几何按照Faces进行分解，显示了Feature 边
<img width="1199" height="799" alt="image" src="https://github.com/user-attachments/assets/c361d534-13b3-4934-9e75-200efa15a788" />

对导入几何进行了网格加密，以显示更光滑的渲染效果，同时显示原边Origin Edge展示边和面的贴合程度
<img width="1196" height="797" alt="image" src="https://github.com/user-attachments/assets/c7172ea4-8d53-4ccf-be9e-08ad4e6572f0" />

界面支持Theme管理，可以选择dark，light配色，有些控件尚未自定义完全，所以显示的控件未按theme变化
<img width="1197" height="803" alt="image" src="https://github.com/user-attachments/assets/38158b39-1fe9-474f-bfe6-3743e9df24d0" />

支持docking停靠系统，可以拖拽实现各种停靠和标签合并
<img width="1197" height="795" alt="image" src="https://github.com/user-attachments/assets/a564d951-41b4-4c4e-b254-9f1121a83b89" />


# CAD Navigator (wxcoin)

基于 wxWidgets、Coin3D 和 OpenCASCADE 技术构建的现代 3D CAD 应用程序。该应用程序提供了全面的 CAD 导航和可视化系统，具有先进的停靠界面和渲染功能。

## 功能特性

### 核心功能
- **3D CAD 可视化**: 使用 Coin3D 和 OpenCASCADE 的高性能 3D 渲染
- **高级停靠系统**: 现代化的可停靠界面，支持自定义布局
- **多格式支持**: 支持 STEP、IGES、STL、OBJ 等 CAD 格式的导入/导出
- **交互式导航**: 基于鼠标的 3D 导航，支持缩放、平移和旋转控制
- **几何体创建**: 内置工具用于创建基本 3D 形状（立方体、球体、圆柱体等）

### 用户界面
- **现代扁平化 UI**: 简洁现代的界面，支持主题（默认、暗色、蓝色）
- **可停靠面板**: 属性面板、对象树、画布、消息输出和性能监控
- **可自定义布局**: 保存、加载和重置停靠布局
- **响应式设计**: 自适应 UI，可根据屏幕尺寸缩放
- **SVG 图标系统**: 可缩放的矢量图标，支持主题感知着色

### 渲染系统
- **高性能渲染**: 优化的渲染管道，带有剔除系统
- **多种渲染模式**: 线框、实体、纹理和透明模式
- **光照控制**: 可配置的光照设置和全局照明
- **网格质量选项**: 可调节的网格分辨率和质量设置
- **性能监控**: 实时性能指标和优化

### 文件操作
- **CAD 格式支持**: STEP、IGES、STL、OBJ、XT 格式
- **导入/导出**: 全面的文件 I/O 操作
- **项目管理**: 保存和加载项目文件
- **最近文件**: 快速访问最近打开的文件

## 技术栈

- **GUI 框架**: wxWidgets 3.x
- **3D 渲染**: Coin3D (Open Inventor)
- **CAD 内核**: OpenCASCADE
- **构建系统**: CMake
- **包管理**: vcpkg
- **编程语言**: C++17

## 系统要求

### Windows 要求
- Visual Studio 2019 或更高版本（支持 C++17）
- CMake 3.20 或更高版本
- vcpkg 包管理器

### 依赖项
以下包由 vcpkg 自动管理：
- `wxwidgets` - GUI 框架
- `coin` - 3D 渲染库
- `opencascade` - CAD 内核

## 构建项目

### 快速构建（Windows）
```batch
# Debug 构建
build.bat

# Release 构建  
build-release.bat
```

### 手动构建过程

1. **配置项目**:
   ```batch
   cmake --preset windows-vcpkg-x64
   ```

2. **构建项目**:
   ```batch
   cmake --build build --config Release
   ```

3. **运行应用程序**:
   ```batch
   build\Release\CADNav.exe
   ```

### 构建选项
- **Debug**: `cmake --build build --config Debug`
- **Release**: `cmake --build build --config Release`

## 项目结构

```
wxcoin/
├── src/                    # 源代码
│   ├── commands/          # 命令系统
│   ├── config/            # 配置管理
│   ├── core/              # 核心服务
│   ├── docking/           # 停靠系统
│   ├── flatui/            # 扁平化 UI 组件
│   ├── geometry/          # 几何处理
│   ├── navigation/        # 3D 导航
│   ├── opencascade/       # OpenCASCADE 集成
│   ├── rendering/         # 渲染系统
│   ├── ui/                # 用户界面
│   └── widgets/           # 自定义控件
├── include/               # 头文件
├── config/                # 配置文件
│   ├── config.ini         # 主配置文件
│   ├── icons/             # SVG 图标
│   └── *.ini              # 主题和设置
├── resources/             # Windows 资源
└── build/                 # 构建输出
```

## 配置

### 主配置文件 (`config/config.ini`)
应用程序使用全面的配置系统：

- **主题设置**: 在默认、暗色和蓝色主题之间切换
- **UI 颜色**: 完整的配色方案自定义
- **字体设置**: 字体配置
- **布局设置**: 停靠面板尺寸和间距
- **渲染设置**: 图形和性能选项

### 主题系统
应用程序支持三种内置主题：
- **默认**: 带有蓝色强调色的浅色主题
- **暗色**: 现代风格的暗色主题
- **蓝色**: 浅蓝色主题变体

## 使用指南

### 基本操作
1. **文件操作**: 使用文件菜单打开、保存和创建新项目
2. **3D 导航**: 
   - 左键鼠标: 旋转视图
   - 右键鼠标: 平移视图
   - 鼠标滚轮: 缩放
3. **对象选择**: 在 3D 视图或对象树中点击对象
4. **属性编辑**: 使用属性面板修改对象属性

### 停靠系统
- **拖拽**: 拖拽面板标题重新排列布局
- **自动隐藏**: 右键点击面板标题启用自动隐藏
- **保存布局**: 使用视图菜单保存/加载自定义布局
- **重置布局**: 恢复默认面板排列

### 渲染选项
- **视图模式**: 在线框、实体和纹理视图之间切换
- **光照**: 调整全局光照和材质属性
- **质量**: 修改网格分辨率和渲染质量
- **性能**: 在性能面板中监控渲染性能

## 开发

### 代码组织
- **模块化设计**: UI、渲染和几何处理之间的清晰分离
- **命令模式**: 用户操作的可扩展命令系统
- **服务定位器**: 核心服务的依赖注入
- **插件架构**: 可扩展的渲染和处理插件

### 关键组件
- **FlatFrameDocking**: 带有停靠系统的主应用程序窗口
- **RenderingToolkitAPI**: 高级渲染接口
- **ConfigManager**: 集中式配置管理
- **CommandDispatcher**: 命令执行和撤销/重做系统

### 添加新功能
1. **命令**: 在 `src/commands/` 中添加新命令
2. **UI 组件**: 在 `src/widgets/` 中创建自定义控件
3. **渲染**: 在 `src/rendering/` 中扩展渲染系统
4. **几何**: 在 `src/geometry/` 中添加新的几何处理器

## 故障排除

### 常见问题
1. **构建失败**: 确保通过 vcpkg 安装了所有依赖项
2. **渲染问题**: 检查显卡驱动程序和 OpenGL 支持
3. **文件导入错误**: 验证文件格式兼容性
4. **性能问题**: 调整渲染质量设置

### 调试模式
在调试模式下运行以获得详细日志：
```batch
cmake --build build --config Debug
build\Debug\CADNav.exe
```

## 许可证

此项目是 FreeCAD 生态系统的一部分，遵循相同的许可条款。

## 贡献

1. Fork 仓库
2. 创建功能分支
3. 进行更改
4. 充分测试
5. 提交拉取请求

## 支持

如有问题和疑问：
- 检查 `config/` 中的配置文件
- 查看 `build/` 中的构建日志
- 在 `config/config.ini` 中启用调试日志

---

**注意**: 此应用程序需要支持 OpenGL 的现代显卡以获得最佳性能。
