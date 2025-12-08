#pragma once

#include <vector>
#include <map>

#include "geometry/FaceDomainTypes.h"  // For TriangleSegment, BoundaryTriangle structs
struct BoundaryTriangle;

/**
 * @brief Manages triangle segment mapping and boundary triangle identification
 * 
 * This class is responsible for:
 * - Building triangle segments that map faces to triangle indices
 * - Identifying boundary triangles (shared by multiple faces)
 * - Providing query methods for triangle-to-face and face-to-triangle mappings
 */
class TriangleMappingManager {
public:
    TriangleMappingManager();
    ~TriangleMappingManager();

    /**
     * @brief Build triangle segments from face mappings
     * @param faceMappings Vector of (faceId, triangleIndices) pairs
     */
    void buildTriangleSegments(const std::vector<std::pair<int, std::vector<int>>>& faceMappings);

    /**
     * @brief Identify boundary triangles from face mappings
     * @param faceMappings Vector of (faceId, triangleIndices) pairs
     */
    void identifyBoundaryTriangles(const std::vector<std::pair<int, std::vector<int>>>& faceMappings);

    /**
     * @brief Get triangle segment by geometry face ID
     * @param geometryFaceId The face ID in the original geometry
     * @return Pointer to segment or nullptr if not found
     */
    const TriangleSegment* getTriangleSegment(int geometryFaceId) const;

    /**
     * @brief Check if a triangle is a boundary triangle
     * @param triangleIndex The triangle index to check
     * @return true if the triangle is shared by multiple faces
     */
    bool isBoundaryTriangle(int triangleIndex) const;

    /**
     * @brief Get boundary triangle information
     * @param triangleIndex The triangle index
     * @return Pointer to boundary triangle or nullptr if not found
     */
    const BoundaryTriangle* getBoundaryTriangle(int triangleIndex) const;

    /**
     * @brief Get geometry face ID for a triangle (single face)
     * @param triangleIndex The triangle index
     * @return Face ID or -1 if not found
     */
    int getGeometryFaceIdForTriangle(int triangleIndex) const;

    /**
     * @brief Get all geometry face IDs that contain a triangle
     * @param triangleIndex The triangle index
     * @return Vector of face IDs (may be empty or contain multiple IDs for boundary triangles)
     */
    std::vector<int> getGeometryFaceIdsForTriangle(int triangleIndex) const;

    /**
     * @brief Get all triangle indices for a geometry face
     * @param geometryFaceId The face ID
     * @return Vector of triangle indices (deprecated, kept for compatibility)
     */
    std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const;

    /**
     * @brief Get all triangle segments
     * @return Reference to the triangle segments vector
     */
    const std::vector<TriangleSegment>& getTriangleSegments() const { return m_triangleSegments; }

    /**
     * @brief Get all boundary triangles
     * @return Reference to the boundary triangles vector
     */
    const std::vector<BoundaryTriangle>& getBoundaryTriangles() const { return m_boundaryTriangles; }

    /**
     * @brief Clear all mappings
     */
    void clear();

private:
    std::vector<TriangleSegment> m_triangleSegments;
    std::vector<BoundaryTriangle> m_boundaryTriangles;
};


