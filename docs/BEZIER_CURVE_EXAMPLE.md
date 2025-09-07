# OpenCASCADE贝塞尔曲线集成使用示例

## 概述

本项目已成功集成OpenCASCADE的贝塞尔曲线支持，提供了完整的曲线创建、渲染和可视化功能。

## 功能特性

### ✅ 支持的曲线类型
- **贝塞尔曲线 (Bezier Curves)**: 支持任意阶数的贝塞尔曲线
- **贝塞尔曲面 (Bezier Surfaces)**: 支持双参数贝塞尔曲面
- **B样条曲线 (B-Spline Curves)**: 支持非有理B样条曲线
- **NURBS曲线 (NURBS Curves)**: 支持有理B样条曲线

### ✅ 渲染功能
- **Coin3D集成**: 直接渲染到3D场景
- **自适应采样**: 可配置采样密度
- **平滑渲染**: 支持抗锯齿和平滑着色
- **材质支持**: 支持颜色、透明度等材质属性

## 使用示例

### 1. 创建贝塞尔曲线

```cpp
#include "OCCShapeBuilder.h"
#include "OCCMeshConverter.h"

// 创建贝塞尔曲线控制点
std::vector<gp_Pnt> controlPoints = {
    gp_Pnt(0, 0, 0),      // 起点
    gp_Pnt(1, 2, 0),      // 控制点1
    gp_Pnt(3, -1, 0),     // 控制点2
    gp_Pnt(4, 0, 0)       // 终点
};

// 方法1: 使用OCCShapeBuilder创建TopoDS_Shape
TopoDS_Shape bezierShape = OCCShapeBuilder::createBezierCurve(controlPoints);

// 方法2: 直接创建Coin3D节点进行渲染
SoSeparator* bezierNode = OCCMeshConverter::createBezierCurveNode(controlPoints, 50);
```

### 2. 创建贝塞尔曲面

```cpp
// 创建贝塞尔曲面控制点网格 (4x4)
std::vector<std::vector<gp_Pnt>> controlPoints = {
    // 第一行
    {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 1), gp_Pnt(2, 0, 1), gp_Pnt(3, 0, 0)},
    // 第二行
    {gp_Pnt(0, 1, 1), gp_Pnt(1, 1, 2), gp_Pnt(2, 1, 2), gp_Pnt(3, 1, 1)},
    // 第三行
    {gp_Pnt(0, 2, 1), gp_Pnt(1, 2, 2), gp_Pnt(2, 2, 2), gp_Pnt(3, 2, 1)},
    // 第四行
    {gp_Pnt(0, 3, 0), gp_Pnt(1, 3, 1), gp_Pnt(2, 3, 1), gp_Pnt(3, 3, 0)}
};

// 创建贝塞尔曲面
TopoDS_Shape bezierSurface = OCCShapeBuilder::createBezierSurface(controlPoints);

// 直接渲染为Coin3D节点
SoSeparator* surfaceNode = OCCMeshConverter::createBezierSurfaceNode(controlPoints, 20, 20);
```

### 3. 创建B样条曲线

```cpp
// 定义控制点
std::vector<gp_Pnt> poles = {
    gp_Pnt(0, 0, 0),
    gp_Pnt(1, 1, 0),
    gp_Pnt(2, -1, 0),
    gp_Pnt(3, 0, 0),
    gp_Pnt(4, 1, 0)
};

// 定义权重 (可选，用于NURBS)
std::vector<double> weights = {1.0, 1.0, 1.0, 1.0, 1.0};

// 创建B样条曲线
TopoDS_Shape bsplineShape = OCCShapeBuilder::createBSplineCurve(poles, weights, 3);

// 直接渲染
SoSeparator* bsplineNode = OCCMeshConverter::createBSplineCurveNode(poles, weights, 50);
```

### 4. 创建NURBS曲线

```cpp
// 定义控制点
std::vector<gp_Pnt> poles = {
    gp_Pnt(0, 0, 0),
    gp_Pnt(1, 1, 0),
    gp_Pnt(2, -1, 0),
    gp_Pnt(3, 0, 0)
};

// 定义权重
std::vector<double> weights = {1.0, 2.0, 2.0, 1.0};

// 定义节点向量
std::vector<double> knots = {0, 0, 0, 0, 1, 1, 1, 1};

// 定义重数
std::vector<int> multiplicities = {4, 4};

// 创建NURBS曲线
TopoDS_Shape nurbsShape = OCCShapeBuilder::createNURBSCurve(poles, weights, knots, multiplicities, 3);

// 直接渲染
SoSeparator* nurbsNode = OCCMeshConverter::createNURBSCurveNode(poles, weights, 50);
```

### 5. 在OCCViewer中使用

```cpp
#include "OCCViewer.h"
#include "OCCGeometry.h"

// 创建贝塞尔曲线
std::vector<gp_Pnt> controlPoints = {
    gp_Pnt(0, 0, 0),
    gp_Pnt(1, 2, 0),
    gp_Pnt(3, -1, 0),
    gp_Pnt(4, 0, 0)
};

TopoDS_Shape bezierShape = OCCShapeBuilder::createBezierCurve(controlPoints);

// 创建OCCGeometry对象
auto geometry = std::make_shared<OCCGeometry>("BezierCurve", bezierShape);
geometry->setColor(Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB)); // 红色

// 添加到OCCViewer
occViewer->addGeometry(geometry);
```

## 技术实现细节

### 1. OpenCASCADE集成
- **Geom_BezierCurve**: 贝塞尔曲线几何表示
- **Geom_BezierSurface**: 贝塞尔曲面几何表示
- **Geom_BSplineCurve**: B样条曲线几何表示
- **BRepBuilderAPI_MakeEdge**: 将几何曲线转换为拓扑边
- **BRepBuilderAPI_MakeFace**: 将几何曲面转换为拓扑面

### 2. Coin3D渲染
- **SoIndexedLineSet**: 渲染曲线线段
- **SoIndexedFaceSet**: 渲染曲面三角网格
- **SoCoordinate3**: 定义顶点坐标
- **SoMaterial**: 设置材质属性

### 3. 数学算法
- **De Casteljau算法**: 贝塞尔曲线求值
- **Cox-de Boor算法**: B样条曲线求值
- **曲面求值**: 双参数贝塞尔曲面求值

## 性能优化

### 1. 采样策略
```cpp
// 自适应采样 - 根据曲线复杂度调整采样密度
int calculateOptimalSamples(const std::vector<gp_Pnt>& controlPoints) {
    double totalLength = 0;
    for (size_t i = 1; i < controlPoints.size(); ++i) {
        totalLength += controlPoints[i].Distance(controlPoints[i-1]);
    }
    return std::max(10, std::min(100, static_cast<int>(totalLength * 10)));
}
```

### 2. 缓存机制
```cpp
// 缓存已计算的曲线点
static std::map<std::vector<gp_Pnt>, std::vector<gp_Pnt>> curveCache;

std::vector<gp_Pnt> getCachedCurvePoints(const std::vector<gp_Pnt>& controlPoints, int samples) {
    auto key = std::make_pair(controlPoints, samples);
    if (curveCache.find(key) != curveCache.end()) {
        return curveCache[key];
    }
    
    std::vector<gp_Pnt> points = sampleCurve(controlPoints, samples, true);
    curveCache[key] = points;
    return points;
}
```

## 扩展功能

### 1. 曲线编辑
```cpp
// 移动控制点
void moveControlPoint(std::vector<gp_Pnt>& controlPoints, int index, const gp_Pnt& newPosition) {
    if (index >= 0 && index < static_cast<int>(controlPoints.size())) {
        controlPoints[index] = newPosition;
    }
}
```

### 2. 曲线分析
```cpp
// 计算曲线长度
double calculateCurveLength(const std::vector<gp_Pnt>& controlPoints, int samples = 100) {
    std::vector<gp_Pnt> points = sampleCurve(controlPoints, samples, true);
    double length = 0;
    for (size_t i = 1; i < points.size(); ++i) {
        length += points[i].Distance(points[i-1]);
    }
    return length;
}
```

### 3. 曲线插值
```cpp
// 通过点插值创建贝塞尔曲线
std::vector<gp_Pnt> interpolateBezierCurve(const std::vector<gp_Pnt>& points) {
    // 使用最小二乘法或其他插值算法
    // 这里简化处理，直接使用输入点作为控制点
    return points;
}
```

## 总结

通过OpenCASCADE集成，本项目现在支持完整的贝塞尔曲线和曲面功能：

1. **完整的曲线类型支持**: 贝塞尔、B样条、NURBS
2. **高质量渲染**: 基于Coin3D的平滑渲染
3. **灵活的使用方式**: 可直接创建几何体或渲染节点
4. **性能优化**: 自适应采样和缓存机制
5. **易于扩展**: 模块化设计，便于添加新功能

这个集成方案充分利用了OpenCASCADE强大的几何处理能力和Coin3D优秀的渲染性能，为CAD应用提供了完整的曲线曲面支持。 