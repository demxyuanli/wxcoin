#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <Inventor/SbVec3f.h>

/**
 * @brief Triangle definition (equivalent to FreeCAD's Facet)
 */
struct MeshTriangle {
    uint32_t I1, I2, I3;  // Vertex indices

    MeshTriangle(uint32_t i1 = 0, uint32_t i2 = 0, uint32_t i3 = 0)
        : I1(i1), I2(i2), I3(i3) {}
};

/**
 * @brief Face domain structure - independent mesh container for each geometry face
 * Similar to FreeCAD's Domain structure but adapted for wxcoin architecture
 */
struct FaceDomain {
    int geometryFaceId;              // Index of the face in the original geometry
    std::vector<gp_Pnt> points;      // Vertices specific to this face (using OpenCASCADE gp_Pnt)
    std::vector<MeshTriangle> triangles;   // Triangles specific to this face
    bool isValid;                    // Whether this face was successfully triangulated

    FaceDomain(int faceId = -1)
        : geometryFaceId(faceId), isValid(false) {}

    bool isEmpty() const {
        return points.empty() || triangles.empty();
    }

    std::size_t getTriangleCount() const {
        return triangles.size();
    }

    std::size_t getVertexCount() const {
        return points.size();
    }

    // Convert to Coin3D compatible format
    void toCoin3DFormat(std::vector<SbVec3f>& vertices, std::vector<int>& indices) const;
};

/**
 * @brief Triangle segment defining the triangles belonging to a face
 * Can handle both contiguous and non-contiguous triangle indices
 * Similar to FreeCAD's segment management but more flexible
 */
struct TriangleSegment {
    int geometryFaceId;              // Which face this segment belongs to
    std::vector<int> triangleIndices; // Actual triangle indices (supports non-contiguous)

    TriangleSegment(int faceId = -1) : geometryFaceId(faceId) {}
    TriangleSegment(int faceId, const std::vector<int>& indices)
        : geometryFaceId(faceId), triangleIndices(indices) {}

    std::size_t getTriangleCount() const {
        return triangleIndices.size();
    }

    bool isEmpty() const {
        return triangleIndices.empty();
    }

    bool contains(int triangleIndex) const {
        return std::find(triangleIndices.begin(), triangleIndices.end(), triangleIndex) != triangleIndices.end();
    }
};

/**
 * @brief Boundary triangle information for triangles shared by multiple faces
 */
struct BoundaryTriangle {
    int triangleIndex;           // Global triangle index
    std::vector<int> faceIds;    // All faces that contain this triangle
    bool isBoundary;             // Whether this is a true boundary triangle

    BoundaryTriangle(int triIdx = -1)
        : triangleIndex(triIdx), isBoundary(false) {}
};

/**
 * @brief Edge index mapping structure for Coin3D line to geometry edge mapping
 */
struct EdgeIndexMapping {
    int geometryEdgeId;  // Index of the edge in the original geometry (from TopExp_Explorer)
    std::vector<int> lineIndices;  // Indices of lines in Coin3D mesh that belong to this edge

    EdgeIndexMapping(int edgeId = -1) : geometryEdgeId(edgeId) {}
};

/**
 * @brief Vertex index mapping structure for Coin3D point to geometry vertex mapping
 */
struct VertexIndexMapping {
    int geometryVertexId;  // Index of the vertex in the original geometry (from TopExp_Explorer)
    int coordinateIndex;   // Index of the coordinate in Coin3D mesh that represents this vertex

    VertexIndexMapping(int vertexId = -1, int coordIdx = -1)
        : geometryVertexId(vertexId), coordinateIndex(coordIdx) {}
};


