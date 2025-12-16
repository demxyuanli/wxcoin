#include "geometry/helper/BRepDisplayModeHandler.h"
#include "geometry/helper/DisplayModeStateManager.h"
#include "geometry/helper/DisplayModeNodeManager.h"
#include "geometry/helper/DisplayModeRenderer.h"
#include "geometry/helper/RenderNodeBuilder.h"
#include "geometry/helper/PointViewBuilder.h"
#include "edges/ModularEdgeComponent.h"
#include "config/EdgeSettingsConfig.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>

BRepDisplayModeHandler::BRepDisplayModeHandler()
    : m_modeSwitch(nullptr)
    , m_useSwitchMode(false)
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
        DisplayModeNodeManager nodeManager;
        nodeManager.resetAllRenderStates(coinNode, edgeComponent);
        
        // Build geometry for current mode (will be reused by all Switch states)
        DisplayModeRenderState state;
        state.surfaceAmbientColor = context.material.ambientColor;
        state.surfaceDiffuseColor = context.material.diffuseColor;
        state.surfaceSpecularColor = context.material.specularColor;
        state.surfaceEmissiveColor = context.material.emissiveColor;
        state.shininess = context.material.shininess;
        state.transparency = context.material.transparency;
        state.originalEdgeColor = context.display.wireframeColor;
        // CRITICAL FIX: For HiddenLine mode, mesh edges should inherit original face color
        // Extract original face color from existing material node if available
        // This ensures mesh edges use the original face color, not white (which is used for surface)
        // If unable to extract original color, use black as fallback (good contrast on white background)
        Quantity_Color originalFaceColor = context.material.diffuseColor;
        bool foundOriginalColor = false;
        if (coinNode && displayMode == RenderingConfig::DisplayMode::HiddenLine) {
            // Try to find original material color from existing nodes
            for (int i = 0; i < coinNode->getNumChildren(); ++i) {
                SoNode* child = coinNode->getChild(i);
                if (child->getTypeId() == SoMaterial::getClassTypeId()) {
                    SoMaterial* mat = static_cast<SoMaterial*>(child);
                    if (mat->diffuseColor.getNum() > 0) {
                        const SbColor& diffuse = mat->diffuseColor[0];
                        float r, g, b;
                        diffuse.getValue(r, g, b);
                        // Only use if not white (white is used for HiddenLine surface)
                        if (!(r == 1.0f && g == 1.0f && b == 1.0f)) {
                            originalFaceColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);
                            foundOriginalColor = true;
                            break;
                        }
                    }
                }
            }
        }
        // If unable to extract original color, use black as fallback
        if (displayMode == RenderingConfig::DisplayMode::HiddenLine && !foundOriginalColor) {
            // Check if context color is white (which means it's been changed for HiddenLine surface)
            if (context.material.diffuseColor.Red() == 1.0 && 
                context.material.diffuseColor.Green() == 1.0 && 
                context.material.diffuseColor.Blue() == 1.0) {
                originalFaceColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);  // Black fallback
            }
        }
        state.meshEdgeColor = originalFaceColor;  // Use original face color for mesh edges, or black if unavailable
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
            DisplayModeStateManager stateManager;
            stateManager.setRenderStateForMode(modeState, mode, modeContext);
            
            // Build only state nodes (no geometry)
            DisplayModeRenderer renderer;
            renderer.buildModeStateNode(stateNode, mode, modeState, modeContext, renderBuilder);
            
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
        DisplayModeStateManager stateManager;
        stateManager.setRenderStateForMode(currentState, displayMode, context);
        
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
            
            // CRITICAL FIX: In HiddenLine mode, clear silhouette edge node to prevent it from showing
            // HiddenLine mode should only show mesh edges, not silhouette edges
            if (displayMode == RenderingConfig::DisplayMode::HiddenLine) {
                edgeComponent->clearSilhouetteEdgeNode();
            }
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

int BRepDisplayModeHandler::getModeSwitchIndex(RenderingConfig::DisplayMode mode) {
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

