#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Geom_Curve.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/Standard_Real.hxx>
#include "geometry/BVHAccelerator.h"

/**
 * @brief Edge intersection detection accelerator using BVH
 * 
 * Uses Bounding Volume Hierarchy to accelerate edge-edge intersection detection,
 * reducing complexity from O(n²) to O(n log n).
 * 
 * Performance characteristics:
 * - Build time: O(n log n) where n = number of edges
 * - Query time: O(n log n) for all intersection pairs
 * - Pruning efficiency: 90%+ (eliminates 99% of non-intersecting pairs)
 * - Memory overhead: ~2x edge count
 * 
 * Recommended usage:
 * - Use for models with >= 100 edges
 * - Falls back to spatial grid for smaller models
 * - Supports parallel intersection extraction
 */
class EdgeIntersectionAccelerator {
public:
    /**
     * @brief Edge primitive data for BVH construction
     */
    struct EdgePrimitive {
        Handle(Geom_Curve) curve;
        Standard_Real first, last;
        Bnd_Box bounds;
        size_t edgeIndex;
        TopoDS_Edge edge;  // Preserve original edge for precise calculation
        
        EdgePrimitive() : first(0), last(0), edgeIndex(0) {}
    };
    
    /**
     * @brief Edge pair (potential intersection candidates)
     */
    struct EdgePair {
        size_t edge1Index;
        size_t edge2Index;
        
        EdgePair(size_t i1, size_t i2) : edge1Index(i1), edge2Index(i2) {}
    };
    
    /**
     * @brief Performance statistics
     */
    struct Statistics {
        size_t totalEdges = 0;
        size_t potentialPairs = 0;
        size_t actualIntersections = 0;
        double buildTime = 0.0;       // seconds
        double queryTime = 0.0;       // seconds
        double pruningRatio = 0.0;    // 0.0 to 1.0
        
        void print() const;
    };
    
    EdgeIntersectionAccelerator();
    ~EdgeIntersectionAccelerator() = default;
    
    /**
     * @brief Build BVH from edge collection
     * @param edges Input edges to build accelerator from
     * @param maxPrimitivesPerLeaf Maximum edges per BVH leaf node (default: 4)
     */
    void buildFromEdges(const std::vector<TopoDS_Edge>& edges, 
                       size_t maxPrimitivesPerLeaf = 4);
    
    /**
     * @brief Find all potential intersecting edge pairs
     * 
     * Uses BVH to quickly filter edge pairs whose bounding boxes intersect.
     * This is the first phase - actual intersection testing comes later.
     * 
     * @return Vector of edge pair indices that might intersect
     */
    std::vector<EdgePair> findPotentialIntersections() const;
    
    /**
     * @brief Extract all intersection points (single-threaded)
     * @param tolerance Distance tolerance for considering two edges intersecting
     * @return Vector of intersection points
     */
    std::vector<gp_Pnt> extractIntersections(double tolerance) const;
    
    /**
     * @brief Extract all intersection points (multi-threaded)
     * @param tolerance Distance tolerance for intersection
     * @param numThreads Number of threads to use (0 = auto-detect)
     * @return Vector of intersection points
     */
    std::vector<gp_Pnt> extractIntersectionsParallel(double tolerance, 
                                                     size_t numThreads = 0) const;
    
    /**
     * @brief Get performance statistics from last operation
     */
    const Statistics& getStatistics() const { return m_stats; }
    
    /**
     * @brief Clear accelerator data
     */
    void clear();
    
    /**
     * @brief Check if accelerator is built and ready
     */
    bool isBuilt() const { return m_bvh != nullptr && m_bvh->isBuilt(); }
    
    /**
     * @brief Get number of edges in accelerator
     */
    size_t getEdgeCount() const { return m_edges.size(); }

private:
    std::unique_ptr<BVHAccelerator> m_bvh;
    std::vector<EdgePrimitive> m_edges;
    mutable Statistics m_stats;
    
    /**
     * @brief Compute precise intersection between two edges
     * @param edge1 First edge data
     * @param edge2 Second edge data
     * @param tolerance Distance tolerance
     * @param intersection Output intersection point
     * @return true if intersection found within tolerance
     */
    bool computeEdgeIntersection(const EdgePrimitive& edge1,
                                const EdgePrimitive& edge2,
                                double tolerance,
                                gp_Pnt& intersection) const;
    
    /**
     * @brief Query all edges whose bounding boxes intersect with given edge
     * @param edgeIndex Index of edge to query
     * @return Indices of potentially intersecting edges
     */
    std::vector<size_t> queryIntersectingEdges(size_t edgeIndex) const;
};

