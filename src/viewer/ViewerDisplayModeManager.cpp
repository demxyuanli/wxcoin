#include "viewer/ViewerDisplayModeManager.h"
#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"
#include "config/LightingConfig.h"
#include "SceneManager.h"

ViewerDisplayModeManager::ViewerDisplayModeManager()
    : m_viewDisplayMode(RenderingConfig::DisplayMode::Solid)
{
}

ViewerDisplayModeManager::~ViewerDisplayModeManager()
{
}

void ViewerDisplayModeManager::setViewDisplayMode(OCCViewer* viewer, RenderingConfig::DisplayMode mode)
{
    if (!viewer) {
        LOG_ERR_S("ViewerDisplayModeManager: Viewer is null");
        return;
    }

    if (m_viewDisplayMode == mode) {
        return; // No change needed
    }

    RenderingConfig::DisplayMode oldMode = m_viewDisplayMode;
    m_viewDisplayMode = mode;

    LOG_INF_S("ViewerDisplayModeManager: Changing view display mode from " +
        std::to_string(static_cast<int>(oldMode)) + " to " +
        std::to_string(static_cast<int>(mode)));

    // Apply mode to all geometries
    applyModeToAllGeometries(viewer, mode);

    // NOTE: Lighting updates are DISABLED during mode switching
    // Lighting should only be updated when explicitly changed by user
    // updateLightingForMode(mode);
}

void ViewerDisplayModeManager::applyToGeometry(OCCGeometry* geometry, RenderingConfig::DisplayMode mode)
{
    if (!geometry) {
        return;
    }

    // CRITICAL FIX: Only set display mode - let GeometryRenderer handle SoSwitch switching
    // Do NOT call updateDisplayMode here as setDisplayMode already does it internally
    geometry->setDisplayMode(mode);

    // NOTE: No additional logic needed - setDisplayMode handles everything
    // - Updates internal state
    // - Calls updateDisplayMode for fast SoSwitch switching
    // - Only rebuilds if coin node doesn't exist
}

void ViewerDisplayModeManager::updateLightingForMode(RenderingConfig::DisplayMode mode)
{
    // CRITICAL FIX: COMPLETELY DISABLE lighting updates during mode switching
    // Lighting should NOT be updated when switching display modes - this causes
    // unnecessary rebuilds and interferes with SoSwitch operation

    // NOTE: Lighting is only updated when explicitly changed by user through lighting controls
    // Display mode changes should only affect which SoSwitch child is active, not lighting

    // This prevents the "LightingConfig callback triggered" that causes rebuilds
    // and ensures SoSwitch switching works correctly

    // Lighting updates are handled separately through LightingConfig callbacks
    // when the user actually changes lighting settings, not display modes
}

void ViewerDisplayModeManager::applyModeToAllGeometries(OCCViewer* viewer, RenderingConfig::DisplayMode mode)
{
    auto geometries = viewer->getAllGeometry();
    LOG_INF_S("ViewerDisplayModeManager: Applying mode to " + std::to_string(geometries.size()) + " geometries");

    for (auto& geometry : geometries) {
        if (geometry) {
            applyToGeometry(geometry.get(), mode);
        }
    }
}


