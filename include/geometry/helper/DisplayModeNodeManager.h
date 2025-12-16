#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
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
};


