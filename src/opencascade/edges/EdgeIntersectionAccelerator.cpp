#include "edges/EdgeIntersectionAccelerator.h"
#include "logger/Logger.h"
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <algorithm>
#include <execution>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <cmath>

void EdgeIntersectionAccelerator::Statistics::print() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);
    oss << "EdgeIntersectionAccelerator Statistics:\n";
    oss << "  Total Edges: " << totalEdges << "\n";
    oss << "  Potential Pairs: " << potentialPairs << "\n";
    oss << "  Actual Intersections: " << actualIntersections << "\n";
    oss << "  Build Time: " << buildTime << "s\n";
    oss << "  Query Time: " << queryTime << "s\n";
    oss << "  Pruning Ratio: " << (pruningRatio * 100.0) << "%\n";
    LOG_INF_S(oss.str());
}

EdgeIntersectionAccelerator::EdgeIntersectionAccelerator()
    : m_bvh(std::make_unique<BVHAccelerator>()) {
}

void EdgeIntersectionAccelerator::buildFromEdges(
    const std::vector<TopoDS_Edge>& edges,
    size_t maxPrimitivesPerLeaf) {
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    m_edges.clear();
    m_edges.reserve(edges.size());
    
    std::vector<TopoDS_Shape> shapeEdges;
    shapeEdges.reserve(edges.size());
    
    // Extract edge data
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto& edge = edges[i];
        
        if (edge.IsNull() || BRep_Tool::Degenerated(edge)) {
            continue;
        }
        
        EdgePrimitive prim;
        prim.edge = edge;
        
        // Get curve
        Standard_Real first, last;
        prim.curve = BRep_Tool::Curve(edge, first, last);
        prim.first = first;
        prim.last = last;
        
        if (prim.curve.IsNull()) {
            continue;
        }
        
        // Compute bounding box
        Bnd_Box box;
        BRepBndLib::Add(edge, box);
        if (!box.IsVoid()) {
            prim.bounds = box;
        }
        
        // CRITICAL FIX: edgeIndex should be the index in m_edges array, not input array
        // This ensures BVH primitive indices correctly map to m_edges
        prim.edgeIndex = m_edges.size();  // Current position in m_edges
        
        m_edges.push_back(prim);
        shapeEdges.push_back(edge);
    }
    
    // Build BVH
    if (!m_edges.empty()) {
        m_bvh->build(shapeEdges, maxPrimitivesPerLeaf);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalEdges = m_edges.size();
    m_stats.buildTime = std::chrono::duration<double>(endTime - startTime).count();
    
    // Validation: ensure m_edges size matches shapeEdges size
    if (m_edges.size() != shapeEdges.size()) {
        LOG_ERR_S("EdgeIntersectionAccelerator: SIZE MISMATCH! m_edges=" + 
                  std::to_string(m_edges.size()) + ", shapeEdges=" + 
                  std::to_string(shapeEdges.size()));
    }
    
    LOG_INF_S("EdgeIntersectionAccelerator: Built from " + 
              std::to_string(edges.size()) + " input edges, " +
              std::to_string(m_edges.size()) + " valid edges in " + 
              std::to_string(m_stats.buildTime) + "s");
}

std::vector<EdgeIntersectionAccelerator::EdgePair> 
EdgeIntersectionAccelerator::findPotentialIntersections() const {
    
    if (!isBuilt()) {
        LOG_WRN_S("EdgeIntersectionAccelerator: Not built, returning empty list");
        return {};
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::vector<EdgePair> pairs;
    size_t worstCasePairs = (m_edges.size() * (m_edges.size() - 1)) / 2;
    pairs.reserve(std::min(worstCasePairs, size_t(10000))); // Reserve reasonable size
    
    // Query BVH for each edge to find potential intersecting edges
    for (size_t i = 0; i < m_edges.size(); ++i) {
        auto candidates = queryIntersectingEdges(i);
        
        for (size_t j : candidates) {
            // Validate candidate index
            if (j >= m_edges.size()) {
                LOG_ERR_S("EdgeIntersectionAccelerator: Invalid candidate index " + 
                          std::to_string(j) + " >= " + std::to_string(m_edges.size()));
                continue;
            }
            
            if (j > i) { // Avoid duplicates and self-intersection
                pairs.push_back(EdgePair(i, j));
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.queryTime = std::chrono::duration<double>(endTime - startTime).count();
    m_stats.potentialPairs = pairs.size();
    
    size_t totalPairs = (m_edges.size() * (m_edges.size() - 1)) / 2;
    m_stats.pruningRatio = totalPairs > 0 ? 
        1.0 - (double)pairs.size() / totalPairs : 0.0;
    
    LOG_INF_S("EdgeIntersectionAccelerator: Found " + 
              std::to_string(pairs.size()) + " potential pairs, " + 
              "pruning ratio: " + std::to_string(m_stats.pruningRatio * 100) + "%");
    
    return pairs;
}

std::vector<gp_Pnt> EdgeIntersectionAccelerator::extractIntersections(
    double tolerance) const {
    
    auto potentialPairs = findPotentialIntersections();
    std::vector<gp_Pnt> intersections;
    intersections.reserve(potentialPairs.size() / 10); // Heuristic: ~10% actual intersections
    
    for (const auto& pair : potentialPairs) {
        gp_Pnt intersection;
        if (computeEdgeIntersection(m_edges[pair.edge1Index],
                                    m_edges[pair.edge2Index],
                                    tolerance,
                                    intersection)) {
            intersections.push_back(intersection);
        }
    }
    
    m_stats.actualIntersections = intersections.size();
    
    return intersections;
}

std::vector<gp_Pnt> EdgeIntersectionAccelerator::extractIntersectionsParallel(
    double tolerance, size_t numThreads) const {
    
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
    }
    
    auto potentialPairs = findPotentialIntersections();
    
    if (potentialPairs.empty()) {
        return {};
    }
    
    // Thread-local result buffers (lock-free design for better performance)
    std::vector<std::vector<gp_Pnt>> threadResults(numThreads);
    for (auto& buf : threadResults) {
        buf.reserve(potentialPairs.size() / numThreads / 10);  // Heuristic: ~10% hit rate
    }
    
    // Parallel processing using std::for_each with execution policy
    // Note: This requires C++17 and may need TBB or other parallel STL implementation
    try {
        std::atomic<size_t> pairIndex{0};
        
        // Manual thread dispatch for better control
        std::vector<std::thread> threads;
        threads.reserve(numThreads);
        
        for (size_t t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, t]() {
                auto& localResults = threadResults[t];
                
                try {
                    while (true) {
                        size_t idx = pairIndex.fetch_add(1);
                        if (idx >= potentialPairs.size()) break;
                        
                        const auto& pair = potentialPairs[idx];
                        
                        // Validate indices before accessing m_edges
                        if (pair.edge1Index >= m_edges.size() || 
                            pair.edge2Index >= m_edges.size()) {
                            LOG_WRN_S("EdgeIntersectionAccelerator: Invalid edge pair indices");
                            continue;
                        }
                        
                        gp_Pnt intersection;
                        
                        if (computeEdgeIntersection(m_edges[pair.edge1Index],
                                                   m_edges[pair.edge2Index],
                                                   tolerance,
                                                   intersection)) {
                            localResults.push_back(intersection);  // Lock-free!
                        }
                    }
                }
                catch (const std::exception& e) {
                    LOG_ERR_S("EdgeIntersectionAccelerator: Thread exception: " + std::string(e.what()));
                }
                catch (...) {
                    LOG_ERR_S("EdgeIntersectionAccelerator: Thread unknown exception");
                }
            });
        }
        
        // Wait for all threads
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    catch (...) {
        // Fallback to sequential if parallel execution fails
        LOG_WRN_S("EdgeIntersectionAccelerator: Parallel execution failed, falling back to sequential");
        return extractIntersections(tolerance);
    }
    
    // Merge thread-local results
    std::vector<gp_Pnt> allIntersections;
    size_t totalSize = 0;
    for (const auto& results : threadResults) {
        totalSize += results.size();
    }
    allIntersections.reserve(totalSize);
    
    for (const auto& results : threadResults) {
        allIntersections.insert(allIntersections.end(),
                               results.begin(), results.end());
    }
    
    // Remove duplicates (spatial deduplication)
    if (!allIntersections.empty()) {
        std::vector<gp_Pnt> uniqueIntersections;
        uniqueIntersections.reserve(allIntersections.size());
        
        for (const auto& point : allIntersections) {
            bool isDuplicate = false;
            
            // Check against already added unique points
            for (const auto& uniquePoint : uniqueIntersections) {
                if (point.Distance(uniquePoint) < tolerance) {
                    isDuplicate = true;
                    break;
                }
            }
            
            if (!isDuplicate) {
                uniqueIntersections.push_back(point);
            }
        }
        
        allIntersections = std::move(uniqueIntersections);
    }
    
    m_stats.actualIntersections = allIntersections.size();
    
    LOG_INF_S("EdgeIntersectionAccelerator: Found " + 
              std::to_string(allIntersections.size()) + " intersections (parallel)");
    
    return allIntersections;
}

bool EdgeIntersectionAccelerator::computeEdgeIntersection(
    const EdgePrimitive& edge1,
    const EdgePrimitive& edge2,
    double tolerance,
    gp_Pnt& intersection) const {
    
    // Validate input curves
    if (edge1.curve.IsNull() || edge2.curve.IsNull()) {
        return false;
    }
    
    // Validate parameter ranges
    if (edge1.first >= edge1.last || edge2.first >= edge2.last) {
        return false;
    }
    
    // Check if parameter ranges are valid (not too small)
    const double MIN_PARAM_RANGE = 1e-10;
    if ((edge1.last - edge1.first) < MIN_PARAM_RANGE || 
        (edge2.last - edge2.first) < MIN_PARAM_RANGE) {
        return false;
    }
    
    try {
        // Use OpenCASCADE's curve-curve extrema algorithm
        GeomAPI_ExtremaCurveCurve extrema(
            edge1.curve, edge2.curve,
            edge1.first, edge1.last,
            edge2.first, edge2.last
        );
        
        if (extrema.NbExtrema() > 0) {
            double minDist = std::numeric_limits<double>::max();
            int minIndex = -1;
            
            for (int i = 1; i <= extrema.NbExtrema(); ++i) {
                try {
                    double dist = extrema.Distance(i);
                    if (dist < minDist) {
                        minDist = dist;
                        minIndex = i;
                    }
                }
                catch (const Standard_OutOfRange&) {
                    // Index out of range in extrema results, skip this extremum
                    continue;
                }
                catch (...) {
                    continue;
                }
            }
            
            if (minIndex > 0 && minDist < tolerance) {
                try {
                    gp_Pnt p1, p2;
                    extrema.Points(minIndex, p1, p2);
                    
                    // Use midpoint as intersection
                    intersection.SetX((p1.X() + p2.X()) / 2.0);
                    intersection.SetY((p1.Y() + p2.Y()) / 2.0);
                    intersection.SetZ((p1.Z() + p2.Z()) / 2.0);
                    
                    return true;
                }
                catch (const Standard_OutOfRange&) {
                    // Points() call failed - index might be invalid
                    return false;
                }
            }
        }
    }
    catch (const Standard_OutOfRange& e) {
        // Curve parameter out of range - log for debugging
        LOG_DBG_S("EdgeIntersectionAccelerator: Standard_OutOfRange in extrema computation");
        return false;
    }
    catch (const Standard_Failure& e) {
        // Other OpenCASCADE failures - this is normal for some edge pairs
        return false;
    }
    catch (...) {
        // Unknown exception - log and return false
        LOG_WRN_S("EdgeIntersectionAccelerator: Unknown exception in computeEdgeIntersection");
        return false;
    }
    
    return false;
}

std::vector<size_t> EdgeIntersectionAccelerator::queryIntersectingEdges(
    size_t edgeIndex) const {
    
    if (edgeIndex >= m_edges.size()) {
        return {};
    }
    
    const auto& edge = m_edges[edgeIndex];
    std::vector<size_t> primitiveIndices;
    
    // Use BVH to efficiently query edges whose bounding boxes intersect
    if (m_bvh && m_bvh->isBuilt()) {
        // Query BVH with this edge's bounding box - O(log n) complexity
        m_bvh->queryBoundingBox(edge.bounds, primitiveIndices);
        
        // Filter out self-intersection
        primitiveIndices.erase(
            std::remove(primitiveIndices.begin(), primitiveIndices.end(), edgeIndex),
            primitiveIndices.end()
        );
    } else {
        // Fallback: test all edges (only if BVH not built)
        for (size_t i = 0; i < m_edges.size(); ++i) {
            if (i == edgeIndex) continue;
            
            // Check bounding box intersection using OpenCASCADE API
            if (!edge.bounds.IsOut(m_edges[i].bounds)) {
                primitiveIndices.push_back(i);
            }
        }
    }
    
    return primitiveIndices;
}

void EdgeIntersectionAccelerator::clear() {
    m_edges.clear();
    m_bvh->clear();
    m_stats = Statistics();
}

// ============================================================================
// Advanced Algorithm Implementations
// ============================================================================

/**
 * @brief Spatial hashing for fast duplicate detection
 * 
 * Uses spatial hashing to achieve O(1) average duplicate checking
 * instead of O(n) linear search.
 */
class SpatialHashDeduplicator {
private:
    std::unordered_map<size_t, std::vector<gp_Pnt>> m_hashTable;
    double m_cellSize;
    
    size_t hashPosition(const gp_Pnt& p) const {
        int64_t x = static_cast<int64_t>(std::floor(p.X() / m_cellSize));
        int64_t y = static_cast<int64_t>(std::floor(p.Y() / m_cellSize));
        int64_t z = static_cast<int64_t>(std::floor(p.Z() / m_cellSize));
        
        // Spatial hash function (prime numbers for better distribution)
        return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
    }
    
public:
    explicit SpatialHashDeduplicator(double tolerance) 
        : m_cellSize(tolerance * 2.0) {}
    
    /**
     * @brief Check if point already exists, if not add it
     * @return true if point is unique (was added)
     */
    bool addUnique(const gp_Pnt& point, double tolerance) {
        size_t hash = hashPosition(point);
        
        auto& cell = m_hashTable[hash];
        
        // Check existing points in this cell
        for (const auto& existing : cell) {
            if (point.Distance(existing) < tolerance) {
                return false;  // Duplicate found
            }
        }
        
        // Also check adjacent cells (safety margin)
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    if (dx == 0 && dy == 0 && dz == 0) continue;
                    
                    gp_Pnt offsetPoint(
                        point.X() + dx * m_cellSize,
                        point.Y() + dy * m_cellSize,
                        point.Z() + dz * m_cellSize
                    );
                    size_t adjHash = hashPosition(offsetPoint);
                    
                    auto it = m_hashTable.find(adjHash);
                    if (it != m_hashTable.end()) {
                        for (const auto& existing : it->second) {
                            if (point.Distance(existing) < tolerance) {
                                return false;  // Duplicate in adjacent cell
                            }
                        }
                    }
                }
            }
        }
        
        // Not a duplicate, add to cell
        cell.push_back(point);
        return true;
    }
};

/**
 * @brief Batch processing optimization for edge pairs
 * 
 * Groups edge pairs into batches for better cache locality
 * and reduced overhead.
 */
std::vector<gp_Pnt> extractIntersectionsBatched(
    const std::vector<EdgeIntersectionAccelerator::EdgePrimitive>& edges,
    const std::vector<EdgeIntersectionAccelerator::EdgePair>& pairs,
    double tolerance,
    size_t batchSize = 1000) {
    
    std::vector<gp_Pnt> intersections;
    intersections.reserve(pairs.size() / 10);  // Heuristic
    
    for (size_t batchStart = 0; batchStart < pairs.size(); batchStart += batchSize) {
        size_t batchEnd = std::min(batchStart + batchSize, pairs.size());
        
        // Process batch
        for (size_t i = batchStart; i < batchEnd; ++i) {
            const auto& pair = pairs[i];
            
            try {
                GeomAPI_ExtremaCurveCurve extrema(
                    edges[pair.edge1Index].curve,
                    edges[pair.edge2Index].curve,
                    edges[pair.edge1Index].first,
                    edges[pair.edge1Index].last,
                    edges[pair.edge2Index].first,
                    edges[pair.edge2Index].last
                );
                
                if (extrema.NbExtrema() > 0) {
                    double minDist = std::numeric_limits<double>::max();
                    int minIndex = -1;
                    
                    for (int j = 1; j <= extrema.NbExtrema(); ++j) {
                        double dist = extrema.Distance(j);
                        if (dist < minDist) {
                            minDist = dist;
                            minIndex = j;
                        }
                    }
                    
                    if (minIndex > 0 && minDist < tolerance) {
                        gp_Pnt p1, p2;
                        extrema.Points(minIndex, p1, p2);
                        
                        gp_Pnt intersection(
                            (p1.X() + p2.X()) / 2.0,
                            (p1.Y() + p2.Y()) / 2.0,
                            (p1.Z() + p2.Z()) / 2.0
                        );
                        
                        intersections.push_back(intersection);
                    }
                }
            }
            catch (const Standard_Failure&) {
                // Geometric calculation failed, skip this pair
                continue;
            }
        }
    }
    
    return intersections;
}

