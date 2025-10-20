#include "geometry/BVHAccelerator.h"
#include <BRepBndLib.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <algorithm>
#include <limits>
#include <cmath>

BVHAccelerator::BVHAccelerator()
    : m_maxPrimitivesPerLeaf(4)
{
    LOG_INF_S("BVHAccelerator initialized");
}

BVHAccelerator::~BVHAccelerator()
{
    clear();
}

bool BVHAccelerator::build(const std::vector<TopoDS_Shape>& shapes, size_t maxPrimitivesPerLeaf)
{
    LOG_INF_S("Building BVH for " + std::to_string(shapes.size()) + " shapes");

    clear();
    m_maxPrimitivesPerLeaf = maxPrimitivesPerLeaf;
    m_shapes = shapes;

    if (shapes.empty()) {
        LOG_WRN_S("No shapes provided for BVH construction");
        return false;
    }

    // Compute primitive bounds
    computePrimitiveBounds(shapes);

    if (m_primitives.empty()) {
        LOG_WRN_S("No valid primitives found");
        return false;
    }

    // Initialize primitive indices
    std::vector<size_t> primitiveIndices(m_primitives.size());
    for (size_t i = 0; i < primitiveIndices.size(); ++i) {
        primitiveIndices[i] = i;
    }

    // Build BVH tree
    m_root = std::make_unique<BVHNode>(NodeType::Internal);
    buildRecursive(m_root.get(), primitiveIndices);

    // Compute world bounds
    m_worldBounds = computeBounds(primitiveIndices);

    LOG_INF_S("BVH construction completed:");
    LOG_INF_S("  Total primitives: " + std::to_string(m_primitives.size()));
    LOG_INF_S("  Total nodes: " + std::to_string(getNodeCount()));
    LOG_INF_S("  Memory usage: " + std::to_string(getMemoryUsage() / 1024) + " KB");

    return true;
}

bool BVHAccelerator::buildFromMesh(const std::vector<gp_Pnt>& vertices,
                                  const std::vector<int>& indices,
                                  size_t maxPrimitivesPerLeaf)
{
    LOG_INF_S("Building BVH for triangle mesh with " + std::to_string(indices.size() / 3) + " triangles");

    clear();
    m_maxPrimitivesPerLeaf = maxPrimitivesPerLeaf;

    if (indices.size() % 3 != 0 || indices.empty()) {
        LOG_ERR_S("Invalid triangle mesh data");
        return false;
    }

    // Compute primitive bounds from mesh
    computePrimitiveBoundsFromMesh(vertices, indices);

    if (m_primitives.empty()) {
        LOG_WRN_S("No valid triangles found");
        return false;
    }

    // Initialize primitive indices
    std::vector<size_t> primitiveIndices(m_primitives.size());
    for (size_t i = 0; i < primitiveIndices.size(); ++i) {
        primitiveIndices[i] = i;
    }

    // Build BVH tree
    m_root = std::make_unique<BVHNode>(NodeType::Internal);
    buildRecursive(m_root.get(), primitiveIndices);

    // Compute world bounds
    m_worldBounds = computeBounds(primitiveIndices);

    LOG_INF_S("Triangle mesh BVH construction completed:");
    LOG_INF_S("  Total triangles: " + std::to_string(m_primitives.size()));
    LOG_INF_S("  Total nodes: " + std::to_string(getNodeCount()));
    LOG_INF_S("  Memory usage: " + std::to_string(getMemoryUsage() / 1024) + " KB");

    return true;
}

void BVHAccelerator::computePrimitiveBounds(const std::vector<TopoDS_Shape>& shapes)
{
    m_primitives.clear();
    m_primitives.reserve(shapes.size());

    for (size_t i = 0; i < shapes.size(); ++i) {
        const TopoDS_Shape& shape = shapes[i];

        if (shape.IsNull()) {
            LOG_WRN_S("Null shape encountered at index " + std::to_string(i));
            continue;
        }

        try {
            // Compute bounding box for the shape
            Bnd_Box bbox;
            BRepBndLib::Add(shape, bbox);

            if (bbox.IsVoid()) {
                LOG_WRN_S("Void bounding box for shape at index " + std::to_string(i));
                continue;
            }

            // Compute centroid
            double xmin, ymin, zmin, xmax, ymax, zmax;
            bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
            gp_Pnt centroid((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);

            // Add primitive
            m_primitives.push_back({bbox, centroid, i});

        } catch (const Standard_Failure& e) {
            LOG_ERR_S("Failed to compute bounds for shape " + std::to_string(i) + ": " + e.GetMessageString());
        }
    }

    LOG_INF_S("Computed bounds for " + std::to_string(m_primitives.size()) + " primitives");
}

void BVHAccelerator::computePrimitiveBoundsFromMesh(const std::vector<gp_Pnt>& vertices,
                                                   const std::vector<int>& indices)
{
    m_primitives.clear();
    size_t triangleCount = indices.size() / 3;
    m_primitives.reserve(triangleCount);

    for (size_t i = 0; i < triangleCount; ++i) {
        size_t baseIndex = i * 3;
        int idx0 = indices[baseIndex];
        int idx1 = indices[baseIndex + 1];
        int idx2 = indices[baseIndex + 2];

        if (idx0 < 0 || idx0 >= (int)vertices.size() ||
            idx1 < 0 || idx1 >= (int)vertices.size() ||
            idx2 < 0 || idx2 >= (int)vertices.size()) {
            LOG_WRN_S("Invalid triangle indices at " + std::to_string(i));
            continue;
        }

        const gp_Pnt& v0 = vertices[idx0];
        const gp_Pnt& v1 = vertices[idx1];
        const gp_Pnt& v2 = vertices[idx2];

        // Compute bounding box
        double xmin = std::min({v0.X(), v1.X(), v2.X()});
        double ymin = std::min({v0.Y(), v1.Y(), v2.Y()});
        double zmin = std::min({v0.Z(), v1.Z(), v2.Z()});
        double xmax = std::max({v0.X(), v1.X(), v2.X()});
        double ymax = std::max({v0.Y(), v1.Y(), v2.Y()});
        double zmax = std::max({v0.Z(), v1.Z(), v2.Z()});

        Bnd_Box bbox;
        bbox.Add(gp_Pnt(xmin, ymin, zmin));
        bbox.Add(gp_Pnt(xmax, ymax, zmax));

        // Compute centroid
        gp_Pnt centroid((v0.X() + v1.X() + v2.X()) / 3.0,
                       (v0.Y() + v1.Y() + v2.Y()) / 3.0,
                       (v0.Z() + v1.Z() + v2.Z()) / 3.0);

        m_primitives.push_back({bbox, centroid, i});
    }

    LOG_INF_S("Computed bounds for " + std::to_string(m_primitives.size()) + " triangles");
}

void BVHAccelerator::buildRecursive(BVHNode* node, const std::vector<size_t>& primitiveIndices)
{
    // Compute node bounds
    node->bounds = computeBounds(primitiveIndices);

    // Check if this should be a leaf node
    if (primitiveIndices.size() <= m_maxPrimitivesPerLeaf) {
        node->type = NodeType::Leaf;
        node->primitives = primitiveIndices;
        return;
    }

    // Find best split
    int bestAxis;
    float bestSplitPos;
    std::vector<size_t> leftIndices, rightIndices;

    findBestSplit(primitiveIndices, bestAxis, bestSplitPos, leftIndices, rightIndices);

    // If split failed, make it a leaf
    if (leftIndices.empty() || rightIndices.empty()) {
        node->type = NodeType::Leaf;
        node->primitives = primitiveIndices;
        return;
    }

    // Create child nodes
    node->left = std::make_unique<BVHNode>(NodeType::Internal);
    node->right = std::make_unique<BVHNode>(NodeType::Internal);

    // Recursively build children
    buildRecursive(node->left.get(), leftIndices);
    buildRecursive(node->right.get(), rightIndices);
}

void BVHAccelerator::findBestSplit(const std::vector<size_t>& primitiveIndices,
                                  int& bestAxis, float& bestSplitPos,
                                  std::vector<size_t>& leftIndices,
                                  std::vector<size_t>& rightIndices)
{
    bestAxis = -1;
    bestSplitPos = 0.0f;
    float bestCost = std::numeric_limits<float>::max();

    // Try splitting along each axis
    for (int axis = 0; axis < 3; ++axis) {
        // Sort primitives along this axis
        std::vector<std::pair<float, size_t>> sortedPrimitives;
        for (size_t idx : primitiveIndices) {
            // CRITICAL FIX: Validate primitive index before accessing m_primitives
            if (idx >= m_primitives.size()) {
                LOG_ERR_S("BVHAccelerator: Invalid primitive index " + std::to_string(idx) + 
                          " >= " + std::to_string(m_primitives.size()) + " in findBestSplit");
                continue;
            }
            float pos = (axis == 0) ? m_primitives[idx].centroid.X() :
                        (axis == 1) ? m_primitives[idx].centroid.Y() :
                                      m_primitives[idx].centroid.Z();
            sortedPrimitives.emplace_back(pos, idx);
        }
        std::sort(sortedPrimitives.begin(), sortedPrimitives.end());

        // Try different split positions
        for (size_t i = 1; i < sortedPrimitives.size(); ++i) {
            // Split at the midpoint between centroids
            float splitPos = (sortedPrimitives[i-1].first + sortedPrimitives[i].first) / 2.0f;

            // Partition primitives
            std::vector<size_t> left, right;
            for (const auto& pair : sortedPrimitives) {
                if (pair.first < splitPos) {
                    left.push_back(pair.second);
                } else {
                    right.push_back(pair.second);
                }
            }

            // Skip if one side is empty
            if (left.empty() || right.empty()) {
                continue;
            }

            // Evaluate SAH cost
            Bnd_Box currentBounds = computeBounds(primitiveIndices);
            float cost = evaluateSAH(currentBounds, axis, splitPos, left, right);
            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestSplitPos = splitPos;
                leftIndices = std::move(left);
                rightIndices = std::move(right);
            }
        }
    }
}

float BVHAccelerator::evaluateSAH(const Bnd_Box& box, int axis, float splitPos,
                                 const std::vector<size_t>& leftIndices,
                                 const std::vector<size_t>& rightIndices) const
{
    Bnd_Box leftBox = computeBounds(leftIndices);
    Bnd_Box rightBox = computeBounds(rightIndices);

    float leftArea = boxSurfaceArea(leftBox);
    float rightArea = boxSurfaceArea(rightBox);
    float totalArea = boxSurfaceArea(box);

    // SAH cost: T = C_trav + (A_left/A_total)*C_left + (A_right/A_total)*C_right
    float cost = TRAVERSAL_COST +
                (leftArea / totalArea) * leftIndices.size() * INTERSECTION_COST +
                (rightArea / totalArea) * rightIndices.size() * INTERSECTION_COST;

    return cost;
}

Bnd_Box BVHAccelerator::computeBounds(const std::vector<size_t>& primitiveIndices) const
{
    Bnd_Box box;
    for (size_t idx : primitiveIndices) {
        // CRITICAL FIX: Validate primitive index before accessing m_primitives
        if (idx >= m_primitives.size()) {
            LOG_ERR_S("BVHAccelerator: Invalid primitive index " + std::to_string(idx) + 
                      " >= " + std::to_string(m_primitives.size()) + " in computeBounds");
            continue;
        }
        box.Add(m_primitives[idx].bounds);
    }
    return box;
}

gp_Pnt BVHAccelerator::computeCentroid(const std::vector<size_t>& primitiveIndices) const
{
    if (primitiveIndices.empty()) {
        return gp_Pnt(0, 0, 0);
    }

    double x = 0, y = 0, z = 0;
    for (size_t idx : primitiveIndices) {
        // CRITICAL FIX: Validate primitive index before accessing m_primitives
        if (idx >= m_primitives.size()) {
            LOG_ERR_S("BVHAccelerator: Invalid primitive index " + std::to_string(idx) + 
                      " >= " + std::to_string(m_primitives.size()) + " in computeCentroid");
            continue;
        }
        const gp_Pnt& centroid = m_primitives[idx].centroid;
        x += centroid.X();
        y += centroid.Y();
        z += centroid.Z();
    }

    size_t count = primitiveIndices.size();
    return gp_Pnt(x / count, y / count, z / count);
}

bool BVHAccelerator::intersectRay(const gp_Pnt& rayOrigin, const gp_Vec& rayDirection,
                                 IntersectionResult& result) const
{
    if (!m_root) {
        return false;
    }

    result = IntersectionResult();
    return intersectRayRecursive(m_root.get(), rayOrigin, rayDirection, result);
}

bool BVHAccelerator::intersectRayRecursive(const BVHNode* node, const gp_Pnt& rayOrigin,
                                          const gp_Vec& rayDirection, IntersectionResult& result) const
{
    // Test ray against node bounding box
    double tmin, tmax;
    if (!intersectBoxRay(node->bounds, rayOrigin, rayDirection, tmin, tmax)) {
        return false;
    }

    bool hit = false;

    if (node->type == NodeType::Leaf) {
        // Test ray against all primitives in this leaf
        for (size_t primitiveIdx : node->primitives) {
            if (intersectPrimitiveRay(primitiveIdx, rayOrigin, rayDirection, result)) {
                hit = true;
            }
        }
    } else {
        // Test children
        bool hitLeft = intersectRayRecursive(node->left.get(), rayOrigin, rayDirection, result);
        bool hitRight = intersectRayRecursive(node->right.get(), rayOrigin, rayDirection, result);
        hit = hitLeft || hitRight;
    }

    return hit;
}

bool BVHAccelerator::intersectBoxRay(const Bnd_Box& box, const gp_Pnt& rayOrigin,
                                    const gp_Vec& rayDirection, double& tmin, double& tmax) const
{
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double invDirX = 1.0 / rayDirection.X();
    double invDirY = 1.0 / rayDirection.Y();
    double invDirZ = 1.0 / rayDirection.Z();

    double tx1 = (xmin - rayOrigin.X()) * invDirX;
    double tx2 = (xmax - rayOrigin.X()) * invDirX;
    tmin = std::min(tx1, tx2);
    tmax = std::max(tx1, tx2);

    double ty1 = (ymin - rayOrigin.Y()) * invDirY;
    double ty2 = (ymax - rayOrigin.Y()) * invDirY;
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));

    double tz1 = (zmin - rayOrigin.Z()) * invDirZ;
    double tz2 = (zmax - rayOrigin.Z()) * invDirZ;
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    return tmax >= tmin && tmax >= 0;
}

bool BVHAccelerator::intersectPrimitiveRay(size_t primitiveIndex, const gp_Pnt& rayOrigin,
                                          const gp_Vec& rayDirection, IntersectionResult& result) const
{
    // CRITICAL FIX: Validate primitive index before accessing m_primitives
    if (primitiveIndex >= m_primitives.size()) {
        LOG_ERR_S("BVHAccelerator: Invalid primitive index " + std::to_string(primitiveIndex) + 
                  " >= " + std::to_string(m_primitives.size()) + " in intersectPrimitiveRay");
        return false;
    }
    
    // For now, just test bounding box intersection
    // In a full implementation, this would test actual geometry intersection
    double tmin, tmax;
    if (intersectBoxRay(m_primitives[primitiveIndex].bounds, rayOrigin, rayDirection, tmin, tmax)) {
        if (tmin >= 0 && tmin < result.distance) {
            result.hit = true;
            result.distance = tmin;
            result.primitiveIndex = primitiveIndex;
            // Approximate intersection point
            result.intersectionPoint = gp_Pnt(
                rayOrigin.X() + tmin * rayDirection.X(),
                rayOrigin.Y() + tmin * rayDirection.Y(),
                rayOrigin.Z() + tmin * rayDirection.Z()
            );
            return true;
        }
    }
    return false;
}

bool BVHAccelerator::intersectPoint(const gp_Pnt& point, IntersectionResult& result) const
{
    if (!m_root) {
        return false;
    }

    result = IntersectionResult();
    return intersectPointRecursive(m_root.get(), point, result);
}

bool BVHAccelerator::intersectPointRecursive(const BVHNode* node, const gp_Pnt& point,
                                           IntersectionResult& result) const
{
    // Test point against node bounding box
    if (!intersectBoxPoint(node->bounds, point)) {
        return false;
    }

    bool hit = false;

    if (node->type == NodeType::Leaf) {
        // Test point against all primitives in this leaf
        for (size_t primitiveIdx : node->primitives) {
            if (intersectPrimitivePoint(primitiveIdx, point, result)) {
                hit = true;
            }
        }
    } else {
        // Test children
        bool hitLeft = intersectPointRecursive(node->left.get(), point, result);
        bool hitRight = intersectPointRecursive(node->right.get(), point, result);
        hit = hitLeft || hitRight;
    }

    return hit;
}

bool BVHAccelerator::intersectBoxPoint(const Bnd_Box& box, const gp_Pnt& point) const
{
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    return point.X() >= xmin && point.X() <= xmax &&
           point.Y() >= ymin && point.Y() <= ymax &&
           point.Z() >= zmin && point.Z() <= zmax;
}

bool BVHAccelerator::intersectPrimitivePoint(size_t primitiveIndex, const gp_Pnt& point,
                                            IntersectionResult& result) const
{
    // CRITICAL FIX: Validate primitive index before accessing m_primitives
    if (primitiveIndex >= m_primitives.size()) {
        LOG_ERR_S("BVHAccelerator: Invalid primitive index " + std::to_string(primitiveIndex) + 
                  " >= " + std::to_string(m_primitives.size()) + " in intersectPrimitivePoint");
        return false;
    }
    
    if (intersectBoxPoint(m_primitives[primitiveIndex].bounds, point)) {
        result.hit = true;
        result.distance = 0.0;  // Point intersection has no distance
        result.primitiveIndex = primitiveIndex;
        result.intersectionPoint = point;
        return true;
    }
    return false;
}

size_t BVHAccelerator::getNodeCount() const
{
    return m_root ? countNodes(m_root.get()) : 0;
}

size_t BVHAccelerator::countNodes(const BVHNode* node) const
{
    if (!node) return 0;

    size_t count = 1;  // Count this node
    if (node->type == NodeType::Internal) {
        count += countNodes(node->left.get());
        count += countNodes(node->right.get());
    }
    return count;
}

size_t BVHAccelerator::getMemoryUsage() const
{
    return m_root ? estimateMemoryUsage(m_root.get()) : 0;
}

size_t BVHAccelerator::estimateMemoryUsage(const BVHNode* node) const
{
    if (!node) return 0;

    size_t size = sizeof(BVHNode);
    size += node->primitives.capacity() * sizeof(size_t);

    if (node->type == NodeType::Internal) {
        size += estimateMemoryUsage(node->left.get());
        size += estimateMemoryUsage(node->right.get());
    }

    return size;
}

void BVHAccelerator::clear()
{
    m_root.reset();
    m_primitives.clear();
    m_shapes.clear();
    m_worldBounds.SetVoid();
}

// Utility functions
Bnd_Box computeShapeBounds(const TopoDS_Shape& shape)
{
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    return box;
}

bool boxContainsPoint(const Bnd_Box& box, const gp_Pnt& point)
{
    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    return point.X() >= xmin && point.X() <= xmax &&
           point.Y() >= ymin && point.Y() <= ymax &&
           point.Z() >= zmin && point.Z() <= zmax;
}

double boxSurfaceArea(const Bnd_Box& box)
{
    if (box.IsVoid()) return 0.0;

    double xmin, ymin, zmin, xmax, ymax, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double dx = xmax - xmin;
    double dy = ymax - ymin;
    double dz = zmax - zmin;

    return 2.0 * (dx * dy + dy * dz + dz * dx);
}

// Bounding box query implementation
size_t BVHAccelerator::queryBoundingBox(const Bnd_Box& queryBox, std::vector<size_t>& results) const
{
    results.clear();
    
    if (!m_root) {
        return 0;
    }
    
    queryBoundingBoxRecursive(m_root.get(), queryBox, results);
    return results.size();
}

void BVHAccelerator::queryBoundingBoxRecursive(const BVHNode* node, const Bnd_Box& queryBox, 
                                                std::vector<size_t>& results) const
{
    if (!node) return;
    
    // Test query box against node bounding box
    if (!intersectBoxBox(node->bounds, queryBox)) {
        return;  // No intersection, prune this branch
    }
    
    if (node->type == NodeType::Leaf) {
        // Leaf node: test all primitives
        for (size_t primitiveIdx : node->primitives) {
            // CRITICAL FIX: Validate primitive index before accessing m_primitives
            if (primitiveIdx >= m_primitives.size()) {
                LOG_ERR_S("BVHAccelerator: Invalid primitive index " + std::to_string(primitiveIdx) + 
                          " >= " + std::to_string(m_primitives.size()));
                continue;
            }
            
            if (intersectBoxBox(m_primitives[primitiveIdx].bounds, queryBox)) {
                results.push_back(primitiveIdx);
            }
        }
    } else {
        // Internal node: recursively test children
        queryBoundingBoxRecursive(node->left.get(), queryBox, results);
        queryBoundingBoxRecursive(node->right.get(), queryBox, results);
    }
}

bool BVHAccelerator::intersectBoxBox(const Bnd_Box& box1, const Bnd_Box& box2) const
{
    if (box1.IsVoid() || box2.IsVoid()) {
        return false;
    }
    
    double xmin1, ymin1, zmin1, xmax1, ymax1, zmax1;
    double xmin2, ymin2, zmin2, xmax2, ymax2, zmax2;
    
    box1.Get(xmin1, ymin1, zmin1, xmax1, ymax1, zmax1);
    box2.Get(xmin2, ymin2, zmin2, xmax2, ymax2, zmax2);
    
    // Separating Axis Theorem (SAT) for AABBs
    return !(xmax1 < xmin2 || xmax2 < xmin1 ||
             ymax1 < ymin2 || ymax2 < ymin1 ||
             zmax1 < zmin2 || zmax2 < zmin1);
}
