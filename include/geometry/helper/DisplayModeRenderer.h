#pragma once

#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/GeometryRenderContext.h"
#include "config/RenderingConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <functional>

class ModularEdgeComponent;
class RenderNodeBuilder;
class WireframeBuilder;
class PointViewBuilder;
class TopoDS_Shape;
struct MeshParameters;
struct TriangleMesh;

class DisplayModeRenderer {
public:
    DisplayModeRenderer() = default;
    ~DisplayModeRenderer() = default;

    void setGeometryBuiltCallback(std::function<void(bool)> callback) {
        m_geometryBuiltCallback = callback;
    }

    void applyRenderState(SoSeparator* coinNode,
                         const DisplayModeRenderState& state,
                         const GeometryRenderContext& context,
                         const TopoDS_Shape& shape,
                         const MeshParameters& params,
                         ModularEdgeComponent* edgeComponent,
                         bool useModularEdgeComponent,
                         RenderNodeBuilder* renderBuilder,
                         WireframeBuilder* wireframeBuilder,
                         PointViewBuilder* pointViewBuilder = nullptr);

    void applyRenderState(SoSeparator* coinNode,
                         const DisplayModeRenderState& state,
                         const GeometryRenderContext& context,
                         const TriangleMesh& mesh,
                         const MeshParameters& params,
                         ModularEdgeComponent* edgeComponent,
                         bool useModularEdgeComponent,
                         RenderNodeBuilder* renderBuilder,
                         WireframeBuilder* wireframeBuilder,
                         PointViewBuilder* pointViewBuilder = nullptr);

    void buildModeStateNode(SoSeparator* parent,
                           RenderingConfig::DisplayMode mode,
                           const DisplayModeRenderState& state,
                           const GeometryRenderContext& context,
                           RenderNodeBuilder* renderBuilder);

    void buildModeNode(SoSeparator* parent,
                      RenderingConfig::DisplayMode mode,
                      const GeometryRenderContext& context,
                      const TopoDS_Shape& shape,
                      const MeshParameters& params,
                      ModularEdgeComponent* edgeComponent,
                      bool useModularEdgeComponent,
                      RenderNodeBuilder* renderBuilder,
                      WireframeBuilder* wireframeBuilder,
                      PointViewBuilder* pointViewBuilder = nullptr);

private:
    std::function<void(bool)> m_geometryBuiltCallback;
};

