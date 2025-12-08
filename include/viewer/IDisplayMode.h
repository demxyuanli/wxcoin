#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "config/RenderingConfig.h"
#include "rendering/GeometryProcessor.h"
#include "geometry/GeometryRenderContext.h"
#include <memory>

// Forward declarations
class ModularEdgeComponent;
class VertexExtractor;

/**
 * @brief Interface for display mode implementations
 * 
 * Each display mode (Points, Wireframe, FlatLines, Shaded) implements this interface
 * to provide its own rendering logic.
 */
class IDisplayMode {
public:
    virtual ~IDisplayMode() = default;

    /**
     * @brief Get the display mode type this implementation represents
     */
    virtual RenderingConfig::DisplayMode getModeType() const = 0;

    /**
     * @brief Build the Coin3D scene graph node for this display mode
     * @param shape The CAD shape to render
     * @param params Mesh generation parameters
     * @param context Rendering context (material, transform, etc.)
     * @param modularEdgeComponent Edge component for edge extraction (can be nullptr)
     * @param vertexExtractor Vertex extractor for point view (can be nullptr)
     * @return SoSeparator node containing the mode-specific rendering
     */
    virtual SoSeparator* buildModeNode(
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const GeometryRenderContext& context,
        ModularEdgeComponent* modularEdgeComponent,
        VertexExtractor* vertexExtractor) = 0;

    /**
     * @brief Get the SoSwitch child index for this mode
     * FreeCAD-style: 0=Points, 1=Wireframe, 2=FlatLines, 3=Shaded
     */
    virtual int getSwitchChildIndex() const = 0;

    /**
     * @brief Check if this mode requires face rendering
     */
    virtual bool requiresFaces() const = 0;

    /**
     * @brief Check if this mode requires edge rendering
     */
    virtual bool requiresEdges() const = 0;
};

