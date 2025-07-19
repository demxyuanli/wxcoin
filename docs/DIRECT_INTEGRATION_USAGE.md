# 直接集成到现有渲染系统的使用指南

## 概述

本指南展示如何直接使用集成到现有OCCMeshConverter和OCCViewer中的高级渲染功能，无需额外的集成类。

## 快速开始

### 1. 启用高级渲染

```cpp
// 在OCCViewer中启用高级渲染
occViewer->enableAdvancedRendering(true);

// 设置质量级别
occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::HIGH);

// 设置渲染模式
occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::HYBRID);
```

### 2. 创建高级渲染几何体

```cpp
// 使用高级渲染创建几何体
auto sphere = occViewer->addGeometryWithAdvancedRendering(
    OCCShapeBuilder::createSphere(1.0), "AdvancedSphere");

auto box = occViewer->addGeometryWithAdvancedRendering(
    OCCShapeBuilder::createBox(1.0, 1.0, 1.0), "AdvancedBox");

auto cylinder = occViewer->addGeometryWithAdvancedRendering(
    OCCShapeBuilder::createCylinder(0.5, 2.0), "AdvancedCylinder");
```

### 3. 创建贝塞尔曲线和曲面

```cpp
// 创建贝塞尔曲线
std::vector<gp_Pnt> curvePoints = {
    gp_Pnt(0, 0, 0),
    gp_Pnt(1, 1, 0),
    gp_Pnt(2, 0, 0),
    gp_Pnt(3, 1, 0)
};
auto bezierCurve = occViewer->addBezierCurve(curvePoints, "MyBezierCurve");

// 创建贝塞尔曲面
std::vector<std::vector<gp_Pnt>> surfaceGrid = {
    {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 1), gp_Pnt(2, 0, 0)},
    {gp_Pnt(0, 1, 1), gp_Pnt(1, 1, 2), gp_Pnt(2, 1, 1)},
    {gp_Pnt(0, 2, 0), gp_Pnt(1, 2, 1), gp_Pnt(2, 2, 0)}
};
auto bezierSurface = occViewer->addBezierSurface(surfaceGrid, "MyBezierSurface");

// 创建B样条曲线
std::vector<gp_Pnt> poles = {
    gp_Pnt(0, 0, 0), gp_Pnt(1, 1, 0), gp_Pnt(2, 0, 0), gp_Pnt(3, 1, 0)
};
std::vector<double> weights = {1.0, 1.0, 1.0, 1.0};
auto bsplineCurve = occViewer->addBSplineCurve(poles, weights, "MyBSplineCurve");
```

## 修改现有命令

### 1. 修改CreateSphereListener

```cpp
// 在CreateSphereListener.cpp中
void CreateSphereListener::execute()
{
    double radius = 1.0;
    TopoDS_Shape sphere = OCCShapeBuilder::createSphere(radius);
    
    // 使用高级渲染（如果启用）
    if (m_occViewer->isAdvancedRenderingEnabled()) {
        m_occViewer->addGeometryWithAdvancedRendering(sphere, "AdvancedSphere");
    } else {
        // 回退到标准渲染
        auto geometry = std::make_shared<OCCGeometry>("Sphere", sphere);
        m_occViewer->addGeometry(geometry);
    }
}
```

### 2. 修改CreateBoxListener

```cpp
// 在CreateBoxListener.cpp中
void CreateBoxListener::execute()
{
    double length = 1.0, width = 1.0, height = 1.0;
    TopoDS_Shape box = OCCShapeBuilder::createBox(length, width, height);
    
    // 使用高级渲染（如果启用）
    if (m_occViewer->isAdvancedRenderingEnabled()) {
        m_occViewer->addGeometryWithAdvancedRendering(box, "AdvancedBox");
    } else {
        // 回退到标准渲染
        auto geometry = std::make_shared<OCCGeometry>("Box", box);
        m_occViewer->addGeometry(geometry);
    }
}
```

### 3. 修改CreateCylinderListener

```cpp
// 在CreateCylinderListener.cpp中
void CreateCylinderListener::execute()
{
    double radius = 0.5, height = 2.0;
    TopoDS_Shape cylinder = OCCShapeBuilder::createCylinder(radius, height);
    
    // 使用高级渲染（如果启用）
    if (m_occViewer->isAdvancedRenderingEnabled()) {
        m_occViewer->addGeometryWithAdvancedRendering(cylinder, "AdvancedCylinder");
    } else {
        // 回退到标准渲染
        auto geometry = std::make_shared<OCCGeometry>("Cylinder", cylinder);
        m_occViewer->addGeometry(geometry);
    }
}
```

## 质量控制

### 1. 设置质量级别

```cpp
// 低质量 - 快速渲染
occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::LOW);

// 中等质量 - 平衡性能和质量
occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::MEDIUM);

// 高质量 - 推荐设置
occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::HIGH);

// 超高质量 - 最高质量
occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::ULTRA);
```

### 2. 设置渲染模式

```cpp
// 线框模式
occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::WIREFRAME);

// 实体模式
occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::SOLID);

// 平滑模式
occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::SMOOTH);

// 细分模式
occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::SUBDIVIDED);

// 混合模式（推荐）
occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::HYBRID);
```

### 3. 精细控制

```cpp
// 设置细分级别
occViewer->setAdvancedSubdivisionLevels(3);

// 设置平滑参数
occViewer->setAdvancedSmoothingParameters(25.0, 5); // 25度折痕角，5次迭代

// 设置细分参数
occViewer->setAdvancedTessellationParameters(0.05, 0.05); // 更精细的细分
```

## 升级现有几何体

### 1. 升级单个几何体

```cpp
// 升级特定的几何体
occViewer->upgradeGeometryToAdvanced("ExistingSphere");
occViewer->upgradeGeometryToAdvanced("ExistingBox");
```

### 2. 升级所有几何体

```cpp
// 升级所有现有几何体到高级渲染
occViewer->upgradeAllGeometriesToAdvanced();
```

## 性能优化

### 1. 缓存管理

```cpp
// 清除渲染缓存以释放内存
occViewer->clearAdvancedRenderingCache();
```

### 2. 动态质量调整

```cpp
// 在交互时降低质量
void onMouseDown()
{
    occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::LOW);
}

void onMouseUp()
{
    occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::HIGH);
}
```

## 用户界面集成

### 1. 添加菜单项

```cpp
// 在菜单创建函数中
void createAdvancedRenderingMenu()
{
    wxMenu* advancedMenu = new wxMenu();
    
    // 启用/禁用高级渲染
    advancedMenu->AppendCheckItem(ID_ADVANCED_RENDERING, "Enable Advanced Rendering");
    
    // 质量级别子菜单
    wxMenu* qualityMenu = new wxMenu();
    qualityMenu->Append(ID_QUALITY_LOW, "Low Quality");
    qualityMenu->Append(ID_QUALITY_MEDIUM, "Medium Quality");
    qualityMenu->Append(ID_QUALITY_HIGH, "High Quality");
    qualityMenu->Append(ID_QUALITY_ULTRA, "Ultra Quality");
    advancedMenu->AppendSubMenu(qualityMenu, "Quality Level");
    
    // 渲染模式子菜单
    wxMenu* modeMenu = new wxMenu();
    modeMenu->Append(ID_MODE_WIREFRAME, "Wireframe");
    modeMenu->Append(ID_MODE_SOLID, "Solid");
    modeMenu->Append(ID_MODE_SMOOTH, "Smooth");
    modeMenu->Append(ID_MODE_SUBDIVIDED, "Subdivided");
    modeMenu->Append(ID_MODE_HYBRID, "Hybrid");
    advancedMenu->AppendSubMenu(modeMenu, "Rendering Mode");
    
    // 高级功能
    advancedMenu->AppendSeparator();
    advancedMenu->Append(ID_UPGRADE_ALL, "Upgrade All to Advanced");
    advancedMenu->Append(ID_CLEAR_CACHE, "Clear Rendering Cache");
    
    // 添加到主菜单栏
    menuBar->Append(advancedMenu, "Advanced Rendering");
}
```

### 2. 处理菜单事件

```cpp
void onAdvancedRenderingToggled(wxCommandEvent& event)
{
    bool enabled = event.IsChecked();
    occViewer->enableAdvancedRendering(enabled);
    
    if (enabled) {
        // 自动升级现有几何体
        occViewer->upgradeAllGeometriesToAdvanced();
    }
}

void onQualityLevelSelected(wxCommandEvent& event)
{
    int id = event.GetId();
    
    switch (id) {
        case ID_QUALITY_LOW:
            occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::LOW);
            break;
        case ID_QUALITY_MEDIUM:
            occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::MEDIUM);
            break;
        case ID_QUALITY_HIGH:
            occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::HIGH);
            break;
        case ID_QUALITY_ULTRA:
            occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::ULTRA);
            break;
    }
}

void onRenderingModeSelected(wxCommandEvent& event)
{
    int id = event.GetId();
    
    switch (id) {
        case ID_MODE_WIREFRAME:
            occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::WIREFRAME);
            break;
        case ID_MODE_SOLID:
            occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::SOLID);
            break;
        case ID_MODE_SMOOTH:
            occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::SMOOTH);
            break;
        case ID_MODE_SUBDIVIDED:
            occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::SUBDIVIDED);
            break;
        case ID_MODE_HYBRID:
            occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::HYBRID);
            break;
    }
}
```

## 完整示例

### 1. 初始化高级渲染

```cpp
class MainApplication {
private:
    OCCViewer* m_occViewer;
    
public:
    void initializeAdvancedRendering()
    {
        // 启用高级渲染
        m_occViewer->enableAdvancedRendering(true);
        
        // 设置默认质量
        m_occViewer->setAdvancedQualityLevel(OCCMeshConverter::QualityLevel::HIGH);
        m_occViewer->setAdvancedRenderingMode(OCCMeshConverter::RenderingMode::HYBRID);
        
        // 设置细分参数
        m_occViewer->setAdvancedSubdivisionLevels(2);
        m_occViewer->setAdvancedSmoothingParameters(30.0, 3);
        m_occViewer->setAdvancedTessellationParameters(0.1, 0.1);
        
        LOG_INF_S("Advanced rendering initialized");
    }
    
    void createDemoScene()
    {
        // 创建基础几何体（自动使用高级渲染）
        auto sphere = m_occViewer->addGeometryWithAdvancedRendering(
            OCCShapeBuilder::createSphere(1.0), "DemoSphere");
        
        auto box = m_occViewer->addGeometryWithAdvancedRendering(
            OCCShapeBuilder::createBox(1.0, 1.0, 1.0), "DemoBox");
        
        auto cylinder = m_occViewer->addGeometryWithAdvancedRendering(
            OCCShapeBuilder::createCylinder(0.5, 2.0), "DemoCylinder");
        
        // 创建贝塞尔曲线
        std::vector<gp_Pnt> curvePoints = {
            gp_Pnt(0, 0, 0), gp_Pnt(1, 1, 0), gp_Pnt(2, 0, 0), gp_Pnt(3, 1, 0)
        };
        auto bezierCurve = m_occViewer->addBezierCurve(curvePoints, "DemoBezierCurve");
        
        // 创建贝塞尔曲面
        std::vector<std::vector<gp_Pnt>> surfaceGrid = {
            {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 1), gp_Pnt(2, 0, 0)},
            {gp_Pnt(0, 1, 1), gp_Pnt(1, 1, 2), gp_Pnt(2, 1, 1)},
            {gp_Pnt(0, 2, 0), gp_Pnt(1, 2, 1), gp_Pnt(2, 2, 0)}
        };
        auto bezierSurface = m_occViewer->addBezierSurface(surfaceGrid, "DemoBezierSurface");
        
        LOG_INF_S("Demo scene created with advanced rendering");
    }
};
```

### 2. 性能监控

```cpp
void monitorPerformance()
{
    // 检查高级渲染是否启用
    if (m_occViewer->isAdvancedRenderingEnabled()) {
        LOG_INF_S("Advanced rendering is enabled");
        
        // 获取几何体数量
        auto geometries = m_occViewer->getAllGeometry();
        LOG_INF_S("Total geometries: " + std::to_string(geometries.size()));
        
        // 清除缓存以优化性能
        m_occViewer->clearAdvancedRenderingCache();
    } else {
        LOG_INF_S("Advanced rendering is disabled");
    }
}
```

## 优势

1. **无缝集成**: 直接集成到现有系统，无需额外类
2. **向后兼容**: 保持现有API不变
3. **渐进式采用**: 可以逐步启用高级渲染
4. **性能优化**: 内置缓存和自适应调整
5. **易于使用**: 简单的API调用即可启用高级功能

## 总结

通过这种直接集成的方式，您可以：

- 在现有代码中轻松启用高级渲染
- 创建高质量的贝塞尔曲线和曲面
- 使用先进的细分和平滑技术
- 动态调整渲染质量
- 保持代码的简洁性和可维护性

这种集成方案让高级渲染功能成为现有系统的一部分，而不是额外的组件，提供了最佳的开发体验和性能。 