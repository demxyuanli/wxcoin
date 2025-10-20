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
        prim.edgeIndex = i;
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
    
    LOG_INF_S("EdgeIntersectionAccelerator: Built from " + 
              std::to_string(m_edges.size()) + " edges in " + 
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
    
    // Thread-safe result collection using mutex
    // For better performance, could use lock-free structures or thread-local storage
    std::vector<gp_Pnt> allIntersections;
    std::mutex resultMutex;
    
    // Parallel processing using std::for_each with execution policy
    // Note: This requires C++17 and may need TBB or other parallel STL implementation
    try {
        std::for_each(std::execution::par_unseq,
            potentialPairs.begin(), potentialPairs.end(),
            [&](const EdgePair& pair) {
                gp_Pnt intersection;
                if (computeEdgeIntersection(m_edges[pair.edge1Index],
                                           m_edges[pair.edge2Index],
                                           tolerance,
                                           intersection)) {
                    std::lock_guard<std::mutex> lock(resultMutex);
                    allIntersections.push_back(intersection);
                }
            });
    }
    catch (...) {
        // Fallback to sequential if parallel execution fails
        LOG_WRN_S("EdgeIntersectionAccelerator: Parallel execution failed, falling back to sequential");
        return extractIntersections(tolerance);
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
                double dist = extrema.Distance(i);
                if (dist < minDist) {
                    minDist = dist;
                    minIndex = i;
                }
            }
            
            if (minIndex > 0 && minDist < tolerance) {
                gp_Pnt p1, p2;
                extrema.Points(minIndex, p1, p2);
                
                // Use midpoint as intersection
                intersection.SetX((p1.X() + p2.X()) / 2.0);
                intersection.SetY((p1.Y() + p2.Y()) / 2.0);
                intersection.SetZ((p1.Z() + p2.Z()) / 2.0);
                
                return true;
            }
        }
    }
    catch (const Standard_Failure&) {
        // Silent failure - some edge pairs cannot be computed
        // This is normal and expected
    }
    
    return false;
}

std::vector<size_t> EdgeIntersectionAccelerator::queryIntersectingEdges(
    size_t edgeIndex) const {
    
    if (edgeIndex >= m_edges.size()) {
        return {};
    }
    
    const auto& edge = m_edges[edgeIndex];
    std::vector<size_t> results;
    
    // Query BVH: find all edges whose bounding boxes intersect with this edge
    // Note: This is a simplified implementation
    // For full BVH query support, BVHAccelerator would need to expose
    // a bounding box query interface
    
    // Current fallback: test all edges (can be optimized with BVH box query)
    for (size_t i = 0; i < m_edges.size(); ++i) {
        if (i == edgeIndex) continue;
        
        // Check bounding box intersection
        if (!edge.bounds.IsOut(m_edges[i].bounds)) {
            results.push_back(i);
        }
    }
    
    return results;
}

void EdgeIntersectionAccelerator::clear() {
    m_edges.clear();
    m_bvh->clear();
    m_stats = Statistics();
}

