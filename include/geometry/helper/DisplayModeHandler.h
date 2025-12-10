#pragma once

#include "config/RenderingConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include "geometry/GeometryRenderContext.h"
#include "EdgeTypes.h"

class SoSeparator;
class SoNode;
class ModularEdgeComponent;
class RenderNodeBuilder;
class WireframeBuilder;
class TopoDS_Shape;
struct MeshParameters;

class DisplayModeHandler {
public:
    DisplayModeHandler();
    ~DisplayModeHandler();
    
    void updateDisplayMode(SoSeparator* coinNode, RenderingConfig::DisplayMode mode,
                          ModularEdgeComponent* edgeComponent);
    
    void handleDisplayMode(SoSeparator* coinNode, 
                           const GeometryRenderContext& context,
                           const TopoDS_Shape& shape,
                           const MeshParameters& params,
                           ModularEdgeComponent* edgeComponent,
                           bool useModularEdgeComponent,
                           RenderNodeBuilder* renderBuilder,
                           WireframeBuilder* wireframeBuilder);

private:
    void findDrawStyleAndMaterial(SoNode* node, SoDrawStyle*& drawStyle, SoMaterial*& material);
    void cleanupEdgeNodes(SoSeparator* coinNode, ModularEdgeComponent* edgeComponent);
};

