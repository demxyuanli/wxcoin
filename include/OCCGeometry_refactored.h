#pragma once

#include "geometry/OCCGeometryCore.h"
#include "geometry/OCCGeometryTransform.h"
#include "geometry/OCCGeometryAppearance.h"
#include "geometry/OCCGeometryMaterial.h"
#include "geometry/OCCGeometryDisplay.h"
#include "geometry/OCCGeometryQuality.h"
#include "geometry/OCCGeometryMesh.h"
#include "GeometryDialogTypes.h"
#include <string>
#include <memory>

/**
 * @brief Unified geometry class combining all geometry modules
 * 
 * This class inherits from all geometry modules to provide a complete
 * geometry management system with modular responsibilities.
 */
class OCCGeometry 
    : public OCCGeometryCore
    , public OCCGeometryTransform
    , public OCCGeometryAppearance
    , public OCCGeometryMaterial
    , public OCCGeometryDisplay
    , public OCCGeometryQuality
    , public OCCGeometryMesh
{
public:
    OCCGeometry(const std::string& name);
    virtual ~OCCGeometry();

    // Override setShape to propagate to all modules
    virtual void setShape(const TopoDS_Shape& shape) override;

    // Override color setter to sync with material
    virtual void setColor(const Quantity_Color& color) override;

    // Override transparency to sync across modules
    virtual void setTransparency(double transparency) override;

    // Coin3D integration - delegated to OCCGeometryMesh
    using OCCGeometryMesh::getCoinNode;
    using OCCGeometryMesh::setCoinNode;
    using OCCGeometryMesh::needsMeshRegeneration;
    using OCCGeometryMesh::setMeshRegenerationNeeded;
    
    // Override buildCoinRepresentation with implementations that use internal shape
    void buildCoinRepresentation(const MeshParameters& params = MeshParameters());
    void buildCoinRepresentation(const MeshParameters& params,
        const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
        const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
        double shininess, double transparency);
    
    void forceCoinRepresentationRebuild(const MeshParameters& params) {
        if (!getShape().IsNull()) {
            OCCGeometryMesh::forceCoinRepresentationRebuild(getShape(), params);
        }
    }

    // Wrapper methods that pass shape from Core module
    void updateCoinRepresentationIfNeeded(const MeshParameters& params);
    SoSeparator* getCoinNodeWithShape();
    
    // Convenience method: regenerateMesh using internal shape
    void regenerateMesh(const MeshParameters& params) {
        if (!getShape().IsNull()) {
            OCCGeometryMesh::regenerateMesh(getShape(), params);
        }
    }

    // Apply advanced parameters from VisualSettingsDialog
    virtual void applyAdvancedParameters(const AdvancedGeometryParameters& params);

    // Update geometry from RenderingConfig settings
    virtual void updateFromRenderingConfig();

    // Force texture update
    using OCCGeometryAppearance::forceTextureUpdate;

    // Face visibility control for edges-only mode
    void setFaceDisplay(bool enable);
    using OCCGeometryDisplay::setFacesVisible;

    // Wireframe overlay
    using OCCGeometryDisplay::setWireframeOverlay;

    // Edge display methods
    using OCCGeometryMesh::hasOriginalEdges;
    using OCCGeometryDisplay::setEdgeDisplay;
    using OCCGeometryDisplay::setFeatureEdgeDisplay;
    using OCCGeometryDisplay::setNormalDisplay;

    // Display mode helpers
    using OCCGeometryDisplay::setShowWireframe;
    using OCCGeometryDisplay::isShowWireframe;

    // Edge component integration
    using OCCGeometryMesh::edgeComponent;
    using OCCGeometryMesh::setEdgeDisplayType;
    using OCCGeometryMesh::isEdgeDisplayTypeEnabled;
    using OCCGeometryMesh::updateEdgeDisplay;

    // Face index mapping
    using OCCGeometryMesh::getFaceIndexMappings;
    using OCCGeometryMesh::setFaceIndexMappings;
    using OCCGeometryMesh::getGeometryFaceIdForTriangle;
    using OCCGeometryMesh::getTrianglesForGeometryFace;
    using OCCGeometryMesh::hasFaceIndexMapping;
    void buildFaceIndexMapping(const MeshParameters& params = MeshParameters());

    // Assembly level
    using OCCGeometryMesh::getAssemblyLevel;
    using OCCGeometryMesh::setAssemblyLevel;

    // LOD support
    using OCCGeometryQuality::addLODLevel;
    using OCCGeometryQuality::getLODLevel;

    // Memory optimization
    using OCCGeometryMesh::releaseTemporaryData;
    using OCCGeometryMesh::optimizeMemory;

private:
    // Helper methods
    void createWireframeRepresentation(const TopoDS_Shape& shape, const MeshParameters& params);

    // Subdivision settings (legacy compatibility)
    bool m_subdivisionEnabled{false};
    int m_subdivisionLevels{2};
};

/**
 * @brief OpenCASCADE box geometry
 */
class OCCBox : public OCCGeometry {
public:
    OCCBox(const std::string& name, double width, double height, double depth);

    void setDimensions(double width, double height, double depth);
    void getSize(double& width, double& height, double& depth) const;

private:
    void buildShape();

    double m_width, m_height, m_depth;
};

/**
 * @brief OpenCASCADE cylinder geometry
 */
class OCCCylinder : public OCCGeometry {
public:
    OCCCylinder(const std::string& name, double radius, double height);

    void setDimensions(double radius, double height);
    void getSize(double& radius, double& height) const;

private:
    void buildShape();

    double m_radius, m_height;
};

/**
 * @brief OpenCASCADE sphere geometry
 */
class OCCSphere : public OCCGeometry {
public:
    OCCSphere(const std::string& name, double radius);

    void setRadius(double radius);
    double getRadius() const { return m_radius; }

private:
    void buildShape();

    double m_radius;
};

/**
 * @brief OpenCASCADE cone geometry
 */
class OCCCone : public OCCGeometry {
public:
    OCCCone(const std::string& name, double bottomRadius, double topRadius, double height);

    void setDimensions(double bottomRadius, double topRadius, double height);
    void getSize(double& bottomRadius, double& topRadius, double& height) const;

private:
    void buildShape();

    double m_bottomRadius, m_topRadius, m_height;
};

/**
 * @brief OpenCASCADE torus geometry
 */
class OCCTorus : public OCCGeometry {
public:
    OCCTorus(const std::string& name, double majorRadius, double minorRadius);

    void setDimensions(double majorRadius, double minorRadius);
    void getSize(double& majorRadius, double& minorRadius) const;

private:
    void buildShape();

    double m_majorRadius, m_minorRadius;
};

/**
 * @brief OpenCASCADE truncated cylinder geometry (frustum)
 */
class OCCTruncatedCylinder : public OCCGeometry {
public:
    OCCTruncatedCylinder(const std::string& name, double bottomRadius, double topRadius, double height);

    void setDimensions(double bottomRadius, double topRadius, double height);
    void getSize(double& bottomRadius, double& topRadius, double& height) const;

private:
    void buildShape();

    double m_bottomRadius, m_topRadius, m_height;
};

