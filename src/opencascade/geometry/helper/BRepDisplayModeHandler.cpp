#include "geometry/helper/BRepDisplayModeHandler.h"
#include "geometry/helper/DisplayModeStateManager.h"
#include "geometry/helper/DisplayModeNodeManager.h"
#include "geometry/helper/DisplayModeRenderer.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/helper/RenderNodeBuilder.h"
#include "geometry/helper/PointViewBuilder.h"
#include "edges/ModularEdgeComponent.h"
#include "config/EdgeSettingsConfig.h"
#include "config/RenderingConfig.h"
#include "rendering/RenderingToolkitAPI.h"
#include "rendering/GeometryProcessor.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>

BRepDisplayModeHandler::BRepDisplayModeHandler()
    : m_modeSwitch(nullptr)
    , m_useSwitchMode(false)
    , m_surfaceSwitch(nullptr)
    , m_edgesSwitch(nullptr)
    , m_pointsSwitch(nullptr)
    , m_surfaceNode(nullptr)
    , m_edgesNode(nullptr)
    , m_pointsNode(nullptr)
    , m_lightModel(nullptr)
    , m_material(nullptr)
    , m_drawStyle(nullptr)
    , m_polygonOffset(nullptr)
    , m_shapeHints(nullptr)
{
}

BRepDisplayModeHandler::~BRepDisplayModeHandler()
{
}

void BRepDisplayModeHandler::setModeSwitch(SoSwitch* modeSwitch)
{
    m_modeSwitch = modeSwitch;
    m_useSwitchMode = (m_modeSwitch != nullptr);
}

void BRepDisplayModeHandler::handleDisplayMode(SoSeparator* coinNode, 
                                                const GeometryRenderContext& context,
                                                const TopoDS_Shape& shape,
                                                const MeshParameters& params,
                                                ModularEdgeComponent* edgeComponent,
                                                bool useModularEdgeComponent,
                                                helper::RenderNodeBuilder* renderBuilder,
                                                helper::WireframeBuilder* wireframeBuilder,
                                                helper::PointViewBuilder* pointViewBuilder) {
    if (!coinNode || !renderBuilder || !wireframeBuilder) {
        return;
    }

    const RenderingConfig::DisplayMode displayMode = context.display.displayMode;
    
    // Read switch mode preference from config
    RenderingConfig& config = RenderingConfig::getInstance();
    bool configUseSwitchMode = config.getDisplaySettings().useSwitchMode;
    
    // Only use Switch mode if config says so
    if (!configUseSwitchMode) {
        LOG_INF_S("BRepDisplayModeHandler::handleDisplayMode: Config says useSwitchMode=false, using Direct mode");
        m_useSwitchMode = false;
        // Fall through to Direct mode code below
    } else {
        // Always use three independent switches (new architecture)
        // Initialize switch structure if not already done
        DisplayModeNodeManager nodeManager;
        
        // Check if three switches already exist in coinNode
        SoSwitch* existingSurfaceSwitch = nullptr;
        SoSwitch* existingEdgesSwitch = nullptr;
        SoSwitch* existingPointsSwitch = nullptr;
        int switchCount = 0;
        for (int i = 0; i < coinNode->getNumChildren(); ++i) {
            SoNode* child = coinNode->getChild(i);
            if (child && child->isOfType(SoSwitch::getClassTypeId())) {
                SoSwitch* sw = static_cast<SoSwitch*>(child);
                if (switchCount == 0) {
                    existingSurfaceSwitch = sw;
                } else if (switchCount == 1) {
                    existingEdgesSwitch = sw;
                } else if (switchCount == 2) {
                    existingPointsSwitch = sw;
                }
                ++switchCount;
            }
        }
        
        // Use existing switches if found in coinNode
        if (existingSurfaceSwitch && existingEdgesSwitch && existingPointsSwitch) {
            m_surfaceSwitch = existingSurfaceSwitch;
            m_edgesSwitch = existingEdgesSwitch;
            m_pointsSwitch = existingPointsSwitch;
            
            // Find surface/edges/points nodes from switches and extract state nodes
            if (m_surfaceSwitch->getNumChildren() > 0) {
                SoNode* surfaceChild = m_surfaceSwitch->getChild(0);
                if (surfaceChild && surfaceChild->isOfType(SoSeparator::getClassTypeId())) {
                    m_surfaceNode = static_cast<SoSeparator*>(surfaceChild);
                    // Extract state nodes from existing surface node (fixed order: Material, DrawStyle, PolygonOffset, ShapeHints)
                    if (m_surfaceNode->getNumChildren() >= 4) {
                        SoNode* child0 = m_surfaceNode->getChild(0);
                        SoNode* child1 = m_surfaceNode->getChild(1);
                        SoNode* child2 = m_surfaceNode->getChild(2);
                        SoNode* child3 = m_surfaceNode->getChild(3);
                        if (child0 && child0->isOfType(SoMaterial::getClassTypeId())) m_material = static_cast<SoMaterial*>(child0);
                        if (child1 && child1->isOfType(SoDrawStyle::getClassTypeId())) m_drawStyle = static_cast<SoDrawStyle*>(child1);
                        if (child2 && child2->isOfType(SoPolygonOffset::getClassTypeId())) m_polygonOffset = static_cast<SoPolygonOffset*>(child2);
                        if (child3 && child3->isOfType(SoShapeHints::getClassTypeId())) m_shapeHints = static_cast<SoShapeHints*>(child3);
                    }
                }
            }
            if (m_edgesSwitch->getNumChildren() > 0) {
                SoNode* edgesChild = m_edgesSwitch->getChild(0);
                if (edgesChild && edgesChild->isOfType(SoSeparator::getClassTypeId())) {
                    m_edgesNode = static_cast<SoSeparator*>(edgesChild);
                }
            }
            if (m_pointsSwitch->getNumChildren() > 0) {
                SoNode* pointsChild = m_pointsSwitch->getChild(0);
                if (pointsChild && pointsChild->isOfType(SoSeparator::getClassTypeId())) {
                    m_pointsNode = static_cast<SoSeparator*>(pointsChild);
                }
            }
            
            // LightModel is now part of m_surfaceNode (created via buildStateNodeFromConfig)
            // No need to find it at coinNode top level anymore
        }
        
        // Initialize switch structure if not already done
        if (!m_surfaceSwitch || !m_edgesSwitch || !m_pointsSwitch) {
            LOG_INF_S("BRepDisplayModeHandler::handleDisplayMode: Initializing new switch structure (config.useSwitchMode=true)");
            nodeManager.resetAllRenderStates(coinNode, edgeComponent);
            initializeSwitchStructure(coinNode, nodeManager, context, shape, params, 
                                    renderBuilder, pointViewBuilder);
        }
        
        // Update m_useSwitchMode to reflect that switches are now available
        m_useSwitchMode = true;
        
        // Get configuration for current display mode
        DisplayModeConfig currentConfig = DisplayModeConfigFactory::getConfig(displayMode, context);
        
        // CRITICAL: Remove and rebuild state nodes for current mode (matching MeshDisplayModeHandler)
        // This ensures state nodes match the current display mode configuration
        if (m_surfaceNode) {
            DisplayModeRenderer renderer;
            
            // Remove existing state nodes from surface node
            for (int i = m_surfaceNode->getNumChildren() - 1; i >= 0; --i) {
                SoNode* child = m_surfaceNode->getChild(i);
                if (child->isOfType(SoLightModel::getClassTypeId()) ||
                    child->isOfType(SoDrawStyle::getClassTypeId()) ||
                    child->isOfType(SoMaterial::getClassTypeId()) ||
                    child->isOfType(SoShapeHints::getClassTypeId()) ||
                    child->isOfType(SoPolygonOffset::getClassTypeId()) ||
                    child->isOfType(SoTexture2::getClassTypeId())) {
                    m_surfaceNode->removeChild(i);
                }
            }
            
            // Build state nodes from current configuration
            renderer.buildStateNodeFromConfig(m_surfaceNode, currentConfig, context, renderBuilder);
        }
        
        // Update switch visibility based on configuration
        updateSwitchVisibility(currentConfig, shape, edgeComponent, useModularEdgeComponent, 
                             nodeManager, coinNode);
        
        return;
    }
    
    // Direct mode: Config says useSwitchMode=false, use legacy direct node manipulation
    LOG_INF_S("BRepDisplayModeHandler::handleDisplayMode: Using Direct mode (config.useSwitchMode=false)");
    m_useSwitchMode = false;
    
    // ===== Step 1: Reset all render states =====
    DisplayModeNodeManager nodeManager;
    nodeManager.resetAllRenderStates(coinNode, edgeComponent);
    
    // ===== Step 2: Initialize render state from context =====
    DisplayModeRenderState state;
    state.surfaceAmbientColor = context.material.ambientColor;
    state.surfaceDiffuseColor = context.material.diffuseColor;
    state.surfaceSpecularColor = context.material.specularColor;
    state.surfaceEmissiveColor = context.material.emissiveColor;
    state.shininess = context.material.shininess;
    state.transparency = context.material.transparency;
    state.originalEdgeColor = context.display.wireframeColor;
    state.meshEdgeColor = context.material.diffuseColor;  // Default to face color for mesh edges
    state.originalEdgeWidth = context.display.wireframeWidth;
    state.meshEdgeWidth = context.display.wireframeWidth;
    state.textureEnabled = context.texture.enabled;
    state.blendMode = context.blend.blendMode;
    state.showPoints = context.display.showPointView;
    state.showSolidWithPoints = context.display.showSolidWithPointView;
    state.surfaceDisplayMode = displayMode;

    // ===== Step 3: Set render state based on display mode =====
    DisplayModeStateManager stateManager;
    stateManager.setRenderStateForMode(state, displayMode, context);

    // ===== Step 4: Apply render state to scene graph =====
    DisplayModeRenderer renderer;
    renderer.applyRenderState(coinNode, state, context, shape, params, edgeComponent, 
                     useModularEdgeComponent, renderBuilder, wireframeBuilder, pointViewBuilder);
}

void BRepDisplayModeHandler::initializeSwitchStructure(SoSeparator* coinNode, 
                                                       DisplayModeNodeManager& nodeManager,
                                                       const GeometryRenderContext& context,
                                                       const TopoDS_Shape& shape,
                                                       const MeshParameters& params,
                                                       helper::RenderNodeBuilder* renderBuilder,
                                                       helper::PointViewBuilder* pointViewBuilder) {
    if (!coinNode || !renderBuilder) {
        return;
    }
    
    // CRITICAL: LightModel is NOT created here - it will be created in handleDisplayMode
    // via buildStateNodeFromConfig as part of m_surfaceNode (matching MeshDisplayModeHandler)
    
    // Create three independent switches following preview canvas structure
    if (!m_surfaceSwitch) {
        m_surfaceSwitch = new SoSwitch();
        m_surfaceSwitch->ref();
    }
    
    if (!m_edgesSwitch) {
        m_edgesSwitch = new SoSwitch();
        m_edgesSwitch->ref();
    }
    
    if (!m_pointsSwitch) {
        m_pointsSwitch = new SoSwitch();
        m_pointsSwitch->ref();
    }
    
    // Create surface node (contains state nodes and geometry) - only if not already exists
    // CRITICAL: State nodes are NOT created here - they will be created/removed/recreated
    // in handleDisplayMode based on current display mode (matching MeshDisplayModeHandler)
    if (!m_surfaceNode) {
        m_surfaceNode = new SoSeparator();
        m_surfaceNode->ref();
        LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: Surface node created (state nodes will be added in handleDisplayMode)");
    }
    
    // Check if any mode requires surface geometry
    bool anyModeRequiresSurface = false;
    RenderingConfig::DisplayMode modes[] = {
        RenderingConfig::DisplayMode::NoShading,
        RenderingConfig::DisplayMode::Points,
        RenderingConfig::DisplayMode::Wireframe,
        RenderingConfig::DisplayMode::Solid,
        RenderingConfig::DisplayMode::FlatLines,
        RenderingConfig::DisplayMode::Transparent,
        RenderingConfig::DisplayMode::HiddenLine
    };
    for (auto mode : modes) {
        GeometryRenderContext modeContext = context;
        modeContext.display.displayMode = mode;
        DisplayModeConfig modeConfig = DisplayModeConfigFactory::getConfig(mode, modeContext);
        if (modeConfig.nodes.requireSurface) {
            anyModeRequiresSurface = true;
            break;
        }
    }
    
    // Build surface geometry if any mode requires it
    // IMPORTANT: When useSwitchMode=true, geometry is ALWAYS created directly into m_surfaceNode
    // (which is a child of m_surfaceSwitch), not into coinNode. This ensures optimal performance
    // and avoids unnecessary node movement operations.
    
    // CRITICAL: Geometry is ALWAYS added AFTER state nodes
    // Count existing state nodes (should be 4: Material, DrawStyle, PolygonOffset, ShapeHints)
    int stateNodeCount = 4;
    int currentChildren = m_surfaceNode->getNumChildren();
    
    // Only add geometry if not already present (geometry comes after state nodes)
    bool hasGeometry = (currentChildren > stateNodeCount);
    
    if (!hasGeometry && anyModeRequiresSurface) {
        LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: Creating surface geometry after state nodes");
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.facesVisible = true;
        renderBuilder->appendSurfaceGeometry(m_surfaceNode, shape, params, surfaceContext);
        
        // Verify geometry was created
        if (m_surfaceNode->getNumChildren() <= stateNodeCount) {
            LOG_WRN_S("BRepDisplayModeHandler::initializeSwitchStructure: Warning - appendSurfaceGeometry did not create any geometry nodes");
        } else {
            LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: Created geometry (total children: " + 
                      std::to_string(m_surfaceNode->getNumChildren()) + ", state nodes: " + 
                      std::to_string(stateNodeCount) + ", geometry nodes: " + 
                      std::to_string(m_surfaceNode->getNumChildren() - stateNodeCount) + ")");
        }
    } else if (!anyModeRequiresSurface) {
        LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: No mode requires surface geometry, skipping geometry creation");
    } else {
        LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: Geometry already exists, skipping creation");
    }
    
    // Add surface node to surface switch (only if not already added)
    if (m_surfaceSwitch->getNumChildren() == 0) {
        m_surfaceSwitch->addChild(m_surfaceNode);
    }
    // CRITICAL: Initialize surface switch based on CURRENT display mode configuration
    // Don't use geometry existence as criteria - use current mode's requireSurface setting
    DisplayModeConfig currentModeConfig = DisplayModeConfigFactory::getConfig(context.display.displayMode, context);
    int surfaceSwitchValue = currentModeConfig.nodes.requireSurface ? 0 : -1;
    m_surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: Surface switch initialized with whichChild=" + 
              std::to_string(surfaceSwitchValue) + " (current mode=" + 
              std::to_string(static_cast<int>(context.display.displayMode)) + 
              ", requireSurface=" + std::string(currentModeConfig.nodes.requireSurface ? "true" : "false") + 
              ", surfaceNode has " + std::to_string(m_surfaceNode->getNumChildren()) + " children)");
    
    // Create edges node (only if not already exists)
    if (!m_edgesNode) {
        m_edgesNode = new SoSeparator();
        m_edgesNode->ref();
    }
    if (m_edgesSwitch->getNumChildren() == 0) {
        m_edgesSwitch->addChild(m_edgesNode);
    }
    // CRITICAL: Initialize edges switch based on CURRENT display mode configuration
    bool showOriginalEdges = currentModeConfig.nodes.requireOriginalEdges && currentModeConfig.edges.originalEdge.enabled;
    bool showMeshEdges = currentModeConfig.nodes.requireMeshEdges && currentModeConfig.edges.meshEdge.enabled;
    bool showEdges = showOriginalEdges || showMeshEdges;
    if (context.display.displayMode == RenderingConfig::DisplayMode::Wireframe) {
        showEdges = true;  // Wireframe always shows edges
    }
    int edgesSwitchValue = showEdges ? 0 : -1;
    m_edgesSwitch->whichChild.setValue(edgesSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: Edges switch initialized with whichChild=" + 
              std::to_string(edgesSwitchValue) + " (showOriginalEdges=" + 
              std::string(showOriginalEdges ? "true" : "false") + ", showMeshEdges=" + 
              std::string(showMeshEdges ? "true" : "false") + ")");
    
    // Create points node - only if not already exists
    if (!m_pointsNode) {
        m_pointsNode = new SoSeparator();
        m_pointsNode->ref();
    }
    
    // Build points geometry if needed
    SoSeparator* existingPointViewNode = nodeManager.findPointViewNode(coinNode);
    bool pointsCreated = false;
    if (!existingPointViewNode && pointViewBuilder) {
        GeometryRenderContext pointsContext = context;
        pointsContext.display.displayMode = RenderingConfig::DisplayMode::Points;
        DisplayModeConfig pointsConfig = DisplayModeConfigFactory::getConfig(RenderingConfig::DisplayMode::Points, pointsContext);
        if (pointsConfig.nodes.requirePoints) {
            pointViewBuilder->createPointViewRepresentation(m_pointsNode, shape, params, context.display);
            pointsCreated = true;
        }
    } else if (existingPointViewNode) {
        // Move existing points to points node
        for (int i = 0; i < existingPointViewNode->getNumChildren(); ++i) {
            SoNode* child = existingPointViewNode->getChild(i);
            if (child) {
                m_pointsNode->addChild(child);
                pointsCreated = true;
            }
        }
    }
    
    if (m_pointsSwitch->getNumChildren() == 0) {
        m_pointsSwitch->addChild(m_pointsNode);
    }
    // CRITICAL: Initialize points switch based on CURRENT display mode configuration
    int pointsSwitchValue = currentModeConfig.nodes.requirePoints ? 0 : -1;
    m_pointsSwitch->whichChild.setValue(pointsSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: Points switch initialized with whichChild=" + 
              std::to_string(pointsSwitchValue) + " (current mode requirePoints=" + 
              std::string(currentModeConfig.nodes.requirePoints ? "true" : "false") + 
              ", pointsCreated=" + std::string(pointsCreated ? "true" : "false") + ")");
    
    // Add switches to coin node (after LightModel)
    // Check if switches are already added
    bool surfaceSwitchAdded = false;
    bool edgesSwitchAdded = false;
    bool pointsSwitchAdded = false;
    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (child == m_surfaceSwitch) surfaceSwitchAdded = true;
        if (child == m_edgesSwitch) edgesSwitchAdded = true;
        if (child == m_pointsSwitch) pointsSwitchAdded = true;
    }
    
    if (!surfaceSwitchAdded) coinNode->addChild(m_surfaceSwitch);
    if (!edgesSwitchAdded) coinNode->addChild(m_edgesSwitch);
    if (!pointsSwitchAdded) coinNode->addChild(m_pointsSwitch);
    
    LOG_INF_S("BRepDisplayModeHandler::initializeSwitchStructure: Switch structure initialized - surfaceNode children=" + 
              std::to_string(m_surfaceNode->getNumChildren()) + ", edgesNode children=" + 
              std::to_string(m_edgesNode->getNumChildren()) + ", pointsNode children=" + 
              std::to_string(m_pointsNode->getNumChildren()));
}

void BRepDisplayModeHandler::updateSwitchVisibility(const DisplayModeConfig& config,
                                                    const TopoDS_Shape& shape,
                                                    ModularEdgeComponent* edgeComponent,
                                                    bool useModularEdgeComponent,
                                                    DisplayModeNodeManager& nodeManager,
                                                    SoSeparator* coinNode) {
    if (!m_surfaceSwitch || !m_edgesSwitch || !m_pointsSwitch) {
        LOG_WRN_S("BRepDisplayModeHandler::updateSwitchVisibility: Switches not initialized");
        return;
    }
    
    LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Updating switch visibility - requireSurface=" + 
              std::string(config.nodes.requireSurface ? "true" : "false") + ", requireOriginalEdges=" + 
              std::string(config.nodes.requireOriginalEdges ? "true" : "false") + ", requireMeshEdges=" + 
              std::string(config.nodes.requireMeshEdges ? "true" : "false") + ", requirePoints=" + 
              std::string(config.nodes.requirePoints ? "true" : "false"));
    
    // Update surface switch visibility
    int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
    int oldSurfaceValue = m_surfaceSwitch->whichChild.getValue();
    m_surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Surface switch: " + std::to_string(oldSurfaceValue) + 
              " -> " + std::to_string(surfaceSwitchValue));
    
    // Update edges switch visibility
    bool showOriginalEdges = config.nodes.requireOriginalEdges && config.edges.originalEdge.enabled;
    bool showMeshEdges = config.nodes.requireMeshEdges && config.edges.meshEdge.enabled;
    
    LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Edge display logic - showOriginalEdges=" + 
              std::string(showOriginalEdges ? "true" : "false") + ", showMeshEdges=" + std::string(showMeshEdges ? "true" : "false") + 
              ", useModularEdgeComponent=" + std::string(useModularEdgeComponent ? "true" : "false") + 
              ", edgeComponent=" + std::string(edgeComponent ? "valid" : "null"));
    
    if (useModularEdgeComponent && edgeComponent) {
        // Clear edges node
        m_edgesNode->removeAllChildren();
        LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Cleared edges node");
        
        // Set edge display flags based on config (matching preview canvas logic)
        edgeComponent->setEdgeDisplayType(EdgeType::Original, showOriginalEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, showMeshEdges);  // Support mesh edges for HiddenLine mode
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        
        // Extract original edges if needed
        if (showOriginalEdges && !edgeComponent->getEdgeNode(EdgeType::Original)) {
            LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Extracting original edges");
            edgeComponent->extractOriginalEdges(shape,
                                               80.0, 0.01, false,
                                               config.edges.originalEdge.color,
                                               config.edges.originalEdge.width,
                                               false,
                                               Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
                                               3.0);
        }
        
        // Apply appearance to original edges if they already exist
        if (showOriginalEdges && edgeComponent->getEdgeNode(EdgeType::Original)) {
            LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Applying appearance to existing original edges");
            edgeComponent->applyAppearanceToEdgeNode(EdgeType::Original,
                                                     config.edges.originalEdge.color,
                                                     config.edges.originalEdge.width,
                                                     0);
        }
        
        // Extract mesh edges if needed (for HiddenLine mode)
        if (showMeshEdges && !edgeComponent->getEdgeNode(EdgeType::Mesh) && !shape.IsNull()) {
            LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Extracting mesh edges from shape");
            Quantity_Color edgeColor = config.edges.meshEdge.color;
            if (config.edges.meshEdge.useEffectiveColor) {
                if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
                    edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                }
            }
            // Extract mesh edges from triangulated surface (matching DisplayModeRenderer logic)
            auto& manager = RenderingToolkitAPI::getManager();
            auto processor = manager.getGeometryProcessor("OpenCASCADE");
            if (processor) {
                MeshParameters defaultParams;
                defaultParams.deflection = 0.01;
                defaultParams.angularDeflection = 0.5;
                TriangleMesh mesh = processor->convertToMesh(shape, defaultParams);
                if (!mesh.triangles.empty()) {
                    edgeComponent->extractMeshEdges(mesh, edgeColor, config.edges.meshEdge.width);
                    LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Mesh edges extracted successfully");
                } else {
                    LOG_WRN_S("BRepDisplayModeHandler::updateSwitchVisibility: Failed to convert shape to mesh for edge extraction");
                }
            } else {
                LOG_WRN_S("BRepDisplayModeHandler::updateSwitchVisibility: OpenCASCADE geometry processor not available");
            }
        }
        
        // Apply appearance to mesh edges if they already exist
        if (showMeshEdges && edgeComponent->getEdgeNode(EdgeType::Mesh)) {
            LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Applying appearance to existing mesh edges");
            Quantity_Color edgeColor = config.edges.meshEdge.color;
            if (config.edges.meshEdge.useEffectiveColor) {
                if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
                    edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                }
            }
            edgeComponent->applyAppearanceToEdgeNode(EdgeType::Mesh,
                                                     edgeColor,
                                                     config.edges.meshEdge.width,
                                                     0);
        }
        
        // Update edge display
        edgeComponent->updateEdgeDisplay(m_edgesNode);
        LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Updated edge display in edges node");
    }
    
    int edgesSwitchValue = (showOriginalEdges || showMeshEdges) ? 0 : -1;
    int oldEdgesValue = m_edgesSwitch->whichChild.getValue();
    m_edgesSwitch->whichChild.setValue(edgesSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Edges switch: " + std::to_string(oldEdgesValue) + 
              " -> " + std::to_string(edgesSwitchValue));
    
    // Update points switch visibility
    int pointsSwitchValue = config.nodes.requirePoints ? 0 : -1;
    int oldPointsValue = m_pointsSwitch->whichChild.getValue();
    m_pointsSwitch->whichChild.setValue(pointsSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Points switch: " + std::to_string(oldPointsValue) + 
              " -> " + std::to_string(pointsSwitchValue));
    
    LOG_INF_S("BRepDisplayModeHandler::updateSwitchVisibility: Switch visibility update completed");
}

void BRepDisplayModeHandler::updateDisplayModeSwitches(SoSeparator* coinNode,
                                                       RenderingConfig::DisplayMode mode,
                                                       ModularEdgeComponent* edgeComponent) {
    if (!coinNode || !m_useSwitchMode) {
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Invalid parameters (coinNode=" + 
                  std::string(coinNode ? "valid" : "null") + ", m_useSwitchMode=" + std::string(m_useSwitchMode ? "true" : "false") + ")");
        return;
    }
    
    LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Starting switch update for mode=" + 
              std::to_string(static_cast<int>(mode)));
    
    // CRITICAL: Always use internal switches (m_surfaceSwitch, m_edgesSwitch, m_pointsSwitch)
    // These are guaranteed to have the correct child nodes (m_surfaceNode, m_edgesNode, m_pointsNode)
    // Do NOT use switches found in coinNode as they may not be correctly initialized
    
    if (!m_surfaceSwitch || !m_edgesSwitch || !m_pointsSwitch) {
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Internal switches not initialized (surface=" + 
                  std::string(m_surfaceSwitch ? "valid" : "null") + ", edges=" + std::string(m_edgesSwitch ? "valid" : "null") + 
                  ", points=" + std::string(m_pointsSwitch ? "valid" : "null") + ")");
        return;
    }
    
    // Ensure switches are added to coinNode (they should already be there from initializeSwitchStructure)
    bool surfaceSwitchInCoinNode = false;
    bool edgesSwitchInCoinNode = false;
    bool pointsSwitchInCoinNode = false;
    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (child == m_surfaceSwitch) {
            surfaceSwitchInCoinNode = true;
        } else if (child == m_edgesSwitch) {
            edgesSwitchInCoinNode = true;
        } else if (child == m_pointsSwitch) {
            pointsSwitchInCoinNode = true;
        }
    }
    
    if (!surfaceSwitchInCoinNode) {
        coinNode->addChild(m_surfaceSwitch);
        LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Added m_surfaceSwitch to coinNode");
    }
    if (!edgesSwitchInCoinNode) {
        coinNode->addChild(m_edgesSwitch);
        LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Added m_edgesSwitch to coinNode");
    }
    if (!pointsSwitchInCoinNode) {
        coinNode->addChild(m_pointsSwitch);
        LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Added m_pointsSwitch to coinNode");
    }
    
    // Verify that switches have correct child nodes
    if (m_surfaceSwitch->getNumChildren() == 0) {
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: m_surfaceSwitch has no children!");
    } else if (m_surfaceSwitch->getChild(0) != m_surfaceNode) {
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: m_surfaceSwitch child mismatch!");
    }
    if (m_edgesSwitch->getNumChildren() == 0) {
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: m_edgesSwitch has no children!");
    } else if (m_edgesSwitch->getChild(0) != m_edgesNode) {
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: m_edgesSwitch child mismatch!");
    }
    if (m_pointsSwitch->getNumChildren() == 0) {
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: m_pointsSwitch has no children!");
    } else if (m_pointsSwitch->getChild(0) != m_pointsNode) {
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: m_pointsSwitch child mismatch!");
    }
    
    // Use internal switches
    SoSwitch* surfaceSwitch = m_surfaceSwitch;
    SoSwitch* edgesSwitch = m_edgesSwitch;
    SoSwitch* pointsSwitch = m_pointsSwitch;
    
    // Create default context for this mode
    // IMPORTANT: Set context values based on mode to ensure correct config generation
    GeometryRenderContext defaultContext;
    defaultContext.display.displayMode = mode;
    
    // Set display flags based on mode
    if (mode == RenderingConfig::DisplayMode::Points) {
        defaultContext.display.facesVisible = false;  // Points mode hides surface
        defaultContext.display.showPointView = true;
        defaultContext.display.showSolidWithPointView = false;  // Pure points mode, not surface+points
    } else if (mode == RenderingConfig::DisplayMode::Wireframe) {
        defaultContext.display.facesVisible = false;  // Wireframe mode hides surface
        defaultContext.display.showPointView = false;
        defaultContext.display.showSolidWithPointView = false;
    } else {
        defaultContext.display.facesVisible = true;  // Other modes show surface
        defaultContext.display.showPointView = false;
        defaultContext.display.showSolidWithPointView = false;
    }
    
    defaultContext.material.ambientColor = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB);
    defaultContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
    defaultContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    defaultContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    defaultContext.material.shininess = 30.0;
    defaultContext.material.transparency = 0.0;
    defaultContext.display.wireframeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    defaultContext.display.wireframeWidth = 1.0;
    defaultContext.texture.enabled = false;
    defaultContext.blend.blendMode = RenderingConfig::BlendMode::None;
    
    // Get configuration for this mode
    DisplayModeConfig config = DisplayModeConfigFactory::getConfig(mode, defaultContext);
    
    LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Config loaded - requireSurface=" + 
              std::string(config.nodes.requireSurface ? "true" : "false") + ", requireOriginalEdges=" + 
              std::string(config.nodes.requireOriginalEdges ? "true" : "false") + ", requirePoints=" + 
              std::string(config.nodes.requirePoints ? "true" : "false") + ", originalEdge.enabled=" + 
              std::string(config.edges.originalEdge.enabled ? "true" : "false"));
    
    // Update switch visibility - simplified: only check if surface node has children
    int surfaceSwitchValue = -1;
    if (config.nodes.requireSurface && m_surfaceNode && m_surfaceNode->getNumChildren() > 0) {
        surfaceSwitchValue = 0;
    }
    int oldSurfaceValue = surfaceSwitch->whichChild.getValue();
    surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Surface switch: " + std::to_string(oldSurfaceValue) + 
              " -> " + std::to_string(surfaceSwitchValue) + " (requireSurface=" + 
              std::string(config.nodes.requireSurface ? "true" : "false") + 
              ", surfaceNode children=" + std::to_string(m_surfaceNode ? m_surfaceNode->getNumChildren() : 0) + ")");
    
    // Update edges switch visibility and extract edges if needed
    bool showOriginalEdges = config.nodes.requireOriginalEdges && config.edges.originalEdge.enabled;
    bool showMeshEdges = config.nodes.requireMeshEdges && config.edges.meshEdge.enabled;
    
    // Process edges if needed (matching preview canvas logic)
    if (m_edgesNode && edgeComponent) {
        // Clear edges node
        m_edgesNode->removeAllChildren();
        
        // Set edge display flags based on config
        edgeComponent->setEdgeDisplayType(EdgeType::Original, showOriginalEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, showMeshEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        
        // Extract original edges if needed
        if (showOriginalEdges && !edgeComponent->getEdgeNode(EdgeType::Original)) {
            LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Original edges will be extracted when shape is available");
            // Note: Original edges extraction requires shape, which is not available in this function
            // They should be extracted during initialization or when shape is available
        }
        
        // Apply appearance to original edges if they already exist
        if (showOriginalEdges && edgeComponent->getEdgeNode(EdgeType::Original)) {
            edgeComponent->applyAppearanceToEdgeNode(EdgeType::Original,
                                                     config.edges.originalEdge.color,
                                                     config.edges.originalEdge.width,
                                                     0);
        }
        
        // Extract mesh edges if needed (for HiddenLine mode)
        // Note: Mesh edges extraction requires shape or mesh data
        // In switch mode, mesh edges should be extracted during initialization
        // or we need to get shape from geometry object
        if (showMeshEdges && !edgeComponent->getEdgeNode(EdgeType::Mesh)) {
            LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Mesh edges will be extracted when shape/mesh is available");
            // Note: Mesh edges extraction requires shape or mesh, which is not available in this function
            // They should be extracted during initialization or when shape/mesh is available
        }
        
        // Apply appearance to mesh edges if they already exist
        if (showMeshEdges && edgeComponent->getEdgeNode(EdgeType::Mesh)) {
            Quantity_Color edgeColor = config.edges.meshEdge.color;
            if (config.edges.meshEdge.useEffectiveColor) {
                if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
                    edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                }
            }
            edgeComponent->applyAppearanceToEdgeNode(EdgeType::Mesh,
                                                     edgeColor,
                                                     config.edges.meshEdge.width,
                                                     0);
        }
        
        // Update edge display
        edgeComponent->updateEdgeDisplay(m_edgesNode);
        
        // Add polygon offset for edges if configured (matching preview canvas logic)
        if (config.postProcessing.polygonOffset.enabled) {
            SoPolygonOffset* edgePolygonOffset = new SoPolygonOffset();
            edgePolygonOffset->factor.setValue((float)config.postProcessing.polygonOffset.factor);
            edgePolygonOffset->units.setValue((float)config.postProcessing.polygonOffset.units);
            edgePolygonOffset->styles.setValue(SoPolygonOffset::LINES);
            m_edgesNode->addChild(edgePolygonOffset);
        } else {
            // Default behavior: use negative offset to bring edges forward
            SoPolygonOffset* edgePolygonOffset = new SoPolygonOffset();
            edgePolygonOffset->factor.setValue(-1.0f);
            edgePolygonOffset->units.setValue(-1.0f);
            edgePolygonOffset->styles.setValue(SoPolygonOffset::LINES);
            m_edgesNode->addChild(edgePolygonOffset);
        }
    }
    
    int edgesSwitchValue = (showOriginalEdges || showMeshEdges) ? 0 : -1;
    // Only show edges switch if edges node has children (edges were extracted)
    if (edgesSwitchValue == 0 && m_edgesNode && m_edgesNode->getNumChildren() == 0) {
        edgesSwitchValue = -1;  // Hide if no edges were extracted
        LOG_WRN_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Edges switch set to -1 because no edges were extracted");
    }
    int oldEdgesValue = edgesSwitch->whichChild.getValue();
    edgesSwitch->whichChild.setValue(edgesSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Edges switch: " + std::to_string(oldEdgesValue) + 
              " -> " + std::to_string(edgesSwitchValue) + " (requireOriginalEdges=" + 
              std::string(config.nodes.requireOriginalEdges ? "true" : "false") + ", requireMeshEdges=" +
              std::string(config.nodes.requireMeshEdges ? "true" : "false") + ", originalEdge.enabled=" + 
              std::string(config.edges.originalEdge.enabled ? "true" : "false") + ", meshEdge.enabled=" +
              std::string(config.edges.meshEdge.enabled ? "true" : "false") + ", showOriginalEdges=" + 
              std::string(showOriginalEdges ? "true" : "false") + ", showMeshEdges=" +
              std::string(showMeshEdges ? "true" : "false") + ", edgesNode children=" + 
              std::to_string(m_edgesNode ? m_edgesNode->getNumChildren() : 0) + ")");
    
    // Update points switch visibility
    int pointsSwitchValue = -1;
    if (config.nodes.requirePoints && m_pointsNode && m_pointsNode->getNumChildren() > 0) {
        pointsSwitchValue = 0;
    }
    int oldPointsValue = pointsSwitch->whichChild.getValue();
    pointsSwitch->whichChild.setValue(pointsSwitchValue);
    LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Points switch: " + std::to_string(oldPointsValue) + 
              " -> " + std::to_string(pointsSwitchValue) + " (requirePoints=" + 
              std::string(config.nodes.requirePoints ? "true" : "false") + 
              ", pointsNode children=" + std::to_string(m_pointsNode ? m_pointsNode->getNumChildren() : 0) + ")");
    
    // CRITICAL: State nodes are NOT updated here - they are removed and rebuilt in handleDisplayMode
    // This function only updates Switch visibility (matching MeshDisplayModeHandler behavior)
    // State nodes will be rebuilt when handleDisplayMode is called with the new mode
    
    LOG_INF_S("BRepDisplayModeHandler::updateDisplayModeSwitches: Switch update completed successfully");
}



