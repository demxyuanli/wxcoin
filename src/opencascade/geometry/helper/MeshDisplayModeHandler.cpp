#include "geometry/helper/MeshDisplayModeHandler.h"
#include "geometry/helper/DisplayModeStateManager.h"
#include "geometry/helper/DisplayModeNodeManager.h"
#include "geometry/helper/DisplayModeRenderer.h"
#include "geometry/helper/RenderNodeBuilder.h"
#include "geometry/helper/PointViewBuilder.h"
#include "edges/ModularEdgeComponent.h"
#include "config/EdgeSettingsConfig.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <OpenCASCADE/Quantity_Color.hxx>

MeshDisplayModeHandler::MeshDisplayModeHandler()
    : m_modeSwitch(nullptr)
    , m_useSwitchMode(false)
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
        state.meshEdgeColor = context.material.diffuseColor;
        state.originalEdgeWidth = context.display.wireframeWidth;
        state.meshEdgeWidth = context.display.wireframeWidth;
        state.textureEnabled = context.texture.enabled;
        state.blendMode = context.blend.blendMode;
        state.showPoints = context.display.showPointView;
        state.showSolidWithPoints = context.display.showSolidWithPointView;
        state.surfaceDisplayMode = displayMode;
        
        DisplayModeStateManager stateManager;
        stateManager.setRenderStateForMode(state, displayMode, context);
        
        // Build geometry once (shared by all modes)
        if (state.showSurface && !mesh.isEmpty() && !mesh.triangles.empty()) {
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
                    coinNode->addChild(meshNode);
                }
            }
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
            DisplayModeStateManager stateMgr;
            stateMgr.setRenderStateForMode(modeState, mode, modeContext);
            
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
        DisplayModeStateManager stateMgr;
        stateMgr.setRenderStateForMode(currentState, displayMode, context);
        
        // CRITICAL: Always update edgeFlags before calling updateEdgeDisplay
        // For pure mesh models, convert showOriginalEdges to showMeshEdges
        // Pure mesh models don't have original edges (TopoDS_Shape), only mesh edges
        bool showMeshEdges = currentState.showMeshEdges;
        if (currentState.showOriginalEdges) {
            showMeshEdges = true;
        }
        if (displayMode == RenderingConfig::DisplayMode::Wireframe) {
            showMeshEdges = true;
        }
        
        if (useModularEdgeComponent && edgeComponent) {
            // Set edgeFlags to match state
            edgeComponent->setEdgeDisplayType(EdgeType::Original, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Mesh, showMeshEdges);
            edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
            edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
            edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
            
            // CRITICAL FIX: In HiddenLine mode, clear silhouette edge node to prevent it from showing
            if (displayMode == RenderingConfig::DisplayMode::HiddenLine) {
                edgeComponent->clearSilhouetteEdgeNode();
            }
        }
        
        // Extract and display mesh edges based on mode
        if (showMeshEdges) {
            if (useModularEdgeComponent && edgeComponent && !mesh.triangles.empty()) {
                if (displayMode == RenderingConfig::DisplayMode::Wireframe) {
                    // Wireframe mode: use original edge color
                    edgeComponent->extractMeshEdges(mesh, currentState.originalEdgeColor, currentState.originalEdgeWidth);
                } else if (displayMode == RenderingConfig::DisplayMode::HiddenLine) {
                    // HiddenLine mode: use effective edge color (black if too light)
                    Quantity_Color effectiveEdgeColor = currentState.meshEdgeColor;
                    if (currentState.meshEdgeColor.Red() > 0.4 && 
                        currentState.meshEdgeColor.Green() > 0.4 && 
                        currentState.meshEdgeColor.Blue() > 0.4) {
                        effectiveEdgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                    }
                    edgeComponent->extractMeshEdges(mesh, effectiveEdgeColor, currentState.meshEdgeWidth);
                } else if (displayMode == RenderingConfig::DisplayMode::NoShading) {
                    // NoShading mode: use original edge color (typically black) for better contrast
                    edgeComponent->extractMeshEdges(mesh, currentState.originalEdgeColor, currentState.meshEdgeWidth);
                } else {
                    // Other modes (Solid, FlatLines, etc.): use mesh edge color
                    edgeComponent->extractMeshEdges(mesh, currentState.meshEdgeColor, currentState.meshEdgeWidth);
                }
                edgeComponent->updateEdgeDisplay(coinNode);
            }
        } else {
            // Always call updateEdgeDisplay to apply edgeFlags changes
            if (useModularEdgeComponent && edgeComponent) {
                edgeComponent->updateEdgeDisplay(coinNode);
            }
        }
        
        // Add points if needed
        if (currentState.showPoints && pointViewBuilder) {
            pointViewBuilder->createPointViewRepresentation(coinNode, mesh, context.display);
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


