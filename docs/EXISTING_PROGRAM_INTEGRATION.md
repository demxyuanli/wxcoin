# 在现有程序中使用高级渲染器

## 概述

本指南展示如何在现有的wxcoin程序中集成和使用高级几何渲染器，包括贝塞尔曲线和网格细分技术。

## 集成方式

### 1. 基本集成

在现有的`MainApplication`或`Canvas`类中添加高级渲染器：

```cpp
// 在头文件中添加
#include "AdvancedRenderingIntegration.h"

class MainApplication {
private:
    // 现有成员
    SceneManager* m_sceneManager;
    OCCViewer* m_occViewer;
    
    // 新增：高级渲染集成
    std::unique_ptr<AdvancedRenderingIntegration> m_advancedRendering;
    
public:
    void initializeAdvancedRendering();
    void createDemoScene();
    void onQualityChanged(int qualityLevel);
    void onRenderingModeChanged(int mode);
};
```

### 2. 初始化高级渲染器

```cpp
void MainApplication::initializeAdvancedRendering()
{
    // 创建高级渲染集成
    m_advancedRendering = std::make_unique<AdvancedRenderingIntegration>(
        m_sceneManager, m_occViewer);
    
    // 初始化，使用混合模式
    m_advancedRendering->initialize(
        AdvancedRenderingIntegration::IntegrationMode::HYBRID_MODE);
    
    // 设置默认质量预设
    m_advancedRendering->setQualityPreset(
        AdvancedRenderingIntegration::QualityPreset::STANDARD);
    
    LOG_INF_S("Advanced rendering system initialized");
}
```

## 使用示例

### 1. 替换现有的几何创建命令

**原来的方式：**
```cpp
// 在CreateSphereListener.cpp中
void CreateSphereListener::execute()
{
    TopoDS_Shape sphere = OCCShapeBuilder::createSphere(1.0);
    auto geometry = std::make_shared<OCCGeometry>("Sphere", sphere);
    m_occViewer->addGeometry(geometry);
}
```

**使用高级渲染器的方式：**
```cpp
// 在CreateSphereListener.cpp中
void CreateSphereListener::execute()
{
    TopoDS_Shape sphere = OCCShapeBuilder::createSphere(1.0);
    
    // 使用高级渲染器创建几何体
    auto geometry = m_advancedRendering->addGeometryWithAdvancedRendering(
        sphere, "AdvancedSphere");
    
    if (geometry) {
        LOG_INF_S("Created sphere with advanced rendering");
    }
}
```

### 2. 添加贝塞尔曲线

```cpp
void MainApplication::createBezierCurveExample()
{
    // 创建贝塞尔曲线控制点
    std::vector<gp_Pnt> controlPoints = {
        gp_Pnt(0, 0, 0),
        gp_Pnt(1, 1, 0),
        gp_Pnt(2, 0, 0),
        gp_Pnt(3, 1, 0)
    };
    
    // 使用高级渲染器创建贝塞尔曲线
    auto curve = m_advancedRendering->addBezierCurve(controlPoints, "DemoBezierCurve");
    
    if (curve) {
        LOG_INF_S("Created Bezier curve with advanced rendering");
    }
}
```

### 3. 添加贝塞尔曲面

```cpp
void MainApplication::createBezierSurfaceExample()
{
    // 创建贝塞尔曲面控制点网格
    std::vector<std::vector<gp_Pnt>> controlGrid = {
        {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 1), gp_Pnt(2, 0, 0)},
        {gp_Pnt(0, 1, 1), gp_Pnt(1, 1, 2), gp_Pnt(2, 1, 1)},
        {gp_Pnt(0, 2, 0), gp_Pnt(1, 2, 1), gp_Pnt(2, 2, 0)}
    };
    
    // 使用高级渲染器创建贝塞尔曲面
    auto surface = m_advancedRendering->addBezierSurface(controlGrid, "DemoBezierSurface");
    
    if (surface) {
        LOG_INF_S("Created Bezier surface with advanced rendering");
    }
}
```

### 4. 升级现有几何体

```cpp
void MainApplication::upgradeExistingGeometries()
{
    // 升级单个几何体
    m_advancedRendering->upgradeGeometryToAdvanced("ExistingSphere");
    
    // 升级所有几何体
    m_advancedRendering->upgradeAllGeometriesToAdvanced();
    
    // 刷新场景
    m_advancedRendering->refreshScene();
}
```

## 用户界面集成

### 1. 添加质量控制菜单

```cpp
// 在菜单创建函数中
void MainApplication::createRenderingMenu()
{
    wxMenu* renderingMenu = new wxMenu();
    
    // 质量预设菜单
    wxMenu* qualityMenu = new wxMenu();
    qualityMenu->Append(ID_QUALITY_INTERACTIVE, "Interactive");
    qualityMenu->Append(ID_QUALITY_STANDARD, "Standard");
    qualityMenu->Append(ID_QUALITY_HIGH, "High Quality");
    qualityMenu->Append(ID_QUALITY_ULTRA, "Ultra Quality");
    
    renderingMenu->AppendSubMenu(qualityMenu, "Quality Preset");
    
    // 渲染模式菜单
    wxMenu* modeMenu = new wxMenu();
    modeMenu->Append(ID_MODE_WIREFRAME, "Wireframe");
    modeMenu->Append(ID_MODE_SOLID, "Solid");
    modeMenu->Append(ID_MODE_SMOOTH, "Smooth");
    modeMenu->Append(ID_MODE_SUBDIVIDED, "Subdivided");
    modeMenu->Append(ID_MODE_HYBRID, "Hybrid");
    
    renderingMenu->AppendSubMenu(modeMenu, "Rendering Mode");
    
    // 高级功能菜单
    renderingMenu->AppendSeparator();
    renderingMenu->Append(ID_UPGRADE_ALL, "Upgrade All to Advanced");
    renderingMenu->Append(ID_OPTIMIZE_PERFORMANCE, "Optimize for Performance");
    renderingMenu->Append(ID_OPTIMIZE_QUALITY, "Optimize for Quality");
    
    // 添加到主菜单栏
    menuBar->Append(renderingMenu, "Advanced Rendering");
}
```

### 2. 处理菜单事件

```cpp
void MainApplication::onQualityPresetSelected(wxCommandEvent& event)
{
    int id = event.GetId();
    
    switch (id) {
        case ID_QUALITY_INTERACTIVE:
            m_advancedRendering->setQualityPreset(
                AdvancedRenderingIntegration::QualityPreset::INTERACTIVE);
            break;
        case ID_QUALITY_STANDARD:
            m_advancedRendering->setQualityPreset(
                AdvancedRenderingIntegration::QualityPreset::STANDARD);
            break;
        case ID_QUALITY_HIGH:
            m_advancedRendering->setQualityPreset(
                AdvancedRenderingIntegration::QualityPreset::HIGH_QUALITY);
            break;
        case ID_QUALITY_ULTRA:
            m_advancedRendering->setQualityPreset(
                AdvancedRenderingIntegration::QualityPreset::ULTRA_QUALITY);
            break;
    }
    
    m_advancedRendering->refreshScene();
}

void MainApplication::onRenderingModeSelected(wxCommandEvent& event)
{
    int id = event.GetId();
    
    switch (id) {
        case ID_MODE_WIREFRAME:
            m_advancedRendering->setRenderingMode(
                AdvancedGeometryRenderer::RenderingMode::WIREFRAME);
            break;
        case ID_MODE_SOLID:
            m_advancedRendering->setRenderingMode(
                AdvancedGeometryRenderer::RenderingMode::SOLID);
            break;
        case ID_MODE_SMOOTH:
            m_advancedRendering->setRenderingMode(
                AdvancedGeometryRenderer::RenderingMode::SMOOTH);
            break;
        case ID_MODE_SUBDIVIDED:
            m_advancedRendering->setRenderingMode(
                AdvancedGeometryRenderer::RenderingMode::SUBDIVIDED);
            break;
        case ID_MODE_HYBRID:
            m_advancedRendering->setRenderingMode(
                AdvancedGeometryRenderer::RenderingMode::HYBRID);
            break;
    }
    
    m_advancedRendering->refreshScene();
}
```

### 3. 添加性能监控面板

```cpp
class PerformancePanel : public wxPanel {
private:
    AdvancedRenderingIntegration* m_advancedRendering;
    wxTextCtrl* m_metricsText;
    wxTimer m_updateTimer;
    
public:
    PerformancePanel(wxWindow* parent, AdvancedRenderingIntegration* advancedRendering)
        : wxPanel(parent), m_advancedRendering(advancedRendering)
    {
        m_metricsText = new wxTextCtrl(this, wxID_ANY, "", 
                                      wxDefaultPosition, wxDefaultSize, 
                                      wxTE_MULTILINE | wxTE_READONLY);
        
        m_updateTimer.Bind(wxEVT_TIMER, &PerformancePanel::onUpdateTimer, this);
        m_updateTimer.Start(1000); // Update every second
    }
    
private:
    void onUpdateTimer(wxTimerEvent& event)
    {
        if (m_advancedRendering) {
            auto metrics = m_advancedRendering->getPerformanceMetrics();
            
            std::stringstream ss;
            ss << "Performance Metrics:\n";
            ss << "Total Geometries: " << metrics.totalGeometries << "\n";
            ss << "Advanced Geometries: " << metrics.advancedGeometries << "\n";
            ss << "Average Rendering Time: " << std::fixed << std::setprecision(3) 
               << metrics.averageRenderingTime << "s\n";
            ss << "Memory Usage: " << std::fixed << std::setprecision(2) 
               << metrics.memoryUsage << " MB\n";
            
            m_metricsText->SetValue(ss.str());
        }
    }
};
```

## 修改现有命令

### 1. 修改CreateBoxListener

```cpp
// 在CreateBoxListener.h中添加
#include "AdvancedRenderingIntegration.h"

class CreateBoxListener : public CommandListener {
private:
    AdvancedRenderingIntegration* m_advancedRendering;
    
public:
    CreateBoxListener(AdvancedRenderingIntegration* advancedRendering)
        : m_advancedRendering(advancedRendering) {}
    
    void execute() override;
};
```

```cpp
// 在CreateBoxListener.cpp中
void CreateBoxListener::execute()
{
    // 获取参数（假设从某个地方获取）
    double length = 1.0, width = 1.0, height = 1.0;
    
    // 创建盒子形状
    TopoDS_Shape box = OCCShapeBuilder::createBox(length, width, height);
    
    // 使用高级渲染器
    if (m_advancedRendering) {
        auto geometry = m_advancedRendering->addGeometryWithAdvancedRendering(
            box, "AdvancedBox");
        
        if (geometry) {
            LOG_INF_S("Created box with advanced rendering");
        }
    } else {
        // 回退到标准渲染
        auto geometry = std::make_shared<OCCGeometry>("Box", box);
        // 假设有访问OCCViewer的方式
        // m_occViewer->addGeometry(geometry);
    }
}
```

### 2. 修改其他创建命令

类似地修改其他几何体创建命令：

```cpp
// CreateSphereListener
void CreateSphereListener::execute()
{
    double radius = 1.0;
    TopoDS_Shape sphere = OCCShapeBuilder::createSphere(radius);
    
    if (m_advancedRendering) {
        m_advancedRendering->addGeometryWithAdvancedRendering(sphere, "AdvancedSphere");
    }
}

// CreateCylinderListener
void CreateCylinderListener::execute()
{
    double radius = 0.5, height = 2.0;
    TopoDS_Shape cylinder = OCCShapeBuilder::createCylinder(radius, height);
    
    if (m_advancedRendering) {
        m_advancedRendering->addGeometryWithAdvancedRendering(cylinder, "AdvancedCylinder");
    }
}

// CreateConeListener
void CreateConeListener::execute()
{
    double radius = 0.5, height = 1.0;
    TopoDS_Shape cone = OCCShapeBuilder::createCone(radius, height);
    
    if (m_advancedRendering) {
        m_advancedRendering->addGeometryWithAdvancedRendering(cone, "AdvancedCone");
    }
}
```

## 动态质量调整

### 1. 根据用户交互调整质量

```cpp
class MainApplication {
private:
    bool m_isInteracting = false;
    
public:
    void onMouseDown(wxMouseEvent& event)
    {
        m_isInteracting = true;
        m_advancedRendering->setInteractiveMode(true);
    }
    
    void onMouseUp(wxMouseEvent& event)
    {
        m_isInteracting = false;
        m_advancedRendering->setInteractiveMode(false);
    }
    
    void onMouseMove(wxMouseEvent& event)
    {
        if (m_isInteracting) {
            // 在交互过程中保持低质量模式
            m_advancedRendering->setQualityPreset(
                AdvancedRenderingIntegration::QualityPreset::INTERACTIVE);
        }
    }
};
```

### 2. 根据硬件性能自动调整

```cpp
void MainApplication::autoAdjustQuality()
{
    // 获取性能指标
    auto metrics = m_advancedRendering->getPerformanceMetrics();
    
    // 根据平均渲染时间调整质量
    if (metrics.averageRenderingTime > 0.1) { // 超过100ms
        m_advancedRendering->optimizeForPerformance();
        LOG_INF_S("Auto-adjusted to performance mode due to slow rendering");
    } else if (metrics.averageRenderingTime < 0.01) { // 低于10ms
        m_advancedRendering->optimizeForQuality();
        LOG_INF_S("Auto-adjusted to quality mode due to fast rendering");
    }
}
```

## 完整集成示例

### 1. 修改MainApplication

```cpp
// MainApplication.h
class MainApplication {
private:
    // 现有成员
    SceneManager* m_sceneManager;
    OCCViewer* m_occViewer;
    
    // 新增：高级渲染集成
    std::unique_ptr<AdvancedRenderingIntegration> m_advancedRendering;
    
    // 命令监听器（需要修改以支持高级渲染）
    std::unique_ptr<CreateBoxListener> m_createBoxListener;
    std::unique_ptr<CreateSphereListener> m_createSphereListener;
    std::unique_ptr<CreateCylinderListener> m_createCylinderListener;
    std::unique_ptr<CreateConeListener> m_createConeListener;
    
public:
    MainApplication();
    ~MainApplication();
    
    void initialize();
    void createUI();
    void createDemoScene();
    
    // 高级渲染相关
    void initializeAdvancedRendering();
    void onQualityPresetSelected(wxCommandEvent& event);
    void onRenderingModeSelected(wxCommandEvent& event);
    void onUpgradeAllGeometries(wxCommandEvent& event);
    void onOptimizePerformance(wxCommandEvent& event);
    void onOptimizeQuality(wxCommandEvent& event);
    
    // 获取器
    AdvancedRenderingIntegration* getAdvancedRendering() { return m_advancedRendering.get(); }
};
```

### 2. 实现MainApplication

```cpp
// MainApplication.cpp
MainApplication::MainApplication()
{
    // 初始化现有系统
    m_sceneManager = new SceneManager();
    m_occViewer = new OCCViewer(m_sceneManager);
    
    // 初始化高级渲染系统
    initializeAdvancedRendering();
    
    // 创建命令监听器（传入高级渲染器）
    m_createBoxListener = std::make_unique<CreateBoxListener>(m_advancedRendering.get());
    m_createSphereListener = std::make_unique<CreateSphereListener>(m_advancedRendering.get());
    m_createCylinderListener = std::make_unique<CreateCylinderListener>(m_advancedRendering.get());
    m_createConeListener = std::make_unique<CreateConeListener>(m_advancedRendering.get());
}

void MainApplication::initializeAdvancedRendering()
{
    m_advancedRendering = std::make_unique<AdvancedRenderingIntegration>(
        m_sceneManager, m_occViewer);
    
    m_advancedRendering->initialize(
        AdvancedRenderingIntegration::IntegrationMode::HYBRID_MODE);
    
    m_advancedRendering->setQualityPreset(
        AdvancedRenderingIntegration::QualityPreset::STANDARD);
    
    LOG_INF_S("Advanced rendering system initialized");
}

void MainApplication::createDemoScene()
{
    // 创建基础几何体
    auto sphere = m_advancedRendering->addGeometryWithAdvancedRendering(
        OCCShapeBuilder::createSphere(1.0), "DemoSphere");
    
    auto box = m_advancedRendering->addGeometryWithAdvancedRendering(
        OCCShapeBuilder::createBox(1.0, 1.0, 1.0), "DemoBox");
    
    auto cylinder = m_advancedRendering->addGeometryWithAdvancedRendering(
        OCCShapeBuilder::createCylinder(0.5, 2.0), "DemoCylinder");
    
    // 创建贝塞尔曲线
    std::vector<gp_Pnt> curvePoints = {
        gp_Pnt(0, 0, 0), gp_Pnt(1, 1, 0), gp_Pnt(2, 0, 0), gp_Pnt(3, 1, 0)
    };
    auto bezierCurve = m_advancedRendering->addBezierCurve(curvePoints, "DemoBezierCurve");
    
    // 创建贝塞尔曲面
    std::vector<std::vector<gp_Pnt>> surfaceGrid = {
        {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 1), gp_Pnt(2, 0, 0)},
        {gp_Pnt(0, 1, 1), gp_Pnt(1, 1, 2), gp_Pnt(2, 1, 1)},
        {gp_Pnt(0, 2, 0), gp_Pnt(1, 2, 1), gp_Pnt(2, 2, 0)}
    };
    auto bezierSurface = m_advancedRendering->addBezierSurface(surfaceGrid, "DemoBezierSurface");
    
    LOG_INF_S("Demo scene created with advanced rendering");
}
```

## 编译和链接

### 1. 更新CMakeLists.txt

```cmake
# 在src/rendering/CMakeLists.txt中添加
add_library(rendering
    # 现有文件
    Canvas.cpp
    CoordinateSystemRenderer.cpp
    # ... 其他现有文件
    
    # 新增文件
    AdvancedGeometryRenderer.cpp
    AdvancedRenderingIntegration.cpp
)

target_link_libraries(rendering
    # 现有库
    opencascade
    input
    config
    logger
    
    # 新增依赖
    ${CMAKE_THREAD_LIBS_INIT}  # 如果需要多线程支持
)
```

### 2. 更新头文件包含

```cpp
// 在需要使用的文件中添加
#include "AdvancedRenderingIntegration.h"
#include "AdvancedGeometryRenderer.h"
```

## 测试和验证

### 1. 创建测试场景

```cpp
void MainApplication::createTestScene()
{
    // 清除现有几何体
    m_advancedRendering->clearAllGeometries();
    
    // 创建测试几何体
    std::vector<std::pair<TopoDS_Shape, std::string>> testGeometries = {
        {OCCShapeBuilder::createSphere(1.0), "TestSphere"},
        {OCCShapeBuilder::createBox(1.0, 1.0, 1.0), "TestBox"},
        {OCCShapeBuilder::createCylinder(0.5, 2.0), "TestCylinder"},
        {OCCShapeBuilder::createCone(0.5, 1.0), "TestCone"},
        {OCCShapeBuilder::createTorus(1.0, 0.3), "TestTorus"}
    };
    
    for (const auto& pair : testGeometries) {
        m_advancedRendering->addGeometryWithAdvancedRendering(pair.first, pair.second);
    }
    
    // 创建贝塞尔曲线
    std::vector<gp_Pnt> curvePoints = {
        gp_Pnt(0, 0, 0), gp_Pnt(1, 1, 0), gp_Pnt(2, 0, 0), gp_Pnt(3, 1, 0)
    };
    m_advancedRendering->addBezierCurve(curvePoints, "TestBezierCurve");
    
    LOG_INF_S("Test scene created");
}
```

### 2. 性能测试

```cpp
void MainApplication::runPerformanceTest()
{
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // 创建大量几何体
    for (int i = 0; i < 100; ++i) {
        std::string name = "PerformanceTest_" + std::to_string(i);
        auto sphere = OCCShapeBuilder::createSphere(0.1);
        m_advancedRendering->addGeometryWithAdvancedRendering(sphere, name);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    LOG_INF_S("Performance test completed in " + std::to_string(duration.count()) + "ms");
    
    // 获取性能指标
    auto metrics = m_advancedRendering->getPerformanceMetrics();
    LOG_INF_S("Total geometries: " + std::to_string(metrics.totalGeometries));
    LOG_INF_S("Advanced geometries: " + std::to_string(metrics.advancedGeometries));
    LOG_INF_S("Average rendering time: " + std::to_string(metrics.averageRenderingTime) + "s");
}
```

## 总结

通过这个集成方案，您可以：

1. **无缝集成**: 高级渲染器与现有系统完美配合
2. **渐进式升级**: 可以逐步将现有几何体升级到高级渲染
3. **灵活控制**: 提供多种质量级别和渲染模式
4. **性能优化**: 根据硬件性能自动调整质量
5. **用户友好**: 提供直观的用户界面控制

这个集成方案让您可以在保持现有功能的同时，享受高级渲染技术带来的视觉质量提升。 