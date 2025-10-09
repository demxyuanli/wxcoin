#pragma once

#include <vector>
#include <memory>

// Forward declarations
class OCCGeometry;

/**
 * @brief Rendering and display mode controller
 * 
 * Manages display modes, wireframe, edges, anti-aliasing, and normals
 */
class RenderingController {
public:
    RenderingController();
    ~RenderingController() = default;

    /**
     * @brief Set wireframe mode for all geometries
     * @param wireframe If true, display in wireframe mode
     */
    void setWireframeMode(bool wireframe);

    /**
     * @brief Check if wireframe mode is enabled
     */
    bool isWireframeMode() const { return m_wireframeMode; }

    /**
     * @brief Set show edges for all geometries
     * @param showEdges If true, display edges
     */
    void setShowEdges(bool showEdges);

    /**
     * @brief Check if show edges is enabled
     */
    bool isShowEdges() const { return m_showEdges; }

    /**
     * @brief Set anti-aliasing
     * @param enabled If true, enable anti-aliasing
     */
    void setAntiAliasing(bool enabled);

    /**
     * @brief Check if anti-aliasing is enabled
     */
    bool isAntiAliasing() const { return m_antiAliasing; }

    /**
     * @brief Set show normals
     * @param showNormals If true, display normals
     */
    void setShowNormals(bool showNormals);

    /**
     * @brief Check if show normals is enabled
     */
    bool isShowNormals() const { return m_showNormals; }

    /**
     * @brief Set normal length
     * @param length Normal line length
     */
    void setNormalLength(double length) { m_normalLength = length; }

    /**
     * @brief Get normal length
     */
    double getNormalLength() const { return m_normalLength; }

    /**
     * @brief Set normal consistency mode
     */
    void setNormalConsistencyMode(bool enabled) { m_normalConsistencyMode = enabled; }

    /**
     * @brief Check if normal consistency mode is enabled
     */
    bool isNormalConsistencyModeEnabled() const { return m_normalConsistencyMode; }

    /**
     * @brief Set normal debug mode
     */
    void setNormalDebugMode(bool enabled) { m_normalDebugMode = enabled; }

    /**
     * @brief Check if normal debug mode is enabled
     */
    bool isNormalDebugModeEnabled() const { return m_normalDebugMode; }

    /**
     * @brief Apply rendering settings to geometry
     * @param geometry Geometry to apply settings to
     */
    void applyRenderingSettings(std::shared_ptr<OCCGeometry> geometry);

    /**
     * @brief Apply rendering settings to all geometries
     * @param geometries Vector of geometries
     */
    void applyRenderingSettingsToAll(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);

private:
    bool m_wireframeMode;
    bool m_showEdges;
    bool m_antiAliasing;
    bool m_showNormals;
    double m_normalLength;
    bool m_normalConsistencyMode;
    bool m_normalDebugMode;
};
