# STEP文件法线方向修正解决方案

## 问题描述

在导入STEP文件时，几何体的某些部位显示正常颜色，而某些部位显示为暗色。这是由于STEP文件中的面法线方向不一致导致的，某些面的法线方向朝内而不是朝外，导致渲染引擎无法正确计算光照。

## 根本原因分析

1. **STEP文件格式特性**：STEP文件中的面法线方向可能不一致，有些面朝外，有些面朝内
2. **OpenCASCADE转换过程**：在STEP到OCCT模型转换过程中，OpenCASCADE的`STEPControl_Reader`可能无法保证所有面的法线方向一致性
3. **渲染引擎依赖**：渲染引擎（如Coin3D）依赖正确的法线方向来计算光照，法线方向错误会导致某些面显示为暗色

## 解决方案

### 1. 改进的NormalValidator类

创建了一个完整的法线验证和修正系统：

```cpp
// 主要功能
- validateNormals(): 验证几何体的法线方向
- autoCorrectNormals(): 自动修正法线方向
- isNormalOutward(): 检查法线是否朝外
- correctFaceNormals(): 修正面法线方向
```

### 2. 集成到STEPReader

在STEP文件导入过程中自动应用法线修正：

```cpp
// 在processSingleShape函数中
TopoDS_Shape consistentShape = NormalValidator::autoCorrectNormals(shape, name);
```

### 3. 手动修正命令

提供了`FixNormalsListener`命令，允许用户手动修正已导入几何体的法线：

```cpp
// 使用方法
CommandResult result = fixNormalsListener.executeCommand("FixNormals", {});
```

### 4. 法线可视化工具

提供了`ShowNormalsListener`命令，用于可视化法线方向：

```cpp
// 使用方法
std::unordered_map<std::string, std::string> params = {
    {"length", "1.0"},
    {"show_correct", "true"},
    {"show_incorrect", "true"}
};
CommandResult result = showNormalsListener.executeCommand("ShowNormals", params);
```

## 使用方法

### 自动修正（推荐）

法线修正会在导入STEP文件时自动执行，无需额外操作。

### 手动修正

如果导入后仍有问题，可以使用手动修正：

1. 在UI中选择"修正法线"命令
2. 系统会自动分析并修正所有几何体的法线方向
3. 修正完成后会自动刷新显示

### 法线可视化

用于调试和验证法线方向：

1. 在UI中选择"显示法线"命令
2. 系统会显示每个面的法线向量
3. 绿色向量表示正确的法线方向
4. 红色向量表示需要修正的法线方向

## 技术实现细节

### 法线方向检测算法

1. 计算几何体的中心点
2. 对每个面：
   - 计算面的中心点
   - 获取面在中心点的法线向量
   - 计算从几何体中心到面中心的向量
   - 通过点积判断法线是否朝外

### 法线修正算法

1. 使用`ShapeFix_Shape`进行初步几何修复
2. 对每个面进行法线方向检查
3. 对方向错误的面使用`face.Reverse()`进行修正
4. 重建几何体结构

### 性能优化

- 使用并行处理处理多个几何体
- 缓存验证结果避免重复计算
- 提供进度回调支持长时间操作

## 测试验证

运行测试脚本验证功能：

```bash
python3 test_normal_correction.py
```

测试包括：
- 基本STEP文件导入测试
- 复杂几何体测试
- 法线验证功能测试
- 性能测试

## 配置选项

可以通过以下参数调整法线修正行为：

```cpp
// 在OptimizationOptions中
struct OptimizationOptions {
    double precision = 1e-6;           // 几何精度
    bool enableNormalCorrection = true; // 启用法线修正
    bool enableParallelProcessing = true; // 启用并行处理
    bool enableCaching = true;        // 启用缓存
};
```

## 故障排除

### 常见问题

1. **修正后仍有暗面**：
   - 检查几何体是否有自相交
   - 尝试调整精度参数
   - 使用法线可视化工具检查

2. **性能问题**：
   - 禁用并行处理
   - 减少几何体复杂度
   - 调整缓存设置

3. **修正失败**：
   - 检查几何体是否有效
   - 查看日志中的错误信息
   - 尝试使用ShapeFix工具预处理

### 日志信息

系统会输出详细的日志信息：

```
[INFO] Starting normal validation for: geometry_name
[INFO] Shape center: (x, y, z)
[INFO] Processed N faces, reversed M faces for consistent normals
[INFO] Normal consistency: X correct, Y corrected
```

## 未来改进

1. **更智能的修正算法**：基于几何体类型选择最佳修正策略
2. **批量处理优化**：支持大型装配体的高效处理
3. **用户界面集成**：在UI中提供法线修正选项
4. **实时预览**：修正前后对比显示

## 相关文件

- `include/NormalValidator.h` - 法线验证器头文件
- `src/NormalValidator.cpp` - 法线验证器实现
- `src/geometry/STEPReader.cpp` - STEP文件读取器
- `src/commands/FixNormalsListener.cpp` - 法线修正命令
- `src/commands/ShowNormalsListener.cpp` - 法线可视化命令
- `test_normal_correction.py` - 测试脚本