#include "geometry/helper/BRepDisplayModeHandler.h"
#include "geometry/helper/DisplayModeStateManager.h"
#include "geometry/helper/DisplayModeNodeManager.h"
#include "geometry/helper/DisplayModeRenderer.h"
#include "geometry/helper/DisplayModeHandler.h"
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
        
        // Step 1: Reset render states and prepare for geometry initialization
        DisplayModeNodeManager nodeManager;
        nodeManager.resetAllRenderStates(coinNode, edgeComponent);
        
        // Get configuration for current display mode (used to determine what geometry to build)
        DisplayModeConfig currentConfig = DisplayModeConfigFactory::getConfig(displayMode, context);
        
        // Build shared surface geometry once if needed (check all modes to see if any require surface)
        // Surface geometry = SoSeparator containing SoIndexedFaceSet/SoFaceSet (coin triangle nodes)
        // This geometry will be reused by all Switch states
        if (!nodeManager.hasSurfaceGeometryNode(coinNode)) {
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
            if (anyModeRequiresSurface) {
                GeometryRenderContext surfaceContext = context;
                surfaceContext.display.facesVisible = true;
                renderBuilder->appendSurfaceGeometry(coinNode, shape, params, surfaceContext);
            }
        }
        
        // Initialize points geometry if needed (build once, controlled by visibility later)
        // Check if Points mode is one of the available modes, and if points node doesn't exist yet
        SoSeparator* existingPointViewNode = nodeManager.findPointViewNode(coinNode);
        if (!existingPointViewNode) {
            // Check if Points mode would require points (build points dataset during initialization)
            GeometryRenderContext pointsContext = context;
            pointsContext.display.displayMode = RenderingConfig::DisplayMode::Points;
            DisplayModeConfig pointsConfig = DisplayModeConfigFactory::getConfig(RenderingConfig::DisplayMode::Points, pointsContext);
            if (pointsConfig.nodes.requirePoints && pointViewBuilder) {
                // Build points dataset during initialization (will be shown/hidden based on mode)
                pointViewBuilder->createPointViewRepresentation(coinNode, shape, params, context.display);
            }
        }
        
        // Step 2: Build state nodes for each mode using configuration (data-driven approach)
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
        
        DisplayModeRenderer renderer;
        for (auto mode : modes) {
            SoSeparator* stateNode = new SoSeparator();
            stateNode->ref();
            
            // Get configuration for this mode (data-driven)
            GeometryRenderContext modeContext = context;
            modeContext.display.displayMode = mode;
            DisplayModeConfig modeConfig = DisplayModeConfigFactory::getConfig(mode, modeContext);
            
            // Build state node from configuration
            renderer.buildStateNodeFromConfig(stateNode, modeConfig, modeContext, renderBuilder);
            
            m_modeSwitch->addChild(stateNode);
            stateNode->unref();
        }
        
        int switchIndex = getModeSwitchIndex(displayMode);
        if (switchIndex >= 0 && switchIndex < m_modeSwitch->getNumChildren()) {
            m_modeSwitch->whichChild.setValue(switchIndex);
        }
        
        // Add Switch node only if not already added (coin nodes are shared/reused)
        if (!nodeManager.hasSwitchNode(coinNode, m_modeSwitch)) {
            coinNode->addChild(m_modeSwitch);
        }
        
        // Step 3: Apply edge and point rendering based on configuration (data-driven approach)
        // Note: Surface geometry is already built in Step 1, state nodes are in Switch
        // This step only handles edges and points (which are outside Switch and controlled separately)
        
        // Apply edge rendering based on config
        if (useModularEdgeComponent && edgeComponent) {
            // Set edge display flags based on config
            bool showOriginalEdges = currentConfig.nodes.requireOriginalEdges && currentConfig.edges.originalEdge.enabled;
            
            edgeComponent->setEdgeDisplayType(EdgeType::Original, showOriginalEdges);
            edgeComponent->setEdgeDisplayType(EdgeType::Mesh, false);  // BREP uses original edges
            edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
            edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
            edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
            
            // Extract original edges if needed (only if not already extracted)
            if (showOriginalEdges && !edgeComponent->getEdgeNode(EdgeType::Original)) {
                edgeComponent->extractOriginalEdges(shape,
                                                   80.0, 0.01, false,
                                                   currentConfig.edges.originalEdge.color,
                                                   currentConfig.edges.originalEdge.width,
                                                   false,
                                                   Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
                                                   3.0);
            }
            
            edgeComponent->updateEdgeDisplay(coinNode);
        }
        
        // Control point view visibility (points node already built in Step 1 if needed)
        // Points node is already in the scene graph, we control its visibility via renderCulling
        SoSeparator* pointViewNode = nodeManager.findPointViewNode(coinNode);
        if (pointViewNode) {
            // Show points if current mode requires them
            pointViewNode->renderCulling = currentConfig.nodes.requirePoints ? SoSeparator::OFF : SoSeparator::ON;
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

