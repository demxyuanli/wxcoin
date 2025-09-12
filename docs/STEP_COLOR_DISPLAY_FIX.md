# STEP文件颜色显示问题修复

## 问题描述

用户反馈导入STEP文件后，各个部件没有显示为不同颜色，所有部件都显示为相同的颜色。

## 问题分析

经过分析，发现以下几个可能的原因：

1. **CAF读取器失败**：如果STEPCAFControl_Reader读取失败，系统会回退到标准STEPControl_Reader
2. **标准读取器颜色问题**：标准读取器总是为所有组件设置相同的默认灰色
3. **单一组件问题**：如果STEP文件只有一个组件，即使CAF成功也无法看到颜色差异
4. **调试信息不足**：缺乏足够的日志信息来诊断问题

## 修复方案

### 1. 增强CAF读取器颜色分配

修改`readSTEPFileWithCAF`方法，确保总是为每个组件分配不同的颜色：

```cpp
// 修改前：只在没有颜色时分配
if (!hasColor) {
    color = distinctColors[componentIndex % distinctColors.size()];
}

// 修改后：总是分配不同颜色
// Always generate distinct colors for better visualization (override any existing color)
color = distinctColors[componentIndex % distinctColors.size()];
hasColor = true;
```

### 2. 修复标准读取器颜色问题

修改`processSingleShape`方法，使用基于名称哈希的颜色分配：

```cpp
// 修改前：所有组件使用相同颜色
Quantity_Color defaultColor(0.8, 0.8, 0.8, Quantity_TOC_RGB);
geometry->setColor(defaultColor);

// 修改后：基于名称哈希分配不同颜色
static std::vector<Quantity_Color> distinctColors = {
    Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), // Red
    Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB), // Green
    // ... 更多颜色
};

std::hash<std::string> hasher;
size_t hashValue = hasher(name);
size_t colorIndex = hashValue % distinctColors.size();
Quantity_Color componentColor = distinctColors[colorIndex];
geometry->setColor(componentColor);
```

### 3. 添加详细的调试信息

在关键位置添加日志输出：

```cpp
// CAF读取器调试信息
LOG_INF_S("Attempting to read STEP file with CAF reader: " + filePath);
LOG_INF_S("Found " + std::to_string(freeShapes.Length()) + " free shapes in CAF document");
LOG_INF_S("Processing " + std::to_string(freeShapes.Length()) + " components with distinct colors");

// 颜色分配调试信息
LOG_INF_S("Assigned color to component " + std::to_string(componentIndex) + 
    " (" + componentName + "): R=" + std::to_string(color.Red()) + 
    " G=" + std::to_string(color.Green()) + " B=" + std::to_string(color.Blue()));

// 最终结果调试信息
for (size_t i = 0; i < result.geometries.size(); i++) {
    Quantity_Color color = result.geometries[i]->getColor();
    LOG_INF_S("Component " + std::to_string(i) + " color: R=" + 
        std::to_string(color.Red()) + " G=" + std::to_string(color.Green()) + 
        " B=" + std::to_string(color.Blue()));
}
```

## 修复效果

### 1. CAF读取器成功时
- 每个组件都会被分配不同的颜色
- 即使STEP文件中有原始颜色，也会被覆盖为更明显的颜色
- 提供详细的颜色分配日志

### 2. CAF读取器失败时
- 标准读取器也会为不同组件分配不同颜色
- 使用基于名称哈希的颜色分配，确保一致性
- 不再出现所有组件都是灰色的问题

### 3. 调试能力增强
- 详细的日志输出帮助诊断问题
- 可以清楚看到每个组件的颜色分配情况
- 能够识别CAF读取器是否成功

## 测试建议

1. **导入多组件STEP文件**：应该看到不同颜色的组件
2. **导入单组件STEP文件**：应该看到明显的颜色（不再是灰色）
3. **检查日志输出**：查看CAF读取器是否成功，颜色分配是否正常
4. **对比FreeCAD**：与FreeCAD的显示效果进行对比

## 技术细节

### 颜色分配算法
- 使用15种预定义的不同颜色
- CAF读取器：按组件索引顺序分配
- 标准读取器：基于名称哈希分配（确保一致性）

### 颜色列表
```cpp
Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), // Red
Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB), // Green
Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB), // Blue
Quantity_Color(1.0, 1.0, 0.0, Quantity_TOC_RGB), // Yellow
Quantity_Color(1.0, 0.0, 1.0, Quantity_TOC_RGB), // Magenta
Quantity_Color(0.0, 1.0, 1.0, Quantity_TOC_RGB), // Cyan
Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB), // Orange
Quantity_Color(0.5, 0.0, 1.0, Quantity_TOC_RGB), // Purple
Quantity_Color(0.0, 0.5, 0.0, Quantity_TOC_RGB), // Dark Green
Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB), // Gray
Quantity_Color(1.0, 0.5, 0.5, Quantity_TOC_RGB), // Light Red
Quantity_Color(0.5, 1.0, 0.5, Quantity_TOC_RGB), // Light Green
Quantity_Color(0.5, 0.5, 1.0, Quantity_TOC_RGB), // Light Blue
Quantity_Color(1.0, 1.0, 0.5, Quantity_TOC_RGB), // Light Yellow
Quantity_Color(1.0, 0.5, 1.0, Quantity_TOC_RGB), // Light Magenta
```

## 总结

通过以上修复，现在STEP文件导入后应该能够正确显示不同颜色的组件，无论是通过CAF读取器还是标准读取器。同时增加了详细的调试信息，便于进一步诊断和优化。