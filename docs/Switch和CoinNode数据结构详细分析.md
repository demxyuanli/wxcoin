# Switch和CoinNode数据结构详细分析

## 概述

本文档详细分析两个关键组件中`SoSwitch`和`coinNode`（SoSeparator）的数据结构：
1. **DisplayModePreviewCanvas** - 显示模式配置的预览画布
2. **DisplayModeHandler** - 显示模式管理处理器

---

## 一、DisplayModePreviewCanvas 数据结构

### 1.1 场景图层次结构

```
m_sceneRoot (SoSeparator) - 场景根节点
├── m_camera (SoPerspectiveCamera) - 透视相机
├── SoDirectionalLight - 方向光源
├── m_lightModel (SoLightModel) - 全局光照模型
│
├── m_surfaceSwitch (SoSwitch) - 控制表面几何体可见性
│   └── child[0]: m_geometryRoot (SoSeparator)
│       └── m_surfaceNode (SoSeparator)
│           ├── m_drawStyle (SoDrawStyle) - 绘制样式
│           ├── m_material (SoMaterial) - 材质属性
│           ├── m_shapeHints (SoShapeHints) - 形状提示（用于透明渲染）
│           ├── m_polygonOffset (SoPolygonOffset) - 深度偏移（防止Z-fighting）
│           └── [Surface Geometry] (SoIndexedFaceSet，来自STEP文件)
│
├── m_edgesSwitch (SoSwitch) - 控制边的可见性
│   └── child[0]: m_edgesNode (SoSeparator)
│       ├── [SoPolygonOffset] (可选，用于边的Z-fighting处理)
│       └── [Edge Nodes from ModularEdgeComponent] - 来自模块化边组件的边节点
│           ├── Original edges (SoLineSet) - 原始边
│           └── Mesh edges (SoLineSet) - 网格边
│
└── m_pointsSwitch (SoSwitch) - 控制点的可见性
    └── child[0]: m_pointsNode (SoSeparator)
        └── [Point View Nodes from PointViewBuilder] - 来自点视图构建器的点节点
            ├── SoCoordinate3 (顶点坐标)
            └── SoPointSet (点渲染)
```

### 1.2 三个独立Switch架构

**核心特点：**
- 使用三个独立的`SoSwitch`节点，分别控制表面、边和点的显示
- 每个Switch只包含一个子节点（child[0]）
- 通过`whichChild`属性控制显示/隐藏

**创建代码：**

```235:248:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    m_surfaceSwitch = new SoSwitch;
    m_surfaceSwitch->ref();
    m_surfaceSwitch->addChild(m_geometryRoot);
    m_sceneRoot->addChild(m_surfaceSwitch);
    
    m_edgesSwitch = new SoSwitch;
    m_edgesSwitch->ref();
    m_edgesSwitch->addChild(m_edgesNode);
    m_sceneRoot->addChild(m_edgesSwitch);
    
    m_pointsSwitch = new SoSwitch;
    m_pointsSwitch->ref();
    m_pointsSwitch->addChild(m_pointsNode);
    m_sceneRoot->addChild(m_pointsSwitch);
```

**Switch控制值说明：**
- `whichChild = 0`：显示第一个子节点（索引0），即显示几何体
- `whichChild = -1`（SO_SWITCH_NONE）：隐藏所有子节点，即隐藏几何体

### 1.3 表面节点结构详解

**节点创建顺序（关键）：**

```202:228:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
void DisplayModePreviewCanvas::createGeometry() {
    m_geometryRoot = new SoSeparator;
    m_geometryRoot->ref();
    
    m_surfaceNode = new SoSeparator;
    m_surfaceNode->ref();
    m_geometryRoot->addChild(m_surfaceNode);
    
    // Add rendering state nodes to surface node in correct order
    // Order is critical for proper transparency rendering:
    // LightModel -> DrawStyle -> Material -> ShapeHints -> PolygonOffset -> Geometry
    // Note: LightModel is already in scene root, so we add the rest here
    if (m_drawStyle) {
        m_surfaceNode->addChild(m_drawStyle);
    }
    if (m_material) {
        m_surfaceNode->addChild(m_material);
    }
    if (m_shapeHints) {
        m_surfaceNode->addChild(m_shapeHints);
    }
    
    // Create polygon offset node and add it to surface node
    m_polygonOffset = new SoPolygonOffset;
    m_polygonOffset->ref();
    m_surfaceNode->addChild(m_polygonOffset);
```

**关键节点顺序（必须严格遵守）：**
1. `SoDrawStyle` - 绘制样式（FILLED/LINES/POINTS），决定如何绘制几何体
2. `SoMaterial` - 材质属性（颜色、透明度），决定几何体的外观
3. `SoShapeHints` - 形状提示，用于透明渲染时的面排序和背面剔除
4. `SoPolygonOffset` - 深度偏移，防止表面和边之间的Z-fighting
5. `[Surface Geometry]` - 实际几何体节点（SoIndexedFaceSet等）

**为什么顺序很重要？**
- Coin3D按照场景图中的顺序处理节点
- 状态节点（DrawStyle、Material等）必须在使用它们的几何体之前设置
- ShapeHints必须在几何体之前设置，透明渲染才能正确工作
- PolygonOffset必须在几何体之前设置，才能影响深度排序

### 1.4 Switch控制逻辑

**表面Switch控制：**

```509:512:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
    m_surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("updateGeometryFromConfig: Surface switch set to " + std::to_string(surfaceSwitchValue) + 
              " (requireSurface=" + (config.nodes.requireSurface ? "true" : "false") + ")");
```

**边和点Switch控制：**

```610:628:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
        m_edgesSwitch->whichChild.setValue(0);
    } else {
        m_edgesSwitch->whichChild.setValue(-1);
    }
    
    m_pointsNode->removeAllChildren();
    
    if (config.nodes.requirePoints && m_pointViewBuilder && m_mesh) {
        GeometryRenderContext defaultContext;
        defaultContext.display.pointViewColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);
        defaultContext.display.pointViewSize = 3.0;
        defaultContext.display.pointViewShape = 0;
        
        m_pointViewBuilder->createPointViewRepresentation(m_pointsNode, *m_mesh, defaultContext.display);
        m_pointsSwitch->whichChild.setValue(0);
        LOG_INF_S("Points view created: " + std::to_string(m_mesh->vertices.size()) + " points");
    } else {
        m_pointsSwitch->whichChild.setValue(-1);
    }
```

**控制逻辑说明：**
- 根据配置决定是否显示表面、边或点
- 如果配置要求显示，设置`whichChild = 0`
- 如果配置不要求显示，设置`whichChild = -1`
- 对于点视图，如果不需要显示，还会清空`m_pointsNode`的所有子节点

---

## 二、DisplayModeHandler 数据结构

### 2.1 两种工作模式

DisplayModeHandler支持两种架构模式，根据配置和场景图结构自动选择：

#### 模式1：Switch模式（新架构）

**场景图结构：**
```
coinNode (SoSeparator) - 根节点
├── SoLightModel - 光照模型（在coinNode顶层，所有Switch共享）
│
├── m_surfaceSwitch (SoSwitch) - 表面Switch
│   └── child[0]: m_surfaceNode (SoSeparator)
│       ├── [State Nodes] - 状态节点（Material、DrawStyle等）
│       └── [Surface Geometry] - 表面几何体
│
├── m_edgesSwitch (SoSwitch) - 边Switch
│   └── child[0]: m_edgesNode (SoSeparator)
│       └── [Edge Nodes] - 边节点
│
└── m_pointsSwitch (SoSwitch) - 点Switch
    └── child[0]: m_pointsNode (SoSeparator)
        └── [Point View Nodes] - 点视图节点
```

**Switch模式检测逻辑：**

```125:149:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    // Check if three independent switches exist in coinNode (new architecture)
    // This is more reliable than checking m_modeSwitch
    SoSwitch* surfaceSwitch = nullptr;
    SoSwitch* edgesSwitch = nullptr;
    SoSwitch* pointsSwitch = nullptr;
    int switchCount = 0;
    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (child && child->isOfType(SoSwitch::getClassTypeId())) {
            SoSwitch* sw = static_cast<SoSwitch*>(child);
            if (switchCount == 0) {
                surfaceSwitch = sw;
            } else if (switchCount == 1) {
                edgesSwitch = sw;
            } else if (switchCount == 2) {
                pointsSwitch = sw;
            }
            ++switchCount;
        }
    }
    
    // Use Switch mode if:
    // 1. Config says useSwitchMode=true, AND
    // 2. Three switches exist in coinNode (or will be created by handlers)
    bool switchesExist = (surfaceSwitch && edgesSwitch && pointsSwitch);
    bool shouldUseSwitchMode = configUseSwitchMode && (switchesExist || m_useSwitchMode);
```

**检测逻辑说明：**
1. 遍历`coinNode`的所有子节点，查找`SoSwitch`类型的节点
2. 按顺序识别三个Switch：第一个是表面Switch，第二个是边Switch，第三个是点Switch
3. 如果配置启用Switch模式（`useSwitchMode=true`）且三个Switch都存在（或将被创建），则使用Switch模式

**Switch模式更新：**

```152:169:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    if (shouldUseSwitchMode) {
        // New Switch structure: Three independent switches (surface, edges, points)
        // Update switch visibility for fast mode switching
        LOG_INF_S("DisplayModeHandler::updateDisplayMode: Switching to mode=" + displayModeToString(mode) + 
                  " using Switch mode (config.useSwitchMode=" + std::string(configUseSwitchMode ? "true" : "false") +
                  ", switchesExist=" + std::string(switchesExist ? "true" : "false") + 
                  ", found " + std::to_string(switchCount) + " switches in coinNode)");
        
        // Try both BREP and Mesh handlers (one will work depending on geometry type)
        if (m_brepHandler) {
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Calling BREP handler for switch update");
            m_brepHandler->updateDisplayModeSwitches(coinNode, mode, edgeComponent);
        }
        if (m_meshHandler) {
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Calling Mesh handler for switch update");
            m_meshHandler->updateDisplayModeSwitches(coinNode, mode, edgeComponent);
        }
        return;
    }
```

**Switch模式优势：**
- **快速切换**：只需改变`whichChild`值，无需删除和重建节点
- **性能优化**：所有显示模式的状态节点和几何体都预先构建好
- **内存开销**：需要更多内存，因为所有模式都同时存在

#### 模式2：Direct模式（旧架构）

**场景图结构：**
```
coinNode (SoSeparator) - 根节点
├── SoLightModel - 光照模型
├── SoDrawStyle - 绘制样式
├── SoMaterial - 材质属性
├── SoShapeHints - 形状提示（可选，用于透明模式）
├── SoPolygonOffset - 多边形偏移（可选）
├── [Surface Geometry] - 表面几何体（保留，不删除）
├── [Edge Nodes] - 边节点（来自ModularEdgeComponent）
└── [Point View Nodes] - 点视图节点（SoSeparator包含SoPointSet）
```

**Direct模式节点删除策略：**

```186:261:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    // Step 2: Reset only state nodes (preserve geometry nodes)
    // Collect state nodes to remove safely (avoid index shifting issues)
    std::vector<SoNode*> stateNodesToRemove;
    std::vector<SoNode*> pointViewNodesToRemove;
    std::vector<SoNode*> hiddenLineNodesToRemove;
    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (!child) continue;
        
        // Keep Switch node if it exists (for Switch mode)
        if (child->isOfType(SoSwitch::getClassTypeId())) {
            continue;
        }
        
        // Remove only state nodes, preserve geometry nodes
        if (child->isOfType(SoDrawStyle::getClassTypeId()) ||
            child->isOfType(SoMaterial::getClassTypeId()) ||
            child->isOfType(SoLightModel::getClassTypeId()) ||
            child->isOfType(SoPolygonOffset::getClassTypeId()) ||
            child->isOfType(SoShapeHints::getClassTypeId()) ||
            child->isOfType(SoTexture2::getClassTypeId())) {
            stateNodesToRemove.push_back(child);
        }
        
        // CRITICAL FIX: Preserve geometry nodes (mesh geometry for pure mesh models)
        // Check if this node contains geometry before considering it for removal
        if (nodeManager.containsGeometryNode(child)) {
            continue;  // Preserve geometry nodes
        }
        
        // Detect and remove point view nodes (SoSeparator containing SoPointSet or SoCoordinate3)
        // Also detect HiddenLine pass nodes (SoSeparator with PolygonModeNode)
        if (child->isOfType(SoSeparator::getClassTypeId())) {
            SoSeparator* sep = static_cast<SoSeparator*>(child);
            if (!sep) continue;  // Safety check
            
            bool isPointViewNode = false;
            bool isHiddenLineNode = false;
            
            // CRITICAL: Initialize PolygonModeNode class if not already initialized (before loop)
            if (PolygonModeNode::getClassTypeId() == SoType::badType()) {
                PolygonModeNode::initClass();
            }
            SoType polygonModeType = PolygonModeNode::getClassTypeId();
            bool polygonModeTypeValid = (polygonModeType != SoType::badType());
            
            int numChildren = sep->getNumChildren();
            for (int j = 0; j < numChildren; ++j) {
                SoNode* subChild = sep->getChild(j);
                if (!subChild) continue;
                
                try {
                    if (subChild->isOfType(SoPointSet::getClassTypeId())) {
                        isPointViewNode = true;
                        break;
                    }
                    if (subChild->isOfType(SoCoordinate3::getClassTypeId())) {
                        isPointViewNode = true;
                        break;
                    }
                    // Check for PolygonModeNode (HiddenLine mode)
                    if (polygonModeTypeValid && subChild->isOfType(polygonModeType)) {
                        isHiddenLineNode = true;
                        break;
                    }
                } catch (...) {
                    // Safety: Skip invalid nodes
                    continue;
                }
            }
            if (isPointViewNode) {
                pointViewNodesToRemove.push_back(child);
            }
            if (isHiddenLineNode) {
                hiddenLineNodesToRemove.push_back(child);
            }
        }
    }
```

**删除策略说明：**
1. **保留的节点**：
   - Switch节点（如果存在，用于Switch模式）
   - 几何体节点（通过`containsGeometryNode`检测）
   
2. **删除的节点**：
   - 状态节点：SoDrawStyle、SoMaterial、SoLightModel、SoPolygonOffset、SoShapeHints、SoTexture2
   - 点视图节点：包含SoPointSet或SoCoordinate3的SoSeparator
   - HiddenLine节点：包含PolygonModeNode的SoSeparator
   - 边节点：通过`ModularEdgeComponent::cleanupEdgeNodes()`删除

3. **安全删除**：
   - 使用向量收集要删除的节点，避免在遍历时直接删除导致索引错位
   - 从后向前删除，避免索引偏移问题

**Direct模式节点添加顺序（关键）：**

```401:495:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    // Step 5: Add nodes in correct order (matching applyRenderState)
    // Order: LightModel -> DrawStyle -> Material -> BlendHints -> PolygonOffset
    
    // Step 5.1: Add LightModel node for proper lighting control
    // Use BASE_COLOR for no-shading modes (NoShading, HiddenLine), PHONG for others
    SoLightModel* lightModel = new SoLightModel();
    lightModel->ref();
    if (!updateState.lightingEnabled || updateState.surfaceDisplayMode == RenderingConfig::DisplayMode::NoShading) {
        lightModel->model.setValue(SoLightModel::BASE_COLOR);  // No lighting, direct color
    } else {
        lightModel->model.setValue(SoLightModel::PHONG);  // Standard Phong lighting
    }
    coinNode->addChild(lightModel);
    lightModel->unref();

    // Step 5.2: Create DrawStyle node for SURFACE geometry only (was deleted by resetAllRenderStates)
    // NOTE: This DrawStyle only controls how SURFACE geometry is rendered (FILLED/LINES/POINTS).
    // Edge rendering is handled separately by ModularEdgeComponent, which creates its own edge nodes
    // with their own DrawStyle (LINES mode) in Step 10. For modes like NoShading that need both
    // filled surface AND wireframe edges, the surface uses FILLED here, and edges are rendered
    // separately via edgeComponent->updateEdgeDisplay() which adds SoIndexedLineSet nodes.
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->ref();
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        // NoShading: FILLED for surface, edges rendered separately via edgeComponent
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    case RenderingConfig::DisplayMode::Points:
        drawStyle->style.setValue(SoDrawStyle::POINTS);
        break;
    case RenderingConfig::DisplayMode::Wireframe:
        drawStyle->style.setValue(SoDrawStyle::LINES);
        break;
    case RenderingConfig::DisplayMode::FlatLines:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    case RenderingConfig::DisplayMode::Solid:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    case RenderingConfig::DisplayMode::Transparent:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    case RenderingConfig::DisplayMode::HiddenLine:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    default:
        drawStyle->style.setValue(SoDrawStyle::FILLED);
        break;
    }
    coinNode->addChild(drawStyle);
    drawStyle->unref();

    // Step 5.3: Create Material node based on render state (was deleted by resetAllRenderStates)
    SoMaterial* material = new SoMaterial();
    material->ref();
    
    // Apply material colors from updateState (which was set by setRenderStateForMode)
    Standard_Real r, g, b;
    
    updateState.surfaceAmbientColor.Values(r, g, b, Quantity_TOC_RGB);
    material->ambientColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    updateState.surfaceDiffuseColor.Values(r, g, b, Quantity_TOC_RGB);
    material->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    updateState.surfaceSpecularColor.Values(r, g, b, Quantity_TOC_RGB);
    material->specularColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    updateState.surfaceEmissiveColor.Values(r, g, b, Quantity_TOC_RGB);
    material->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    material->shininess.setValue(static_cast<float>(updateState.shininess / 100.0));
    material->transparency.setValue(static_cast<float>(updateState.transparency));
    
    coinNode->addChild(material);
    material->unref();

    // Step 5.4: Add BlendHints for Transparent mode
    if (updateState.blendMode == RenderingConfig::BlendMode::Alpha && updateState.transparency > 0.0) {
        SoShapeHints* blendHints = new SoShapeHints();
        blendHints->ref();
        blendHints->faceType.setValue(SoShapeHints::UNKNOWN_FACE_TYPE);
        blendHints->vertexOrdering.setValue(SoShapeHints::UNKNOWN_ORDERING);
        coinNode->addChild(blendHints);
        blendHints->unref();
    }

    // Step 5.5: Add PolygonOffset for modes that render surface
    // HiddenLine mode needs special offset (push back), other surface modes use default
    if (updateState.showSurface) {
        SoPolygonOffset* polygonOffset = new SoPolygonOffset();
        polygonOffset->ref();
        if (mode == RenderingConfig::DisplayMode::HiddenLine) {
            polygonOffset->factor.setValue(1.0f);  // Push surface back
            polygonOffset->units.setValue(1.0f);
        }
        // For other modes (Solid, Transparent, etc.), use default PolygonOffset values
        coinNode->addChild(polygonOffset);
        polygonOffset->unref();
    }
```

**节点添加顺序说明：**
1. **SoLightModel** - 光照模型，决定是否使用光照计算
   - NoShading模式：BASE_COLOR（无光照，直接使用颜色）
   - 其他模式：PHONG（标准Phong光照模型）

2. **SoDrawStyle** - 绘制样式，**仅控制表面几何体的绘制方式**
   - **重要说明**：这个DrawStyle只影响表面几何体（SoIndexedFaceSet等），不影响边的绘制
   - **统一设置为FILLED**：所有需要显示表面的模式都使用FILLED，因为：
     - 边的绘制由独立的边节点处理（在Step 10中通过`edgeComponent->updateEdgeDisplay()`添加）
     - 边节点内部有自己的DrawStyle（LINES模式），由`BaseEdgeRenderer`创建
     - 对于不显示表面的模式（Wireframe、Points），DrawStyle设置无效，但仍设置为FILLED保持一致性
   - **表面可见性控制**：通过`showSurface`标志控制（在Switch模式中通过Switch的whichChild，在Direct模式中通过是否添加表面几何体）

3. **SoMaterial** - 材质属性，设置颜色、透明度等
   - 从`updateState`中获取颜色值
   - 支持环境光、漫反射、镜面反射、自发光颜色
   - 设置光泽度和透明度

4. **SoShapeHints** - 形状提示（仅透明模式需要）
   - 透明模式：UNKNOWN_FACE_TYPE + UNKNOWN_ORDERING（允许Coin3D处理面排序）
   - 用于正确的透明渲染

5. **SoPolygonOffset** - 多边形偏移（仅需要表面的模式）
   - HiddenLine模式：factor=1.0, units=1.0（将表面推后，让边显示在前面）
   - 其他模式：使用默认值（防止Z-fighting）

**Direct模式优势：**
- **灵活性高**：可以动态修改任何节点
- **内存占用低**：只保留当前模式的节点
- **实现简单**：直接操作节点，无需管理Switch

**Direct模式劣势：**
- **切换速度慢**：需要删除和重建节点
- **性能开销**：每次切换都要重新构建场景图

**重要说明：表面和边的分离渲染**

在Direct模式中，表面几何体和边是**分开渲染**的：

```
coinNode (SoSeparator)
├── SoLightModel
├── SoDrawStyle (FILLED) ← 只影响表面几何体
├── SoMaterial (surface material)
├── [Surface Geometry] (SoIndexedFaceSet) ← 使用上面的DrawStyle
│
└── [Edge Nodes] (由ModularEdgeComponent添加，在Step 10)
    └── SoSeparator (edge node)
        ├── SoMaterial (edge color)
        ├── SoDrawStyle (LINES, lineWidth) ← 边自己的DrawStyle
        ├── SoCoordinate3
        └── SoIndexedLineSet ← 边的几何体
```

**NoShading模式示例：**
- 表面：使用`SoDrawStyle::FILLED`，渲染为填充面
- 边：通过`edgeComponent->updateEdgeDisplay()`添加独立的边节点，内部使用`SoDrawStyle::LINES`
- 两者同时显示，实现"填充面+线框边"的效果

这就是为什么NoShading模式可以同时显示填充面和线框边：它们使用不同的DrawStyle节点，分别控制各自的渲染方式。

---

## 三、关键差异对比

### 3.1 架构差异

| 特性 | DisplayModePreviewCanvas | DisplayModeHandler |
|------|--------------------------|-------------------|
| **Switch数量** | 固定3个独立Switch | 动态检测（0-3个） |
| **模式支持** | 仅Switch模式 | Switch + Direct双模式 |
| **几何来源** | STEP文件 | BREP和Mesh几何体 |
| **复杂度** | 简化（专为预览优化） | 完整功能实现 |
| **使用场景** | 预览对话框 | 主视图窗口 |

### 3.2 Switch控制值详解

**SoSwitch::whichChild 值说明：**

| 值 | 含义 | 使用场景 |
|-------|---------|-------|
| `0` | 显示第一个子节点（索引0） | 显示几何体 |
| `-1` (SO_SWITCH_NONE) | 隐藏所有子节点 | 隐藏几何体 |
| `n` (n > 0) | 显示索引为n的子节点 | 多模式切换（当前实现未使用） |

**Switch模式决策逻辑：**

```67:74:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
    // Read switch mode configuration from RenderingConfig
    // Default to true (Switch mode) if config not available
    RenderingConfig& config = RenderingConfig::getInstance();
    m_useSwitchMode = config.getDisplaySettings().useSwitchMode;
    
    LOG_INF_S("DisplayModeHandler::DisplayModeHandler: Initialized with useSwitchMode=" + 
              std::string(m_useSwitchMode ? "true" : "false") + " (from config)");
```

**决策流程：**
1. 从`RenderingConfig`读取`useSwitchMode`配置
2. 如果配置为true，优先使用Switch模式
3. 检测`coinNode`中是否存在三个Switch节点
4. 如果存在三个Switch或配置要求使用Switch模式，则使用Switch模式
5. 否则使用Direct模式

---

## 四、节点生命周期管理

### 4.1 引用计数机制

两个程序都使用Coin3D的引用计数系统来管理节点生命周期：

**创建节点时增加引用：**

```126:127:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    m_sceneRoot = new SoSeparator;
    m_sceneRoot->ref();
```

```235:237:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    m_surfaceSwitch = new SoSwitch;
    m_surfaceSwitch->ref();
    m_surfaceSwitch->addChild(m_geometryRoot);
```

**销毁时减少引用：**

```77:118:wxcoin/src/opencascade/geometry/helper/DisplayModePreviewCanvas.cpp
    if (m_sceneRoot) {
        m_sceneRoot->unref();
    }
    if (m_surfaceNode) {
        m_surfaceNode->unref();
    }
    if (m_edgesNode) {
        m_edgesNode->unref();
    }
    if (m_pointsNode) {
        m_pointsNode->unref();
    }
    if (m_geometryRoot) {
        m_geometryRoot->unref();
    }
    if (m_camera) {
        m_camera->unref();
    }
    if (m_material) {
        m_material->unref();
    }
    if (m_drawStyle) {
        m_drawStyle->unref();
    }
    if (m_lightModel) {
        m_lightModel->unref();
    }
    if (m_shapeHints) {
        m_shapeHints->unref();
    }
    if (m_polygonOffset) {
        m_polygonOffset->unref();
    }
    if (m_surfaceSwitch) {
        m_surfaceSwitch->unref();
    }
    if (m_edgesSwitch) {
        m_edgesSwitch->unref();
    }
    if (m_pointsSwitch) {
        m_pointsSwitch->unref();
    }
```

**引用计数规则：**
- 创建节点后立即调用`ref()`增加引用计数
- 将节点添加到父节点时，父节点会自动增加引用计数
- 销毁时调用`unref()`减少引用计数
- 当引用计数为0时，节点自动销毁

---

## 五、关键实现要点

### 5.1 节点顺序的重要性

Coin3D场景图中的节点顺序直接影响渲染结果：

1. **状态节点必须在几何体之前**
   - LightModel、DrawStyle、Material等状态节点必须在使用它们的几何体之前设置
   - Coin3D按照场景图遍历顺序应用状态

2. **ShapeHints必须在几何体之前**
   - 透明渲染需要正确的面排序
   - ShapeHints必须在几何体之前设置才能生效

3. **PolygonOffset必须在几何体之前**
   - 深度偏移需要在渲染几何体之前设置
   - 用于防止Z-fighting（深度冲突）

### 5.2 Switch模式 vs Direct模式权衡

| 方面 | Switch模式 | Direct模式 |
|--------|-------------|-------------|
| **性能** | 快速（只需改变whichChild） | 较慢（需要删除/添加节点） |
| **内存** | 较高（所有模式都预构建） | 较低（只保留当前模式） |
| **灵活性** | 较低（预定义模式） | 较高（动态修改） |
| **复杂度** | 较高（需要管理Switch） | 较低（直接操作节点） |
| **适用场景** | 频繁切换显示模式 | 偶尔切换或需要动态修改 |

### 5.3 几何体节点保护机制

在Direct模式中，几何体节点在模式切换时**始终被保留**：

```208:212:wxcoin/src/opencascade/geometry/helper/DisplayModeHandler.cpp
        // CRITICAL FIX: Preserve geometry nodes (mesh geometry for pure mesh models)
        // Check if this node contains geometry before considering it for removal
        if (nodeManager.containsGeometryNode(child)) {
            continue;  // Preserve geometry nodes
        }
```

**保护机制说明：**
- 使用`DisplayModeNodeManager::containsGeometryNode()`检测几何体节点
- 几何体节点（如SoIndexedFaceSet）不会被删除
- 只有状态节点（DrawStyle、Material等）会被删除和重建
- 这避免了重复加载和转换几何体的开销

### 5.4 透明渲染的特殊处理

**透明模式需要特殊配置：**

1. **SoShapeHints配置**：
   - `faceType = UNKNOWN_FACE_TYPE`：告诉Coin3D面类型未知
   - `vertexOrdering = UNKNOWN_ORDERING`：允许Coin3D自动处理顶点顺序
   - 这些设置允许Coin3D正确排序透明面

2. **SoGLRenderAction配置**：
   - 设置透明度类型：`SORTED_OBJECT_BLEND`或`SORTED_OBJECT_SORTED_TRIANGLE_BLEND`
   - 设置渲染遍数：根据场景复杂度选择2-3遍

3. **渲染顺序**：
   - 透明对象需要从后向前渲染
   - Coin3D会自动处理排序

---

## 六、数据流分析

### 6.1 DisplayModePreviewCanvas数据流

```
配置更新 (DisplayModeConfig)
    ↓
updateGeometryFromConfig()
    ↓
更新状态节点值 (Material、DrawStyle等)
    ↓
设置Switch可见性 (whichChild)
    ↓
更新边和点节点
    ↓
触发重绘 (Refresh)
    ↓
onPaint() 渲染场景
```

### 6.2 DisplayModeHandler数据流

**Switch模式：**
```
模式切换请求
    ↓
updateDisplayMode()
    ↓
检测Switch模式 (shouldUseSwitchMode)
    ↓
调用Handler的updateDisplayModeSwitches()
    ↓
更新状态节点值（不删除重建）
    ↓
设置Switch可见性 (whichChild)
    ↓
场景图自动更新
```

**Direct模式：**
```
模式切换请求
    ↓
updateDisplayMode()
    ↓
检测Direct模式
    ↓
收集要删除的状态节点
    ↓
删除状态节点（保留几何体）
    ↓
创建新的状态节点
    ↓
按正确顺序添加到coinNode
    ↓
更新边和点节点
    ↓
场景图更新
```

---

## 七、总结

### 7.1 DisplayModePreviewCanvas特点

- 使用**三个独立的SoSwitch节点**（表面、边、点）
- **固定结构**，专为预览优化
- 通过`whichChild`属性直接控制Switch
- **简化实现**，适合预览场景

### 7.2 DisplayModeHandler特点

- 支持**两种架构**：
  - **Switch模式**：三个独立Switch（新架构，快速切换）
  - **Direct模式**：直接节点操作（旧架构，更灵活）
- **自动检测**并使用合适的模式
- **保护几何体节点**，在模式切换时不被删除
- **完整功能**，支持所有显示模式

### 7.3 共同遵循的原则

1. **节点顺序至关重要**：状态节点必须在几何体之前
2. **引用计数管理**：正确使用ref()/unref()管理节点生命周期
3. **几何体保护**：切换模式时保留几何体，只重建状态节点
4. **透明渲染特殊处理**：需要正确的ShapeHints和渲染设置

两个实现都遵循Coin3D的最佳实践，在场景图组织和节点生命周期管理方面保持一致。

