#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include "rendering/PolygonModeNode.h"

class SoNode;
class ModularEdgeComponent;

class DisplayModeNodeManager {
public:
    DisplayModeNodeManager() = default;
    ~DisplayModeNodeManager() = default;

    void findDrawStyleAndMaterial(SoNode* node, SoDrawStyle*& drawStyle, SoMaterial*& material);
    void cleanupEdgeNodes(SoSeparator* coinNode, ModularEdgeComponent* edgeComponent);
    void resetAllRenderStates(SoSeparator* coinNode, ModularEdgeComponent* edgeComponent);
    
    // Check if a node contains geometry (mesh) nodes (SoIndexedFaceSet or SoFaceSet)
    bool containsGeometryNode(SoNode* node) const;
    
    // Find point view node in the scene graph (SoSeparator containing SoPointSet or SoCoordinate3)
    SoSeparator* findPointViewNode(SoSeparator* coinNode) const;
    
    // Check if surface geometry node exists in the scene graph
    bool hasSurfaceGeometryNode(SoSeparator* coinNode) const;
    
    // Check if Switch node already exists in the scene graph
    bool hasSwitchNode(SoSeparator* coinNode, SoSwitch* modeSwitch) const;
};


