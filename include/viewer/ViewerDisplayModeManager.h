#pragma once

#include "config/RenderingConfig.h"
#include <vector>
#include <memory>

class OCCGeometry;
class OCCViewer;

/**
 * @brief Manager for view-level display mode
 * 
 * Manages display mode at the viewer level (applies to all objects in the view).
 * This is the global display mode that affects all geometries in the scene.
 */
class ViewerDisplayModeManager {
public:
    ViewerDisplayModeManager();
    ~ViewerDisplayModeManager();

    /**
     * @brief Set view-level display mode
     * Applies the mode to all geometries in the viewer
     */
    void setViewDisplayMode(OCCViewer* viewer, RenderingConfig::DisplayMode mode);

    /**
     * @brief Get current view-level display mode
     */
    RenderingConfig::DisplayMode getViewDisplayMode() const { return m_viewDisplayMode; }

    /**
     * @brief Apply view display mode to a specific geometry
     * This is called for each geometry when view mode changes
     */
    void applyToGeometry(OCCGeometry* geometry, RenderingConfig::DisplayMode mode);

    /**
     * @brief Check if view display mode has changed
     */
    bool hasModeChanged(RenderingConfig::DisplayMode newMode) const {
        return m_viewDisplayMode != newMode;
    }

    /**
     * @brief Update lighting based on display mode
     */
    void updateLightingForMode(RenderingConfig::DisplayMode mode);

private:
    RenderingConfig::DisplayMode m_viewDisplayMode;
    
    void applyModeToAllGeometries(OCCViewer* viewer, RenderingConfig::DisplayMode mode);
};



