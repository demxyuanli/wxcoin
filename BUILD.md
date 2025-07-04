# FreeCADNavigation 构建说明

## 环境要求

- Visual Studio 2022 (带有 C++ 工具)
- CMake 3.20 或更高版本
- vcpkg 包管理器

## 依赖配置

### 1. 安装 vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git D:\repos\vcpkg
cd D:\repos\vcpkg
.\bootstrap-vcpkg.bat
```

### 2. 安装项目依赖

```bash
.\vcpkg install wxwidgets:x64-windows
.\vcpkg install coin:x64-windows
```

## 构建方式

### 方式一：使用构建脚本（推荐）

#### Debug 版本
```bash
build.bat
```

#### Release 版本
```bash
build-release.bat
```

### 方式二：使用 CMake 预设

#### 配置项目
```bash
# Debug 配置
cmake --preset windows-vcpkg-x64

# Release 配置
cmake --preset windows-vcpkg-x64-release
```

#### 构建项目
```bash
# Debug 构建
cmake --build --preset windows-vcpkg-x64-debug

# Release 构建
cmake --build --preset windows-vcpkg-x64-release-build
```

### 方式三：使用 Visual Studio

1. 打开 Visual Studio 2022
2. 选择 "打开文件夹" 并选择项目根目录
3. Visual Studio 会自动检测 `CMakeSettings.json` 和 `CMakePresets.json`
4. 在配置下拉菜单中选择 `x64-Debug-vcpkg` 或 `x64-Release-vcpkg`
5. 构建项目

### 方式四：手动 CMake 命令

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE="D:/repos/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  -DwxWidgets_ROOT_DIR="D:/repos/vcpkg/installed/x64-windows"

# 构建项目
cmake --build . --config Debug
```

## 配置文件说明

### CMakePresets.json
- 现代 CMake 预设文件
- 包含 Debug 和 Release 配置
- 支持跨平台构建

### CMakeSettings.json
- Visual Studio 专用配置文件
- 提供 IDE 集成支持
- 包含调试和发布配置

### CMakeLists.txt
- 移除了硬编码路径
- 支持环境变量配置
- 更好的跨平台兼容性

## 环境变量配置

可以设置以下环境变量来自定义构建：

```bash
set VCPKG_ROOT=D:\repos\vcpkg
set wxWidgets_ROOT_DIR=D:\repos\vcpkg\installed\x64-windows
```

## 故障排除

### 常见问题

1. **找不到 wxWidgets**
   - 确保 vcpkg 已正确安装 wxwidgets
   - 检查 `wxWidgets_ROOT_DIR` 路径是否正确

2. **找不到 Coin3D**
   - 确保 vcpkg 已正确安装 coin
   - 检查 CMake 是否能找到 Coin::Coin 目标

3. **构建失败**
   - 检查 Visual Studio 2022 是否正确安装
   - 确保 CMake 版本满足要求
   - 查看构建日志获取详细错误信息

### 调试构建

如果遇到问题，可以使用详细输出：

```bash
cmake --preset windows-vcpkg-x64 --verbose
cmake --build --preset windows-vcpkg-x64-debug --verbose
``` 