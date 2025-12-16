#include "geometry/helper/DisplayModeNodeManager.h"
#include "edges/ModularEdgeComponent.h"
#include "rendering/PolygonModeNode.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <vector>

void DisplayModeNodeManager::findDrawStyleAndMaterial(SoNode* node, SoDrawStyle*& drawStyle, SoMaterial*& material) {
    if (!node) return;
    
    if (node->isOfType(SoDrawStyle::getClassTypeId())) {
        if (!drawStyle) {
            drawStyle = static_cast<SoDrawStyle*>(node);
        }
    } else if (node->isOfType(SoMaterial::getClassTypeId())) {
        if (!material) {
            material = static_cast<SoMaterial*>(node);
        }
    } else if (node->isOfType(SoSeparator::getClassTypeId())) {
        SoSeparator* sep = static_cast<SoSeparator*>(node);
        for (int i = 0; i < sep->getNumChildren(); ++i) {
            findDrawStyleAndMaterial(sep->getChild(i), drawStyle, material);
        }
    }
}

void DisplayModeNodeManager::cleanupEdgeNodes(SoSeparator* coinNode, ModularEdgeComponent* edgeComponent) {
    if (!coinNode || !edgeComponent) {
        return;
    }

    SoSeparator* originalEdgeNode = edgeComponent->getEdgeNode(EdgeType::Original);
    SoSeparator* featureEdgeNode = edgeComponent->getEdgeNode(EdgeType::Feature);
    SoSeparator* meshEdgeNode = edgeComponent->getEdgeNode(EdgeType::Mesh);
    SoSeparator* highlightEdgeNode = edgeComponent->getEdgeNode(EdgeType::Highlight);
    SoSeparator* normalLineNode = edgeComponent->getEdgeNode(EdgeType::VerticeNormal);
    SoSeparator* faceNormalLineNode = edgeComponent->getEdgeNode(EdgeType::FaceNormal);
    SoSeparator* silhouetteEdgeNode = edgeComponent->getEdgeNode(EdgeType::Silhouette);
    
    for (int i = coinNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = coinNode->getChild(i);
        if (!child) continue;
        
        if (child == originalEdgeNode || child == featureEdgeNode ||
            child == meshEdgeNode || child == highlightEdgeNode ||
            child == normalLineNode || child == faceNormalLineNode ||
            child == silhouetteEdgeNode) {
            coinNode->removeChild(i);
        }
    }
}

// Check if a node contains geometry (mesh) nodes
bool DisplayModeNodeManager::containsGeometryNode(SoNode* node) const {
    if (!node) return false;
    
    // Direct geometry nodes
    if (node->isOfType(SoIndexedFaceSet::getClassTypeId()) ||
        node->isOfType(SoFaceSet::getClassTypeId())) {
        return true;
    }
    
    // Check inside SoSeparator
    if (node->isOfType(SoSeparator::getClassTypeId())) {
        SoSeparator* sep = static_cast<SoSeparator*>(node);
        for (int i = 0; i < sep->getNumChildren(); ++i) {
            if (containsGeometryNode(sep->getChild(i))) {
                return true;
            }
        }
    }
    
    return false;
}

void DisplayModeNodeManager::resetAllRenderStates(SoSeparator* coinNode, ModularEdgeComponent* edgeComponent) {
    if (!coinNode) {
        return;
    }
    
    if (edgeComponent) {
        cleanupEdgeNodes(coinNode, edgeComponent);
    }
    
    std::vector<SoNode*> toRemove;
    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (!child) continue;
        
        // Keep Switch node (for Switch mode)
        if (child->isOfType(SoSwitch::getClassTypeId())) {
            continue;
        }
        
        // CRITICAL FIX: Preserve geometry nodes (mesh geometry for pure mesh models)
        // Geometry nodes are SoSeparator containing SoIndexedFaceSet or SoFaceSet
        if (containsGeometryNode(child)) {
            continue;  // Preserve geometry nodes
        }
        
        toRemove.push_back(child);
    }
    
    for (auto* node : toRemove) {
        coinNode->removeChild(node);
    }
}


