# CMake 不断 Reconfigure 问题解决方案

## 问题现象

每次运行 `cmake --build build` 时，CMake 都会重新配置（reconfigure），显示：
```
-- Running vcpkg install
-- Configuring done
-- Generating done
```

这导致：
- 编译前等待很长时间
- 无法充分利用增量编译
- 开发效率低下

## 常见原因和解决方案

### 1. 【最可能】vcpkg manifest 模式导致

**问题：** 项目使用了 vcpkg manifest 模式（vcpkg.json），每次构建都会检查依赖

**检查：**
```bash
# 查看是否有 vcpkg.json
ls vcpkg.json

# 查看 CMake 输出
cmake --build build 2>&1 | grep "Running vcpkg"
```

**解决方案 A - 禁用 manifest 模式（推荐用于开发）：**

在 `CMakeLists.txt` 的开头添加：

```cmake
# 禁用 vcpkg manifest 模式以加快重新编译
set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "" FORCE)
```

或者在命令行配置时指定：

```bash
cmake -S . -B build -DVCPKG_MANIFEST_MODE=OFF
```

**解决方案 B - 使用经典模式安装依赖：**

```bash
# 一次性安装所有依赖
vcpkg install wxwidgets:x64-windows opencascade:x64-windows coin:x64-windows

# 然后重新配置（只需一次）
cmake -S . -B build
```

### 2. add_custom_command 导致文件时间戳问题

**问题：** 复制配置文件的命令每次都会更新时间戳

**当前代码问题：**
```cmake
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CONFIG_SOURCE_DIR}"
            "${TARGET_DIR}/config"
    COMMENT "Copying config directory..."
)
```

**修复：** 在 `CMakeLists.txt` 中替换为：

```cmake
# 只在配置时复制一次，而不是每次构建
if(NOT EXISTS "${CMAKE_BINARY_DIR}/config_copied.stamp")
    file(COPY "${CONFIG_SOURCE_DIR}/" 
         DESTINATION "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/config")
    file(WRITE "${CMAKE_BINARY_DIR}/config_copied.stamp" "")
endif()
```

或者更精确的方式：

```cmake
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/config.stamp"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CONFIG_SOURCE_DIR}"
            "${TARGET_DIR}/config"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_BINARY_DIR}/config.stamp"
    DEPENDS "${CONFIG_SOURCE_DIR}"
    COMMENT "Copying config directory..."
)

add_custom_target(CopyConfig ALL
    DEPENDS "${CMAKE_BINARY_DIR}/config.stamp"
)

add_dependencies(${PROJECT_NAME} CopyConfig)
```

### 3. CMake 缓存文件损坏

**解决：**

```bash
# 删除缓存文件
rm -rf build/CMakeCache.txt build/CMakeFiles

# 或者完全重新生成
rm -rf build
cmake -S . -B build
```

### 4. 时间戳比较问题

**问题：** 生成的文件时间戳比源文件新，导致 CMake 认为需要重新配置

**检查：**

```bash
# 查看关键文件的时间戳
ls -lt build/CMakeCache.txt
ls -lt CMakeLists.txt
ls -lt vcpkg.json
```

**修复：**

```bash
# 更新所有 CMakeLists.txt 的时间戳
find . -name "CMakeLists.txt" -exec touch {} \;

# 重新配置
cmake -S . -B build
```

### 5. 依赖文件列表不稳定

**问题：** 使用了 `file(GLOB ...)` 动态收集源文件

**检查：**

```bash
grep -r "file(GLOB" src/
```

**修复：** 将 GLOB 改为显式列出文件

**错误：**
```cmake
file(GLOB SOURCES "src/*.cpp")
```

**正确：**
```cmake
set(SOURCES
    src/file1.cpp
    src/file2.cpp
    src/file3.cpp
)
```

## 快速诊断步骤

### 步骤 1：确定重新配置的原因

```bash
# 运行构建并查看详细输出
cmake --build build --verbose 2>&1 | tee build.log

# 查看日志中的关键信息
grep -i "re-running\|reconfigure\|configuring" build.log
```

### 步骤 2：检查哪个文件触发了重新配置

```bash
# 查看 CMake 重新运行的原因
cmake --build build --trace-expand 2>&1 | grep "re-run"
```

### 步骤 3：查看 CMakeFiles 依赖

```bash
# 查看 CMake 跟踪的依赖文件
cat build/CMakeFiles/3.*/CMakeSystem.cmake
cat build/CMakeFiles/generate.stamp.depend
```

## 推荐解决方案（按优先级）

### 🔥 立即实施（90%的问题）

**修改 `CMakeLists.txt`**，在 `project(CADNav)` 后添加：

```cmake
project(CADNav)

# 禁用 vcpkg manifest 自动运行
set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "" FORCE)

# 减少不必要的文件操作
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)
```

**然后重新配置一次：**

```bash
rm -rf build
cmake -S . -B build -DVCPKG_MANIFEST_MODE=OFF
```

### ⚠️ 如果问题依然存在

**修改配置文件复制逻辑**，找到这段代码：

```cmake
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CONFIG_SOURCE_DIR}"
            "${TARGET_DIR}/config"
    ...
)
```

**替换为：**

```cmake
# 只在首次构建或配置文件变化时复制
set(CONFIG_STAMP "${CMAKE_BINARY_DIR}/config.stamp")
add_custom_command(
    OUTPUT ${CONFIG_STAMP}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CONFIG_SOURCE_DIR}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config"
    COMMAND ${CMAKE_COMMAND} -E touch ${CONFIG_STAMP}
    DEPENDS ${CONFIG_SOURCE_DIR}
    COMMENT "Copying config directory (only when changed)"
    VERBATIM
)

add_custom_target(ConfigCopy ALL DEPENDS ${CONFIG_STAMP})
add_dependencies(${PROJECT_NAME} ConfigCopy)
```

### 🎯 最佳实践（长期）

**创建独立的配置脚本**（`cmake/ConfigureBuild.cmake`）：

```cmake
# Only run during configuration phase, not build phase
if(NOT CONFIGURED_ONCE)
    message(STATUS "First-time configuration")
    
    # Copy config files
    file(COPY "${CMAKE_SOURCE_DIR}/config/"
         DESTINATION "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/config")
    
    # Mark as configured
    set(CONFIGURED_ONCE TRUE CACHE BOOL "Configuration done" FORCE)
endif()
```

在主 `CMakeLists.txt` 中包含：

```cmake
include(cmake/ConfigureBuild.cmake)
```

## 验证修复

修复后验证：

```bash
# 第一次构建
cmake --build build --config Release

# 修改一个源文件（例如添加注释）
# src/ui/PerformancePanel.cpp

# 第二次构建（应该不再重新配置）
cmake --build build --config Release
```

**预期结果：**
- ✅ 不应该看到 "Running vcpkg install"
- ✅ 不应该看到 "Configuring done"
- ✅ 直接开始编译
- ✅ 编译时间：5-10秒（而不是30-60秒）

## 监控和调试

### 启用详细输出

```bash
cmake --build build --config Release --verbose
```

### 查看 CMake 跟踪

```bash
cmake --build build --trace-expand 2>&1 | less
```

### 检查文件依赖

```bash
# 查看哪些文件会触发重新配置
cat build/CMakeFiles/generate.stamp.depend
```

## 常见错误

### ❌ 错误 1：每次都运行 vcpkg install

**症状：**
```
-- Running vcpkg install
Detecting compiler hash...
```

**解决：**
```cmake
set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "" FORCE)
```

### ❌ 错误 2：配置文件不断复制

**症状：**
```
Copying config directory and all subdirectories...
```

**解决：** 使用 stamp 文件或只在配置阶段复制

### ❌ 错误 3：时间戳循环

**症状：** build/CMakeFiles 的时间戳不断更新

**解决：** 使用 `file(TOUCH_NOCREATE)` 或 stamp 文件

## 完整修复示例

**修改你的 `CMakeLists.txt`：**

```cmake
cmake_minimum_required(VERSION 3.20)
project(CADNav)

# === 添加这些优化 ===

# 1. 禁用 vcpkg manifest 自动运行（开发时）
if(NOT DEFINED VCPKG_MANIFEST_MODE)
    set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "Disable vcpkg manifest mode" FORCE)
endif()

# 2. 减少 CMake 重新检查
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY TRUE)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY TRUE)

# === 原有内容继续 ===

# ... 你的其他配置 ...

# 3. 修复配置文件复制（找到并替换原来的 add_custom_command）
if(MSVC)
    set(CONFIG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/config")
    set(CONFIG_STAMP "${CMAKE_BINARY_DIR}/config_copy.stamp")
    
    add_custom_command(
        OUTPUT ${CONFIG_STAMP}
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CONFIG_SOURCE_DIR}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/config"
        COMMAND ${CMAKE_COMMAND} -E touch ${CONFIG_STAMP}
        DEPENDS "${CONFIG_SOURCE_DIR}"
        COMMENT "Copying config directory (when changed)"
    )
    
    add_custom_target(ConfigCopy ALL DEPENDS ${CONFIG_STAMP})
    add_dependencies(${PROJECT_NAME} ConfigCopy)
endif()
```

**重新配置：**

```bash
# 清理
rm -rf build/CMakeCache.txt

# 重新配置
cmake -S . -B build -DVCPKG_MANIFEST_MODE=OFF

# 测试构建
cmake --build build --config Release
```

## 总结

**最快解决方案（5分钟）：**

```bash
# 1. 编辑 CMakeLists.txt，在 project() 后添加
set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "" FORCE)

# 2. 删除缓存
rm -rf build/CMakeCache.txt

# 3. 重新配置
cmake -S . -B build

# 4. 测试
cmake --build build --config Release
```

这应该能解决 90% 的重新配置问题！


