#pragma once

#include <memory>
#include <vector>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "rendering/GeometryProcessor.h"
#include "EdgeTypes.h"
#include "geometry/GeometryRenderContext.h"

// Forward declarations
class SoSeparator;
class EdgeComponent;
class ModularEdgeComponent;
class TopoDS_Shape;

/**
 * @brief Face index mapping structure for Coin3D triangle to geometry face mapping
 */
struct FaceIndexMapping {
    int geometryFaceId;  // Index of the face in the original geometry (from TopExp_Explorer)
    std::vector<int> triangleIndices;  // Indices of triangles in Coin3D mesh that belong to this face

    FaceIndexMapping(int faceId = -1) : geometryFaceId(faceId) {}
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

/**
 * @brief Geometry mesh generation and management
 * 
 * Manages Coin3D mesh representation, regeneration, and face index mapping
 */
class OCCGeometryMesh {
public:
    OCCGeometryMesh();
    virtual ~OCCGeometryMesh();

    // Coin3D integration
    SoSeparator* getCoinNode() { return m_coinNode; }
    void setCoinNode(SoSeparator* node);

    // Mesh generation - new modular interface
    void buildCoinRepresentation(
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const GeometryRenderContext& context);
    
    // Legacy interface for backward compatibility
    void regenerateMesh(const TopoDS_Shape& shape, const MeshParameters& params);
    void buildCoinRepresentation(const TopoDS_Shape& shape, const MeshParameters& params = MeshParameters());
    void buildCoinRepresentation(const TopoDS_Shape& shape, const MeshParameters& params,
        const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
        const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
        double shininess, double transparency);

    // Performance optimization
    bool needsMeshRegeneration() const { return m_meshRegenerationNeeded; }
    void setMeshRegenerationNeeded(bool needed) { m_meshRegenerationNeeded = needed; }
    void updateCoinRepresentationIfNeeded(const TopoDS_Shape& shape, const MeshParameters& params);
    void forceCoinRepresentationRebuild(const TopoDS_Shape& shape, const MeshParameters& params);

    // Edge component integration
    std::unique_ptr<ModularEdgeComponent> modularEdgeComponent;  // Modular component

    // Edge component control
    bool useModularEdgeComponent = true;  // Switch between old and new component

    void setEdgeDisplayType(EdgeType type, bool show);
    bool isEdgeDisplayTypeEnabled(EdgeType type) const;
    void updateEdgeDisplay();
    bool hasOriginalEdges() const;

    // Modular edge component methods
    void enableModularEdgeComponent(bool enable);
    bool isUsingModularEdgeComponent() const { return useModularEdgeComponent; }

    // Assembly level for hierarchical explode
    int getAssemblyLevel() const { return m_assemblyLevel; }
    void setAssemblyLevel(int level) { m_assemblyLevel = level; }

    // Face index mapping for Coin3D triangle to geometry face mapping
    const std::vector<FaceIndexMapping>& getFaceIndexMappings() const { return m_faceIndexMappings; }
    void setFaceIndexMappings(const std::vector<FaceIndexMapping>& mappings);

    // Edge index mapping for Coin3D line to geometry edge mapping
    const std::vector<EdgeIndexMapping>& getEdgeIndexMappings() const { return m_edgeIndexMappings; }
    void setEdgeIndexMappings(const std::vector<EdgeIndexMapping>& mappings);

    // Vertex index mapping for Coin3D point to geometry vertex mapping
    const std::vector<VertexIndexMapping>& getVertexIndexMappings() const { return m_vertexIndexMappings; }
    void setVertexIndexMappings(const std::vector<VertexIndexMapping>& mappings);

    // Build reverse mapping for fast lookups
    void buildReverseMapping();

    // Query methods for face index mapping
    int getGeometryFaceIdForTriangle(int triangleIndex) const;
    std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const;
    bool hasFaceIndexMapping() const { return !m_faceIndexMappings.empty(); }

    // Query methods for edge index mapping
    int getGeometryEdgeIdForLine(int lineIndex) const;
    std::vector<int> getLinesForGeometryEdge(int geometryEdgeId) const;
    bool hasEdgeIndexMapping() const { return !m_edgeIndexMappings.empty(); }

    // Query methods for vertex index mapping
    int getGeometryVertexIdForCoordinate(int coordinateIndex) const;
    int getCoordinateForGeometryVertex(int geometryVertexId) const;
    bool hasVertexIndexMapping() const { return !m_vertexIndexMappings.empty(); }

    // Point view rendering
    void createPointViewRepresentation(const TopoDS_Shape& shape, const MeshParameters& params,
                                      const ::DisplaySettings& displaySettings);

    // Build face index mapping during mesh generation
    void buildFaceIndexMapping(const TopoDS_Shape& shape, const MeshParameters& params = MeshParameters());

    // Wireframe appearance update
    void updateWireframeMaterial(const Quantity_Color& color);

    // Memory optimization
    void releaseTemporaryData();
    void optimizeMemory();

protected:
    // Protected helper for wireframe generation
    void createWireframeRepresentation(const TopoDS_Shape& shape, const MeshParameters& params);

    SoSeparator* m_coinNode;
    bool m_coinNeedsUpdate;
    bool m_meshRegenerationNeeded;
    MeshParameters m_lastMeshParams;
    int m_assemblyLevel;
    std::vector<FaceIndexMapping> m_faceIndexMappings;
    std::vector<EdgeIndexMapping> m_edgeIndexMappings;
    std::vector<VertexIndexMapping> m_vertexIndexMappings;

    // Performance optimization: reverse mapping for O(1) lookups
    std::unordered_map<int, int> m_triangleToFaceMap;
    std::unordered_map<int, int> m_lineToEdgeMap;
    std::unordered_map<int, int> m_coordinateToVertexMap;
    bool m_hasReverseMapping = false;
};
