#pragma once

#include "config/RenderingConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include "geometry/GeometryRenderContext.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/helper/DisplayModeNodeManager.h"
#include "rendering/GeometryProcessor.h"

class SoSeparator;
class SoSwitch;
class ModularEdgeComponent;
namespace helper {
    class RenderNodeBuilder;
    class WireframeBuilder;
    class PointViewBuilder;
}
struct MeshParameters;
struct DisplayModeConfig;
struct TriangleMesh;

class MeshDisplayModeHandler {
public:
    MeshDisplayModeHandler();
    ~MeshDisplayModeHandler();
    
    void setModeSwitch(SoSwitch* modeSwitch);
    
    void handleDisplayMode(SoSeparator* coinNode, 
                           const GeometryRenderContext& context,
                           const TriangleMesh& mesh,
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
                                   const TriangleMesh& mesh,
                                   const MeshParameters& params,
                                   helper::RenderNodeBuilder* renderBuilder,
                                   helper::PointViewBuilder* pointViewBuilder);
    
    void updateSwitchVisibility(const DisplayModeConfig& config,
                               const TriangleMesh& mesh,
                               RenderingConfig::DisplayMode displayMode,
                               ModularEdgeComponent* edgeComponent,
                               bool useModularEdgeComponent,
                               DisplayModeNodeManager& nodeManager,
                               SoSeparator* coinNode);
    
    int getModeSwitchIndex(RenderingConfig::DisplayMode mode);
    
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
};


