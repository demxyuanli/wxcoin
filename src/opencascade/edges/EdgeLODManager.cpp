#include "edges/EdgeLODManager.h"
#include "edges/EdgeExtractor.h"
#include "logger/Logger.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <algorithm>
#include <cmath>

EdgeLODManager::EdgeLODManager()
    : m_lodEnabled(true)
    , m_currentLODLevel(LODLevel::Maximum)
    , m_transitionHysteresis(10.0)
    , m_thresholds(DEFAULT_THRESHOLDS)
{
    LOG_INF_S("EdgeLODManager initialized with default thresholds");
}

EdgeLODManager::~EdgeLODManager()
{
    clear();
}

void EdgeLODManager::setLODThresholds(const LODThresholds& thresholds)
{
    m_thresholds = thresholds;
    LOG_INF_S("LOD thresholds updated: minimal=" + std::to_string(thresholds.minimalDistance) +
              ", low=" + std::to_string(thresholds.lowDistance) +
              ", medium=" + std::to_string(thresholds.mediumDistance) +
              ", high=" + std::to_string(thresholds.highDistance));
}

EdgeLODManager::LODLevel EdgeLODManager::getLODLevel(double distance) const
{
    if (distance > m_thresholds.minimalDistance) {
        return LODLevel::Minimal;
    } else if (distance > m_thresholds.lowDistance) {
        return LODLevel::Low;
    } else if (distance > m_thresholds.mediumDistance) {
        return LODLevel::Medium;
    } else if (distance > m_thresholds.highDistance) {
        return LODLevel::High;
    } else {
        return LODLevel::Maximum;
    }
}

void EdgeLODManager::generateLODLevels(const TopoDS_Shape& shape,
                                      const gp_Pnt& cameraPos,
                                      LODStats* lodStats)
{
    if (!m_lodEnabled) {
        LOG_DBG_S("LOD generation skipped - LOD disabled");
        return;
    }

    LOG_INF_S("Generating LOD levels for shape");

    // Calculate distance for LOD determination
    double distance = calculateDistanceToShape(shape, cameraPos);
    LODLevel targetLOD = getLODLevel(distance);

    // Clear existing data
    clear();

    // Generate all LOD levels (pre-compute for smooth transitions)
    generateMinimalLOD(shape);
    generateLowLOD(shape);
    generateMediumLOD(shape);
    generateHighLOD(shape);
    generateMaximumLOD(shape);

    // Update current LOD
    m_currentLODLevel = targetLOD;
    m_lastCameraPos = cameraPos;

    // Calculate statistics
    m_lodStats.totalEdges = 0;
    m_lodStats.minimalEdges = m_lodEdgeData[LODLevel::Minimal].size() / 2;
    m_lodStats.lowEdges = m_lodEdgeData[LODLevel::Low].size() / 2;
    m_lodStats.mediumEdges = m_lodEdgeData[LODLevel::Medium].size() / 2;
    m_lodStats.highEdges = m_lodEdgeData[LODLevel::High].size() / 2;
    m_lodStats.maximumEdges = m_lodEdgeData[LODLevel::Maximum].size() / 2;

    // Estimate memory usage
    m_lodStats.memoryUsageMB = 0.0;
    for (const auto& pair : m_lodEdgeData) {
        m_lodStats.memoryUsageMB += estimateMemoryUsage(pair.second);
    }

    // Copy stats if requested
    if (lodStats) {
        *lodStats = m_lodStats;
    }

    LOG_INF_S("LOD generation completed:");
    LOG_INF_S("  Current distance: " + std::to_string(distance));
    LOG_INF_S("  Target LOD: " + std::to_string(static_cast<int>(targetLOD)));
    LOG_INF_S("  Minimal edges: " + std::to_string(m_lodStats.minimalEdges));
    LOG_INF_S("  Low edges: " + std::to_string(m_lodStats.lowEdges));
    LOG_INF_S("  Medium edges: " + std::to_string(m_lodStats.mediumEdges));
    LOG_INF_S("  High edges: " + std::to_string(m_lodStats.highEdges));
    LOG_INF_S("  Maximum edges: " + std::to_string(m_lodStats.maximumEdges));
    LOG_INF_S("  Memory usage: " + std::to_string(m_lodStats.memoryUsageMB) + " MB");
}

const std::vector<gp_Pnt>& EdgeLODManager::getLODEdges(LODLevel level) const
{
    auto it = m_lodEdgeData.find(level);
    if (it != m_lodEdgeData.end()) {
        return it->second;
    }

    // Fallback to maximum detail if requested level not found
    static const std::vector<gp_Pnt> emptyVector;
    LOG_WRN_S("Requested LOD level not found, returning empty vector");
    return emptyVector;
}

bool EdgeLODManager::updateLODLevel(const gp_Pnt& cameraPos)
{
    if (!m_lodEnabled) {
        return false;
    }

    // Calculate distance (we'd need the shape for accurate calculation)
    // For now, use a simple distance-based approach
    double distance = cameraPos.Distance(gp_Pnt(0, 0, 0)); // Placeholder

    LODLevel newLOD = getLODLevel(distance);

    if (newLOD != m_currentLODLevel) {
        LOG_INF_S("LOD level changed from " + std::to_string(static_cast<int>(m_currentLODLevel)) +
                  " to " + std::to_string(static_cast<int>(newLOD)) +
                  " (distance: " + std::to_string(distance) + ")");

        m_currentLODLevel = newLOD;
        m_lastCameraPos = cameraPos;
        return true;
    }

    return false;
}

void EdgeLODManager::clear()
{
    m_lodEdgeData.clear();
    m_lodStats = LODStats{0, 0, 0, 0, 0, 0, 0.0};
    LOG_DBG_S("EdgeLODManager cleared");
}

void EdgeLODManager::generateMinimalLOD(const TopoDS_Shape& shape)
{
    // Minimal LOD: Only show major structural edges or hide completely
    // For now, use a very coarse sampling
    EdgeExtractor extractor;
    auto edges = extractor.extractOriginalEdges(shape, 5.0, 5.0); // Very coarse

    // Further reduce by keeping only every Nth edge
    std::vector<gp_Pnt> minimalEdges;
    for (size_t i = 0; i < edges.size(); i += 12) { // Keep only every 6th edge
        if (i + 1 < edges.size()) {
            minimalEdges.push_back(edges[i]);
            minimalEdges.push_back(edges[i + 1]);
        }
    }

    m_lodEdgeData[LODLevel::Minimal] = std::move(minimalEdges);
}

void EdgeLODManager::generateLowLOD(const TopoDS_Shape& shape)
{
    // Low LOD: Simplified edges, reduced detail
    EdgeExtractor extractor;
    auto edges = extractor.extractOriginalEdges(shape, 10.0, 2.0); // Coarse sampling

    // Reduce edge count by filtering short edges and simplifying
    std::vector<gp_Pnt> lowEdges;
    for (size_t i = 0; i < edges.size(); i += 8) { // Keep every 4th edge
        if (i + 1 < edges.size()) {
            lowEdges.push_back(edges[i]);
            lowEdges.push_back(edges[i + 1]);
        }
    }

    m_lodEdgeData[LODLevel::Low] = std::move(lowEdges);
}

void EdgeLODManager::generateMediumLOD(const TopoDS_Shape& shape)
{
    // Medium LOD: Standard detail level
    EdgeExtractor extractor;
    auto edges = extractor.extractOriginalEdges(shape, 40.0, 1.0); // Medium sampling

    // Moderate reduction
    std::vector<gp_Pnt> mediumEdges;
    for (size_t i = 0; i < edges.size(); i += 4) { // Keep every 2nd edge
        if (i + 1 < edges.size()) {
            mediumEdges.push_back(edges[i]);
            mediumEdges.push_back(edges[i + 1]);
        }
    }

    m_lodEdgeData[LODLevel::Medium] = std::move(mediumEdges);
}

void EdgeLODManager::generateHighLOD(const TopoDS_Shape& shape)
{
    // High LOD: Detailed but not maximum
    EdgeExtractor extractor;
    auto edges = extractor.extractOriginalEdges(shape, 60.0, 0.5); // Fine sampling

    // Light reduction
    std::vector<gp_Pnt> highEdges;
    for (size_t i = 0; i < edges.size(); i += 2) { // Keep most edges
        if (i + 1 < edges.size()) {
            highEdges.push_back(edges[i]);
            highEdges.push_back(edges[i + 1]);
        }
    }

    m_lodEdgeData[LODLevel::High] = std::move(highEdges);
}

void EdgeLODManager::generateMaximumLOD(const TopoDS_Shape& shape)
{
    // Maximum LOD: Full detail
    EdgeExtractor extractor;
    auto edges = extractor.extractOriginalEdges(shape, 80.0, 0.01); // Full detail

    m_lodEdgeData[LODLevel::Maximum] = std::move(edges);
}

double EdgeLODManager::calculateDistanceToShape(const TopoDS_Shape& shape, const gp_Pnt& cameraPos) const
{
    // Calculate bounding box center and distance to camera
    Bnd_Box bbox;
    BRepBndLib::Add(shape, bbox);

    if (bbox.IsVoid()) {
        return 0.0; // Very close if no bounding box
    }

    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    // Calculate center of bounding box
    gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);

    // Return distance from camera to center
    return cameraPos.Distance(center);
}

size_t EdgeLODManager::estimateMemoryUsage(const std::vector<gp_Pnt>& points) const
{
    // Estimate: each point uses ~24 bytes (3 doubles)
    // Plus some overhead for vector
    const double bytesPerPoint = 24.0;
    const double overhead = 64.0; // Vector overhead
    return static_cast<size_t>((points.size() * bytesPerPoint + overhead) / (1024.0 * 1024.0)); // MB
}


