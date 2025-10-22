# CMake 增量编译优化指南

## 问题分析

当前编译系统存在以下问题导致增量编译效率低：

1. **过多的PUBLIC依赖** - 导致级联重新编译
2. **缺少编译缓存** - 没有使用ccache/sccache
3. **依赖传播过度** - 修改底层头文件会触发全局重新编译
4. **预编译头配置不当** - 某些模块可能使用了预编译头但配置不合理

## 优化方案

### 1. 安装编译缓存工具（推荐）

#### Windows用户 - 安装sccache

```powershell
# 使用scoop安装
scoop install sccache

# 或使用cargo安装
cargo install sccache

# 验证安装
sccache --version
```

#### Linux用户 - 安装ccache

```bash
# Ubuntu/Debian
sudo apt-get install ccache

# CentOS/RHEL
sudo yum install ccache

# 验证安装
ccache --version
```

**效果：第二次及以后的编译速度提升50-90%**

### 2. 使用优化的CMake配置

在项目根目录的`CMakeLists.txt`中添加：

```cmake
# 在project()之后添加
include(cmake/CompileOptimizations.cmake)
include(cmake/DependencyHelpers.cmake)
```

### 3. 修改依赖关系（逐步进行）

#### 原则

- **PRIVATE**: 仅实现需要的依赖（大多数情况）
- **PUBLIC**: 接口暴露给用户的依赖（谨慎使用）
- **INTERFACE**: 头文件库或纯接口

#### 示例

**❌ 错误做法**（导致级联重新编译）：
```cmake
target_link_libraries(MyLib PUBLIC
    OtherLib1
    OtherLib2
    OtherLib3
)
```

**✅ 正确做法**（减少重新编译范围）：
```cmake
target_link_libraries(MyLib 
    PRIVATE OtherLib1 OtherLib2  # 仅实现需要
    PUBLIC InterfaceLib          # 接口暴露需要
)
```

### 4. 创建接口库分离依赖

对于通用头文件，创建接口库：

```cmake
add_library(CommonHeaders INTERFACE)
target_include_directories(CommonHeaders INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)
```

然后其他库链接到接口库：

```cmake
target_link_libraries(MyLib PRIVATE CommonHeaders)
```

### 5. 减少头文件依赖

#### 使用前向声明

**❌ 在头文件中**：
```cpp
#include "HeavyClass.h"  // 触发重新编译

class MyClass {
    HeavyClass* ptr;  // 指针或引用
};
```

**✅ 使用前向声明**：
```cpp
class HeavyClass;  // 前向声明，不触发重新编译

class MyClass {
    HeavyClass* ptr;  // 指针或引用
};

// 在.cpp文件中包含
#include "HeavyClass.h"
```

#### PIMPL模式（对于大型类）

```cpp
// Header file
class MyClass {
public:
    MyClass();
    ~MyClass();
    void doSomething();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;  // 隐藏实现细节
};

// Source file
class MyClass::Impl {
    // 所有实现细节，包含的头文件不影响使用者
};
```

## 实施步骤

### 第1步：启用编译缓存（立即生效）

1. 安装sccache（Windows）或ccache（Linux）
2. 重新配置CMake：
```bash
cmake -S . -B build
```
3. 首次编译会慢，但之后会快很多

### 第2步：修改主CMakeLists.txt

在`CMakeLists.txt`第3行后添加：

```cmake
project(CADNav)

# 添加优化配置
include(cmake/CompileOptimizations.cmake)
include(cmake/DependencyHelpers.cmake)
```

### 第3步：逐步修改依赖关系（可选，逐模块进行）

按依赖顺序从底层到顶层修改：

1. **CADLogger, CADCore** - 基础模块
2. **CADOCC, CADGeometry** - 几何模块
3. **CADRendering, CADView** - 渲染模块
4. **UIFrame** - UI模块

每修改一个模块后测试编译是否正常。

## 效果预期

### 使用编译缓存后

| 场景 | 当前 | 优化后 | 提升 |
|------|------|--------|------|
| 首次完整编译 | 5-10分钟 | 5-10分钟 | - |
| 修改单个.cpp文件 | 30-60秒 | 5-10秒 | **6-12x** |
| 修改单个.h文件 | 2-5分钟 | 10-30秒 | **12-30x** |
| 清理后重新编译 | 5-10分钟 | 30-60秒 | **5-20x** |

### 修改依赖关系后

| 场景 | 当前 | 优化后 | 提升 |
|------|------|--------|------|
| 修改底层头文件 | 重新编译80% | 重新编译20% | **4x** |
| 修改UI代码 | 重新编译50% | 重新编译5% | **10x** |

## 快速验证

测试增量编译效率：

```bash
# 1. 完整编译
cmake --build build --config Release

# 2. 记录时间
# 修改一个.cpp文件，例如 src/ui/PerformancePanel.cpp
# 添加一行注释

# 3. 重新编译，查看时间
cmake --build build --config Release

# 预期：如果安装了sccache，应该在10秒内完成
```

## 监控编译缓存效果

### 查看sccache统计

```bash
sccache --show-stats
```

输出示例：
```
Compile requests: 1000
Cache hits: 950 (95%)    # 命中率高 = 效果好
Cache misses: 50
```

### 清理缓存

如果缓存占用空间过大：

```bash
# 查看缓存大小
sccache --show-stats

# 清理缓存
sccache --stop-server
```

## 常见问题

### Q: 安装sccache后为什么没有加速？

A: 检查是否正确配置：
```bash
# 验证CMake是否使用了sccache
cmake -S . -B build 2>&1 | grep -i sccache

# 应该看到：
# -- Found sccache: ...
# -- Using sccache for compilation caching
```

### Q: 修改了一个头文件，为什么还是重新编译很多文件？

A: 
1. 检查该头文件是否被很多其他头文件包含
2. 考虑使用前向声明减少依赖
3. 检查是否使用了PUBLIC链接传播依赖

### Q: 编译缓存会占用多少空间？

A: 
- sccache默认缓存10GB
- 可以配置：`$env:SCCACHE_CACHE_SIZE = "20G"`

### Q: 如何确认优化生效？

A: 
1. 修改一个常用的.cpp文件
2. 重新编译并计时
3. 应该在10秒内完成（之前可能需要30-60秒）

## 进阶优化

### 使用Ninja生成器（更快的构建系统）

```bash
# 安装Ninja（Windows）
scoop install ninja

# 使用Ninja
cmake -S . -B build -G Ninja
ninja -C build
```

**效果：比MSBuild快30-50%**

### 使用Unity Build（对于大项目）

在`CMakeLists.txt`中：

```cmake
set(CMAKE_UNITY_BUILD ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 10)
```

**注意：可能导致某些编译错误，需要调试**

## 总结

**必做优化（最大效果）：**
1. ✅ 安装sccache/ccache - **提升6-12倍**
2. ✅ 使用CompileOptimizations.cmake - **提升20-30%**

**可选优化（长期改进）：**
3. ⚠️ 修改PUBLIC为PRIVATE依赖 - **提升30-50%**
4. ⚠️ 减少头文件依赖 - **提升20-40%**

**建议顺序：**
1. 先做必做优化（30分钟）
2. 使用一周，体验效果
3. 根据实际情况决定是否做可选优化

---

**立即开始：**

```bash
# Windows
scoop install sccache

# 重新配置
cmake -S . -B build

# 完整编译一次（稍慢）
cmake --build build --config Release

# 之后的编译将显著加速！
```


