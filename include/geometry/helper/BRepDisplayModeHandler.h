#pragma once

#include "config/RenderingConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include "geometry/GeometryRenderContext.h"
#include "geometry/helper/DisplayModeHandler.h"
#include <OpenCASCADE/TopoDS_Shape.hxx>

class SoSeparator;
class SoSwitch;
class ModularEdgeComponent;
class RenderNodeBuilder;
class WireframeBuilder;
class PointViewBuilder;
struct MeshParameters;

class BRepDisplayModeHandler {
public:
    BRepDisplayModeHandler();
    ~BRepDisplayModeHandler();
    
    void setModeSwitch(SoSwitch* modeSwitch);
    
    void handleDisplayMode(SoSeparator* coinNode, 
                           const GeometryRenderContext& context,
                           const TopoDS_Shape& shape,
                           const MeshParameters& params,
                           ModularEdgeComponent* edgeComponent,
                           bool useModularEdgeComponent,
                           RenderNodeBuilder* renderBuilder,
                           WireframeBuilder* wireframeBuilder,
                           PointViewBuilder* pointViewBuilder = nullptr);

private:
    int getModeSwitchIndex(RenderingConfig::DisplayMode mode);
    
    SoSwitch* m_modeSwitch;
    bool m_useSwitchMode;
};




