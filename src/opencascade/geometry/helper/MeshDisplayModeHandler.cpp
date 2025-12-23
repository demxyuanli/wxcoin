#include "geometry/helper/MeshDisplayModeHandler.h"
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
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoTexture2.h>
#include <OpenCASCADE/Quantity_Color.hxx>

MeshDisplayModeHandler::MeshDisplayModeHandler()
    : m_modeSwitch(nullptr)
    , m_useSwitchMode(false)
    , m_surfaceSwitch(nullptr)
    , m_edgesSwitch(nullptr)
    , m_pointsSwitch(nullptr)
    , m_surfaceNode(nullptr)
    , m_edgesNode(nullptr)
    , m_pointsNode(nullptr)
{
}

MeshDisplayModeHandler::~MeshDisplayModeHandler()
{
}

void MeshDisplayModeHandler::setModeSwitch(SoSwitch* modeSwitch)
{
    m_modeSwitch = modeSwitch;
    m_useSwitchMode = (m_modeSwitch != nullptr);
}

void MeshDisplayModeHandler::handleDisplayMode(SoSeparator* coinNode, 
                                               const GeometryRenderContext& context,
                                               const TriangleMesh& mesh,
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
    if (configUseSwitchMode) {
        // Preview canvas approach: Three independent switches for surface, edges, and points
        // Structure matches DisplayModePreviewCanvas for consistency
        
        DisplayModeNodeManager nodeManager;
        nodeManager.resetAllRenderStates(coinNode, edgeComponent);
        
        // Get configuration for current display mode
        DisplayModeConfig currentConfig = DisplayModeConfigFactory::getConfig(displayMode, context);
        
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
            
            // Find surface/edges/points nodes from switches
            if (m_surfaceSwitch->getNumChildren() > 0) {
                SoNode* surfaceChild = m_surfaceSwitch->getChild(0);
                if (surfaceChild && surfaceChild->isOfType(SoSeparator::getClassTypeId())) {
                    m_surfaceNode = static_cast<SoSeparator*>(surfaceChild);
                    LOG_INF_S("MeshDisplayModeHandler::handleDisplayMode: Found existing surface node with " + 
                              std::to_string(m_surfaceNode->getNumChildren()) + " children");
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
            
            std::string msg = "MeshDisplayModeHandler::handleDisplayMode: Using existing switches from coinNode - ";
            msg += "surfaceNode=";
            msg += (m_surfaceNode ? "valid" : "null");
            msg += ", edgesNode=";
            msg += (m_edgesNode ? "valid" : "null");
            msg += ", pointsNode=";
            msg += (m_pointsNode ? "valid" : "null");
            LOG_INF_S(msg);
        }
        
        // Initialize switch structure (only once)
        if (!m_surfaceSwitch || !m_edgesSwitch || !m_pointsSwitch) {
            LOG_INF_S("MeshDisplayModeHandler::handleDisplayMode: Initializing new switch structure (config.useSwitchMode=true)");
            initializeSwitchStructure(coinNode, nodeManager, context, mesh, params, 
                                    renderBuilder, pointViewBuilder);
        }
        
        // Update m_useSwitchMode to reflect that switches are now available
        m_useSwitchMode = true;
        
        // Update state nodes based on current mode configuration
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
        updateSwitchVisibility(currentConfig, mesh, displayMode, edgeComponent, useModularEdgeComponent, 
                             nodeManager, coinNode);
        
        return;
    }
    
    // Direct mode: Config says useSwitchMode=false, or switches not available
    LOG_INF_S("MeshDisplayModeHandler::handleDisplayMode: Config says useSwitchMode=false, using Direct mode");
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
    state.meshEdgeColor = context.material.diffuseColor;
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

    // CRITICAL: For pure mesh models, convert showOriginalEdges to showMeshEdges
    // Pure mesh models don't have original edges (TopoDS_Shape), only mesh edges
    if (state.showOriginalEdges) {
        state.showMeshEdges = true;
        state.showOriginalEdges = false;
    }

    // ===== Step 4: Apply render state to scene graph =====
    DisplayModeRenderer renderer;
    renderer.applyRenderState(coinNode, state, context, mesh, params, edgeComponent, 
                     useModularEdgeComponent, renderBuilder, wireframeBuilder, pointViewBuilder);
}

void MeshDisplayModeHandler::initializeSwitchStructure(SoSeparator* coinNode, 
                                                        DisplayModeNodeManager& nodeManager,
                                                        const GeometryRenderContext& context,
                                                        const TriangleMesh& mesh,
                                                        const MeshParameters& params,
                                                        helper::RenderNodeBuilder* renderBuilder,
                                                        helper::PointViewBuilder* pointViewBuilder) {
    if (!coinNode || !renderBuilder) {
        return;
    }
    
    // Create three independent switches following preview canvas structure
    m_surfaceSwitch = new SoSwitch();
    m_surfaceSwitch->ref();
    
    m_edgesSwitch = new SoSwitch();
    m_edgesSwitch->ref();
    
    m_pointsSwitch = new SoSwitch();
    m_pointsSwitch->ref();
    
    // Create surface node (contains state nodes and geometry)
    m_surfaceNode = new SoSeparator();
    m_surfaceNode->ref();
    
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
    // First, try to move existing geometry from coinNode to m_surfaceNode
    bool geometryMoved = false;
    if (nodeManager.hasSurfaceGeometryNode(coinNode)) {
        // Move existing geometry to surface node
        for (int i = coinNode->getNumChildren() - 1; i >= 0; --i) {
            SoNode* child = coinNode->getChild(i);
            if (child && nodeManager.containsGeometryNode(child)) {
                coinNode->removeChild(i);
                m_surfaceNode->addChild(child);
                geometryMoved = true;
                LOG_INF_S("MeshDisplayModeHandler::initializeSwitchStructure: Moved existing geometry node to surface node");
            }
        }
    }
    
    // If no geometry was moved and any mode requires surface, create new geometry
    if (!geometryMoved && anyModeRequiresSurface && !mesh.isEmpty() && !mesh.triangles.empty()) {
        LOG_INF_S("MeshDisplayModeHandler::initializeSwitchStructure: Creating new surface geometry");
        auto& manager = RenderingToolkitAPI::getManager();
        auto backend = manager.getRenderBackend("Coin3D");
        if (backend) {
            auto sceneNode = backend->createSceneNode(mesh, false, 
                context.material.diffuseColor, context.material.ambientColor,
                context.material.specularColor, context.material.emissiveColor,
                context.material.shininess, context.material.transparency);
            if (sceneNode) {
                SoSeparator* meshNode = sceneNode.get();
                meshNode->ref();
                m_surfaceNode->addChild(meshNode);
                LOG_INF_S("MeshDisplayModeHandler::initializeSwitchStructure: Created mesh geometry node");
            }
        }
    } else if (!anyModeRequiresSurface) {
        LOG_INF_S("MeshDisplayModeHandler::initializeSwitchStructure: No mode requires surface geometry, skipping geometry creation");
    }
    
    // Add surface node to surface switch (only if not already added)
    if (m_surfaceSwitch->getNumChildren() == 0) {
        m_surfaceSwitch->addChild(m_surfaceNode);
    }
    // Initialize surface switch to show (0) if geometry exists, hide (-1) otherwise
    m_surfaceSwitch->whichChild.setValue((m_surfaceNode->getNumChildren() > 0) ? 0 : -1);
    LOG_INF_S("MeshDisplayModeHandler::initializeSwitchStructure: Surface switch initialized with whichChild=" + 
              std::to_string(m_surfaceSwitch->whichChild.getValue()) + " (surfaceNode has " + 
              std::to_string(m_surfaceNode->getNumChildren()) + " children)");
    
    // Create edges node (only if not already exists)
    if (!m_edgesNode) {
        m_edgesNode = new SoSeparator();
        m_edgesNode->ref();
    }
    if (m_edgesSwitch->getNumChildren() == 0) {
        m_edgesSwitch->addChild(m_edgesNode);
    }
    m_edgesSwitch->whichChild.setValue(-1);  // Hide by default
    LOG_INF_S("MeshDisplayModeHandler::initializeSwitchStructure: Edges switch initialized with whichChild=-1");
    
    // Create points node - only if not already exists
    if (!m_pointsNode) {
        m_pointsNode = new SoSeparator();
        m_pointsNode->ref();
    }
    
    // Build points geometry if needed
    SoSeparator* existingPointViewNode = nodeManager.findPointViewNode(coinNode);
    bool pointsCreated = false;
    if (!existingPointViewNode && !mesh.isEmpty() && !mesh.vertices.empty() && pointViewBuilder) {
        GeometryRenderContext pointsContext = context;
        pointsContext.display.displayMode = RenderingConfig::DisplayMode::Points;
        DisplayModeConfig pointsConfig = DisplayModeConfigFactory::getConfig(RenderingConfig::DisplayMode::Points, pointsContext);
        if (pointsConfig.nodes.requirePoints) {
            pointViewBuilder->createPointViewRepresentation(m_pointsNode, mesh, context.display);
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
    m_pointsSwitch->whichChild.setValue(pointsCreated ? 0 : -1);
    LOG_INF_S("MeshDisplayModeHandler::initializeSwitchStructure: Points switch initialized with whichChild=" + 
              std::to_string(m_pointsSwitch->whichChild.getValue()) + " (pointsCreated=" + 
              std::string(pointsCreated ? "true" : "false") + ")");
    
    // Add switches to coin node
    coinNode->addChild(m_surfaceSwitch);
    coinNode->addChild(m_edgesSwitch);
    coinNode->addChild(m_pointsSwitch);
    
    LOG_INF_S("MeshDisplayModeHandler::initializeSwitchStructure: Switch structure initialized - surfaceNode children=" + 
              std::to_string(m_surfaceNode->getNumChildren()) + ", edgesNode children=" + 
              std::to_string(m_edgesNode->getNumChildren()) + ", pointsNode children=" + 
              std::to_string(m_pointsNode->getNumChildren()));
}

void MeshDisplayModeHandler::updateSwitchVisibility(const DisplayModeConfig& config,
                                                    const TriangleMesh& mesh,
                                                    RenderingConfig::DisplayMode displayMode,
                                                    ModularEdgeComponent* edgeComponent,
                                                    bool useModularEdgeComponent,
                                                    DisplayModeNodeManager& nodeManager,
                                                    SoSeparator* coinNode) {
    if (!m_surfaceSwitch || !m_edgesSwitch || !m_pointsSwitch) {
        LOG_WRN_S("MeshDisplayModeHandler::updateSwitchVisibility: Switches not initialized");
        return;
    }
    
    LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Updating switch visibility for displayMode=" + 
              std::to_string(static_cast<int>(displayMode)) + " - requireSurface=" + 
              std::string(config.nodes.requireSurface ? "true" : "false") + ", requireMeshEdges=" + 
              std::string(config.nodes.requireMeshEdges ? "true" : "false") + ", requireOriginalEdges=" + 
              std::string(config.nodes.requireOriginalEdges ? "true" : "false") + ", requirePoints=" + 
              std::string(config.nodes.requirePoints ? "true" : "false"));
    
    // Update surface switch visibility
    int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
    int oldSurfaceValue = m_surfaceSwitch->whichChild.getValue();
    m_surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Surface switch: " + std::to_string(oldSurfaceValue) + 
              " -> " + std::to_string(surfaceSwitchValue));
    
    // Update edges switch visibility
    // For pure mesh models, convert requireOriginalEdges to requireMeshEdges
    bool showMeshEdges = config.nodes.requireMeshEdges || config.nodes.requireOriginalEdges;
    if (displayMode == RenderingConfig::DisplayMode::Wireframe) {
        showMeshEdges = true;  // Wireframe always shows edges
        LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Wireframe mode - forcing showMeshEdges=true");
    }
    
    bool showEdges = showMeshEdges && config.edges.meshEdge.enabled;
    
    LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Edge display logic - showMeshEdges=" + 
              std::string(showMeshEdges ? "true" : "false") + ", meshEdge.enabled=" + 
              std::string(config.edges.meshEdge.enabled ? "true" : "false") + ", showEdges=" + 
              std::string(showEdges ? "true" : "false") + ", useModularEdgeComponent=" + 
              std::string(useModularEdgeComponent ? "true" : "false") + ", edgeComponent=" + 
              std::string(edgeComponent ? "valid" : "null"));
    
    if (useModularEdgeComponent && edgeComponent) {
        // Clear edges node
        m_edgesNode->removeAllChildren();
        LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Cleared edges node");
        
        // Set edge display flags
        edgeComponent->setEdgeDisplayType(EdgeType::Original, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, showEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        
        // Extract mesh edges if needed
        if (showEdges && !edgeComponent->getEdgeNode(EdgeType::Mesh) && !mesh.triangles.empty()) {
            LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Extracting mesh edges");
            Quantity_Color edgeColor = config.edges.meshEdge.color;
            
            // Handle effective color for HiddenLine mode
            if (config.edges.meshEdge.useEffectiveColor) {
                if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
                    edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                    LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Applied effective color (black)");
                }
            }
            
            // For Wireframe and NoShading, use original edge color if available
            if (displayMode == RenderingConfig::DisplayMode::Wireframe ||
                displayMode == RenderingConfig::DisplayMode::NoShading) {
                edgeColor = config.edges.originalEdge.color;
                LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Using original edge color for Wireframe/NoShading");
            }
            
            edgeComponent->extractMeshEdges(mesh, edgeColor, config.edges.meshEdge.width);
        }
        
        // Apply appearance if edges already exist
        if (showEdges && edgeComponent->getEdgeNode(EdgeType::Mesh)) {
            LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Applying appearance to existing mesh edges");
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
        
        // Clear silhouette edge node for HiddenLine mode
        if (displayMode == RenderingConfig::DisplayMode::HiddenLine) {
            LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Clearing silhouette edge node for HiddenLine mode");
            edgeComponent->clearSilhouetteEdgeNode();
        }
        
        // Update edge display
        edgeComponent->updateEdgeDisplay(m_edgesNode);
        LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Updated edge display in edges node");
    }
    
    int edgesSwitchValue = showEdges ? 0 : -1;
    int oldEdgesValue = m_edgesSwitch->whichChild.getValue();
    m_edgesSwitch->whichChild.setValue(edgesSwitchValue);
    LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Edges switch: " + std::to_string(oldEdgesValue) + 
              " -> " + std::to_string(edgesSwitchValue));
    
    // Update points switch visibility
    int pointsSwitchValue = config.nodes.requirePoints ? 0 : -1;
    int oldPointsValue = m_pointsSwitch->whichChild.getValue();
    m_pointsSwitch->whichChild.setValue(pointsSwitchValue);
    LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Points switch: " + std::to_string(oldPointsValue) + 
              " -> " + std::to_string(pointsSwitchValue));
    
    LOG_INF_S("MeshDisplayModeHandler::updateSwitchVisibility: Switch visibility update completed");
}

void MeshDisplayModeHandler::updateDisplayModeSwitches(SoSeparator* coinNode,
                                                        RenderingConfig::DisplayMode mode,
                                                        ModularEdgeComponent* edgeComponent) {
    if (!coinNode || !m_useSwitchMode) {
        LOG_WRN_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Invalid parameters (coinNode=" + 
                  std::string(coinNode ? "valid" : "null") + ", m_useSwitchMode=" + std::string(m_useSwitchMode ? "true" : "false") + ")");
        return;
    }
    
    LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Starting switch update for mode=" + 
              std::to_string(static_cast<int>(mode)));
    
    // Find the three switches in coinNode (they should be direct children)
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
    
    LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Found " + std::to_string(switchCount) + 
              " switch nodes in coinNode");
    
    // If switches are not found, they may not be initialized yet
    // In that case, use internal switches if available
    if (!surfaceSwitch && m_surfaceSwitch) {
        surfaceSwitch = m_surfaceSwitch;
        LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Using internal m_surfaceSwitch");
    }
    if (!edgesSwitch && m_edgesSwitch) {
        edgesSwitch = m_edgesSwitch;
        LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Using internal m_edgesSwitch");
    }
    if (!pointsSwitch && m_pointsSwitch) {
        pointsSwitch = m_pointsSwitch;
        LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Using internal m_pointsSwitch");
    }
    
    if (!surfaceSwitch || !edgesSwitch || !pointsSwitch) {
        LOG_WRN_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Switches not initialized (surface=" + 
                  std::string(surfaceSwitch ? "valid" : "null") + ", edges=" + std::string(edgesSwitch ? "valid" : "null") + 
                  ", points=" + std::string(pointsSwitch ? "valid" : "null") + ")");
        return;
    }
    
    // Create default context for this mode
    GeometryRenderContext defaultContext;
    defaultContext.display.displayMode = mode;
    defaultContext.display.facesVisible = true;
    defaultContext.display.showPointView = false;
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
    
    LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Config loaded - requireSurface=" + 
              std::string(config.nodes.requireSurface ? "true" : "false") + ", requireMeshEdges=" + 
              std::string(config.nodes.requireMeshEdges ? "true" : "false") + ", requireOriginalEdges=" + 
              std::string(config.nodes.requireOriginalEdges ? "true" : "false") + ", requirePoints=" + 
              std::string(config.nodes.requirePoints ? "true" : "false") + ", meshEdge.enabled=" + 
              std::string(config.edges.meshEdge.enabled ? "true" : "false"));
    
    // Update switch visibility
    int surfaceSwitchValue = config.nodes.requireSurface ? 0 : -1;
    int oldSurfaceValue = surfaceSwitch->whichChild.getValue();
    surfaceSwitch->whichChild.setValue(surfaceSwitchValue);
    LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Surface switch: " + std::to_string(oldSurfaceValue) + 
              " -> " + std::to_string(surfaceSwitchValue) + " (requireSurface=" + 
              std::string(config.nodes.requireSurface ? "true" : "false") + ")");
    
    // Update edges switch visibility (for mesh, convert requireOriginalEdges to requireMeshEdges)
    bool showMeshEdges = config.nodes.requireMeshEdges || config.nodes.requireOriginalEdges;
    if (mode == RenderingConfig::DisplayMode::Wireframe) {
        showMeshEdges = true;  // Wireframe always shows edges
        LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Wireframe mode - forcing showMeshEdges=true");
    }
    bool showEdges = showMeshEdges && config.edges.meshEdge.enabled;
    int edgesSwitchValue = showEdges ? 0 : -1;
    int oldEdgesValue = edgesSwitch->whichChild.getValue();
    edgesSwitch->whichChild.setValue(edgesSwitchValue);
    LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Edges switch: " + std::to_string(oldEdgesValue) + 
              " -> " + std::to_string(edgesSwitchValue) + " (requireMeshEdges=" + 
              std::string(config.nodes.requireMeshEdges ? "true" : "false") + ", requireOriginalEdges=" + 
              std::string(config.nodes.requireOriginalEdges ? "true" : "false") + ", meshEdge.enabled=" + 
              std::string(config.edges.meshEdge.enabled ? "true" : "false") + ", showMeshEdges=" + 
              std::string(showMeshEdges ? "true" : "false") + ", showEdges=" + std::string(showEdges ? "true" : "false") + ")");
    
    // Update points switch visibility
    int pointsSwitchValue = config.nodes.requirePoints ? 0 : -1;
    int oldPointsValue = pointsSwitch->whichChild.getValue();
    pointsSwitch->whichChild.setValue(pointsSwitchValue);
    LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Points switch: " + std::to_string(oldPointsValue) + 
              " -> " + std::to_string(pointsSwitchValue) + " (requirePoints=" + 
              std::string(config.nodes.requirePoints ? "true" : "false") + ")");
    
    LOG_INF_S("MeshDisplayModeHandler::updateDisplayModeSwitches: Switch update completed successfully");
}

int MeshDisplayModeHandler::getModeSwitchIndex(RenderingConfig::DisplayMode mode) {
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        return 0;
    case RenderingConfig::DisplayMode::Points:
        return 1;
    case RenderingConfig::DisplayMode::Wireframe:
        return 2;
    case RenderingConfig::DisplayMode::Solid:
        return 3;
    case RenderingConfig::DisplayMode::FlatLines:
        return 4;
    case RenderingConfig::DisplayMode::Transparent:
        return 5;
    case RenderingConfig::DisplayMode::HiddenLine:
        return 6;
    default:
        return 3;
    }
}



