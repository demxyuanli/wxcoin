#pragma once

#include "config/RenderingConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include "geometry/GeometryRenderContext.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/helper/DisplayModeNodeManager.h"
#include "geometry/helper/DisplayModeHandler.h"
#include <OpenCASCADE/TopoDS_Shape.hxx>

class SoSeparator;
class SoSwitch;
class SoLightModel;
class SoMaterial;
class SoDrawStyle;
class SoPolygonOffset;
class SoShapeHints;
class ModularEdgeComponent;
namespace helper {
    class RenderNodeBuilder;
    class WireframeBuilder;
    class PointViewBuilder;
}
struct MeshParameters;
struct DisplayModeConfig;

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
                           helper::RenderNodeBuilder* renderBuilder,
                           helper::WireframeBuilder* wireframeBuilder,
                           helper::PointViewBuilder* pointViewBuilder = nullptr);
    
    // Update switch visibility for fast mode switching (called from updateDisplayMode)
    void updateDisplayModeSwitches(SoSeparator* coinNode,
                                   RenderingConfig::DisplayMode mode,
                                   ModularEdgeComponent* edgeComponent);

private:
    void initializeSwitchStructure(SoSeparator* coinNode, 
                                   DisplayModeNodeManager& nodeManager,
                                   const GeometryRenderContext& context,
                                   const TopoDS_Shape& shape,
                                   const MeshParameters& params,
                                   helper::RenderNodeBuilder* renderBuilder,
                                   helper::PointViewBuilder* pointViewBuilder);
    
    void updateSwitchVisibility(const DisplayModeConfig& config,
                               const TopoDS_Shape& shape,
                               ModularEdgeComponent* edgeComponent,
                               bool useModularEdgeComponent,
                               DisplayModeNodeManager& nodeManager,
                               SoSeparator* coinNode);
    
    SoSwitch* m_modeSwitch;
    bool m_useSwitchMode;
    
    // Three independent switches following preview canvas structure
    SoSwitch* m_surfaceSwitch;
    SoSwitch* m_edgesSwitch;
    SoSwitch* m_pointsSwitch;
    
    // Surface node containing state nodes and geometry
    SoSeparator* m_surfaceNode;
    
    // Edge and point nodes
    SoSeparator* m_edgesNode;
    SoSeparator* m_pointsNode;
    
    // State nodes - created once and reused (shared by direct and switch modes)
    SoLightModel* m_lightModel;  // Always at coinNode top level
    SoMaterial* m_material;      // In m_surfaceNode
    SoDrawStyle* m_drawStyle;    // In m_surfaceNode
    SoPolygonOffset* m_polygonOffset;  // In m_surfaceNode
    SoShapeHints* m_shapeHints;  // In m_surfaceNode
};






