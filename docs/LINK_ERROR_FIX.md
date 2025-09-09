# 链接错误修复说明

## 问题描述

编译时出现链接错误，提示无法解析NormalValidator的以下外部符号：

1. `NormalValidator::validateNormals`
2. `NormalValidator::calculateShapeCenter`
3. `NormalValidator::isNormalOutward`
4. `NormalValidator::autoCorrectNormals`

## 根本原因

NormalValidator.cpp文件没有被包含在任何CMakeLists.txt中，导致链接器找不到这些函数的实现。

## 解决方案

将NormalValidator.cpp和NormalValidator.h添加到geometry模块的CMakeLists.txt中，因为：

1. NormalValidator主要用于几何体处理
2. geometry模块已经有OpenCASCADE依赖
3. STEPReader已经在geometry模块中使用了NormalValidator

## 修复内容

### 修改文件：`src/geometry/CMakeLists.txt`

**添加源文件**：
```cmake
set(GEOMETRY_SOURCES
    # ... 其他源文件 ...
    ${CMAKE_SOURCE_DIR}/src/NormalValidator.cpp
)
```

**添加头文件**：
```cmake
set(GEOMETRY_HEADERS
    # ... 其他头文件 ...
    ${CMAKE_SOURCE_DIR}/include/NormalValidator.h
)
```

## 验证

修复后，重新编译项目应该能够成功链接，因为：

1. NormalValidator.cpp现在会被编译到CADGeometry库中
2. CADCommands库已经链接到CADGeometry库
3. 所有NormalValidator的符号都会被正确解析

## 相关文件

- `src/geometry/CMakeLists.txt` - 已修改
- `src/NormalValidator.cpp` - 实现文件
- `include/NormalValidator.h` - 头文件
- `src/commands/FixNormalsListener.cpp` - 使用NormalValidator
- `src/commands/ShowNormalsListener.cpp` - 使用NormalValidator
- `src/geometry/STEPReader.cpp` - 使用NormalValidator

## 编译命令

在Windows环境中重新编译：

```cmd
cd D:\source\repos\wxcoin\build
cmake --build . --config Release
```

或者使用Visual Studio重新构建解决方案。