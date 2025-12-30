# Coin3D 升级指南

## 当前状态

项目使用 vcpkg 管理 Coin3D 依赖，当前版本为 4.06。

## 升级步骤

### 方法 1: 使用 vcpkg 升级（推荐）

#### 1. 检查当前版本

```powershell
# 检查本地安装的版本
vcpkg list coin

# 或者检查 vcpkg 仓库中的可用版本
vcpkg search coin
```

#### 2. 移除旧版本并安装新版本

```powershell
# 移除旧版本
vcpkg remove coin:x64-windows

# 更新 vcpkg 仓库（获取最新版本信息）
vcpkg update

# 安装最新版本的 Coin3D
vcpkg install coin:x64-windows

# 或者安装特定版本（如果 vcpkg 支持）
# vcpkg install coin:x64-windows --version=4.1.0
```

#### 3. 如果使用本地 vcpkg_installed 目录

如果项目使用本地 `vcpkg_installed` 目录（通过 CMake 自动安装），需要：

```powershell
# 删除本地安装的旧版本
Remove-Item -Recurse -Force vcpkg_installed\x64-windows\coin -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force vcpkg_installed\x64-windows\share\coin -ErrorAction SilentlyContinue

# 重新配置 CMake（会自动安装新版本）
cmake --preset windows-vcpkg-x64
```

### 方法 2: 手动指定版本（如果 vcpkg 支持）

某些版本的 vcpkg 支持通过 portfile 指定版本：

```powershell
# 检查 vcpkg 仓库中的 coin port
cd $env:VCPKG_ROOT
git log --oneline ports/coin/portfile.cmake | Select-Object -First 10

# 查看可用的版本标签
git tag | Select-String coin
```

### 方法 3: 使用 vcpkg.json manifest（可选）

如果希望更好地管理版本，可以创建 `vcpkg.json`：

```json
{
  "dependencies": [
    {
      "name": "coin",
      "version>=": "4.1.0"
    },
    {
      "name": "wxwidgets"
    },
    {
      "name": "opencascade"
    }
  ]
}
```

然后在 `CMakeLists.txt` 中启用 manifest 模式：

```cmake
set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Enable vcpkg manifest mode" FORCE)
```

### 4. 重新编译项目

升级后需要重新编译：

```powershell
# 清理旧的构建文件（可选，但推荐）
Remove-Item -Recurse -Force build\* -ErrorAction SilentlyContinue

# 重新配置
cmake --preset windows-vcpkg-x64

# 重新编译
cmake --build build --config Release --parallel
```

## 验证升级

### 检查版本

```cpp
// 在代码中检查版本
#include <Inventor/SoDB.h>
#include <Inventor/SoVersion.h>

// 打印 Coin3D 版本信息
std::cout << "Coin3D Version: " << SoVersion::getVersion() << std::endl;
std::cout << "Coin3D Version String: " << SoVersion::getVersionString() << std::endl;
```

### 检查兼容性

升级后需要检查：

1. **API 兼容性**: 检查是否有 API 变更
   - 查看 Coin3D 的 CHANGELOG
   - 检查编译错误和警告

2. **行为变更**: 测试渲染功能
   - 3D 视图渲染
   - 显示模式切换
   - 交互操作

3. **性能**: 对比升级前后的性能

## 常见问题

### 问题 1: vcpkg 找不到新版本

**解决方案**:
```powershell
# 更新 vcpkg 仓库
cd $env:VCPKG_ROOT
git pull

# 或者重新克隆 vcpkg
```

### 问题 2: 编译错误

**可能原因**:
- API 变更
- 头文件路径变化
- 链接库名称变化

**解决方案**:
- 查看 Coin3D 的迁移指南
- 检查 CMake 配置中的 `Coin3D_INCLUDE_DIRS` 和链接库名称

### 问题 3: 运行时错误

**可能原因**:
- DLL 版本不匹配
- 缺少依赖库

**解决方案**:
```powershell
# 检查 DLL 依赖
dumpbin /dependents build\Release\CADVisBird.exe | findstr coin

# 确保使用正确的 DLL 版本
```

## 回退方案

如果升级后出现问题，可以回退到旧版本：

```powershell
# 移除新版本
vcpkg remove coin:x64-windows

# 安装特定旧版本（需要找到对应的 commit）
cd $env:VCPKG_ROOT
git checkout <commit-hash> -- ports/coin
vcpkg install coin:x64-windows
git checkout master -- ports/coin
```

## 参考资源

- [Coin3D 官方文档](https://github.com/coin3d/coin)
- [vcpkg 文档](https://vcpkg.io/)
- [Coin3D 版本历史](https://github.com/coin3d/coin/releases)

