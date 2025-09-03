# 增强版轮廓渲染预览程序使用指南

## 概述

增强版轮廓渲染预览程序是一个展示多种实时轮廓渲染技术的示例应用。它提供了交互式的3D预览和实时参数调整功能。

## 功能特性

### 1. 多种轮廓渲染方法

- **Basic (无轮廓)**：基础渲染，不应用任何轮廓效果
- **Inverted Hull (反向外壳)**：通过渲染放大的背面来创建轮廓
- **Screen Space (屏幕空间)**：在屏幕空间进行边缘检测
- **Geometry Silhouette (几何轮廓)**：提取几何体的轮廓边
- **Stencil Buffer (模板缓冲)**：使用模板缓冲技术创建轮廓
- **Multi-Pass (多通道)**：结合多种技术获得最佳效果

### 2. 实时参数调整

#### 基础参数
- **Edge Intensity**：轮廓强度 (0.0-2.0)
- **Thickness**：轮廓厚度 (0.1-5.0像素)
- **Depth Weight**：深度边缘权重 (0.0-2.0)
- **Normal Weight**：法向量边缘权重 (0.0-2.0)
- **Depth Threshold**：深度阈值 (0.0-0.05)
- **Normal Threshold**：法向量阈值 (0.0-2.0)

#### 高级参数
- **Crease Angle**：折痕角度阈值 (0-180度)
- **Fade Distance**：轮廓淡出距离
- **Adaptive Thickness**：自适应厚度
- **Anti-aliasing**：抗锯齿
- **Sample Count**：采样数量 (1, 2, 4, 8, 16)

### 3. 交互控制

- **鼠标左键拖动**：旋转模型
- **Ctrl+S**：打开设置对话框
- **Ctrl+R**：重置视图
- **Ctrl+1~5**：快速切换渲染方法

## 使用方法

### 编译运行

```bash
# 在workspace目录下
cd src/ui
mkdir build
cd build
cmake -f ../CMakeLists_outline_preview.txt ..
make
./EnhancedOutlinePreview
```

### 基本操作

1. **选择渲染方法**
   - 使用菜单栏的"Method"菜单选择不同的轮廓渲染方法
   - 或使用快捷键Ctrl+1到Ctrl+5快速切换

2. **调整参数**
   - 选择"View > Settings"打开设置对话框
   - 在"Basic Settings"标签页调整基础参数
   - 在"Advanced Settings"标签页调整高级参数
   - 参数修改会实时反映在预览窗口中

3. **查看性能**
   - 设置对话框底部显示实时性能统计
   - 包括渲染时间、绘制调用次数和三角形数量

## 各渲染方法详解

### Inverted Hull（反向外壳）

**原理**：
1. 先渲染放大的模型背面（黑色）
2. 再渲染正常的模型正面
3. 背面露出的部分形成轮廓

**优点**：
- 实现简单，性能好
- 轮廓均匀连续
- 适合凸面物体

**缺点**：
- 凹面物体效果不佳
- 无法显示内部边缘

**参数调整建议**：
- Thickness: 1.0-2.0
- Edge Intensity: 0.8-1.0

### Screen Space（屏幕空间）

**原理**：
1. 渲染场景到深度缓冲
2. 对深度进行边缘检测
3. 将检测到的边缘叠加到最终图像

**优点**：
- 性能与几何复杂度无关
- 可以检测所有类型的边缘
- 适合复杂场景

**缺点**：
- 精度受分辨率限制
- 可能产生锯齿

**参数调整建议**：
- Depth Weight: 1.5-2.0
- Depth Threshold: 0.001-0.005
- Anti-aliasing: 开启

### Geometry Silhouette（几何轮廓）

**原理**：
1. 分析几何拓扑结构
2. 计算相邻面法向量与视线的关系
3. 提取轮廓边并渲染

**优点**：
- 精确的轮廓提取
- 可以区分不同类型的边
- 支持样式化渲染

**缺点**：
- 计算复杂度高
- 需要完整的拓扑信息

**参数调整建议**：
- Crease Angle: 30-60度
- Normal Weight: 1.0-1.5

### Multi-Pass（多通道）

**原理**：
结合多种技术：
1. 使用反向外壳渲染外轮廓
2. 使用线框渲染内部边缘
3. 可选的屏幕空间增强

**优点**：
- 最高质量的轮廓效果
- 同时显示外轮廓和内部边缘
- 高度可定制

**缺点**：
- 性能开销最大
- 参数调整复杂

**参数调整建议**：
- Inner Edge Intensity: 0.3-0.5
- Silhouette Boost: 1.2-1.8
- 所有基础参数都很重要

## 性能优化建议

1. **低端硬件**
   - 使用Inverted Hull方法
   - 降低Sample Count
   - 关闭Anti-aliasing

2. **复杂场景**
   - 使用Screen Space方法
   - 适当提高Depth Threshold
   - 考虑降低分辨率

3. **高质量需求**
   - 使用Multi-Pass方法
   - 开启所有高级选项
   - 提高Sample Count

## 常见问题

**Q: 轮廓出现闪烁**
A: 尝试增加Depth Threshold或开启Anti-aliasing

**Q: 轮廓太粗/太细**
A: 调整Thickness参数，注意不同方法的有效范围不同

**Q: 性能太低**
A: 切换到更简单的渲染方法，或降低采样质量

**Q: 内部边缘不显示**
A: 使用Geometry Silhouette或Multi-Pass方法

## 扩展开发

如果需要在自己的项目中使用这些轮廓渲染技术：

1. 包含必要的头文件：
```cpp
#include "ui/EnhancedOutlinePreviewCanvas.h"
#include "rendering/shaders/outline_shaders.h"
```

2. 创建轮廓渲染器：
```cpp
EnhancedOutlinePreviewCanvas* canvas = new EnhancedOutlinePreviewCanvas(parent);
canvas->setOutlineMethod(OutlineMethod::INVERTED_HULL);
canvas->updateOutlineParams(params);
```

3. 集成到渲染管线：
```cpp
// 在渲染循环中
canvas->render();
```

## 总结

增强版轮廓渲染预览程序提供了一个完整的轮廓渲染解决方案，涵盖了从简单到复杂的多种技术。通过实时预览和参数调整，可以快速找到适合特定需求的轮廓效果。无论是用于CAD软件、游戏引擎还是技术可视化，都能找到合适的渲染方法。