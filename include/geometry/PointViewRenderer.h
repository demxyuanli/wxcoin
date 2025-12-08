#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "geometry/GeometryRenderContext.h"

class VertexExtractor;
struct MeshParameters;

/**
 * @brief Renders point view representation of geometry
 * 
 * This class is responsible for creating Coin3D nodes that display
 * geometry vertices as points, with support for different point shapes
 * (square, circle, triangle).
 */
class PointViewRenderer {
public:
    PointViewRenderer();
    ~PointViewRenderer();

    /**
     * @brief Create point view representation node
     * @param shape The OpenCASCADE shape to render
     * @param params Mesh generation parameters
     * @param displaySettings Display settings (color, size, shape)
     * @param vertexExtractor Optional vertex extractor (if null, will generate mesh)
     * @return SoSeparator node containing point view representation
     */
    SoSeparator* createPointViewNode(
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const DisplaySettings& displaySettings,
        VertexExtractor* vertexExtractor = nullptr);

private:
    SoSeparator* createSquarePoints(
        const std::vector<gp_Pnt>& vertices,
        const DisplaySettings& displaySettings);

    SoSeparator* createCirclePoints(
        const std::vector<gp_Pnt>& vertices,
        const DisplaySettings& displaySettings);

    SoSeparator* createTrianglePoints(
        const std::vector<gp_Pnt>& vertices,
        const DisplaySettings& displaySettings);
};

