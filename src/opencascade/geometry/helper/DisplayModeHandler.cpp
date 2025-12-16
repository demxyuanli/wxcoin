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
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <vector>
#include <sstream>
#include <iomanip>

// Static member initialization
bool DisplayModeHandler::m_geometryBuilt = false;

DisplayModeHandler::DisplayModeHandler() 
    : m_brepHandler(std::make_unique<BRepDisplayModeHandler>())
    , m_meshHandler(std::make_unique<MeshDisplayModeHandler>())
    , m_modeSwitch(nullptr)
    , m_useSwitchMode(false)
{
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
    m_useSwitchMode = (m_modeSwitch != nullptr);
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

    if (m_useSwitchMode && m_modeSwitch) {
        // Calculate switch index based on mode
        int switchIndex = 3; // Default to Solid
        switch (mode) {
        case RenderingConfig::DisplayMode::NoShading: switchIndex = 0; break;
        case RenderingConfig::DisplayMode::Points: switchIndex = 1; break;
        case RenderingConfig::DisplayMode::Wireframe: switchIndex = 2; break;
        case RenderingConfig::DisplayMode::Solid: switchIndex = 3; break;
        case RenderingConfig::DisplayMode::FlatLines: switchIndex = 4; break;
        case RenderingConfig::DisplayMode::Transparent: switchIndex = 5; break;
        case RenderingConfig::DisplayMode::HiddenLine: switchIndex = 6; break;
        default: switchIndex = 3; break;
        }
        
        if (switchIndex >= 0 && switchIndex < m_modeSwitch->getNumChildren()) {
            m_modeSwitch->whichChild.setValue(switchIndex);
            
            // Log Switch mode change with expected state
            // Note: Actual state was set in handleDisplayMode, this is just switching
            // We output expected state for debugging
            DisplayModeRenderState expectedState;
            GeometryRenderContext dummyContext;
            dummyContext.display.facesVisible = true;
            dummyContext.display.showPointView = false;
            dummyContext.display.showSolidWithPointView = false;
            dummyContext.material.ambientColor = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB);
            dummyContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
            dummyContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
            dummyContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            dummyContext.material.shininess = 30.0;
            dummyContext.material.transparency = 0.0;
            dummyContext.display.wireframeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            dummyContext.display.wireframeWidth = 1.0;
            dummyContext.texture.enabled = false;
            dummyContext.blend.blendMode = RenderingConfig::BlendMode::None;
            
            DisplayModeStateManager stateManager;
            stateManager.setRenderStateForMode(expectedState, mode, dummyContext);
            return;
        }
    }
    
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
            bool isPointViewNode = false;
            bool isHiddenLineNode = false;
            for (int j = 0; j < sep->getNumChildren(); ++j) {
                SoNode* subChild = sep->getChild(j);
                if (!subChild) continue;
                if (subChild->isOfType(SoPointSet::getClassTypeId())) {
                    isPointViewNode = true;
                    break;
                }
                if (subChild->isOfType(SoCoordinate3::getClassTypeId())) {
                    isPointViewNode = true;
                    break;
                }
                // Check for PolygonModeNode (HiddenLine mode)
                if (subChild->isOfType(PolygonModeNode::getClassTypeId())) {
                    isHiddenLineNode = true;
                    break;
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
    
    // Remove collected state nodes (safe: removeChild handles ref/unref correctly)
    for (auto* node : stateNodesToRemove) {
        coinNode->removeChild(node);
    }
    
    // Remove point view nodes
    for (auto* node : pointViewNodesToRemove) {
        coinNode->removeChild(node);
    }
    
    // Remove HiddenLine pass nodes
    for (auto* node : hiddenLineNodesToRemove) {
        coinNode->removeChild(node);
    }
    
    // Step 3: Build context - prioritize originalDiffuseColor if provided
    GeometryRenderContext updateContext;
    updateContext.display.displayMode = mode;
    updateContext.display.facesVisible = true;
    updateContext.display.showPointView = false;
    updateContext.display.showSolidWithPointView = false;
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
    updateState.showSolidWithPoints = updateContext.display.showSolidWithPointView;
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

    // Step 5.2: Create DrawStyle node (was deleted by resetAllRenderStates)
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->ref();
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
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
                                            RenderNodeBuilder* renderBuilder,
                                            WireframeBuilder* wireframeBuilder,
                                            PointViewBuilder* pointViewBuilder) {
    if (!m_brepHandler) {
        return;
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
                                            RenderNodeBuilder* renderBuilder,
                                            WireframeBuilder* wireframeBuilder,
                                            PointViewBuilder* pointViewBuilder) {
    if (!m_meshHandler) {
        return;
    }
    
    m_meshHandler->handleDisplayMode(coinNode, context, mesh, params,
                                     edgeComponent, useModularEdgeComponent,
                                     renderBuilder, wireframeBuilder, pointViewBuilder);
    
    setGeometryBuilt(true);
}

