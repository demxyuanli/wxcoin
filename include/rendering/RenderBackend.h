#pragma once

#include <memory>
#include <string>
#include <vector>
#include "GeometryProcessor.h"
#include <OpenCASCADE/Quantity_Color.hxx>

// Include Coin3D headers for complete type definitions
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoMaterial.h>

// Custom deleter for SoSeparator (uses unref() instead of delete)
struct SoSeparatorDeleter {
    void operator()(SoSeparator* ptr) const {
        if (ptr) {
            ptr->unref();
        }
    }
};

// Type alias for unique_ptr with custom deleter
using SoSeparatorPtr = std::unique_ptr<SoSeparator, SoSeparatorDeleter>;

/**
 * @brief Rendering backend interface
 */
class RenderBackend {
public:
    virtual ~RenderBackend() = default;

    /**
     * @brief Initialize the rendering backend
     * @param config Configuration parameters
     * @return true if initialization successful
     */
    virtual bool initialize(const std::string& config = "") = 0;

    /**
     * @brief Shutdown the rendering backend
     */
    virtual void shutdown() = 0;



    /**
     * @brief Create scene node from mesh with custom material
     * @param mesh Input mesh
     * @param selected Selection state
     * @param diffuseColor Diffuse color
     * @param ambientColor Ambient color
     * @param specularColor Specular color
     * @param emissiveColor Emissive color
     * @param shininess Material shininess
     * @param transparency Material transparency
     * @return Scene node
     */
    virtual SoSeparatorPtr createSceneNode(const TriangleMesh& mesh, bool selected,
                                         const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
                                         const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
                                         double shininess, double transparency) = 0;

    /**
     * @brief Update existing scene node
     * @param node Scene node to update
     * @param mesh New mesh data
     */
    virtual void updateSceneNode(SoSeparator* node, const TriangleMesh& mesh) = 0;

    /**
     * @brief Update existing scene node from shape
     * @param node Scene node to update
     * @param shape Input shape
     * @param params Meshing parameters
     */
    virtual void updateSceneNode(SoSeparator* node, const TopoDS_Shape& shape, const MeshParameters& params) = 0;

    /**
     * @brief Create scene node from shape
     * @param shape Input shape
     * @param params Meshing parameters
     * @param selected Selection state
     * @return Scene node
     */
    virtual SoSeparatorPtr createSceneNode(const TopoDS_Shape& shape, 
                                          const MeshParameters& params = MeshParameters(),
                                          bool selected = false) = 0;

    /**
     * @brief Create scene node from shape with custom material
     * @param shape Input shape
     * @param params Meshing parameters
     * @param selected Selection state
     * @param diffuseColor Diffuse color
     * @param ambientColor Ambient color
     * @param specularColor Specular color
     * @param emissiveColor Emissive color
     * @param shininess Material shininess
     * @param transparency Material transparency
     * @return Scene node
     */
    virtual SoSeparatorPtr createSceneNode(const TopoDS_Shape& shape, 
                                          const MeshParameters& params,
                                          bool selected,
                                          const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
                                          const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
                                          double shininess, double transparency) = 0;

    /**
     * @brief Set edge display settings
     * @param show Show edges flag
     * @param angle Feature edge angle threshold
     */
    virtual void setEdgeSettings(bool show, double angle = 45.0) = 0;

    /**
     * @brief Set smoothing settings
     * @param enabled Enable smoothing
     * @param creaseAngle Crease angle threshold
     * @param iterations Smoothing iterations
     */
    virtual void setSmoothingSettings(bool enabled, double creaseAngle = 30.0, int iterations = 2) = 0;

    /**
     * @brief Set subdivision settings
     * @param enabled Enable subdivision
     * @param levels Subdivision levels
     */
    virtual void setSubdivisionSettings(bool enabled, int levels = 2) = 0;

    /**
     * @brief Get backend name
     * @return Backend identifier
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Check if backend is available
     * @return true if backend can be used
     */
    virtual bool isAvailable() const = 0;
};

/**
 * @brief Coin3D specific backend interface
 */
class Coin3DBackend : public RenderBackend {
public:


    /**
     * @brief Create Coin3D separator node with custom material
     * @param mesh Input mesh
     * @param selected Selection state
     * @param diffuseColor Diffuse color
     * @param ambientColor Ambient color
     * @param specularColor Specular color
     * @param emissiveColor Emissive color
     * @param shininess Material shininess
     * @param transparency Material transparency
     * @return SoSeparator node
     */
    virtual SoSeparator* createCoinNode(const TriangleMesh& mesh, bool selected,
                                      const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
                                      const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
                                      double shininess, double transparency) = 0;

    /**
     * @brief Update Coin3D separator node
     * @param node SoSeparator node to update
     * @param mesh New mesh data
     */
    virtual void updateCoinNode(SoSeparator* node, const TriangleMesh& mesh) = 0;

    /**
     * @brief Create coordinate node
     * @param mesh Input mesh
     * @return SoCoordinate3 node
     */
    virtual SoCoordinate3* createCoordinateNode(const TriangleMesh& mesh) = 0;

    /**
     * @brief Create face set node
     * @param mesh Input mesh
     * @return SoIndexedFaceSet node
     */
    virtual SoIndexedFaceSet* createFaceSetNode(const TriangleMesh& mesh) = 0;

    /**
     * @brief Create normal node
     * @param mesh Input mesh
     * @return SoNormal node
     */
    virtual SoNormal* createNormalNode(const TriangleMesh& mesh) = 0;

    /**
     * @brief Create edge set node
     * @param mesh Input mesh
     * @return SoIndexedLineSet node
     */
    virtual SoIndexedLineSet* createEdgeSetNode(const TriangleMesh& mesh) = 0;
}; 