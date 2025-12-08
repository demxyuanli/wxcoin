#pragma once

#include "IDisplayMode.h"
#include "config/RenderingConfig.h"
#include <memory>
#include <unordered_map>

/**
 * @brief Manager for display mode implementations
 * 
 * Centralized management of all display modes (Points, Wireframe, FlatLines, Shaded).
 * Provides unified interface for mode switching and node building.
 */
class ViewerModManager {
public:
    ViewerModManager();
    ~ViewerModManager();

    /**
     * @brief Get display mode implementation for a given mode type
     */
    IDisplayMode* getMode(RenderingConfig::DisplayMode mode) const;

    /**
     * @brief Get display mode implementation by SoSwitch child index
     */
    IDisplayMode* getModeByIndex(int index) const;

    /**
     * @brief Get SoSwitch child index for a display mode
     */
    int getModeIndex(RenderingConfig::DisplayMode mode) const;

    /**
     * @brief Build mode node for a given display mode
     */
    SoSeparator* buildModeNode(
        RenderingConfig::DisplayMode mode,
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const GeometryRenderContext& context,
        ModularEdgeComponent* modularEdgeComponent,
        VertexExtractor* vertexExtractor) const;

    /**
     * @brief Build all mode nodes for SoSwitch (FreeCAD-style)
     * Returns vector of mode nodes in order: [Points, Wireframe, FlatLines, Shaded]
     */
    std::vector<SoSeparator*> buildAllModeNodes(
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const GeometryRenderContext& context,
        ModularEdgeComponent* modularEdgeComponent,
        VertexExtractor* vertexExtractor) const;

private:
    std::unordered_map<RenderingConfig::DisplayMode, std::unique_ptr<IDisplayMode>> m_modes;
    std::vector<IDisplayMode*> m_modeIndex; // Indexed by SoSwitch child index

    void initializeModes();
};



