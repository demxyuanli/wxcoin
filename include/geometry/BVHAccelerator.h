#pragma once

#include <vector>
#include <memory>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "logger/Logger.h"

/**
 * @brief Bounding Volume Hierarchy (BVH) Accelerator for fast intersection testing
 *
 * BVH provides O(log n) intersection queries for large geometric datasets.
 * This implementation uses axis-aligned bounding boxes (AABB) and
 * Surface Area Heuristic (SAH) for optimal tree construction.
 */
class BVHAccelerator {
public:
    /**
     * @brief BVH node types
     */
    enum class NodeType {
        Internal,  // Internal node with children
        Leaf       // Leaf node containing primitives
    };

    /**
     * @brief BVH tree node
     */
    struct BVHNode {
        NodeType type;
        Bnd_Box bounds;                    // Axis-aligned bounding box
        std::unique_ptr<BVHNode> left;     // Left child (for internal nodes)
        std::unique_ptr<BVHNode> right;    // Right child (for internal nodes)
        std::vector<size_t> primitives;    // Primitive indices (for leaf nodes)

        BVHNode() : type(NodeType::Leaf) {}
        BVHNode(NodeType t) : type(t) {}
    };

    /**
     * @brief Primitive information for BVH construction
     */
    struct Primitive {
        Bnd_Box bounds;          // Bounding box of the primitive
        gp_Pnt centroid;         // Centroid for splitting decisions
        size_t index;            // Original index in the shape array
    };

    /**
     * @brief Intersection result
     */
    struct IntersectionResult {
        bool hit = false;                    // Whether intersection occurred
        double distance = std::numeric_limits<double>::max();  // Distance to intersection
        size_t primitiveIndex = SIZE_MAX;    // Index of intersected primitive
        gp_Pnt intersectionPoint;            // Intersection point
    };

    BVHAccelerator();
    ~BVHAccelerator();

    /**
     * @brief Build BVH for a set of shapes
     * @param shapes Vector of shapes to build BVH for
     * @param maxPrimitivesPerLeaf Maximum primitives per leaf node
     * @return True if build successful
     */
    bool build(const std::vector<TopoDS_Shape>& shapes, size_t maxPrimitivesPerLeaf = 4);

    /**
     * @brief Build BVH for triangle mesh
     * @param vertices Triangle vertices
     * @param indices Triangle indices
     * @param maxPrimitivesPerLeaf Maximum triangles per leaf node
     * @return True if build successful
     */
    bool buildFromMesh(const std::vector<gp_Pnt>& vertices,
                      const std::vector<int>& indices,
                      size_t maxPrimitivesPerLeaf = 4);

    /**
     * @brief Test ray intersection with BVH
     * @param rayOrigin Ray origin point
     * @param rayDirection Ray direction (should be normalized)
     * @param result Intersection result (output)
     * @return True if intersection found
     */
    bool intersectRay(const gp_Pnt& rayOrigin, const gp_Vec& rayDirection, IntersectionResult& result) const;

    /**
     * @brief Test point intersection (point-in-shape test)
     * @param point Test point
     * @param result Intersection result (output)
     * @return True if point intersects any shape
     */
    bool intersectPoint(const gp_Pnt& point, IntersectionResult& result) const;

    /**
     * @brief Query all primitives whose bounding boxes intersect with given box
     * @param queryBox Bounding box to query
     * @param results Output vector of primitive indices
     * @return Number of primitives found
     */
    size_t queryBoundingBox(const Bnd_Box& queryBox, std::vector<size_t>& results) const;

    /**
     * @brief Get bounding box of the entire BVH
     * @return World bounding box
     */
    const Bnd_Box& getBounds() const { return m_worldBounds; }

    /**
     * @brief Get number of nodes in BVH
     * @return Total node count
     */
    size_t getNodeCount() const;

    /**
     * @brief Get memory usage of BVH
     * @return Memory usage in bytes
     */
    size_t getMemoryUsage() const;

    /**
     * @brief Clear BVH data
     */
    void clear();

    /**
     * @brief Check if BVH is built
     * @return True if BVH is ready for queries
     */
    bool isBuilt() const { return m_root != nullptr; }

private:
    std::unique_ptr<BVHNode> m_root;           // Root of BVH tree
    std::vector<Primitive> m_primitives;       // Primitive data
    std::vector<TopoDS_Shape> m_shapes;        // Original shapes
    Bnd_Box m_worldBounds;                     // World bounding box

    // BVH construction parameters
    size_t m_maxPrimitivesPerLeaf;

    // Helper methods for BVH construction
    void buildRecursive(BVHNode* node, const std::vector<size_t>& primitiveIndices);
    void computePrimitiveBounds(const std::vector<TopoDS_Shape>& shapes);
    void computePrimitiveBoundsFromMesh(const std::vector<gp_Pnt>& vertices, const std::vector<int>& indices);

    // SAH (Surface Area Heuristic) splitting
    float evaluateSAH(const Bnd_Box& box, int axis, float splitPos,
                     const std::vector<size_t>& leftIndices,
                     const std::vector<size_t>& rightIndices) const;
    void findBestSplit(const std::vector<size_t>& primitiveIndices,
                      int& bestAxis, float& bestSplitPos,
                      std::vector<size_t>& leftIndices,
                      std::vector<size_t>& rightIndices);

    // Intersection testing
    bool intersectRayRecursive(const BVHNode* node, const gp_Pnt& rayOrigin,
                              const gp_Vec& rayDirection, IntersectionResult& result) const;
    bool intersectBoxRay(const Bnd_Box& box, const gp_Pnt& rayOrigin,
                        const gp_Vec& rayDirection, double& tmin, double& tmax) const;
    bool intersectPrimitiveRay(size_t primitiveIndex, const gp_Pnt& rayOrigin,
                               const gp_Vec& rayDirection, IntersectionResult& result) const;

    bool intersectPointRecursive(const BVHNode* node, const gp_Pnt& point, IntersectionResult& result) const;
    bool intersectBoxPoint(const Bnd_Box& box, const gp_Pnt& point) const;
    bool intersectPrimitivePoint(size_t primitiveIndex, const gp_Pnt& point, IntersectionResult& result) const;

    // Bounding box query
    void queryBoundingBoxRecursive(const BVHNode* node, const Bnd_Box& queryBox, std::vector<size_t>& results) const;
    bool intersectBoxBox(const Bnd_Box& box1, const Bnd_Box& box2) const;

    // Utility methods
    Bnd_Box computeBounds(const std::vector<size_t>& primitiveIndices) const;
    gp_Pnt computeCentroid(const std::vector<size_t>& primitiveIndices) const;
    size_t countNodes(const BVHNode* node) const;
    size_t estimateMemoryUsage(const BVHNode* node) const;

    // Constants
    static constexpr float TRAVERSAL_COST = 1.0f;
    static constexpr float INTERSECTION_COST = 2.0f;
};

// Utility functions
Bnd_Box computeShapeBounds(const TopoDS_Shape& shape);
bool boxContainsPoint(const Bnd_Box& box, const gp_Pnt& point);
double boxSurfaceArea(const Bnd_Box& box);

