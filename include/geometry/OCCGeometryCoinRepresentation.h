#pragma once

#include <memory>
#include <vector>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/Poly_Triangulation.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include "rendering/GeometryProcessor.h"
#include "EdgeTypes.h"
#include "geometry/GeometryRenderContext.h"

// Forward declarations
class SoSeparator;
class SoSwitch;
class EdgeComponent;
class ModularEdgeComponent;
class TopoDS_Shape;

// Helper classes
namespace helper {
    class CoinNodeManager;
    class RenderNodeBuilder;
    class DisplayModeHandler;
    class WireframeBuilder;
    class PointViewBuilder;
    class FaceDomainMapper;
}


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

/**
 * @brief Coin3D representation builder and manager for OpenCASCADE geometry
 * 
 * Manages Coin3D scene graph representation, rendering, display modes,
 * face domain mapping, and all Coin3D-related functionality
 */
class GeomCoinRepresentation {
public:
    GeomCoinRepresentation();
    virtual ~GeomCoinRepresentation();

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

    // NEW: Independent vertex extractor for point view (separated from edges)
    std::unique_ptr<class VertexExtractor> m_vertexExtractor;

    void setEdgeDisplayType(EdgeType type, bool show);
    bool isEdgeDisplayTypeEnabled(EdgeType type) const;
    void updateEdgeDisplay();
    bool hasOriginalEdges() const;

    // Modular edge component methods
    void enableModularEdgeComponent(bool enable);
    bool isUsingModularEdgeComponent() const { return useModularEdgeComponent; }

    // NEW: Independent vertex extractor for point view
    class VertexExtractor* getVertexExtractor() { return m_vertexExtractor.get(); }
    const class VertexExtractor* getVertexExtractor() const { return m_vertexExtractor.get(); }

    // Assembly level for hierarchical explode
    int getAssemblyLevel() const { return m_assemblyLevel; }
    void setAssemblyLevel(int level) { m_assemblyLevel = level; }

    // New Domain-based face mapping system
    const std::vector<FaceDomain>& getFaceDomains() const { return m_faceDomains; }
    const std::vector<TriangleSegment>& getTriangleSegments() const { return m_triangleSegments; }
    const std::vector<BoundaryTriangle>& getBoundaryTriangles() const { return m_boundaryTriangles; }

    // Query methods for new Domain-based system
    const FaceDomain* getFaceDomain(int geometryFaceId) const;
    const TriangleSegment* getTriangleSegment(int geometryFaceId) const;
    bool isBoundaryTriangle(int triangleIndex) const;
    const BoundaryTriangle* getBoundaryTriangle(int triangleIndex) const;

    // Enhanced face-triangle mapping with boundary triangle awareness
    int getGeometryFaceIdForTriangle(int triangleIndex) const;
    std::vector<int> getGeometryFaceIdsForTriangle(int triangleIndex) const;
    std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const;
    bool hasFaceDomainMapping() const { return !m_faceDomains.empty(); }

    // Legacy compatibility method - now delegates to domain system
    bool hasFaceIndexMapping() const { return hasFaceDomainMapping(); }





    // Point view rendering
    void createPointViewRepresentation(const TopoDS_Shape& shape, const MeshParameters& params,
                                      const ::DisplaySettings& displaySettings);


protected:
    // Helper function for face triangulation
    bool triangulateFace(const TopoDS_Face& face, FaceDomain& domain);

    // Domain-based mapping system helper functions
    void buildFaceDomains(const TopoDS_Shape& shape,
                         const std::vector<TopoDS_Face>& faces,
                         const MeshParameters& params);
    void buildTriangleSegments(const std::vector<std::pair<int, std::vector<int>>>& faceMappings);
    void identifyBoundaryTriangles(const std::vector<std::pair<int, std::vector<int>>>& faceMappings);

    // Protected helper for wireframe generation
    void createWireframeRepresentation(const TopoDS_Shape& shape, const MeshParameters& params);

    // Wireframe appearance update
    void updateWireframeMaterial(const Quantity_Color& color);
    
    // Fast display mode update without mesh rebuild
    void updateDisplayMode(RenderingConfig::DisplayMode mode);

    // Memory optimization
    void releaseTemporaryData();
    void optimizeMemory();

    // Coin3D scene graph
    SoSeparator* m_coinNode;
    SoSwitch* m_modeSwitch;  // SoSwitch for fast mode switching (FreeCAD-style)
    bool m_coinNeedsUpdate;
    bool m_meshRegenerationNeeded;
    MeshParameters m_lastMeshParams;
    int m_assemblyLevel;

    // New Domain-based mapping system (FreeCAD-inspired)
    std::vector<FaceDomain> m_faceDomains;              // Independent mesh containers per face
    std::vector<TriangleSegment> m_triangleSegments;    // Triangle index ranges per face
    std::vector<BoundaryTriangle> m_boundaryTriangles;  // Triangles shared by multiple faces

    // Helper classes for modular architecture
    std::unique_ptr<helper::CoinNodeManager> m_nodeManager;
    std::unique_ptr<helper::RenderNodeBuilder> m_renderBuilder;
    std::unique_ptr<helper::DisplayModeHandler> m_displayHandler;
    std::unique_ptr<helper::WireframeBuilder> m_wireframeBuilder;
    std::unique_ptr<helper::PointViewBuilder> m_pointViewBuilder;
    std::unique_ptr<helper::FaceDomainMapper> m_faceMapper;
};
