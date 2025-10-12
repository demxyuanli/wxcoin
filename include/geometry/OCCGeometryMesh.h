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
    void setFaceIndexMappings(const std::vector<FaceIndexMapping>& mappings) { m_faceIndexMappings = mappings; }

    // Query methods for face index mapping
    int getGeometryFaceIdForTriangle(int triangleIndex) const;
    std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const;
    bool hasFaceIndexMapping() const { return !m_faceIndexMappings.empty(); }

    // Build face index mapping during mesh generation
    void buildFaceIndexMapping(const TopoDS_Shape& shape, const MeshParameters& params = MeshParameters());

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
};
