#pragma once

#include "../IDisplayMode.h"

/**
 * @brief FlatLines display mode implementation
 * 
 * Displays faces with wireframe overlay (SolidWireframe/HiddenLine modes).
 */
class FlatLinesMode : public IDisplayMode {
public:
    FlatLinesMode();
    ~FlatLinesMode() override = default;

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



