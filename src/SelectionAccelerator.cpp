#include "SelectionAccelerator.h"
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <algorithm>
#include <sstream>
#include <iomanip>

SelectionAccelerator::SelectionAccelerator()
    : m_selectionMode(SelectionMode::Shapes)
    , m_bvh(std::make_unique<BVHAccelerator>())
{
    LOG_INF_S("SelectionAccelerator initialized");
}

SelectionAccelerator::~SelectionAccelerator()
{
    clear();
}

bool SelectionAccelerator::build(const std::vector<TopoDS_Shape>& shapes, SelectionMode mode)
{
    LOG_INF_S("Building selection accelerator for " + std::to_string(shapes.size()) +
              " shapes, mode: " + std::to_string(static_cast<int>(mode)));

    clear();
    m_shapes = shapes;
    m_selectionMode = mode;

    if (shapes.empty()) {
        LOG_WRN_S("No shapes provided for selection acceleration");
        return false;
    }

    try {
        switch (mode) {
            case SelectionMode::Shapes:
                buildForShapes(shapes);
                break;
            case SelectionMode::Faces:
                buildForFaces(shapes);
                break;
            case SelectionMode::Edges:
                buildForEdges(shapes);
                break;
            case SelectionMode::Vertices:
                buildForVertices(shapes);
                break;
        }

        LOG_INF_S("Selection accelerator built successfully");
        LOG_INF_S("Performance stats: " + getPerformanceStats());

        return true;
    } catch (const std::exception& e) {
        LOG_ERR_S("Failed to build selection accelerator: " + std::string(e.what()));
        return false;
    }
}

void SelectionAccelerator::buildForShapes(const std::vector<TopoDS_Shape>& shapes)
{
    if (!m_bvh->build(shapes)) {
        throw std::runtime_error("Failed to build BVH for shapes");
    }
}

void SelectionAccelerator::buildForFaces(const std::vector<TopoDS_Shape>& shapes)
{
    std::vector<TopoDS_Shape> allFaces;
    for (const auto& shape : shapes) {
        auto faces = extractFaces(shape);
        allFaces.insert(allFaces.end(), faces.begin(), faces.end());
    }

    if (!m_bvh->build(allFaces)) {
        throw std::runtime_error("Failed to build BVH for faces");
    }

    // Store face-to-shape mapping
    m_shapes = allFaces;  // For face selection, shapes become faces
}

void SelectionAccelerator::buildForEdges(const std::vector<TopoDS_Shape>& shapes)
{
    std::vector<TopoDS_Shape> allEdges;
    for (const auto& shape : shapes) {
        auto edges = extractEdges(shape);
        allEdges.insert(allEdges.end(), edges.begin(), edges.end());
    }

    if (!m_bvh->build(allEdges)) {
        throw std::runtime_error("Failed to build BVH for edges");
    }

    m_shapes = allEdges;
}

void SelectionAccelerator::buildForVertices(const std::vector<TopoDS_Shape>& shapes)
{
    std::vector<TopoDS_Shape> allVertices;
    for (const auto& shape : shapes) {
        auto vertices = extractVertices(shape);
        allVertices.insert(allVertices.end(), vertices.begin(), vertices.end());
    }

    if (!m_bvh->build(allVertices)) {
        throw std::runtime_error("Failed to build BVH for vertices");
    }

    m_shapes = allVertices;
}

std::vector<TopoDS_Shape> SelectionAccelerator::extractFaces(const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> faces;
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        faces.push_back(exp.Current());
    }
    return faces;
}

std::vector<TopoDS_Shape> SelectionAccelerator::extractEdges(const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> edges;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        edges.push_back(exp.Current());
    }
    return edges;
}

std::vector<TopoDS_Shape> SelectionAccelerator::extractVertices(const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Shape> vertices;
    for (TopExp_Explorer exp(shape, TopAbs_VERTEX); exp.More(); exp.Next()) {
        vertices.push_back(exp.Current());
    }
    return vertices;
}

bool SelectionAccelerator::selectByRay(const gp_Pnt& rayOrigin, const gp_Vec& rayDirection,
                                      SelectionResult& result)
{
    if (!isReady()) {
        LOG_WRN_S("Selection accelerator not ready");
        return false;
    }

    m_rayTestsPerformed++;

    BVHAccelerator::IntersectionResult bvhResult;
    if (m_bvh->intersectRay(rayOrigin, rayDirection, bvhResult)) {
        result.found = true;
        result.shapeIndex = bvhResult.primitiveIndex;
        result.selectedShape = m_shapes[bvhResult.primitiveIndex];
        result.intersectionPoint = bvhResult.intersectionPoint;
        result.distance = bvhResult.distance;

        m_selectionsFound++;
        return true;
    }

    return false;
}

bool SelectionAccelerator::selectByPoint(const gp_Pnt& point, SelectionResult& result)
{
    if (!isReady()) {
        LOG_WRN_S("Selection accelerator not ready");
        return false;
    }

    m_pointTestsPerformed++;

    BVHAccelerator::IntersectionResult bvhResult;
    if (m_bvh->intersectPoint(point, bvhResult)) {
        result.found = true;
        result.shapeIndex = bvhResult.primitiveIndex;
        result.selectedShape = m_shapes[bvhResult.primitiveIndex];
        result.intersectionPoint = bvhResult.intersectionPoint;
        result.distance = bvhResult.distance;

        m_selectionsFound++;
        return true;
    }

    return false;
}

size_t SelectionAccelerator::selectByRectangle(const gp_Pnt& rectMin, const gp_Pnt& rectMax,
                                             const std::vector<double>& viewMatrix,
                                             const std::vector<double>& projectionMatrix,
                                             const std::vector<int>& viewport,
                                             std::vector<SelectionResult>& results)
{
    if (!isReady()) {
        LOG_WRN_S("Selection accelerator not ready");
        return 0;
    }

    results.clear();
    size_t selectionCount = 0;

    // For rectangle selection, we need to test each shape against the rectangle
    // This is a simplified implementation - in practice, you'd want to use
    // more sophisticated frustum culling or screen-space bounds testing

    for (size_t i = 0; i < m_shapes.size(); ++i) {
        const auto& shape = m_shapes[i];

        // Get shape bounds
        Bnd_Box bbox;
        BRepBndLib::Add(shape, bbox);

        if (bbox.IsVoid()) continue;

        // Convert bounding box corners to screen space and check if any
        // part intersects with the selection rectangle
        // This is a simplified check - real implementation would be more sophisticated

        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

        // Test a few representative points
        std::vector<gp_Pnt> testPoints = {
            gp_Pnt(xmin, ymin, zmin),
            gp_Pnt(xmax, ymax, zmax),
            gp_Pnt((xmin+xmax)/2, (ymin+ymax)/2, (zmin+zmax)/2)
        };

        bool shapeSelected = false;
        for (const auto& point : testPoints) {
            // Convert to screen space (simplified - real implementation needs proper transformation)
            // For now, just do a simple bounds check
            if (pointInRectangle(point, rectMin, rectMax)) {
                SelectionResult result;
                result.found = true;
                result.shapeIndex = i;
                result.selectedShape = shape;
                result.intersectionPoint = point;
                results.push_back(result);
                selectionCount++;
                shapeSelected = true;
                break;
            }
        }
    }

    LOG_INF_S("Rectangle selection found " + std::to_string(selectionCount) + " items");
    return selectionCount;
}

bool SelectionAccelerator::pointInRectangle(const gp_Pnt& point, const gp_Pnt& rectMin,
                                          const gp_Pnt& rectMax)
{
    // Simplified 2D rectangle test (assuming XY plane)
    // Real implementation would need proper screen space conversion
    return point.X() >= rectMin.X() && point.X() <= rectMax.X() &&
           point.Y() >= rectMin.Y() && point.Y() <= rectMax.Y();
}

bool SelectionAccelerator::setSelectionMode(SelectionMode mode)
{
    if (mode == m_selectionMode) {
        return false;  // No change needed
    }

    LOG_INF_S("Changing selection mode from " + std::to_string(static_cast<int>(m_selectionMode)) +
              " to " + std::to_string(static_cast<int>(mode)));

    m_selectionMode = mode;
    // Note: Changing mode requires rebuilding the accelerator
    return true;
}

std::string SelectionAccelerator::getPerformanceStats() const
{
    std::stringstream ss;
    ss << "Selection Accelerator Stats:\n";
    ss << "  Mode: " << static_cast<int>(m_selectionMode) << "\n";
    ss << "  Shapes: " << m_shapes.size() << "\n";
    ss << "  BVH Nodes: " << (m_bvh ? m_bvh->getNodeCount() : 0) << "\n";
    ss << "  Memory: " << (m_bvh ? m_bvh->getMemoryUsage() / 1024 : 0) << " KB\n";
    ss << "  Ray tests: " << m_rayTestsPerformed << "\n";
    ss << "  Point tests: " << m_pointTestsPerformed << "\n";
    ss << "  Selections: " << m_selectionsFound << "\n";

    if (m_rayTestsPerformed > 0) {
        double hitRate = static_cast<double>(m_selectionsFound) / m_rayTestsPerformed * 100.0;
        ss << "  Hit rate: " << std::fixed << std::setprecision(1) << hitRate << "%\n";
    }

    return ss.str();
}

void SelectionAccelerator::clear()
{
    m_bvh->clear();
    m_shapes.clear();
    m_rayTestsPerformed = 0;
    m_pointTestsPerformed = 0;
    m_selectionsFound = 0;

    LOG_INF_S("SelectionAccelerator cleared");
}

// Utility functions
gp_Vec normalizeVector(const gp_Vec& vec)
{
    double mag = vectorMagnitude(vec);
    if (mag > 1e-10) {
        return gp_Vec(vec.X() / mag, vec.Y() / mag, vec.Z() / mag);
    }
    return gp_Vec(0, 0, 0);
}

double vectorMagnitude(const gp_Vec& vec)
{
    return std::sqrt(vec.X() * vec.X() + vec.Y() * vec.Y() + vec.Z() * vec.Z());
}

gp_Vec crossProduct(const gp_Vec& a, const gp_Vec& b)
{
    return gp_Vec(
        a.Y() * b.Z() - a.Z() * b.Y(),
        a.Z() * b.X() - a.X() * b.Z(),
        a.X() * b.Y() - a.Y() * b.X()
    );
}
