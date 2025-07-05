# 构建说明

## 项目概述

这是一个基于wxWidgets、Coin3D和OpenCASCADE的3D CAD应用程序，采用模块化CMake构建系统。

## 依赖项

### 必需依赖
- **wxWidgets** - 跨平台GUI框架
- **Coin3D** - 3D图形渲染引擎
- **OpenCASCADE** - CAD几何建模内核

### 可选依赖
- **vcpkg** - C++包管理器（推荐）

## 使用vcpkg构建（推荐）

### 1. 安装vcpkg

```bash
# 克隆vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Windows
.\bootstrap-vcpkg.bat

# Linux/macOS
./bootstrap-vcpkg.sh
```

### 2. 安装依赖包

```bash
# 安装基本依赖
vcpkg install wxwidgets:x64-windows
vcpkg install coin:x64-windows
vcpkg install opencascade:x64-windows

# 或者使用vcpkg.json自动安装
vcpkg install --triplet x64-windows
```

### 3. 配置环境变量

```bash
# Windows (PowerShell)
$env:VCPKG_ROOT = "D:\repos\vcpkg"
$env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"

# Linux/macOS
export VCPKG_ROOT="/path/to/vcpkg"
export CMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

### 4. 构建项目

#### 使用CMake预设（推荐）

```bash
# 配置Debug版本
cmake --preset debug

# 构建Debug版本
cmake --build --preset debug

# 配置Release版本
cmake --preset release

# 构建Release版本
cmake --build --preset release
```

#### 使用传统CMake命令

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DwxWidgets_ROOT_DIR="$VCPKG_ROOT/installed/x64-windows"

# 构建项目
cmake --build . --config Debug
```

#### 使用构建脚本

```bash
# Windows
.\build.bat          # Debug版本
.\build-release.bat  # Release版本

# Linux/macOS
./build.sh           # Debug版本
./build-release.sh   # Release版本
```

## 手动构建（不使用vcpkg）

### Windows

1. 安装Visual Studio 2022或更高版本
2. 手动安装wxWidgets、Coin3D和OpenCASCADE
3. 设置环境变量：
   ```
   wxWidgets_ROOT_DIR=C:\path\to\wxwidgets
   COIN3D_ROOT=C:\path\to\coin3d
   OpenCASCADE_DIR=C:\path\to\opencascade
   ```

### Linux

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libwxgtk3.0-gtk3-dev
sudo apt-get install libcoin80-dev
sudo apt-get install libopencascade-dev

# 构建
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### macOS

```bash
# 使用Homebrew
brew install cmake wxwidgets coin3d opencascade

# 构建
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## 模块化架构

项目采用模块化CMake架构，包含以下模块：

- **CADCore** - 核心功能模块
- **CADOCC** - OpenCASCADE几何建模模块
- **CADRendering** - 渲染引擎模块
- **CADInput** - 输入管理模块
- **NavCubeLib** - 导航控制模块
- **CADUI** - 用户界面模块

## OpenCASCADE集成

项目集成了OpenCASCADE作为CAD几何建模内核：

### 功能特性
- **精确几何建模** - 支持NURBS曲面和实体建模
- **布尔运算** - 并集、交集、差集操作
- **文件I/O** - 支持STEP、IGES、BREP格式
- **网格化** - 将CAD几何转换为三角网格
- **测量工具** - 距离、角度、体积、表面积计算

### 使用示例
```cpp
// 创建OpenCASCADE几何
auto box = std::make_shared<OCCBox>("MyBox", 10.0, 10.0, 10.0);
auto sphere = std::make_shared<OCCSphere>("MySphere", 5.0);

// 添加到查看器
occViewer->addGeometry(box);
occViewer->addGeometry(sphere);

// 布尔运算
TopoDS_Shape result = OCCShapeBuilder::booleanUnion(box->getShape(), sphere->getShape());

// 导出为STEP文件
OCCBrepConverter::saveToSTEP(result, "output.step");
```

## 故障排除

### 常见问题

1. **找不到wxWidgets**
   - 确保设置了正确的wxWidgets_ROOT_DIR
   - 检查wxWidgets是否正确安装

2. **找不到Coin3D**
   - 确保Coin3D库在系统路径中
   - 检查COIN3D_ROOT环境变量

3. **找不到OpenCASCADE**
   - 确保OpenCASCADE正确安装
   - 检查OpenCASCADE_DIR环境变量

4. **链接错误**
   - 确保所有依赖库的架构匹配（x64 vs x86）
   - 检查Debug/Release配置匹配

### 调试构建

```bash
# 启用详细输出
cmake --build . --config Debug --verbose

# 查看CMake配置
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
```

## 开发环境

### 推荐IDE
- **Visual Studio 2022** (Windows)
- **CLion** (跨平台)
- **Qt Creator** (跨平台)
- **VS Code** (跨平台)

### 代码格式化
项目使用clang-format进行代码格式化，配置文件：`.clang-format`

### 单元测试
使用Google Test框架进行单元测试：

```bash
# 构建测试
cmake --build . --target tests

# 运行测试
ctest --verbose
```

## 贡献指南

1. Fork项目
2. 创建功能分支
3. 提交更改
4. 创建Pull Request

## 许可证

本项目采用MIT许可证，详见LICENSE文件。 