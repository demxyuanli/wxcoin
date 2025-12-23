#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/helper/BRepDisplayModeHandler.h"
#include "geometry/helper/MeshDisplayModeHandler.h"
#include "geometry/helper/DisplayModeStateManager.h"
#include "geometry/helper/DisplayModeNodeManager.h"
#include "rendering/PolygonModeNode.h"
#include "geometry/helper/RenderNodeBuilder.h"
#include "geometry/helper/WireframeBuilder.h"
#include "geometry/helper/PointViewBuilder.h"
#include "edges/ModularEdgeComponent.h"
#include "config/EdgeSettingsConfig.h"
#include "config/RenderingConfig.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/SoType.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <vector>
#include <sstream>
#include <iomanip>

namespace helper {

// Static member initialization
bool DisplayModeHandler::m_geometryBuilt = false;

// Helper function to convert DisplayMode to string for logging
static std::string displayModeToString(RenderingConfig::DisplayMode mode) {
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        return "NoShading";
    case RenderingConfig::DisplayMode::Points:
        return "Points";
    case RenderingConfig::DisplayMode::Wireframe:
        return "Wireframe";
    case RenderingConfig::DisplayMode::Solid:
        return "Solid";
    case RenderingConfig::DisplayMode::FlatLines:
        return "FlatLines";
    case RenderingConfig::DisplayMode::Transparent:
        return "Transparent";
    case RenderingConfig::DisplayMode::HiddenLine:
        return "HiddenLine";
    case RenderingConfig::DisplayMode::Custom:
        return "Custom";
    default:
        return "Unknown";
    }
}

DisplayModeHandler::DisplayModeHandler() 
    : m_brepHandler(std::make_unique<BRepDisplayModeHandler>())
    , m_meshHandler(std::make_unique<MeshDisplayModeHandler>())
    , m_modeSwitch(nullptr)
    , m_useSwitchMode(false)
{
    // Read switch mode configuration from RenderingConfig
    // Default to true (Switch mode) if config not available
    RenderingConfig& config = RenderingConfig::getInstance();
    m_useSwitchMode = config.getDisplaySettings().useSwitchMode;
    
    LOG_INF_S("DisplayModeHandler::DisplayModeHandler: Initialized with useSwitchMode=" + 
              std::string(m_useSwitchMode ? "true" : "false") + " (from config)");
}

DisplayModeHandler::~DisplayModeHandler() {
}

bool DisplayModeHandler::isGeometryBuilt() const {
    return m_geometryBuilt;
}

void DisplayModeHandler::setGeometryBuilt(bool built) {
    m_geometryBuilt = built;
}

void DisplayModeHandler::setModeSwitch(SoSwitch* modeSwitch) {
    m_modeSwitch = modeSwitch;
    
    // Read switch mode preference from config
    // If config says useSwitchMode=true, we'll use Switch mode even if m_modeSwitch is null
    // (the new architecture uses three independent switches, not m_modeSwitch)
    RenderingConfig& config = RenderingConfig::getInstance();
    bool configUseSwitchMode = config.getDisplaySettings().useSwitchMode;
    
    // Use Switch mode if:
    // 1. Config says useSwitchMode=true (preferred), OR
    // 2. m_modeSwitch is provided (legacy support)
    m_useSwitchMode = configUseSwitchMode || (m_modeSwitch != nullptr);
    
    LOG_INF_S("DisplayModeHandler::setModeSwitch: m_modeSwitch=" + 
              std::string(m_modeSwitch ? "valid" : "null") + 
              ", config.useSwitchMode=" + std::string(configUseSwitchMode ? "true" : "false") +
              ", m_useSwitchMode=" + std::string(m_useSwitchMode ? "true" : "false"));
    
    if (m_brepHandler) {
        m_brepHandler->setModeSwitch(modeSwitch);
    }
    if (m_meshHandler) {
        m_meshHandler->setModeSwitch(modeSwitch);
    }
}

void DisplayModeHandler::updateDisplayMode(SoSeparator* coinNode, RenderingConfig::DisplayMode mode,
                                           ModularEdgeComponent* edgeComponent,
                                           const Quantity_Color* originalDiffuseColor) {
    if (!coinNode) {
        return;
    }

    // Read switch mode preference from config
    RenderingConfig& config = RenderingConfig::getInstance();
    bool configUseSwitchMode = config.getDisplaySettings().useSwitchMode;
    
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
    
    LOG_INF_S("DisplayModeHandler::updateDisplayMode: Switching to mode=" + displayModeToString(mode) + 
              " using Direct mode (config.useSwitchMode=" + std::string(configUseSwitchMode ? "true" : "false") +
              ", m_useSwitchMode=" + std::string(m_useSwitchMode ? "true" : "false") + 
              ", m_modeSwitch=" + std::string(m_modeSwitch ? "valid" : "null") + ")");
    
    // Log non-Switch mode update with full state
    // Step 1: Extract material info from existing nodes BEFORE reset (they will be deleted)
    SoDrawStyle* tempDrawStyle = nullptr;
    SoMaterial* tempMaterial = nullptr;
    DisplayModeNodeManager nodeManager;
    nodeManager.findDrawStyleAndMaterial(coinNode, tempDrawStyle, tempMaterial);
    
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
    
    // Remove edge nodes
    if (edgeComponent) {
        nodeManager.cleanupEdgeNodes(coinNode, edgeComponent);
    }
    
    // Remove collected state nodes (use index-based removal for safety)
    // CRITICAL: Remove from back to front to avoid index shifting issues
    // This ensures that when we remove a node, indices of remaining nodes don't change
    if (coinNode) {
        for (int i = coinNode->getNumChildren() - 1; i >= 0; --i) {
            try {
                SoNode* child = coinNode->getChild(i);
                if (!child) continue;
                
                // Check if this node should be removed by comparing pointers
                bool shouldRemove = false;
                for (auto* node : stateNodesToRemove) {
                    if (node && child == node) {
                        shouldRemove = true;
                        break;
                    }
                }
                if (!shouldRemove) {
                    for (auto* node : pointViewNodesToRemove) {
                        if (node && child == node) {
                            shouldRemove = true;
                            break;
                        }
                    }
                }
                if (!shouldRemove) {
                    for (auto* node : hiddenLineNodesToRemove) {
                        if (node && child == node) {
                            shouldRemove = true;
                            break;
                        }
                    }
                }
                
                if (shouldRemove) {
                    coinNode->removeChild(i);  // Use index instead of pointer for safety
                }
            } catch (...) {
                // Safety: If any operation fails, continue with next node
                // This prevents crash if node becomes invalid during iteration
                continue;
            }
        }
    }
    
    // Step 3: Build context - prioritize originalDiffuseColor if provided
    GeometryRenderContext updateContext;
    updateContext.display.displayMode = mode;
    updateContext.display.facesVisible = true;
    updateContext.display.showPointView = false;
    updateContext.display.wireframeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    updateContext.display.wireframeWidth = 1.0;
    
    // Force reset of non-diffuse material properties to prevent pollution from previous modes
    updateContext.material.ambientColor = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB);
    updateContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    updateContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    updateContext.material.shininess = 30.0;
    updateContext.material.transparency = 0.0;
    
    // Use originalDiffuseColor if provided, otherwise extract from existing material
    if (originalDiffuseColor) {  // Keep original diffuse color but reset everything else
        updateContext.material.diffuseColor = *originalDiffuseColor;
        // Extract other material properties from existing node if available
        if (tempMaterial) {
            if (tempMaterial->shininess.getNum() > 0) {
                updateContext.material.shininess = tempMaterial->shininess[0] * 100.0;
            }
            if (tempMaterial->transparency.getNum() > 0) {
                updateContext.material.transparency = tempMaterial->transparency[0];
            }
        }
    } else if (tempMaterial) {
        // Extract material from existing node
        // SoMaterial uses SoMFColor (multi-value), access first value with [0]
        // Only inherit diffuse, ignore others
        if (tempMaterial->diffuseColor.getNum() > 0) {
            const SbColor& diffuse = tempMaterial->diffuseColor[0];
            float r, g, b;
            diffuse.getValue(r, g, b);
            updateContext.material.diffuseColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
        } else {
            updateContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        }
        if (tempMaterial->shininess.getNum() > 0) {
            updateContext.material.shininess = tempMaterial->shininess[0] * 100.0;
        }
        if (tempMaterial->transparency.getNum() > 0) {
            updateContext.material.transparency = tempMaterial->transparency[0];
        }
    } else {
        // Use default material
        // Already set to defaults above
        updateContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
    }
    
    updateContext.texture.enabled = false;
    // Transparent mode needs Alpha blend mode
    updateContext.blend.blendMode = (mode == RenderingConfig::DisplayMode::Transparent) 
        ? RenderingConfig::BlendMode::Alpha 
        : RenderingConfig::BlendMode::None;
    
    // For Transparent mode, get transparency from RenderingConfig if not already set
    if (mode == RenderingConfig::DisplayMode::Transparent && updateContext.material.transparency <= 0.0) {
        RenderingConfig& config = RenderingConfig::getInstance();
        const auto& materialSettings = config.getMaterialSettings();
        if (materialSettings.transparency > 0.0) {
            updateContext.material.transparency = materialSettings.transparency;
        } else {
            updateContext.material.transparency = 0.5;  // Default transparency for Transparent mode
        }
    }
    
    // Step 4: Generate and set render state
    DisplayModeRenderState updateState;
    updateState.surfaceAmbientColor = updateContext.material.ambientColor;
    updateState.surfaceDiffuseColor = updateContext.material.diffuseColor;
    updateState.surfaceSpecularColor = updateContext.material.specularColor;
    updateState.surfaceEmissiveColor = updateContext.material.emissiveColor;
    updateState.shininess = updateContext.material.shininess;
    updateState.transparency = updateContext.material.transparency;
    updateState.originalEdgeColor = updateContext.display.wireframeColor;
    updateState.meshEdgeColor = updateContext.material.diffuseColor;
    updateState.originalEdgeWidth = updateContext.display.wireframeWidth;
    updateState.meshEdgeWidth = updateContext.display.wireframeWidth;
    updateState.textureEnabled = updateContext.texture.enabled;
    updateState.blendMode = updateContext.blend.blendMode;
    updateState.showPoints = updateContext.display.showPointView;
    updateState.surfaceDisplayMode = mode;
    
    DisplayModeStateManager stateManager;
    stateManager.setRenderStateForMode(updateState, mode, updateContext);

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
    // NOTE: This DrawStyle only controls how SURFACE geometry is rendered.
    // - Surface visibility is controlled by showSurface flag (via Switch or geometry presence)
    // - Edge rendering is handled separately by ModularEdgeComponent in Step 10
    // - For all modes that show surface, use FILLED (edges are rendered separately as SoIndexedLineSet)
    // - For modes that don't show surface (Wireframe, Points), this DrawStyle has no effect
    //   but we still set it to FILLED for consistency
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->ref();
    // All surface rendering modes use FILLED - edges are handled separately
    drawStyle->style.setValue(SoDrawStyle::FILLED);
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
        blendHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        blendHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
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

    // Step 10: Update edge display
    if (edgeComponent) {
        // Set edgeFlags to match state before updating display
        edgeComponent->setEdgeDisplayType(EdgeType::Original, updateState.showOriginalEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, updateState.showMeshEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        
        // Update edge display (this will add/remove edge nodes based on flags)
        edgeComponent->updateEdgeDisplay(coinNode);
    }
    
    coinNode->touch();
}

void DisplayModeHandler::handleDisplayMode(SoSeparator* coinNode, 
                                            const GeometryRenderContext& context,
                                            const TopoDS_Shape& shape,
                                            const MeshParameters& params,
                                            ModularEdgeComponent* edgeComponent,
                                            bool useModularEdgeComponent,
                                            helper::RenderNodeBuilder* renderBuilder,
                                            helper::WireframeBuilder* wireframeBuilder,
                                            helper::PointViewBuilder* pointViewBuilder) {
    if (!m_brepHandler) {
        return;
    }
    
    // Check face count and force Switch mode if threshold exceeded
    RenderingConfig& config = RenderingConfig::getInstance();
    int forceSwitchThreshold = config.getDisplaySettings().forceSwitchModeThreshold;
    bool configUseSwitchMode = config.getDisplaySettings().useSwitchMode;
    
    int faceCount = 0;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next(), ++faceCount);
    
    if (faceCount > forceSwitchThreshold && !configUseSwitchMode) {
        LOG_WRN_S("========================================");
        LOG_WRN_S("PERFORMANCE WARNING: Geometry has " + std::to_string(faceCount) + " faces");
        LOG_WRN_S("This exceeds the threshold of " + std::to_string(forceSwitchThreshold) + " faces");
        LOG_WRN_S("FORCING Switch mode regardless of UseSwitchMode=false setting");
        LOG_WRN_S("Direct mode would be too slow for this complex geometry");
        LOG_WRN_S("Mode switching performance would be severely degraded");
        LOG_WRN_S("========================================");
        // Override config setting for this geometry - force Switch mode
        // Temporarily modify config so handler will use Switch mode
        RenderingConfig::DisplaySettings settings = config.getDisplaySettings();
        settings.useSwitchMode = true;
        config.setDisplaySettings(settings);
        m_useSwitchMode = true;
    } else if (faceCount > forceSwitchThreshold) {
        LOG_INF_S("DisplayModeHandler: Geometry has " + std::to_string(faceCount) + 
                  " faces (threshold: " + std::to_string(forceSwitchThreshold) + 
                  "), using Switch mode for optimal performance");
    }
    
    m_brepHandler->handleDisplayMode(coinNode, context, shape, params, 
                                     edgeComponent, useModularEdgeComponent,
                                     renderBuilder, wireframeBuilder, pointViewBuilder);
    
    setGeometryBuilt(true);
}

// Overload for direct mesh creation (for STL/OBJ mesh-only geometries)
void DisplayModeHandler::handleDisplayMode(SoSeparator* coinNode, 
                                            const GeometryRenderContext& context,
                                            const TriangleMesh& mesh,
                                            const MeshParameters& params,
                                            ModularEdgeComponent* edgeComponent,
                                            bool useModularEdgeComponent,
                                            helper::RenderNodeBuilder* renderBuilder,
                                            helper::WireframeBuilder* wireframeBuilder,
                                            helper::PointViewBuilder* pointViewBuilder) {
    if (!m_meshHandler) {
        return;
    }
    
    // Check triangle count and force Switch mode if threshold exceeded
    RenderingConfig& config = RenderingConfig::getInstance();
    int forceSwitchThreshold = config.getDisplaySettings().forceSwitchModeThreshold;
    bool configUseSwitchMode = config.getDisplaySettings().useSwitchMode;
    
    int triangleCount = static_cast<int>(mesh.triangles.size() / 3);
    
    if (triangleCount > forceSwitchThreshold && !configUseSwitchMode) {
        LOG_WRN_S("========================================");
        LOG_WRN_S("PERFORMANCE WARNING: Geometry has " + std::to_string(triangleCount) + " triangles");
        LOG_WRN_S("This exceeds the threshold of " + std::to_string(forceSwitchThreshold) + " triangles");
        LOG_WRN_S("FORCING Switch mode regardless of UseSwitchMode=false setting");
        LOG_WRN_S("Direct mode would be too slow for this complex geometry");
        LOG_WRN_S("Mode switching performance would be severely degraded");
        LOG_WRN_S("========================================");
        // Override config setting for this geometry - force Switch mode
        // Temporarily modify config so handler will use Switch mode
        RenderingConfig::DisplaySettings settings = config.getDisplaySettings();
        settings.useSwitchMode = true;
        config.setDisplaySettings(settings);
        m_useSwitchMode = true;
    } else if (triangleCount > forceSwitchThreshold) {
        LOG_INF_S("DisplayModeHandler: Geometry has " + std::to_string(triangleCount) + 
                  " triangles (threshold: " + std::to_string(forceSwitchThreshold) + 
                  "), using Switch mode for optimal performance");
    }
    
    m_meshHandler->handleDisplayMode(coinNode, context, mesh, params,
                                     edgeComponent, useModularEdgeComponent,
                                     renderBuilder, wireframeBuilder, pointViewBuilder);
    
    setGeometryBuilt(true);
}

}
