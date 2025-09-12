# STEP文件单一组件颜色显示改进

## 问题分析

从用户提供的日志可以看出：

```
[2025-09-12 13:36:40] [INF] Found 1 free shapes in CAF document
[2025-09-12 13:36:40] [INF] Processing 1 components with distinct colors
[2025-09-12 13:36:40] [INF] Assigned color to component 0 (ATU01038_): R=1.000000 G=0.000000 B=0.000000
```

**问题**：这个STEP文件只包含1个组件，所以无法看到颜色差异的效果。CAF读取器工作正常，但单一组件无法展示多颜色功能。

## 解决方案

### 1. 单一组件分解功能

添加了自动分解功能，当检测到单一组件时，尝试将其分解为更小的子组件：

```cpp
// For single component, try to decompose into sub-shapes for better visualization
if (tryDecomposition && freeShapes.Length() == 1) {
    std::vector<TopoDS_Shape> subShapes;
    decomposeShape(shape, subShapes);
    
    if (subShapes.size() > 1) {
        LOG_INF_S("Decomposed single component into " + std::to_string(subShapes.size()) + " sub-components");
        
        // Process each sub-shape with different colors
        for (size_t j = 0; j < subShapes.size(); j++) {
            processComponent(subShapes[j], baseName + "_Part_" + std::to_string(j), 
                componentIndex, result.geometries, result.entityMetadata);
            componentIndex++;
        }
        continue; // Skip the original single component processing
    }
}
```

### 2. 形状分解算法

实现了智能的形状分解算法：

```cpp
static void decomposeShape(const TopoDS_Shape& shape, std::vector<TopoDS_Shape>& subShapes) {
    // 1. 首先尝试分解为实体（Solids）
    for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
        subShapes.push_back(exp.Current());
    }
    
    // 2. 如果没有实体，尝试分解为壳（Shells）
    if (subShapes.empty()) {
        for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) {
            subShapes.push_back(exp.Current());
        }
    }
    
    // 3. 如果仍然没有子形状，尝试分解为面（Faces）
    if (subShapes.empty()) {
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            subShapes.push_back(exp.Current());
        }
    }
    
    // 4. 如果完全无法分解，使用原始形状
    if (subShapes.empty()) {
        subShapes.push_back(shape);
    }
}
```

### 3. 统一组件处理

创建了统一的组件处理函数，确保颜色分配的一致性：

```cpp
static void processComponent(const TopoDS_Shape& shape, const std::string& componentName, 
    int componentIndex, std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    std::vector<STEPReader::STEPEntityInfo>& entityMetadata) {
    
    // 使用相同的颜色分配算法
    Quantity_Color color = distinctColors[componentIndex % distinctColors.size()];
    
    // 创建几何对象和实体信息
    auto geometry = std::make_shared<OCCGeometry>(componentName);
    geometry->setShape(shape);
    geometry->setColor(color);
    geometry->setTransparency(0.0);
    
    // 添加到结果中
    geometries.push_back(geometry);
    entityMetadata.push_back(entityInfo);
}
```

## 功能特性

### 1. **智能分解**
- 自动检测单一组件
- 尝试多种分解策略（实体→壳→面）
- 保持原始形状的完整性

### 2. **颜色一致性**
- 使用相同的15种预定义颜色
- 确保分解后的组件颜色明显不同
- 保持与多组件STEP文件相同的颜色方案

### 3. **命名规范**
- 分解后的组件命名为：`BaseName_Part_0`, `BaseName_Part_1`, ...
- 保持原始组件名称的识别性
- 便于在对象树中识别

### 4. **错误处理**
- 分解失败时回退到原始处理
- 详细的日志记录
- 确保系统稳定性

## 预期效果

### 修改前
- 单一组件显示为单一红色
- 无法看到颜色差异效果
- 用户体验不佳

### 修改后
- 单一组件自动分解为多个子组件
- 每个子组件显示不同颜色
- 用户可以清楚看到各个部分
- 提供更好的可视化效果

## 测试建议

1. **重新编译项目**：应用新的分解功能
2. **测试单一组件STEP文件**：验证分解效果
3. **测试多组件STEP文件**：确保原有功能不受影响
4. **检查日志输出**：查看分解过程的详细信息

## 技术优势

1. **向后兼容**：不影响现有的多组件处理逻辑
2. **智能适应**：根据组件数量自动选择处理策略
3. **性能优化**：只在必要时进行分解
4. **用户友好**：提供更好的可视化体验

## 总结

通过添加单一组件分解功能，现在即使是只有一个组件的STEP文件也能展示多颜色效果，大大提升了用户体验和可视化效果。