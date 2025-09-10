# CMakeLists.txt 集成更新说明

## 概述

本文档说明了如何将新增的EnhancedOutlinePass相关程序集成到现有的CMakeLists.txt构建系统中。

## 新增文件

### 1. OpenCASCADE模块新增文件

**源文件 (src/opencascade/viewer/)**:
- `EnhancedOutlinePass.cpp` - 增强版轮廓渲染实现
- `OutlinePassManager.cpp` - 轮廓渲染管理器

**头文件 (include/viewer/)**:
- `EnhancedOutlinePass.h` - 增强版轮廓渲染头文件
- `OutlinePassManager.h` - 轮廓渲染管理器头文件

### 2. UI模块新增文件

**源文件 (src/ui/)**:
- `EnhancedOutlineSettingsDialog.cpp` - 增强版轮廓设置对话框
- `EnhancedOutlinePreviewCanvas.cpp` - 增强版预览画布

**头文件 (include/ui/)**:
- `EnhancedOutlineSettingsDialog.h` - 增强版轮廓设置对话框头文件
- `EnhancedOutlinePreviewCanvas.h` - 增强版预览画布头文件

## CMakeLists.txt 更新内容

### 1. src/opencascade/CMakeLists.txt 更新

```cmake
# 在 OPENCASCADE_SOURCES 中添加
${CMAKE_CURRENT_SOURCE_DIR}/viewer/EnhancedOutlinePass.cpp
${CMAKE_CURRENT_SOURCE_DIR}/viewer/OutlinePassManager.cpp

# 在 OPENCASCADE_HEADERS 中添加
${CMAKE_SOURCE_DIR}/include/viewer/EnhancedOutlinePass.h
${CMAKE_SOURCE_DIR}/include/viewer/OutlinePassManager.h
```

### 2. src/ui/CMakeLists.txt 更新

```cmake
# 在 UIDialogEdges 库中添加新文件
add_library(UIDialogEdges STATIC
    # 现有文件...
    ${CMAKE_CURRENT_SOURCE_DIR}/EnhancedOutlineSettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/EnhancedOutlinePreviewCanvas.cpp
    
    # 现有头文件...
    ${CMAKE_SOURCE_DIR}/include/ui/EnhancedOutlineSettingsDialog.h
    ${CMAKE_SOURCE_DIR}/include/ui/EnhancedOutlinePreviewCanvas.h
)

# 更新链接库依赖
target_link_libraries(UIDialogEdges PUBLIC 
    CADCore 
    CADConfig 
    CADLogger 
    CADRendering 
    CADOCC  # 新增：用于访问EnhancedOutlinePass
    CADFlatUI 
    widgets 
    ${wxWidgets_LIBRARIES}
)
```

## 依赖关系

### EnhancedOutlinePass 依赖
- **CADCore**: 核心功能
- **CADLogger**: 日志记录
- **CADView**: 视图管理
- **CADRenderingToolkit**: 渲染工具包
- **Coin::Coin**: Coin3D库
- **OpenCASCADE**: OpenCASCADE几何内核

### EnhancedOutlineSettingsDialog 依赖
- **CADCore**: 核心功能
- **CADConfig**: 配置管理
- **CADLogger**: 日志记录
- **CADRendering**: 渲染功能
- **CADOCC**: OpenCASCADE模块（用于访问EnhancedOutlinePass）
- **CADFlatUI**: UI组件
- **widgets**: 自定义控件
- **wxWidgets**: GUI框架

### EnhancedOutlinePreviewCanvas 依赖
- **CADCore**: 核心功能
- **CADLogger**: 日志记录
- **CADOCC**: OpenCASCADE模块（用于访问EnhancedOutlinePass）
- **wxWidgets**: GUI框架
- **Coin::Coin**: Coin3D库

## 构建顺序

根据依赖关系，构建顺序应该是：

1. **CADCore** - 核心模块
2. **CADLogger** - 日志模块
3. **CADConfig** - 配置模块
4. **CADRendering** - 渲染模块
5. **CADView** - 视图模块
6. **CADRenderingToolkit** - 渲染工具包
7. **CADOCC** - OpenCASCADE模块（包含EnhancedOutlinePass）
8. **CADFlatUI** - UI组件
9. **widgets** - 自定义控件
10. **UIDialogEdges** - UI对话框（包含EnhancedOutlineSettingsDialog）

## 编译选项

### 必需的编译选项
- **C++17**: EnhancedOutlinePass使用了C++17特性
- **OpenGL**: 需要OpenGL支持
- **Coin3D**: 需要Coin3D库支持
- **wxWidgets**: 需要wxWidgets GUI框架

### 可选的编译选项
- **OpenGL调试**: 可以启用OpenGL调试输出
- **性能分析**: 可以启用性能监控功能

## 平台特定配置

### Windows
```cmake
# Windows特定配置
if(WIN32)
    target_compile_definitions(CADOCC PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_definitions(UIDialogEdges PRIVATE WIN32_LEAN_AND_MEAN)
endif()
```

### Linux
```cmake
# Linux特定配置
if(UNIX AND NOT APPLE)
    target_link_libraries(CADOCC PRIVATE pthread)
    target_link_libraries(UIDialogEdges PRIVATE pthread)
endif()
```

### macOS
```cmake
# macOS特定配置
if(APPLE)
    target_link_libraries(CADOCC PRIVATE "-framework OpenGL")
    target_link_libraries(UIDialogEdges PRIVATE "-framework OpenGL")
endif()
```

## 测试和验证

### 编译测试
```bash
# 清理构建目录
rm -rf build/
mkdir build
cd build

# 配置项目
cmake ..

# 编译
make -j$(nproc)
```

### 功能测试
1. **基本编译**: 确保所有新文件都能正确编译
2. **链接测试**: 确保所有依赖都能正确链接
3. **运行时测试**: 确保EnhancedOutlinePass能正常工作
4. **UI测试**: 确保EnhancedOutlineSettingsDialog能正常显示

## 故障排除

### 常见编译错误

#### 1. 找不到头文件
```
fatal error: 'viewer/EnhancedOutlinePass.h' file not found
```
**解决方案**: 检查include路径是否正确设置

#### 2. 链接错误
```
undefined reference to `EnhancedOutlinePass::EnhancedOutlinePass'
```
**解决方案**: 检查源文件是否添加到CMakeLists.txt中

#### 3. 依赖错误
```
undefined reference to `SceneManager::getCamera'
```
**解决方案**: 检查CADOCC模块是否正确链接

### 调试技巧

#### 1. 启用详细输出
```bash
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
```

#### 2. 检查依赖图
```bash
cmake .. --graphviz=deps.dot
dot -Tpng deps.dot -o deps.png
```

#### 3. 检查包含路径
```bash
make VERBOSE=1 2>&1 | grep -E "(include|library)"
```

## 维护说明

### 添加新文件
1. 将源文件添加到相应的`*_SOURCES`变量中
2. 将头文件添加到相应的`*_HEADERS`变量中
3. 更新依赖关系（如果需要）

### 修改依赖关系
1. 更新`target_link_libraries`调用
2. 确保构建顺序正确
3. 测试编译和链接

### 版本控制
- 所有CMakeLists.txt更改都应该提交到版本控制系统
- 保持CMakeLists.txt文件的整洁和可读性
- 添加适当的注释说明复杂的依赖关系

## 总结

通过以上更新，新的EnhancedOutlinePass相关程序已经成功集成到CMakeLists.txt构建系统中。这些更新确保了：

1. **正确的文件包含**: 所有新文件都被正确添加到构建系统中
2. **正确的依赖关系**: 所有依赖关系都被正确设置
3. **跨平台兼容**: 支持Windows、Linux和macOS平台
4. **可维护性**: 清晰的代码结构和注释

现在可以正常编译和使用EnhancedOutlinePass功能了。