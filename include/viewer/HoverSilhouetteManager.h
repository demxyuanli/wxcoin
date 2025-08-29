#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <wx/gdicmn.h>

// Forward declarations
class SceneManager;
class OCCGeometry;
class SoSeparator;
class PickingService;
class DynamicSilhouetteRenderer;

/**
 * @brief Manages silhouette highlighting for hovered geometries
 * 
 * Provides real-time outline highlighting when the mouse hovers over
 * geometry objects. Uses per-geometry silhouette rendering for
 * immediate visual feedback.
 */
class HoverSilhouetteManager {
public:
    /**
     * @brief Constructor
     * @param sceneManager Scene manager for rendering context
     * @param occRoot Root node for OpenCASCADE geometry
     * @param pickingService Service for geometry picking
     */
    HoverSilhouetteManager(SceneManager* sceneManager,
                          SoSeparator* occRoot,
                          PickingService* pickingService);
    
    /**
     * @brief Destructor
     */
    ~HoverSilhouetteManager();

    /**
     * @brief Update hover silhouette based on screen position
     * @param screenPos Screen position to check for hover
     */
    void updateHoverSilhouetteAt(const wxPoint& screenPos);

    /**
     * @brief Set silhouette for a specific geometry
     * @param geometry Geometry to highlight (nullptr to clear)
     */
    void setHoveredSilhouette(std::shared_ptr<OCCGeometry> geometry);

    /**
     * @brief Disable all hover silhouettes
     */
    void disableAll();

private:
    SceneManager* m_sceneManager;
    SoSeparator* m_occRoot;
    PickingService* m_pickingService;

    // Per-geometry silhouette renderers
    std::unordered_map<std::string, std::unique_ptr<DynamicSilhouetteRenderer>> m_silhouetteRenderers;

    // Track last hovered geometry to avoid unnecessary updates
    std::weak_ptr<OCCGeometry> m_lastHoverGeometry;
};