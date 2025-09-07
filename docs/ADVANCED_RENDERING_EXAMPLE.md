# 高级几何渲染系统使用示例

## 概述

本项目已成功实现了集成贝塞尔曲线和网格细分技术的高级几何渲染系统。该系统提供了多种渲染模式和质量级别，能够生成高质量的几何渲染结果。

## 主要功能

### ✅ 渲染模式
- **WIREFRAME**: 线框渲染
- **SOLID**: 实体渲染
- **SMOOTH**: 平滑着色
- **SUBDIVIDED**: 细分曲面
- **HYBRID**: 混合模式

### ✅ 质量级别
- **LOW**: 基础细分，无平滑
- **MEDIUM**: 自适应细分，基础平滑
- **HIGH**: 细分曲面，高级平滑
- **ULTRA**: 最高质量，多级细分

### ✅ 贝塞尔曲线支持
- 高质量曲线渲染
- 控制点显示
- 控制多边形显示
- 可配置采样密度

### ✅ 网格细分技术
- Catmull-Clark细分
- Loop细分
- 法线平滑
- 自适应细分

## 使用示例

### 1. 基本使用

```cpp
#include "AdvancedGeometryRenderer.h"
#include "OCCShapeBuilder.h"

// 创建渲染器
AdvancedGeometryRenderer renderer(sceneManager);

// 设置质量级别
renderer.setQualityLevel(AdvancedGeometryRenderer::QualityLevel::HIGH);

// 设置渲染模式
renderer.setRenderingMode(AdvancedGeometryRenderer::RenderingMode::HYBRID);

// 渲染几何体
TopoDS_Shape sphere = OCCShapeBuilder::createSphere(1.0);
auto result = renderer.renderGeometry(sphere, "MySphere");

if (result.success) {
    renderer.addToScene(result);
}
```

### 2. 贝塞尔曲线渲染

```cpp
// 创建贝塞尔曲线控制点
std::vector<gp_Pnt> controlPoints = {
    gp_Pnt(0, 0, 0),
    gp_Pnt(1, 1, 0),
    gp_Pnt(2, 0, 0),
    gp_Pnt(3, 1, 0)
};

// 配置贝塞尔选项
AdvancedGeometryRenderer::BezierOptions bezierOpts;
bezierOpts.samples = 100;
bezierOpts.thickness = 0.05;
bezierOpts.showControlPoints = true;
bezierOpts.showControlPolygon = true;
bezierOpts.curveColor = Quantity_Color(0.0, 0.0, 1.0, Quantity_TOC_RGB);
bezierOpts.controlColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);

// 渲染贝塞尔曲线
auto curveResult = renderer.renderBezierCurveAdvanced(controlPoints, "MyBezierCurve", bezierOpts);
if (curveResult.success) {
    renderer.addToScene(curveResult);
}
```

### 3. 贝塞尔曲面渲染

```cpp
// 创建贝塞尔曲面控制点网格
std::vector<std::vector<gp_Pnt>> controlGrid = {
    {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 1), gp_Pnt(2, 0, 0)},
    {gp_Pnt(0, 1, 1), gp_Pnt(1, 1, 2), gp_Pnt(2, 1, 1)},
    {gp_Pnt(0, 2, 0), gp_Pnt(1, 2, 1), gp_Pnt(2, 2, 0)}
};

// 渲染贝塞尔曲面
auto surfaceResult = renderer.renderBezierSurface(controlGrid, "MyBezierSurface");
if (surfaceResult.success) {
    renderer.addToScene(surfaceResult);
}
```

### 4. B样条和NURBS曲线

```cpp
// B样条曲线
std::vector<gp_Pnt> poles = {
    gp_Pnt(0, 0, 0), gp_Pnt(1, 1, 0), gp_Pnt(2, 0, 0), gp_Pnt(3, 1, 0)
};
std::vector<double> weights = {1.0, 1.0, 1.0, 1.0};

auto bsplineResult = renderer.renderBSplineCurve(poles, weights, "MyBSpline");
if (bsplineResult.success) {
    renderer.addToScene(bsplineResult);
}

// NURBS曲线
std::vector<double> nurbsWeights = {1.0, 2.0, 1.0, 2.0};
auto nurbsResult = renderer.renderNURBSCurve(poles, nurbsWeights, "MyNURBS");
if (nurbsResult.success) {
    renderer.addToScene(nurbsResult);
}
```

### 5. 高级渲染选项

```cpp
// 配置表面渲染选项
AdvancedGeometryRenderer::SurfaceOptions surfaceOpts;
surfaceOpts.useSubdivision = true;
surfaceOpts.subdivisionLevels = 3;
surfaceOpts.useNormalSmoothing = true;
surfaceOpts.creaseAngle = 45.0;
surfaceOpts.smoothingIterations = 5;
surfaceOpts.useAdaptiveTessellation = true;
surfaceOpts.maxEdgeLength = 0.05;
surfaceOpts.curvatureThreshold = 0.05;
surfaceOpts.quality = AdvancedGeometryRenderer::QualityLevel::ULTRA;

// 应用高级渲染
TopoDS_Shape complexShape = OCCShapeBuilder::createTorus(1.0, 0.3);
auto advancedResult = renderer.renderGeometryAdvanced(complexShape, "ComplexShape", surfaceOpts);
if (advancedResult.success) {
    renderer.addToScene(advancedResult);
}
```

### 6. 批量渲染

```cpp
// 准备批量渲染的几何体
std::vector<std::pair<TopoDS_Shape, std::string>> geometries = {
    {OCCShapeBuilder::createSphere(1.0), "Sphere1"},
    {OCCShapeBuilder::createBox(1.0, 1.0, 1.0), "Box1"},
    {OCCShapeBuilder::createCylinder(0.5, 2.0), "Cylinder1"},
    {OCCShapeBuilder::createCone(0.5, 1.0), "Cone1"}
};

// 批量渲染
auto batchResults = renderer.renderGeometryBatch(geometries);

// 添加到场景
for (const auto& result : batchResults) {
    if (result.success) {
        renderer.addToScene(result);
    }
}
```

### 7. 性能监控

```cpp
// 获取性能统计
auto stats = renderer.getPerformanceStats();
std::cout << "Performance Statistics:" << std::endl;
std::cout << "Total geometries: " << stats.totalGeometries << std::endl;
std::cout << "Cached geometries: " << stats.cachedGeometries << std::endl;
std::cout << "Total rendering time: " << stats.totalRenderingTime << "s" << std::endl;
std::cout << "Average rendering time: " << stats.averageRenderingTime << "s" << std::endl;
std::cout << "Subdivision operations: " << stats.subdivisionOperations << std::endl;
std::cout << "Smoothing operations: " << stats.smoothingOperations << std::endl;

// 重置性能统计
renderer.resetPerformanceStats();
```

### 8. 缓存管理

```cpp
// 检查缓存
if (renderer.isCached("MyGeometry")) {
    auto cachedResult = renderer.getCachedResult("MyGeometry");
    renderer.addToScene(cachedResult);
}

// 从缓存中移除
renderer.removeFromCache("MyGeometry");

// 清空缓存
renderer.clearCache();
```

### 9. 动态质量调整

```cpp
// 根据用户交互动态调整质量
void onQualityChanged(int qualityLevel) {
    switch (qualityLevel) {
        case 0:
            renderer.setQualityLevel(AdvancedGeometryRenderer::QualityLevel::LOW);
            break;
        case 1:
            renderer.setQualityLevel(AdvancedGeometryRenderer::QualityLevel::MEDIUM);
            break;
        case 2:
            renderer.setQualityLevel(AdvancedGeometryRenderer::QualityLevel::HIGH);
            break;
        case 3:
            renderer.setQualityLevel(AdvancedGeometryRenderer::QualityLevel::ULTRA);
            break;
    }
}

// 根据渲染模式调整
void onRenderingModeChanged(int mode) {
    switch (mode) {
        case 0:
            renderer.setRenderingMode(AdvancedGeometryRenderer::RenderingMode::WIREFRAME);
            break;
        case 1:
            renderer.setRenderingMode(AdvancedGeometryRenderer::RenderingMode::SOLID);
            break;
        case 2:
            renderer.setRenderingMode(AdvancedGeometryRenderer::RenderingMode::SMOOTH);
            break;
        case 3:
            renderer.setRenderingMode(AdvancedGeometryRenderer::RenderingMode::SUBDIVIDED);
            break;
        case 4:
            renderer.setRenderingMode(AdvancedGeometryRenderer::RenderingMode::HYBRID);
            break;
    }
}
```

### 10. 完整应用示例

```cpp
class GeometryRenderingApp {
private:
    AdvancedGeometryRenderer m_renderer;
    SceneManager* m_sceneManager;
    
public:
    GeometryRenderingApp(SceneManager* sceneManager) 
        : m_renderer(sceneManager), m_sceneManager(sceneManager) {
        
        // 初始化渲染器
        m_renderer.setQualityLevel(AdvancedGeometryRenderer::QualityLevel::HIGH);
        m_renderer.setRenderingMode(AdvancedGeometryRenderer::RenderingMode::HYBRID);
    }
    
    void createDemoScene() {
        // 创建基础几何体
        createBasicGeometries();
        
        // 创建贝塞尔曲线
        createBezierCurves();
        
        // 创建贝塞尔曲面
        createBezierSurfaces();
        
        // 创建复杂几何体
        createComplexGeometries();
    }
    
private:
    void createBasicGeometries() {
        // 球体
        auto sphere = OCCShapeBuilder::createSphere(1.0);
        auto sphereResult = m_renderer.renderGeometry(sphere, "DemoSphere");
        if (sphereResult.success) {
            m_renderer.addToScene(sphereResult);
        }
        
        // 立方体
        auto box = OCCShapeBuilder::createBox(1.0, 1.0, 1.0);
        auto boxResult = m_renderer.renderGeometry(box, "DemoBox");
        if (boxResult.success) {
            m_renderer.addToScene(boxResult);
        }
    }
    
    void createBezierCurves() {
        // 创建多条贝塞尔曲线
        std::vector<std::vector<gp_Pnt>> curves = {
            {gp_Pnt(0, 0, 0), gp_Pnt(1, 1, 0), gp_Pnt(2, 0, 0)},
            {gp_Pnt(0, 0, 1), gp_Pnt(1, 1, 1), gp_Pnt(2, 0, 1), gp_Pnt(3, 1, 1)},
            {gp_Pnt(0, 0, 2), gp_Pnt(1, 1, 2), gp_Pnt(2, 0, 2), gp_Pnt(3, 1, 2), gp_Pnt(4, 0, 2)}
        };
        
        for (size_t i = 0; i < curves.size(); ++i) {
            std::string name = "BezierCurve_" + std::to_string(i);
            auto result = m_renderer.renderBezierCurve(curves[i], name);
            if (result.success) {
                m_renderer.addToScene(result);
            }
        }
    }
    
    void createBezierSurfaces() {
        // 创建贝塞尔曲面
        std::vector<std::vector<gp_Pnt>> surfaceGrid = {
            {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 1), gp_Pnt(2, 0, 0)},
            {gp_Pnt(0, 1, 1), gp_Pnt(1, 1, 2), gp_Pnt(2, 1, 1)},
            {gp_Pnt(0, 2, 0), gp_Pnt(1, 2, 1), gp_Pnt(2, 2, 0)}
        };
        
        auto surfaceResult = m_renderer.renderBezierSurface(surfaceGrid, "DemoBezierSurface");
        if (surfaceResult.success) {
            m_renderer.addToScene(surfaceResult);
        }
    }
    
    void createComplexGeometries() {
        // 创建复杂几何体（环面）
        auto torus = OCCShapeBuilder::createTorus(1.0, 0.3);
        
        // 使用高级渲染选项
        AdvancedGeometryRenderer::SurfaceOptions advancedOpts;
        advancedOpts.useSubdivision = true;
        advancedOpts.subdivisionLevels = 3;
        advancedOpts.useNormalSmoothing = true;
        advancedOpts.quality = AdvancedGeometryRenderer::QualityLevel::ULTRA;
        
        auto torusResult = m_renderer.renderGeometryAdvanced(torus, "DemoTorus", advancedOpts);
        if (torusResult.success) {
            m_renderer.addToScene(torusResult);
        }
    }
};
```

## 性能优化建议

### 1. 缓存策略
- 对于静态几何体，充分利用缓存机制
- 定期清理不需要的缓存项
- 使用合适的缓存键生成策略

### 2. 质量级别选择
- 交互时使用LOW或MEDIUM质量
- 静止时使用HIGH或ULTRA质量
- 根据硬件性能动态调整

### 3. 批量处理
- 使用批量渲染减少开销
- 合并相似的渲染操作
- 利用并行处理能力

### 4. 内存管理
- 及时释放不需要的渲染结果
- 监控内存使用情况
- 使用智能指针管理资源

## 总结

通过这个高级几何渲染系统，项目现在具备了：

1. **完整的贝塞尔曲线支持**: 包括曲线、曲面、B样条和NURBS
2. **先进的网格细分技术**: 多种细分算法和自适应处理
3. **灵活的质量控制**: 多种质量级别和渲染模式
4. **高效的缓存机制**: 提高渲染性能
5. **完善的性能监控**: 实时跟踪渲染性能
6. **易于使用的API**: 简洁的接口设计

这个系统为CAD应用提供了专业的几何渲染能力，能够生成高质量的视觉效果，同时保持良好的性能表现。 