#pragma once

#include "IDisplayMode.h"
#include "ViewerModManager.h"
#include "config/RenderingConfig.h"
#include <Inventor/nodes/SoSwitch.h>
#include <memory>

class TopoDS_Shape;
class ModularEdgeComponent;
class VertexExtractor;
struct GeometryRenderContext;
struct MeshParameters;

/**
 * @brief Manager for object-level display mode
 * 
 * Manages display mode at the object level (each geometry has its own mode).
 * This handles the SoSwitch-based fast mode switching for individual objects.
 */
class ObjectDisplayModeManager {
public:
    ObjectDisplayModeManager();
    ~ObjectDisplayModeManager();

    /**
     * @brief Set object-level display mode
     * Uses SoSwitch for fast switching if available
     */
    void setObjectDisplayMode(SoSwitch* modeSwitch, RenderingConfig::DisplayMode mode);

    /**
     * @brief Get current object-level display mode
     */
    RenderingConfig::DisplayMode getObjectDisplayMode() const { return m_objectDisplayMode; }

    /**
     * @brief Build SoSwitch with all mode nodes for an object
     * Returns the SoSwitch node with all mode children
     */
    SoSwitch* buildModeSwitch(
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const GeometryRenderContext& context,
        ModularEdgeComponent* modularEdgeComponent,
        VertexExtractor* vertexExtractor);

    /**
     * @brief Update display mode without rebuilding (fast switch)
     * Uses SoSwitch whichChild for instant switching
     */
    void updateDisplayMode(SoSwitch* modeSwitch, RenderingConfig::DisplayMode mode);

    /**
     * @brief Get ViewerModManager instance
     */
    ViewerModManager* getModManager() const { return m_viewerModManager.get(); }

private:
    RenderingConfig::DisplayMode m_objectDisplayMode;
    std::unique_ptr<ViewerModManager> m_viewerModManager;
    
    int getModeIndex(RenderingConfig::DisplayMode mode) const;
};



