#pragma once

#include "../IDisplayMode.h"

/**
 * @brief Wireframe display mode implementation
 * 
 * Displays original edges and feature edges from CAD geometry, no faces.
 */
class WireframeMode : public IDisplayMode {
public:
    WireframeMode();
    ~WireframeMode() override = default;

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



