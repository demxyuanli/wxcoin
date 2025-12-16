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
    
    try {
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
            if (!sep) return;  // Safety check
            
            int numChildren = sep->getNumChildren();
            for (int i = 0; i < numChildren; ++i) {
                try {
                    SoNode* child = sep->getChild(i);
                    if (child) {
                        findDrawStyleAndMaterial(child, drawStyle, material);
                    }
                } catch (...) {
                    // Safety: Skip invalid child nodes
                    // Continue with next child
                    continue;
                }
            }
        }
    } catch (...) {
        // Safety: If node access fails, just return
        // This prevents crash if node becomes invalid during traversal
        return;
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
// Geometry nodes are SoIndexedFaceSet or SoFaceSet (coin triangle nodes)
bool DisplayModeNodeManager::containsGeometryNode(SoNode* node) const {
    if (!node) return false;
    
    try {
        // Direct geometry nodes: SoIndexedFaceSet or SoFaceSet (coin triangle nodes)
        if (node->isOfType(SoIndexedFaceSet::getClassTypeId()) ||
            node->isOfType(SoFaceSet::getClassTypeId())) {
            return true;
        }
        
        // Check inside SoSeparator
        if (node->isOfType(SoSeparator::getClassTypeId())) {
            SoSeparator* sep = static_cast<SoSeparator*>(node);
            if (!sep) return false;  // Safety check
            
            int numChildren = sep->getNumChildren();
            for (int i = 0; i < numChildren; ++i) {
                try {
                    SoNode* child = sep->getChild(i);
                    if (child && containsGeometryNode(child)) {
                        return true;
                    }
                } catch (...) {
                    // Safety: Skip invalid child nodes
                    continue;
                }
            }
        }
    } catch (...) {
        // Safety: If node access fails, assume no geometry
        return false;
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
        
        // CRITICAL FIX: Preserve surface geometry nodes
        // Surface nodes are SoSeparator containing SoIndexedFaceSet/SoFaceSet (coin triangle nodes)
        // SoIndexedFaceSet/SoFaceSet are the actual triangle geometry data nodes
        if (containsGeometryNode(child)) {
            continue;  // Preserve surface geometry nodes
        }
        
        toRemove.push_back(child);
    }
    
    for (auto* node : toRemove) {
        coinNode->removeChild(node);
    }
}

SoSeparator* DisplayModeNodeManager::findPointViewNode(SoSeparator* coinNode) const {
    if (!coinNode) {
        return nullptr;
    }
    
    try {
        for (int i = 0; i < coinNode->getNumChildren(); ++i) {
            SoNode* child = coinNode->getChild(i);
            if (!child) continue;
            
            if (child->isOfType(SoSeparator::getClassTypeId())) {
                SoSeparator* sep = static_cast<SoSeparator*>(child);
                if (!sep) continue;
                
                // Check if this separator contains SoPointSet or SoCoordinate3 (point view indicators)
                int numChildren = sep->getNumChildren();
                for (int j = 0; j < numChildren; ++j) {
                    try {
                        SoNode* subChild = sep->getChild(j);
                        if (!subChild) continue;
                        
                        if (subChild->isOfType(SoPointSet::getClassTypeId()) ||
                            subChild->isOfType(SoCoordinate3::getClassTypeId())) {
                            return sep;  // Found point view node
                        }
                    } catch (...) {
                        continue;
                    }
                }
            }
        }
    } catch (...) {
        return nullptr;
    }
    
    return nullptr;
}

bool DisplayModeNodeManager::hasSurfaceGeometryNode(SoSeparator* coinNode) const {
    if (!coinNode) {
        return false;
    }
    
    try {
        for (int i = 0; i < coinNode->getNumChildren(); ++i) {
            SoNode* child = coinNode->getChild(i);
            if (!child) continue;
            
            // Skip Switch node (it contains state nodes, not geometry)
            if (child->isOfType(SoSwitch::getClassTypeId())) {
                continue;
            }
            
            // Check if this node is a surface node (SoSeparator containing SoIndexedFaceSet/SoFaceSet)
            // SoIndexedFaceSet/SoFaceSet are the coin triangle nodes (actual triangle geometry data)
            if (containsGeometryNode(child)) {
                return true;
            }
        }
    } catch (...) {
        return false;
    }
    
    return false;
}

bool DisplayModeNodeManager::hasSwitchNode(SoSeparator* coinNode, SoSwitch* modeSwitch) const {
    if (!coinNode || !modeSwitch) {
        return false;
    }
    
    try {
        for (int i = 0; i < coinNode->getNumChildren(); ++i) {
            SoNode* child = coinNode->getChild(i);
            if (child == modeSwitch) {
                return true;  // Found the Switch node
            }
        }
    } catch (...) {
        return false;
    }
    
    return false;
}


