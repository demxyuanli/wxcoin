#pragma once

#include "viewer/ImageOutlinePass.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

// Forward declarations
class SceneManager;
class OCCGeometry;
class SoSeparator;
class DynamicSilhouetteRenderer;

/**
 * @brief Manages outline display for all geometries in the scene
 * 
 * Coordinates between image-based outline rendering (ImageOutlinePass) and
 * per-geometry silhouette rendering for hybrid outline display modes.
 */
class OutlineDisplayManager {
public:
    /**
     * @brief Constructor
     * @param sceneManager Scene manager for rendering context
     * @param occRoot Root node for OpenCASCADE geometry
     * @param geometries Pointer to geometry collection
     */
    OutlineDisplayManager(SceneManager* sceneManager,
                         SoSeparator* occRoot,
                         std::vector<std::shared_ptr<OCCGeometry>>* geometries);
    
    /**
     * @brief Destructor
     */
    ~OutlineDisplayManager();

    /**
     * @brief Enable or disable outline rendering
     * @param enabled True to enable outline rendering
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if outline rendering is enabled
     * @return True if outline rendering is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Called when a new geometry is added to the scene
     * @param geometry The newly added geometry
     */
    void onGeometryAdded(const std::shared_ptr<OCCGeometry>& geometry);

    /**
     * @brief Update outline rendering for all geometries
     */
    void updateAll();

    /**
     * @brief Clear all outline renderers
     */
    void clearAll();

    /**
     * @brief Set outline rendering parameters
     * @param params New parameters for image-based outline rendering
     */
    void setParams(const ImageOutlineParams& params);
    
    /**
     * @brief Get current outline rendering parameters
     * @return Current outline parameters
     */
    ImageOutlineParams getParams() const;

    /**
     * @brief Refresh outline rendering for all geometries
     */
    void refreshOutlineAll();

private:
    void ensureForGeometry(const std::shared_ptr<OCCGeometry>& geometry);

    SceneManager* m_sceneManager;
    SoSeparator* m_occRoot;
    std::vector<std::shared_ptr<OCCGeometry>>* m_geometries;

    // Image-based outline pass
    std::unique_ptr<ImageOutlinePass> m_imagePass;

    // Per-geometry silhouette renderers (legacy/fallback)
    std::unordered_map<std::string, std::unique_ptr<DynamicSilhouetteRenderer>> m_outlineByName;

    bool m_enabled = false;
};