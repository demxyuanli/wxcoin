#include "geometry/TriangleMappingManager.h"
#include "geometry/FaceDomainTypes.h"  // For TriangleSegment, BoundaryTriangle structs
#include "logger/Logger.h"
#include <algorithm>

TriangleMappingManager::TriangleMappingManager()
{
}

TriangleMappingManager::~TriangleMappingManager()
{
}

void TriangleMappingManager::buildTriangleSegments(const std::vector<std::pair<int, std::vector<int>>>& faceMappings)
{
    // Build TriangleSegment for each face using actual triangle indices
    // This supports non-contiguous triangle indices unlike the old range-based approach
    m_triangleSegments.clear();
    m_triangleSegments.reserve(faceMappings.size());

    for (const auto& [faceId, triangleIndices] : faceMappings) {
        TriangleSegment segment(faceId, triangleIndices);
        m_triangleSegments.push_back(std::move(segment));
    }
}

void TriangleMappingManager::identifyBoundaryTriangles(const std::vector<std::pair<int, std::vector<int>>>& faceMappings)
{
    // Track which triangles are used by which faces
    std::map<int, std::vector<int>> triangleToFacesMap;

    for (const auto& [faceId, triangleIndices] : faceMappings) {
        for (int triangleIndex : triangleIndices) {
            triangleToFacesMap[triangleIndex].push_back(faceId);
        }
    }

    // Identify boundary triangles (those shared by multiple faces)
    m_boundaryTriangles.clear();
    for (const auto& [triangleIndex, faceIds] : triangleToFacesMap) {
        if (faceIds.size() > 1) {
            BoundaryTriangle boundaryTri(triangleIndex);
            boundaryTri.faceIds = faceIds;
            boundaryTri.isBoundary = true;
            m_boundaryTriangles.push_back(boundaryTri);
        }
    }
}

const TriangleSegment* TriangleMappingManager::getTriangleSegment(int geometryFaceId) const
{
    for (const auto& segment : m_triangleSegments) {
        if (segment.geometryFaceId == geometryFaceId) {
            return &segment;
        }
    }
    return nullptr;
}

bool TriangleMappingManager::isBoundaryTriangle(int triangleIndex) const
{
    for (const auto& boundaryTri : m_boundaryTriangles) {
        if (boundaryTri.triangleIndex == triangleIndex) {
            return boundaryTri.isBoundary;
        }
    }
    return false;
}

const BoundaryTriangle* TriangleMappingManager::getBoundaryTriangle(int triangleIndex) const
{
    for (const auto& boundaryTri : m_boundaryTriangles) {
        if (boundaryTri.triangleIndex == triangleIndex) {
            return &boundaryTri;
        }
    }
    return nullptr;
}

int TriangleMappingManager::getGeometryFaceIdForTriangle(int triangleIndex) const
{
    // Search through triangle segments to find which face contains this triangle
    for (const auto& segment : m_triangleSegments) {
        if (segment.contains(triangleIndex)) {
            return segment.geometryFaceId;
        }
    }
    return -1;
}

std::vector<int> TriangleMappingManager::getGeometryFaceIdsForTriangle(int triangleIndex) const
{
    // Check if it's a boundary triangle first
    const BoundaryTriangle* boundaryTri = getBoundaryTriangle(triangleIndex);
    if (boundaryTri) {
        return boundaryTri->faceIds;
    }

    // Otherwise, return single face if found
    int faceId = getGeometryFaceIdForTriangle(triangleIndex);
    if (faceId >= 0) {
        return {faceId};
    }
    return {};
}

std::vector<int> TriangleMappingManager::getTrianglesForGeometryFace(int geometryFaceId) const
{
    // Deprecated: Now we use FaceDomain directly instead of triangle indices
    // This method is kept for backward compatibility but should not be used
    LOG_WRN_S("getTrianglesForGeometryFace is deprecated - use getFaceDomain instead for face " + std::to_string(geometryFaceId));
    return {};
}

void TriangleMappingManager::clear()
{
    m_triangleSegments.clear();
    m_boundaryTriangles.clear();
}


