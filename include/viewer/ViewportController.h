#pragma once

#include <memory>
#include <OpenCASCADE/gp_Pnt.hxx>

// Forward declarations
class SceneManager;
class OCCGeometry;
class SoCamera;

/**
 * @brief Viewport and view manipulation controller
 * 
 * Handles camera positioning, fitting, and view operations
 */
class ViewportController {
public:
    explicit ViewportController(SceneManager* sceneManager);
    ~ViewportController() = default;

    /**
     * @brief Fit all geometry in view
     */
    void fitAll();

    /**
     * @brief Fit specific geometry in view
     * @param name Geometry name to fit
     */
    void fitGeometry(const std::string& name);

    /**
     * @brief Request view refresh
     */
    void requestViewRefresh();

    /**
     * @brief Get camera position
     * @return Current camera position as gp_Pnt
     */
    gp_Pnt getCameraPosition() const;

    /**
     * @brief Set preserve view on add flag
     * @param preserve If true, preserve current view when adding geometry
     */
    void setPreserveViewOnAdd(bool preserve) { m_preserveViewOnAdd = preserve; }

    /**
     * @brief Check if preserve view on add is enabled
     */
    bool isPreserveViewOnAdd() const { return m_preserveViewOnAdd; }

private:
    SceneManager* m_sceneManager;
    bool m_preserveViewOnAdd;
    
    SoCamera* getCamera() const;
};
