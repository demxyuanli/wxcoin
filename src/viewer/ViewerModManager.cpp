#include "viewer/ViewerModManager.h"
#include "viewer/modes/PointsMode.h"
#include "viewer/modes/WireframeMode.h"
#include "viewer/modes/FlatLinesMode.h"
#include "viewer/modes/ShadedMode.h"
#include "logger/Logger.h"

ViewerModManager::ViewerModManager() {
    initializeModes();
}

ViewerModManager::~ViewerModManager() {
    // Modes are managed by unique_ptr, will be automatically destroyed
}

void ViewerModManager::initializeModes() {
    // Create mode implementations
    m_modes[RenderingConfig::DisplayMode::Points] = std::make_unique<PointsMode>();
    m_modes[RenderingConfig::DisplayMode::Wireframe] = std::make_unique<WireframeMode>();
    m_modes[RenderingConfig::DisplayMode::SolidWireframe] = std::make_unique<FlatLinesMode>();
    m_modes[RenderingConfig::DisplayMode::HiddenLine] = std::make_unique<FlatLinesMode>();
    m_modes[RenderingConfig::DisplayMode::Solid] = std::make_unique<ShadedMode>();
    m_modes[RenderingConfig::DisplayMode::NoShading] = std::make_unique<ShadedMode>();

    // Build index array for fast lookup by SoSwitch child index
    m_modeIndex.resize(4, nullptr);
    for (auto& pair : m_modes) {
        int index = pair.second->getSwitchChildIndex();
        if (index >= 0 && index < 4) {
            m_modeIndex[index] = pair.second.get();
        }
    }
}

IDisplayMode* ViewerModManager::getMode(RenderingConfig::DisplayMode mode) const {
    auto it = m_modes.find(mode);
    if (it != m_modes.end()) {
        return it->second.get();
    }
    
    // Fallback: return Shaded mode for unknown modes
    auto shadedIt = m_modes.find(RenderingConfig::DisplayMode::Solid);
    if (shadedIt != m_modes.end()) {
        return shadedIt->second.get();
    }
    
    return nullptr;
}

IDisplayMode* ViewerModManager::getModeByIndex(int index) const {
    if (index >= 0 && index < static_cast<int>(m_modeIndex.size())) {
        return m_modeIndex[index];
    }
    return nullptr;
}

int ViewerModManager::getModeIndex(RenderingConfig::DisplayMode mode) const {
    IDisplayMode* modeImpl = getMode(mode);
    if (modeImpl) {
        return modeImpl->getSwitchChildIndex();
    }
    return 3; // Default to Shaded mode index
}

SoSeparator* ViewerModManager::buildModeNode(
    RenderingConfig::DisplayMode mode,
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context,
    ModularEdgeComponent* modularEdgeComponent,
    VertexExtractor* vertexExtractor) const {
    
    IDisplayMode* modeImpl = getMode(mode);
    if (!modeImpl) {
        LOG_ERR_S("ViewerModManager: No implementation found for display mode " + std::to_string(static_cast<int>(mode)));
        return nullptr;
    }
    
    return modeImpl->buildModeNode(shape, params, context, modularEdgeComponent, vertexExtractor);
}

std::vector<SoSeparator*> ViewerModManager::buildAllModeNodes(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context,
    ModularEdgeComponent* modularEdgeComponent,
    VertexExtractor* vertexExtractor) const {
    
    std::vector<SoSeparator*> modeNodes(4, nullptr);
    
    // Build each mode node in order: Points, Wireframe, FlatLines, Shaded
    // CRITICAL: Use the correct DisplayMode for each index based on button mappings
    // Index 0: Points -> DisplayMode::Points
    // Index 1: Wireframe -> DisplayMode::Wireframe
    // Index 2: FlatLines -> DisplayMode::SolidWireframe (FlatLines and ShadedWireframe both use this)
    // Index 3: Shaded -> DisplayMode::Solid (Shaded and NoShading both use this, but NoShading is handled in ShadedMode::buildModeNode)
    
    // Build Points mode (index 0)
    IDisplayMode* pointsMode = getMode(RenderingConfig::DisplayMode::Points);
    if (pointsMode) {
        GeometryRenderContext modeContext = context;
        modeContext.display.displayMode = RenderingConfig::DisplayMode::Points;
        modeContext.display.wireframeMode = false;
        modeContext.display.facesVisible = false;
        modeContext.display.showPointView = true;
        modeNodes[0] = pointsMode->buildModeNode(shape, params, modeContext, modularEdgeComponent, vertexExtractor);
        if (!modeNodes[0]) {
            LOG_WRN_S("ViewerModManager: Points mode node returned nullptr - shape.IsNull()=" + 
                std::string(shape.IsNull() ? "true" : "false") + 
                ", vertexExtractor=" + std::string(vertexExtractor ? "available" : "null"));
        }
    } else {
        LOG_ERR_S("ViewerModManager: Points mode implementation not found");
    }
    
    // Build Wireframe mode (index 1)
    IDisplayMode* wireframeMode = getMode(RenderingConfig::DisplayMode::Wireframe);
    if (wireframeMode) {
        GeometryRenderContext modeContext = context;
        modeContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        modeContext.display.wireframeMode = true;
        modeContext.display.facesVisible = false;
        modeContext.display.showPointView = false;
        modeNodes[1] = wireframeMode->buildModeNode(shape, params, modeContext, modularEdgeComponent, vertexExtractor);
        if (!modeNodes[1]) {
            LOG_WRN_S("ViewerModManager: Wireframe mode node returned nullptr - shape.IsNull()=" + 
                std::string(shape.IsNull() ? "true" : "false") + 
                ", modularEdgeComponent=" + std::string(modularEdgeComponent ? "available" : "null"));
        }
    } else {
        LOG_ERR_S("ViewerModManager: Wireframe mode implementation not found");
    }
    
    // Build FlatLines mode (index 2) - used for SolidWireframe and HiddenLine
    IDisplayMode* flatLinesMode = getMode(RenderingConfig::DisplayMode::SolidWireframe);
    if (flatLinesMode) {
        GeometryRenderContext modeContext = context;
        // Use SolidWireframe as default, but the actual mode will be set when switching
        modeContext.display.displayMode = RenderingConfig::DisplayMode::SolidWireframe;
        modeContext.display.wireframeMode = false;
        modeContext.display.facesVisible = true;
        modeContext.display.showPointView = false;
        modeNodes[2] = flatLinesMode->buildModeNode(shape, params, modeContext, modularEdgeComponent, vertexExtractor);
        if (!modeNodes[2]) {
            LOG_WRN_S("ViewerModManager: FlatLines mode node returned nullptr - shape.IsNull()=" + 
                std::string(shape.IsNull() ? "true" : "false"));
        }
    } else {
        LOG_ERR_S("ViewerModManager: FlatLines mode implementation not found");
    }
    
    // Build Shaded mode (index 3) - used for Solid and NoShading
    IDisplayMode* shadedMode = getMode(RenderingConfig::DisplayMode::Solid);
    if (shadedMode) {
        GeometryRenderContext modeContext = context;
        // Use Solid as default, but NoShading will be handled in ShadedMode::buildModeNode
        modeContext.display.displayMode = RenderingConfig::DisplayMode::Solid;
        modeContext.display.wireframeMode = false;
        modeContext.display.facesVisible = true;
        modeContext.display.showPointView = false;
        modeNodes[3] = shadedMode->buildModeNode(shape, params, modeContext, modularEdgeComponent, vertexExtractor);
    }
    
    // Log warnings for nullptr nodes
    for (int i = 0; i < 4; ++i) {
        if (!modeNodes[i]) {
            LOG_WRN_S("ViewerModManager: Mode node " + std::to_string(i) + " returned nullptr");
        }
    }
    
    return modeNodes;
}


