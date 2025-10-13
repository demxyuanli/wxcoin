#pragma once

#include <memory>
#include <vector>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "viewer/config/NormalDisplayConfig.h"
#include "OCCGeometry.h"

class EdgeDisplayManager;

/**
 * @brief Service for managing normal display functionality
 *
 * This service encapsulates all normal display related operations,
 * providing a clean interface for controlling surface normal visualization.
 */
class NormalDisplayService {
public:
    NormalDisplayService();
    ~NormalDisplayService();

    // Configuration management
    void setNormalDisplayConfig(const NormalDisplayConfig& config);
    const NormalDisplayConfig& getNormalDisplayConfig() const;

    // Normal display control
    void setShowNormals(bool showNormals, EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams);
    bool isShowNormals() const;

    void setNormalLength(double length);
    double getNormalLength() const;

    void setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect);
    void getNormalColor(Quantity_Color& correct, Quantity_Color& incorrect) const;

    // Advanced features
    void setNormalConsistencyMode(bool enabled);
    bool isNormalConsistencyModeEnabled() const;

    void setNormalDebugMode(bool enabled);
    bool isNormalDebugModeEnabled() const;

    // Operations
    void refreshNormalDisplay(EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams);
    void toggleNormalDisplay(EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams);

    // Utility methods
    void enableNormalDebugVisualization(EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams);

private:
    NormalDisplayConfig m_config;
    EdgeDisplayManager* m_edgeDisplayManager;

    // Internal helper methods
    void updateNormalDisplaySettings();
    void forceNormalRegeneration(EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams);
};
