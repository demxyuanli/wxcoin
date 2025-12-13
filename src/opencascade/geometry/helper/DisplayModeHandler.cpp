#include "geometry/helper/DisplayModeHandler.h"
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
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <vector>
#include <sstream>
#include <iomanip>

DisplayModeHandler::DisplayModeHandler() 
    : m_modeSwitch(nullptr)
    , m_useSwitchMode(false)
{
}

DisplayModeHandler::~DisplayModeHandler() {
}

void DisplayModeHandler::setModeSwitch(SoSwitch* modeSwitch) {
    m_modeSwitch = modeSwitch;
    m_useSwitchMode = (m_modeSwitch != nullptr);
}

void DisplayModeHandler::updateDisplayMode(SoSeparator* coinNode, RenderingConfig::DisplayMode mode,
                                           ModularEdgeComponent* edgeComponent,
                                           const Quantity_Color* originalDiffuseColor) {
    if (!coinNode) {
        return;
    }

    if (m_useSwitchMode && m_modeSwitch) {
        int switchIndex = getModeSwitchIndex(mode);
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
            
            setRenderStateForMode(expectedState, mode, dummyContext);
            return;
        }
    }
    
    // Log non-Switch mode update with full state
    // Step 1: Extract material info from existing nodes BEFORE reset (they will be deleted)
    SoDrawStyle* tempDrawStyle = nullptr;
    SoMaterial* tempMaterial = nullptr;
    findDrawStyleAndMaterial(coinNode, tempDrawStyle, tempMaterial);
    
    // Step 2: Reset only state nodes (preserve geometry nodes)
    // Collect state nodes to remove safely (avoid index shifting issues)
    std::vector<SoNode*> stateNodesToRemove;
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
    }
    
    // Remove edge nodes
    if (edgeComponent) {
        cleanupEdgeNodes(coinNode, edgeComponent);
    }
    
    // Remove collected state nodes (safe: removeChild handles ref/unref correctly)
    for (auto* node : stateNodesToRemove) {
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
    
    // Use originalDiffuseColor if provided, otherwise extract from existing material
    if (originalDiffuseColor) {
        updateContext.material.diffuseColor = *originalDiffuseColor;
        // Extract other material properties from existing node if available
        if (tempMaterial) {
            if (tempMaterial->ambientColor.getNum() > 0) {
                const SbColor& ambient = tempMaterial->ambientColor[0];
                float r, g, b;
                ambient.getValue(r, g, b);
                updateContext.material.ambientColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
            } else {
                updateContext.material.ambientColor = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB);
            }
            if (tempMaterial->specularColor.getNum() > 0) {
                const SbColor& specular = tempMaterial->specularColor[0];
                float r, g, b;
                specular.getValue(r, g, b);
                updateContext.material.specularColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
            } else {
                updateContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
            }
            if (tempMaterial->emissiveColor.getNum() > 0) {
                const SbColor& emissive = tempMaterial->emissiveColor[0];
                float r, g, b;
                emissive.getValue(r, g, b);
                updateContext.material.emissiveColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
            } else {
                updateContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            }
            if (tempMaterial->shininess.getNum() > 0) {
                updateContext.material.shininess = tempMaterial->shininess[0] * 100.0;
            } else {
                updateContext.material.shininess = 30.0;
            }
            if (tempMaterial->transparency.getNum() > 0) {
                updateContext.material.transparency = tempMaterial->transparency[0];
            } else {
                updateContext.material.transparency = 0.0;
            }
        } else {
            // Use defaults
            updateContext.material.ambientColor = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB);
            updateContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
            updateContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            updateContext.material.shininess = 30.0;
            updateContext.material.transparency = 0.0;
        }
    } else if (tempMaterial) {
        // Extract material from existing node
        // SoMaterial uses SoMFColor (multi-value), access first value with [0]
        if (tempMaterial->diffuseColor.getNum() > 0) {
            const SbColor& diffuse = tempMaterial->diffuseColor[0];
            float r, g, b;
            diffuse.getValue(r, g, b);
            updateContext.material.diffuseColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
        } else {
            updateContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        }
        if (tempMaterial->ambientColor.getNum() > 0) {
            const SbColor& ambient = tempMaterial->ambientColor[0];
            float r, g, b;
            ambient.getValue(r, g, b);
            updateContext.material.ambientColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
        } else {
            updateContext.material.ambientColor = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB);
        }
        if (tempMaterial->specularColor.getNum() > 0) {
            const SbColor& specular = tempMaterial->specularColor[0];
            float r, g, b;
            specular.getValue(r, g, b);
            updateContext.material.specularColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
        } else {
            updateContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        }
        if (tempMaterial->emissiveColor.getNum() > 0) {
            const SbColor& emissive = tempMaterial->emissiveColor[0];
            float r, g, b;
            emissive.getValue(r, g, b);
            updateContext.material.emissiveColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
        } else {
            updateContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        }
        if (tempMaterial->shininess.getNum() > 0) {
            updateContext.material.shininess = tempMaterial->shininess[0] * 100.0;
        } else {
            updateContext.material.shininess = 30.0;
        }
        if (tempMaterial->transparency.getNum() > 0) {
            updateContext.material.transparency = tempMaterial->transparency[0];
        } else {
            updateContext.material.transparency = 0.0;
        }
    } else {
        // Use default material
        updateContext.material.ambientColor = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB);
        updateContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        updateContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        updateContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        updateContext.material.shininess = 30.0;
        updateContext.material.transparency = 0.0;
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
    
    setRenderStateForMode(updateState, mode, updateContext);

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
        drawStyle->style.setValue(SoDrawStyle::LINES);
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
    if (!coinNode || !renderBuilder || !wireframeBuilder) {
        return;
    }

    const RenderingConfig::DisplayMode displayMode = context.display.displayMode;
    
   
    if (m_useSwitchMode && m_modeSwitch) {
        // FreeCAD approach: Geometry is built once and shared, Switch only contains state nodes
        // This prevents memory explosion (7x geometry copies -> 1x geometry + 7x small state nodes)
        
        // Step 1: Build shared geometry once (outside Switch)
        resetAllRenderStates(coinNode, edgeComponent);
        
        // Build geometry for current mode (will be reused by all Switch states)
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
        
        // Build geometry once (shared by all modes)
        if (state.showSurface) {
            GeometryRenderContext surfaceContext = context;
            surfaceContext.display.facesVisible = true;
            renderBuilder->appendSurfaceGeometry(coinNode, shape, params, surfaceContext);
        }
        
        // Step 2: Build state nodes for each mode (only Material, DrawStyle, LightModel, etc.)
        m_modeSwitch->removeAllChildren();
        
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
            SoSeparator* stateNode = new SoSeparator();
            stateNode->ref();
            
            // Build state for this mode
            DisplayModeRenderState modeState = state;
            GeometryRenderContext modeContext = context;
            modeContext.display.displayMode = mode;
            setRenderStateForMode(modeState, mode, modeContext);
            
            // Build only state nodes (no geometry)
            buildModeStateNode(stateNode, mode, modeState, modeContext, renderBuilder);
            
            m_modeSwitch->addChild(stateNode);
            stateNode->unref();
        }
        
        int switchIndex = getModeSwitchIndex(displayMode);
        if (switchIndex >= 0 && switchIndex < m_modeSwitch->getNumChildren()) {
            m_modeSwitch->whichChild.setValue(switchIndex);
        }
        
        coinNode->addChild(m_modeSwitch);
        
        // Step 3: Add edges and points (outside Switch, controlled separately)
        // Note: Edges and points are not in Switch as they may need independent control
        DisplayModeRenderState currentState = state;
        setRenderStateForMode(currentState, displayMode, context);
        
        // CRITICAL: Always update edgeFlags before calling updateEdgeDisplay
        // This ensures edge visibility matches the state, not stale edgeFlags
        if (useModularEdgeComponent && edgeComponent) {
            // Set edgeFlags to match state
            edgeComponent->setEdgeDisplayType(EdgeType::Original, currentState.showOriginalEdges);
            edgeComponent->setEdgeDisplayType(EdgeType::Mesh, currentState.showMeshEdges);
            // Clear other edge types that shouldn't be shown in display mode
            edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
            edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
            edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        }
        
        // Add edges if needed
        if (currentState.showOriginalEdges && useModularEdgeComponent && edgeComponent) {
            edgeComponent->extractOriginalEdges(
                shape, 
                80.0,
                0.01,
                false,
                currentState.originalEdgeColor,
                currentState.originalEdgeWidth,
                false,
                Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
                3.0
            );
        }
        
        // Always call updateEdgeDisplay to apply edgeFlags changes
        // This ensures edges are removed when showOriginalEdges is false
        if (useModularEdgeComponent && edgeComponent) {
            edgeComponent->updateEdgeDisplay(coinNode);
        }
        
        // Add points if needed
        if (currentState.showPoints && pointViewBuilder) {
            pointViewBuilder->createPointViewRepresentation(coinNode, shape, params, context.display);
        }
        
        return;
    }
    
    // ===== Step 1: Reset all render states =====
    resetAllRenderStates(coinNode, edgeComponent);
    
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
    setRenderStateForMode(state, displayMode, context);

    // ===== Step 4: Apply render state to scene graph =====
    applyRenderState(coinNode, state, context, shape, params, edgeComponent, 
                     useModularEdgeComponent, renderBuilder, wireframeBuilder, pointViewBuilder);
}

void DisplayModeHandler::setRenderStateForMode(DisplayModeRenderState& state, 
                                               RenderingConfig::DisplayMode displayMode,
                                               const GeometryRenderContext& context) {
    // Reset all state flags first
    state.showSurface = false;
    state.showOriginalEdges = false;
    state.showMeshEdges = false;
    state.wireframeMode = false;
    state.textureEnabled = context.texture.enabled;
    state.lightingEnabled = true;
    state.showPoints = context.display.showPointView;  // Show points if enabled, regardless of mode
    state.showSolidWithPoints = context.display.showSolidWithPointView;
    
    // Reset material to context defaults
    state.surfaceAmbientColor = context.material.ambientColor;
    state.surfaceDiffuseColor = context.material.diffuseColor;
    state.surfaceSpecularColor = context.material.specularColor;
    state.surfaceEmissiveColor = context.material.emissiveColor;
    state.shininess = context.material.shininess;
    state.transparency = context.material.transparency;
    state.blendMode = context.blend.blendMode;
    state.surfaceDisplayMode = displayMode;
    
    // Set state based on display mode
    switch (displayMode) {
    case RenderingConfig::DisplayMode::NoShading: {
        // NoShading: Show faces without lighting + original edges (not mesh edges)
        state.showSurface = true;
        state.showOriginalEdges = true;
        state.wireframeMode = false;
        state.textureEnabled = false;
        state.lightingEnabled = false;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::NoShading;
        // Disable lighting effects but preserve original diffuseColor
        state.surfaceAmbientColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.surfaceSpecularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.surfaceEmissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.shininess = 0.0;
        break;
    }

    case RenderingConfig::DisplayMode::Points: {
        // Points: Show point view if enabled
        state.showPoints = context.display.showPointView;
        state.showSurface = context.display.showPointView && context.display.showSolidWithPointView;
        state.wireframeMode = false;
        // CRITICAL: Force disable edges in Points mode to prevent residual edges from previous modes
        state.showOriginalEdges = false;
        state.showMeshEdges = false;
        break;
    }

    case RenderingConfig::DisplayMode::Wireframe: {
        // Wireframe: Show only original edges, optionally with hidden surface removal
        state.showSurface = context.display.facesVisible;  // Hidden surface removal
        state.showOriginalEdges = true;
        state.wireframeMode = true;
        // Hidden surface uses no-shading
        if (state.showSurface) {
            state.surfaceDisplayMode = RenderingConfig::DisplayMode::NoShading;
            state.textureEnabled = false;
            state.lightingEnabled = false;
            state.surfaceAmbientColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            state.surfaceSpecularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            state.surfaceEmissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            state.shininess = 0.0;
        }
        break;
    }

    case RenderingConfig::DisplayMode::FlatLines: {
        // FlatLines: Show both faces and original edges (not mesh edges)
        state.showSurface = true;
        state.showOriginalEdges = true;
        state.wireframeMode = false;  // Surface is not wireframe
        break;
    }

    case RenderingConfig::DisplayMode::Solid: {
        // Solid: Standard shaded rendering
        state.showSurface = true;
        state.wireframeMode = false;
        state.lightingEnabled = true;
        break;
    }

    case RenderingConfig::DisplayMode::Transparent: {
        // Transparent: Apply transparency and alpha blending
        state.showSurface = true;
        state.wireframeMode = false;
        if (state.transparency <= 0.0) {
            state.transparency = 0.5;
        }
        state.blendMode = RenderingConfig::BlendMode::Alpha;
        break;
    }

    case RenderingConfig::DisplayMode::HiddenLine: {
        // HiddenLine: White background surface + mesh edges inheriting original face colors
        state.showSurface = true;
        state.showMeshEdges = true;
        state.wireframeMode = false;
        state.textureEnabled = false;
        state.lightingEnabled = false;
        state.surfaceDisplayMode = RenderingConfig::DisplayMode::NoShading;
        // HiddenLine uses white background (technical drawing style)
        state.surfaceAmbientColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        state.surfaceDiffuseColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        state.surfaceSpecularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.surfaceEmissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        state.shininess = 0.0;
        // Mesh edges inherit original face color
        state.meshEdgeColor = context.material.diffuseColor;
        break;
    }

    default: {
        // Default: Use context as-is
        state.showSurface = true;
        state.wireframeMode = false;
        break;
    }
    }
    
    // Log state combination for debugging
    std::ostringstream logMsg;
    logMsg << "DisplayMode [" << RenderingConfig::getDisplayModeName(displayMode) << "] State:\n";
    logMsg << "  Display Components:\n";
    logMsg << "    showSurface: " << (state.showSurface ? "true" : "false") << "\n";
    logMsg << "    showOriginalEdges: " << (state.showOriginalEdges ? "true" : "false") << "\n";
    logMsg << "    showMeshEdges: " << (state.showMeshEdges ? "true" : "false") << "\n";
    logMsg << "    showPoints: " << (state.showPoints ? "true" : "false") << "\n";
    logMsg << "  Surface Properties:\n";
    logMsg << "    wireframeMode: " << (state.wireframeMode ? "true" : "false") << "\n";
    logMsg << "    textureEnabled: " << (state.textureEnabled ? "true" : "false") << "\n";
    logMsg << "    lightingEnabled: " << (state.lightingEnabled ? "true" : "false") << "\n";
    logMsg << "    surfaceDisplayMode: " << RenderingConfig::getDisplayModeName(state.surfaceDisplayMode) << "\n";
    logMsg << "  Material:\n";
    logMsg << "    ambient: (" << std::fixed << std::setprecision(2) 
           << state.surfaceAmbientColor.Red() << ", "
           << state.surfaceAmbientColor.Green() << ", "
           << state.surfaceAmbientColor.Blue() << ")\n";
    logMsg << "    diffuse: (" << state.surfaceDiffuseColor.Red() << ", "
           << state.surfaceDiffuseColor.Green() << ", "
           << state.surfaceDiffuseColor.Blue() << ")\n";
    logMsg << "    specular: (" << state.surfaceSpecularColor.Red() << ", "
           << state.surfaceSpecularColor.Green() << ", "
           << state.surfaceSpecularColor.Blue() << ")\n";
    logMsg << "    shininess: " << state.shininess << "\n";
    logMsg << "    transparency: " << state.transparency << "\n";
    logMsg << "  Edges:\n";
    logMsg << "    originalEdgeColor: (" << state.originalEdgeColor.Red() << ", "
           << state.originalEdgeColor.Green() << ", "
           << state.originalEdgeColor.Blue() << ")\n";
    logMsg << "    originalEdgeWidth: " << state.originalEdgeWidth << "\n";
    logMsg << "    meshEdgeColor: (" << state.meshEdgeColor.Red() << ", "
           << state.meshEdgeColor.Green() << ", "
           << state.meshEdgeColor.Blue() << ")\n";
    logMsg << "    meshEdgeWidth: " << state.meshEdgeWidth << "\n";
    logMsg << "  Blend:\n";
    logMsg << "    blendMode: " << static_cast<int>(state.blendMode) << "\n";
    
    LOG_INF_S(logMsg.str());
}

void DisplayModeHandler::applyRenderState(SoSeparator* coinNode,
                                         const DisplayModeRenderState& state,
                                         const GeometryRenderContext& context,
                                         const TopoDS_Shape& shape,
                                         const MeshParameters& params,
                                         ModularEdgeComponent* edgeComponent,
                                         bool useModularEdgeComponent,
                                         RenderNodeBuilder* renderBuilder,
                                         WireframeBuilder* wireframeBuilder,
                                         PointViewBuilder* pointViewBuilder) {
    if (!coinNode || !renderBuilder || !wireframeBuilder) {
        return;
    }
    
    SoPolygonOffset* polygonOffset = nullptr;

    // Render surface if needed
    // CRITICAL: For HiddenLine mode with mesh edges, skip normal surface pass
    // because wireframePass will render both white surface and black lines in one pass
    bool skipNormalSurfacePass = (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine && 
                                  state.showMeshEdges);
    
    if (state.showSurface && !skipNormalSurfacePass) {
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.wireframeMode = state.wireframeMode;
        surfaceContext.display.facesVisible = state.showSurface;  // Use showSurface as facesVisible
        surfaceContext.display.displayMode = state.surfaceDisplayMode;
        surfaceContext.texture.enabled = state.textureEnabled;
        surfaceContext.material.ambientColor = state.surfaceAmbientColor;
        surfaceContext.material.diffuseColor = state.surfaceDiffuseColor;
        surfaceContext.material.specularColor = state.surfaceSpecularColor;
        surfaceContext.material.emissiveColor = state.surfaceEmissiveColor;
        surfaceContext.material.shininess = state.shininess;
        surfaceContext.material.transparency = state.transparency;
        surfaceContext.blend.blendMode = state.blendMode;

        // Add LightModel node for proper lighting control
        // Use BASE_COLOR for no-shading modes (NoShading, HiddenLine), PHONG for others
        SoLightModel* lightModel = new SoLightModel();
        if (!state.lightingEnabled || state.surfaceDisplayMode == RenderingConfig::DisplayMode::NoShading) {
            lightModel->model.setValue(SoLightModel::BASE_COLOR);  // No lighting, direct color
        } else {
            lightModel->model.setValue(SoLightModel::PHONG);  // Standard Phong lighting
        }
        coinNode->addChild(lightModel);
        
        coinNode->addChild(renderBuilder->createDrawStyleNode(surfaceContext));
        coinNode->addChild(renderBuilder->createMaterialNode(surfaceContext));
        renderBuilder->appendTextureNodes(coinNode, surfaceContext);
        renderBuilder->appendBlendHints(coinNode, surfaceContext);
        
        // For HiddenLine mode, set polygon offset to push surface back (+1.0)
        // This ensures white background is behind the black lines
        if (!polygonOffset) {
            polygonOffset = renderBuilder->createPolygonOffsetNode();
            if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine) {
                polygonOffset->factor.setValue(1.0f);
                polygonOffset->units.setValue(1.0f);
            }
            coinNode->addChild(polygonOffset);
        }
        
        renderBuilder->appendSurfaceGeometry(coinNode, shape, params, surfaceContext);
    }

    // Render original edges if needed
    // CRITICAL: Always update edgeFlags before calling updateEdgeDisplay
    // This ensures edge visibility matches the state, not stale edgeFlags
    if (useModularEdgeComponent && edgeComponent) {
        // Set edgeFlags to match state
        edgeComponent->setEdgeDisplayType(EdgeType::Original, state.showOriginalEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, state.showMeshEdges);
        // Clear other edge types that shouldn't be shown in display mode
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
    }
    
    if (state.showOriginalEdges) {
        GeometryRenderContext wireContext = context;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        wireContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        wireContext.display.wireframeColor = state.originalEdgeColor;
        wireContext.display.wireframeWidth = state.originalEdgeWidth;

        coinNode->addChild(renderBuilder->createDrawStyleNode(wireContext));
        coinNode->addChild(renderBuilder->createMaterialNode(wireContext));
        
        // Add polygon offset for wireframe edges to prevent Z-fighting with hidden surface
        // This is especially important when hidden surface removal is enabled
        if (context.display.displayMode == RenderingConfig::DisplayMode::Wireframe && state.showSurface) {
            SoPolygonOffset* edgeOffset = new SoPolygonOffset();
            edgeOffset->factor.setValue(-1.0f);  // Push edges forward
            edgeOffset->units.setValue(-1.0f);
            coinNode->addChild(edgeOffset);
        }
        
        if (useModularEdgeComponent && edgeComponent) {
            edgeComponent->extractOriginalEdges(
                shape, 
                80.0,
                0.01,
                false,
                state.originalEdgeColor,
                state.originalEdgeWidth,
                false,
                Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
                3.0
            );
            edgeComponent->updateEdgeDisplay(coinNode);
        } else {
            wireframeBuilder->createWireframeRepresentation(coinNode, shape, params);
        }
    } else {
        // CRITICAL: When showOriginalEdges is false, still call updateEdgeDisplay
        // to ensure edge nodes are removed from the scene graph
        if (useModularEdgeComponent && edgeComponent) {
            edgeComponent->updateEdgeDisplay(coinNode);
        }
    }

    // Render mesh edges if needed
    if (state.showMeshEdges) {
        // For HiddenLine mode, use PolygonModeNode for fast rendering (FreeCAD approach)
        if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine) {
            // HiddenLine mode: Render white background surface + black lines in ONE pass
            // This prevents Z-fighting and ensures proper hidden line removal
            // NOTE: We skip the normal surface pass above to avoid rendering white surface twice
            
            // Single pass: White background surface (with polygon offset +1.0 to push back) + 
            //              Black wireframe lines (with polygon offset -1.0 to push forward)
            PolygonModeNode* polygonMode = new PolygonModeNode();
            polygonMode->ref();
            polygonMode->mode.setValue(PolygonModeNode::LINE);
            polygonMode->lineWidth.setValue(static_cast<float>(state.meshEdgeWidth));
            polygonMode->disableLighting.setValue(TRUE);
            // Negative offset pushes lines forward (in front of surface)
            polygonMode->polygonOffsetFactor.setValue(-1.0f);
            polygonMode->polygonOffsetUnits.setValue(-1.0f);
            
            // Create a separator for the HiddenLine pass (white surface + black lines)
            SoSeparator* hiddenLinePass = new SoSeparator();
            hiddenLinePass->ref();
            
            // Step 1: Render white background surface (with +1.0 offset to push back)
            // This is the ONLY place we render the white surface in HiddenLine mode
            SoPolygonOffset* surfaceOffset = new SoPolygonOffset();
            surfaceOffset->factor.setValue(1.0f);  // Push surface back
            surfaceOffset->units.setValue(1.0f);
            hiddenLinePass->addChild(surfaceOffset);
            
            // White material for background surface
            SoMaterial* whiteMaterial = new SoMaterial();
            whiteMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
            whiteMaterial->ambientColor.setValue(1.0f, 1.0f, 1.0f);
            whiteMaterial->emissiveColor.setValue(1.0f, 1.0f, 1.0f);
            hiddenLinePass->addChild(whiteMaterial);
            
            // No lighting for white surface
            SoLightModel* noLightModel = new SoLightModel();
            noLightModel->model.setValue(SoLightModel::BASE_COLOR);
            hiddenLinePass->addChild(noLightModel);
            
            // Add filled surface geometry (will be white background)
            GeometryRenderContext whiteSurfaceContext = context;
            whiteSurfaceContext.display.wireframeMode = false;
            whiteSurfaceContext.display.facesVisible = true;
            whiteSurfaceContext.display.displayMode = RenderingConfig::DisplayMode::NoShading;
            whiteSurfaceContext.material.diffuseColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
            whiteSurfaceContext.material.ambientColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
            whiteSurfaceContext.material.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            whiteSurfaceContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            whiteSurfaceContext.material.shininess = 0.0;
            whiteSurfaceContext.material.transparency = 0.0;
            
            renderBuilder->appendSurfaceGeometry(hiddenLinePass, shape, params, whiteSurfaceContext);
            
            // Step 2: Render black wireframe lines (with -1.0 offset to push forward)
            // The PolygonModeNode will render the same geometry in LINE mode
            SoMaterial* edgeMaterial = new SoMaterial();
            edgeMaterial->diffuseColor.setValue(
                static_cast<float>(state.meshEdgeColor.Red()),
                static_cast<float>(state.meshEdgeColor.Green()),
                static_cast<float>(state.meshEdgeColor.Blue())
            );
            edgeMaterial->emissiveColor.setValue(
                static_cast<float>(state.meshEdgeColor.Red()),
                static_cast<float>(state.meshEdgeColor.Green()),
                static_cast<float>(state.meshEdgeColor.Blue())
            );
            hiddenLinePass->addChild(edgeMaterial);
            
            // Add polygon mode node (will render geometry as lines)
            hiddenLinePass->addChild(polygonMode);
            
            // Re-add the same geometry, but PolygonModeNode will render it as lines
            GeometryRenderContext wireframeContext = context;
            wireframeContext.display.wireframeMode = false;  // Keep filled geometry, PolygonModeNode will make it lines
            wireframeContext.display.facesVisible = true;
            wireframeContext.display.displayMode = RenderingConfig::DisplayMode::NoShading;
            wireframeContext.material.diffuseColor = state.meshEdgeColor;  // Use edge color
            wireframeContext.material.ambientColor = state.meshEdgeColor;
            wireframeContext.material.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            wireframeContext.material.emissiveColor = state.meshEdgeColor;
            wireframeContext.material.shininess = 0.0;
            wireframeContext.material.transparency = 0.0;
            
            renderBuilder->appendSurfaceGeometry(hiddenLinePass, shape, params, wireframeContext);
            
            // Add to scene graph - scene graph will hold reference
            coinNode->addChild(hiddenLinePass);
            hiddenLinePass->unref();  // Scene graph now owns it, safe to unref
            polygonMode->unref();      // hiddenLinePass owns it, safe to unref
        } else {
            // For other modes, use traditional mesh edge extraction
            if (useModularEdgeComponent && edgeComponent) {
                auto& manager = RenderingToolkitAPI::getManager();
                auto processor = manager.getGeometryProcessor("OpenCASCADE");
                if (processor) {
                    TriangleMesh mesh = processor->convertToMesh(shape, params);
                    if (!mesh.triangles.empty()) {
                        edgeComponent->extractMeshEdges(mesh, state.meshEdgeColor, state.meshEdgeWidth);
                        edgeComponent->updateEdgeDisplay(coinNode);
                    }
                }
            }
        }
    }

    // Render points if needed
    if (state.showPoints && pointViewBuilder) {
        pointViewBuilder->createPointViewRepresentation(coinNode, shape, params, context.display);
    }
}

void DisplayModeHandler::findDrawStyleAndMaterial(SoNode* node, SoDrawStyle*& drawStyle, SoMaterial*& material) {
    if (!node) return;
    
    if (node->isOfType(SoDrawStyle::getClassTypeId())) {
        if (!drawStyle) {
            drawStyle = static_cast<SoDrawStyle*>(node);
        }
    } else if (node->isOfType(SoMaterial::getClassTypeId())) {
        if (!material) {
            material = static_cast<SoMaterial*>(node);
        }
    } else if (node->isOfType(SoSeparator::getClassTypeId())) {
        SoSeparator* sep = static_cast<SoSeparator*>(node);
        for (int i = 0; i < sep->getNumChildren(); ++i) {
            findDrawStyleAndMaterial(sep->getChild(i), drawStyle, material);
        }
    }
}

void DisplayModeHandler::cleanupEdgeNodes(SoSeparator* coinNode, ModularEdgeComponent* edgeComponent) {
    if (!coinNode || !edgeComponent) {
        return;
    }

    SoSeparator* originalEdgeNode = edgeComponent->getEdgeNode(EdgeType::Original);
    SoSeparator* featureEdgeNode = edgeComponent->getEdgeNode(EdgeType::Feature);
    SoSeparator* meshEdgeNode = edgeComponent->getEdgeNode(EdgeType::Mesh);
    SoSeparator* highlightEdgeNode = edgeComponent->getEdgeNode(EdgeType::Highlight);
    SoSeparator* normalLineNode = edgeComponent->getEdgeNode(EdgeType::VerticeNormal);
    SoSeparator* faceNormalLineNode = edgeComponent->getEdgeNode(EdgeType::FaceNormal);
    SoSeparator* silhouetteEdgeNode = edgeComponent->getEdgeNode(EdgeType::Silhouette);
    
    for (int i = coinNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = coinNode->getChild(i);
        if (!child) continue;
        
        if (child == originalEdgeNode || child == featureEdgeNode ||
            child == meshEdgeNode || child == highlightEdgeNode ||
            child == normalLineNode || child == faceNormalLineNode ||
            child == silhouetteEdgeNode) {
            coinNode->removeChild(i);
        }
    }
}

void DisplayModeHandler::resetAllRenderStates(SoSeparator* coinNode, ModularEdgeComponent* edgeComponent) {
    if (!coinNode) {
        return;
    }
    
    // Remove all edge nodes first
    if (edgeComponent) {
        cleanupEdgeNodes(coinNode, edgeComponent);
    }
    
    // Collect nodes to remove safely (avoid index shifting issues)
    std::vector<SoNode*> toRemove;
    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (!child) continue;
        
        // Keep Switch node if it exists (for Switch mode)
        if (child->isOfType(SoSwitch::getClassTypeId())) {
            continue;
        }
        
        // Collect all other rendering-related nodes for removal
        // This includes: DrawStyle, Material, PolygonOffset, Texture nodes, Geometry nodes, etc.
        toRemove.push_back(child);
    }
    
    // Remove collected nodes (safe: removeChild handles ref/unref correctly)
    for (auto* node : toRemove) {
        coinNode->removeChild(node);
    }
}

int DisplayModeHandler::getModeSwitchIndex(RenderingConfig::DisplayMode mode) {
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

void DisplayModeHandler::buildModeStateNode(SoSeparator* parent,
                                           RenderingConfig::DisplayMode mode,
                                           const DisplayModeRenderState& state,
                                           const GeometryRenderContext& context,
                                           RenderNodeBuilder* renderBuilder) {
    if (!parent || !renderBuilder) {
        return;
    }
    
    // Build only state nodes (Material, DrawStyle, LightModel, PolygonOffset)
    // Geometry is shared and built outside Switch
    
    GeometryRenderContext stateContext = context;
    stateContext.display.wireframeMode = state.wireframeMode;
    stateContext.display.facesVisible = state.showSurface;
    stateContext.display.displayMode = state.surfaceDisplayMode;
    stateContext.texture.enabled = state.textureEnabled;
    stateContext.material.ambientColor = state.surfaceAmbientColor;
    stateContext.material.diffuseColor = state.surfaceDiffuseColor;
    stateContext.material.specularColor = state.surfaceSpecularColor;
    stateContext.material.emissiveColor = state.surfaceEmissiveColor;
    stateContext.material.shininess = state.shininess;
    stateContext.material.transparency = state.transparency;
    stateContext.blend.blendMode = state.blendMode;
    
    // Add LightModel
    SoLightModel* lightModel = new SoLightModel();
    if (!state.lightingEnabled || state.surfaceDisplayMode == RenderingConfig::DisplayMode::NoShading) {
        lightModel->model.setValue(SoLightModel::BASE_COLOR);
    } else {
        lightModel->model.setValue(SoLightModel::PHONG);
    }
    parent->addChild(lightModel);
    
    // Add DrawStyle and Material
    parent->addChild(renderBuilder->createDrawStyleNode(stateContext));
    parent->addChild(renderBuilder->createMaterialNode(stateContext));
    
    // Add Texture nodes if needed
    renderBuilder->appendTextureNodes(parent, stateContext);
    
    // Add Blend hints if needed
    renderBuilder->appendBlendHints(parent, stateContext);
    
    // Add PolygonOffset if needed
    SoPolygonOffset* polygonOffset = renderBuilder->createPolygonOffsetNode();
    if (mode == RenderingConfig::DisplayMode::HiddenLine) {
        polygonOffset->factor.setValue(1.0f);
        polygonOffset->units.setValue(1.0f);
    }
    parent->addChild(polygonOffset);
}

void DisplayModeHandler::buildModeNode(SoSeparator* parent,
                                       RenderingConfig::DisplayMode mode,
                                       const GeometryRenderContext& context,
                                       const TopoDS_Shape& shape,
                                       const MeshParameters& params,
                                       ModularEdgeComponent* edgeComponent,
                                       bool useModularEdgeComponent,
                                       RenderNodeBuilder* renderBuilder,
                                       WireframeBuilder* wireframeBuilder,
                                       PointViewBuilder* pointViewBuilder) {
    if (!parent || !renderBuilder || !wireframeBuilder) {
        return;
    }

    // ===== Step 1: Reset all render states =====
    resetAllRenderStates(parent, edgeComponent);
    
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
    state.surfaceDisplayMode = mode;

    // ===== Step 3: Set render state based on display mode =====
    setRenderStateForMode(state, mode, context);

    // ===== Step 4: Apply render state to scene graph =====
    applyRenderState(parent, state, context, shape, params, edgeComponent, 
                     useModularEdgeComponent, renderBuilder, wireframeBuilder, pointViewBuilder);
}
