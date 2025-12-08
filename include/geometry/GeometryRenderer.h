#pragma once

#include <memory>
#include <vector>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/Poly_Triangulation.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include "rendering/GeometryProcessor.h"
#include "EdgeTypes.h"
#include "geometry/GeometryRenderContext.h"
#include "geometry/FaceDomainManager.h"
#include "geometry/TriangleMappingManager.h"
#include "geometry/CoinSceneBuilder.h"
#include "geometry/PointViewRenderer.h"

// Forward declarations
class SoSeparator;
class SoSwitch;
class EdgeComponent;
class ModularEdgeComponent;
class TopoDS_Shape;

// Forward declare structs from FaceDomainTypes.h
struct FaceDomain;
struct TriangleSegment;
struct BoundaryTriangle;

/**
 * @brief Geometry renderer - manages Coin3D scene graph for geometry rendering
 * 
 * This class coordinates rendering of OpenCASCADE geometry to Coin3D scene graphs.
 * It delegates specific responsibilities to specialized managers:
 * - FaceDomainManager: Face domain system
 * - TriangleMappingManager: Triangle mapping and boundary identification
 * - CoinSceneBuilder: Coin3D scene graph construction
 * - PointViewRenderer: Point view rendering
 * - ObjectDisplayModeManager: Display mode management
 */
class GeometryRenderer {
public:
    GeometryRenderer();
    virtual ~GeometryRenderer();

    // Coin3D integration
    SoSeparator* getCoinNode() { return m_coinNode; }
    void setCoinNode(SoSeparator* node);

    // Shape management
    const TopoDS_Shape& getShape() const { return m_shape; }
    void setShape(const TopoDS_Shape& shape) { m_shape = shape; }

    // Main rendering interface
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
    std::unique_ptr<ModularEdgeComponent> modularEdgeComponent;

    // Edge component control
    bool useModularEdgeComponent = true;

    // Independent vertex extractor for point view
    std::unique_ptr<class VertexExtractor> m_vertexExtractor;

    void setEdgeDisplayType(EdgeType type, bool show);
    bool isEdgeDisplayTypeEnabled(EdgeType type) const;
    void updateEdgeDisplay();
    bool hasOriginalEdges() const;

    // Modular edge component methods
    void enableModularEdgeComponent(bool enable);
    bool isUsingModularEdgeComponent() const { return useModularEdgeComponent; }

    // Vertex extractor access
    class VertexExtractor* getVertexExtractor() { return m_vertexExtractor.get(); }
    const class VertexExtractor* getVertexExtractor() const { return m_vertexExtractor.get(); }

    // Assembly level for hierarchical explode
    int getAssemblyLevel() const { return m_assemblyLevel; }
    void setAssemblyLevel(int level) { m_assemblyLevel = level; }

    // Face domain system - delegated to FaceDomainManager
    const std::vector<FaceDomain>& getFaceDomains() const;
    const FaceDomain* getFaceDomain(int geometryFaceId) const;
    bool hasFaceDomainMapping() const;

    // Triangle mapping system - delegated to TriangleMappingManager
    const std::vector<TriangleSegment>& getTriangleSegments() const;
    const std::vector<BoundaryTriangle>& getBoundaryTriangles() const;
    const TriangleSegment* getTriangleSegment(int geometryFaceId) const;
    bool isBoundaryTriangle(int triangleIndex) const;
    const BoundaryTriangle* getBoundaryTriangle(int triangleIndex) const;
    int getGeometryFaceIdForTriangle(int triangleIndex) const;
    std::vector<int> getGeometryFaceIdsForTriangle(int triangleIndex) const;
    std::vector<int> getTrianglesForGeometryFace(int geometryFaceId) const;

    // Legacy compatibility method
    bool hasFaceIndexMapping() const { return hasFaceDomainMapping(); }

    // Point view rendering - delegated to PointViewRenderer
    void createPointViewRepresentation(const TopoDS_Shape& shape, const MeshParameters& params,
                                      const DisplaySettings& displaySettings);

    // Fast display mode update without mesh rebuild
    void updateDisplayMode(RenderingConfig::DisplayMode mode);

    // Wireframe appearance update
    void updateWireframeMaterial(const Quantity_Color& color);

    // Memory optimization
    void releaseTemporaryData();
    void optimizeMemory();

protected:
    // Legacy helper - will be removed after migration
    void createWireframeRepresentation(const TopoDS_Shape& shape, const MeshParameters& params);

private:
    // Manager instances
    std::unique_ptr<FaceDomainManager> m_faceDomainManager;
    std::unique_ptr<TriangleMappingManager> m_triangleMappingManager;
    std::unique_ptr<CoinSceneBuilder> m_sceneBuilder;
    std::unique_ptr<PointViewRenderer> m_pointViewRenderer;

    // Coin3D scene graph
    SoSeparator* m_coinNode;
    SoSwitch* m_modeSwitch;
    std::unique_ptr<class ObjectDisplayModeManager> m_objectDisplayModeManager;
    
    // State flags
    bool m_coinNeedsUpdate;
    bool m_meshRegenerationNeeded;
    MeshParameters m_lastMeshParams;
    int m_assemblyLevel;

    // Current shape
    TopoDS_Shape m_shape;
};

