#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "EdgeTypes.h"

/**
 * @brief Edge Level of Detail (LOD) management system
 *
 * Manages multiple levels of detail for edge display based on viewing distance.
 * Reduces rendering load for distant objects while maintaining quality for close objects.
 */
class EdgeLODManager {
public:
    /**
     * @brief LOD level definitions
     */
    enum class LODLevel {
        Minimal,    // Very distant - minimal or no edges
        Low,        // Distant - simplified edges
        Medium,     // Medium distance - standard edges
        High,       // Close - detailed edges
        Maximum     // Very close - maximum detail
    };

    /**
     * @brief Distance thresholds for LOD transitions (in world units)
     */
    struct LODThresholds {
        double minimalDistance;     // Below this: LOD 0 (Minimal)
        double lowDistance;         // Below this: LOD 1 (Low)
        double mediumDistance;      // Below this: LOD 2 (Medium)
        double highDistance;        // Below this: LOD 3 (High)
        // Above highDistance: LOD 4 (Maximum)
    };

    /**
     * @brief LOD statistics for a shape
     */
    struct LODStats {
        size_t totalEdges;
        size_t minimalEdges;    // LOD 0
        size_t lowEdges;        // LOD 1
        size_t mediumEdges;     // LOD 2
        size_t highEdges;       // LOD 3
        size_t maximumEdges;    // LOD 4

        double memoryUsageMB;  // Estimated memory usage
    };

    EdgeLODManager();
    ~EdgeLODManager();

    /**
     * @brief Set distance thresholds for LOD transitions
     * @param thresholds Distance thresholds for each LOD level
     */
    void setLODThresholds(const LODThresholds& thresholds);

    /**
     * @brief Get current distance thresholds
     * @return Current LOD thresholds
     */
    const LODThresholds& getLODThresholds() const { return m_thresholds; }

    /**
     * @brief Generate LOD levels for a shape
     * @param shape The CAD shape
     * @param cameraPos Camera position for distance calculation
     * @param lodStats Output statistics (optional)
     */
    void generateLODLevels(const TopoDS_Shape& shape,
                          const gp_Pnt& cameraPos,
                          LODStats* lodStats = nullptr);

    /**
     * @brief Get appropriate LOD level for a given distance
     * @param distance Distance from camera to object
     * @return Recommended LOD level
     */
    LODLevel getLODLevel(double distance) const;

    /**
     * @brief Get edge points for a specific LOD level
     * @param level LOD level
     * @return Vector of edge points for rendering
     */
    const std::vector<gp_Pnt>& getLODEdges(LODLevel level) const;

    /**
     * @brief Get current active LOD level
     * @return Currently active LOD level
     */
    LODLevel getCurrentLODLevel() const { return m_currentLODLevel; }

    /**
     * @brief Update LOD level based on camera position
     * @param cameraPos New camera position
     * @return True if LOD level changed
     */
    bool updateLODLevel(const gp_Pnt& cameraPos);

    /**
     * @brief Get LOD statistics
     * @return Current LOD statistics
     */
    const LODStats& getLODStats() const { return m_lodStats; }

    /**
     * @brief Clear all LOD data
     */
    void clear();

    /**
     * @brief Enable/disable LOD system
     * @param enabled True to enable LOD
     */
    void setLODEnabled(bool enabled) { m_lodEnabled = enabled; }

    /**
     * @brief Check if LOD is enabled
     * @return True if LOD is enabled
     */
    bool isLODEnabled() const { return m_lodEnabled; }

    /**
     * @brief Set transition hysteresis to prevent flickering
     * @param hysteresis Distance hysteresis for LOD transitions
     */
    void setTransitionHysteresis(double hysteresis) { m_transitionHysteresis = hysteresis; }

    /**
     * @brief Get transition hysteresis
     * @return Current hysteresis value
     */
    double getTransitionHysteresis() const { return m_transitionHysteresis; }

private:
    // LOD data storage
    std::unordered_map<LODLevel, std::vector<gp_Pnt>> m_lodEdgeData;

    // Configuration
    LODThresholds m_thresholds;
    bool m_lodEnabled;
    double m_transitionHysteresis;

    // State
    LODLevel m_currentLODLevel;
    gp_Pnt m_lastCameraPos;
    LODStats m_lodStats;

    // Helper methods
    void generateMinimalLOD(const TopoDS_Shape& shape);
    void generateLowLOD(const TopoDS_Shape& shape);
    void generateMediumLOD(const TopoDS_Shape& shape);
    void generateHighLOD(const TopoDS_Shape& shape);
    void generateMaximumLOD(const TopoDS_Shape& shape);

    double calculateDistanceToShape(const TopoDS_Shape& shape, const gp_Pnt& cameraPos) const;
    size_t estimateMemoryUsage(const std::vector<gp_Pnt>& points) const;

    // Default thresholds
    static const LODThresholds DEFAULT_THRESHOLDS;
};

// Default LOD thresholds
inline const EdgeLODManager::LODThresholds EdgeLODManager::DEFAULT_THRESHOLDS = {
    1000.0,  // minimalDistance: > 1000 units - minimal edges
    500.0,   // lowDistance: > 500 units - low detail
    200.0,   // mediumDistance: > 200 units - medium detail
    50.0     // highDistance: > 50 units - high detail
              // < 50 units - maximum detail
};


