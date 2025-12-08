#pragma once

#include "../IDisplayMode.h"

/**
 * @brief Points display mode implementation
 * 
 * Displays only vertices as points, no faces or edges.
 */
class PointsMode : public IDisplayMode {
public:
    PointsMode();
    ~PointsMode() override = default;

    RenderingConfig::DisplayMode getModeType() const override;
    SoSeparator* buildModeNode(
        const TopoDS_Shape& shape,
        const MeshParameters& params,
        const GeometryRenderContext& context,
        ModularEdgeComponent* modularEdgeComponent,
        VertexExtractor* vertexExtractor) override;
    int getSwitchChildIndex() const override;
    bool requiresFaces() const override;
    bool requiresEdges() const override;
};



