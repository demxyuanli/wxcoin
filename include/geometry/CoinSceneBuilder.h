#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/RenderingConfig.h"

class ObjectDisplayModeManager;
class ModularEdgeComponent;
class VertexExtractor;
class FaceDomainManager;
class TriangleMappingManager;
struct MeshParameters;
struct GeometryRenderContext;

/**
 * @brief Builds Coin3D scene graph for geometry rendering
 * 
 * This class is responsible for constructing the Coin3D scene graph structure,
 * including transform nodes, shape hints, and mode switching via SoSwitch.
 * It coordinates with ObjectDisplayModeManager to build mode-specific nodes.
 */
class CoinSceneBuilder {
public:
    CoinSceneBuilder();
    ~CoinSceneBuilder();

    /**
     * @brief Build complete scene graph for a geometry
     * @param shape The OpenCASCADE shape to render
     * @param params Mesh generation parameters
     * @param context Rendering context (materials, display settings, etc.)
     * @param modeManager Object-level display mode manager
     * @param edgeComponent Edge component for edge rendering
     * @param vertexExtractor Vertex extractor for point view
     * @param faceDomainManager Face domain manager (optional, for face mapping)
     * @param triangleMappingManager Triangle mapping manager (optional, for triangle mapping)
     * @return Root SoSeparator node containing the complete scene graph
     */
    SoSeparator* buildSceneGraph(
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const GeometryRenderContext& context,
        ObjectDisplayModeManager* modeManager,
        ModularEdgeComponent* edgeComponent,
        VertexExtractor* vertexExtractor,
        FaceDomainManager* faceDomainManager = nullptr,
        TriangleMappingManager* triangleMappingManager = nullptr);

    /**
     * @brief Update display mode by switching SoSwitch whichChild
     * @param modeSwitch The SoSwitch node to update
     * @param mode The display mode to switch to
     * @param modeManager Object-level display mode manager
     */
    void updateDisplayMode(
        SoSwitch* modeSwitch,
        RenderingConfig::DisplayMode mode,
        ObjectDisplayModeManager* modeManager);

    /**
     * @brief Update wireframe material color
     * @param coinNode Root Coin3D node
     * @param color New wireframe color
     */
    void updateWireframeMaterial(SoSeparator* coinNode, const Quantity_Color& color);

    /**
     * @brief Update material in a mode node for special display modes
     * @param modeNode The mode separator node
     * @param mode The display mode to apply
     */
    void updateMaterialInModeNode(SoSeparator* modeNode, RenderingConfig::DisplayMode mode);

    /**
     * @brief Create root separator node with proper caching settings
     * @return New SoSeparator node with caching disabled
     */
    static SoSeparator* createRootNode();

    /**
     * @brief Add transform and shape hints to root node
     * @param root Root separator node
     * @param context Rendering context containing transform and display settings
     */
    static void addTransformAndHints(SoSeparator* root, const GeometryRenderContext& context);

private:
    /**
     * @brief Setup edge display for non-wireframe modes
     */
    void setupEdgeDisplay(
        SoSwitch* modeSwitch,
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const GeometryRenderContext& context,
        ModularEdgeComponent* edgeComponent);
};

