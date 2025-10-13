#pragma once

#include <memory>
#include <vector>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "OCCGeometry.h"

class EdgeDisplayManager;

/**
 * @brief Service for managing rendering modes and display options
 *
 * This service encapsulates all rendering mode related operations,
 * providing a clean interface for controlling wireframe, shading, edges, etc.
 */
class RenderModeManager {
public:
    RenderModeManager();
    ~RenderModeManager();

    // Wireframe mode management
    void setWireframeMode(bool wireframe, std::vector<std::shared_ptr<OCCGeometry>>& geometries);
    bool isWireframeMode() const;

    // Shading mode management
    void setShadingMode(bool shading);
    bool isShadingMode() const;

    // Edge display management
    void setShowEdges(bool showEdges, EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams);
    bool isShowEdges() const;

    // Anti-aliasing management
    void setAntiAliasing(bool enabled);
    bool isAntiAliasing() const;

    // Batch operations for geometries
    void applyWireframeToAllGeometries(std::vector<std::shared_ptr<OCCGeometry>>& geometries);
    void applyShadingToAllGeometries(std::vector<std::shared_ptr<OCCGeometry>>& geometries);

    // Configuration access
    void setWireframeModeInternal(bool mode) { m_wireframeMode = mode; }
    void setShadingModeInternal(bool mode) { m_shadingMode = mode; }
    void setShowEdgesInternal(bool show) { m_showEdges = show; }
    void setAntiAliasingInternal(bool enabled) { m_antiAliasing = enabled; }

private:
    // Rendering mode states
    bool m_wireframeMode = false;
    bool m_shadingMode = false;
    bool m_showEdges = true;
    bool m_antiAliasing = true;

    // Helper methods
    void updateRenderingToolkitConfiguration(bool showEdges);
};
