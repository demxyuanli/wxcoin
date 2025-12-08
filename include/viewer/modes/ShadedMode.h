#pragma once

#include "../IDisplayMode.h"

/**
 * @brief Shaded display mode implementation
 * 
 * Displays faces with shading (Solid/NoShading modes).
 */
class ShadedMode : public IDisplayMode {
public:
    ShadedMode();
    ~ShadedMode() override = default;

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



