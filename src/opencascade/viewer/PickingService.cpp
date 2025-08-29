#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/PickingService.h"
#include "OCCGeometry.h"
#include "SceneManager.h"
#include "Canvas.h"
#include "logger/Logger.h"

#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoGroup.h>

PickingService::PickingService(SceneManager* sceneManager, 
                               std::vector<std::shared_ptr<OCCGeometry>>* geometries)
    : m_sceneManager(sceneManager), m_geometries(geometries) {
    LOG_DBG("PickingService created", "PickingService");
}

PickingService::~PickingService() {
    LOG_DBG("PickingService destroyed", "PickingService");
}

std::shared_ptr<OCCGeometry> PickingService::pickGeometryAtScreen(const wxPoint& screenPos) {
    return pickGeometryAtScreen(screenPos, 3); // Default tolerance of 3 pixels
}

std::shared_ptr<OCCGeometry> PickingService::pickGeometryAtScreen(const wxPoint& screenPos, int tolerance) {
    if (!m_enabled || !m_sceneManager || !m_geometries) {
        return nullptr;
    }
    
    auto picked = pickAllGeometriesAtScreen(screenPos);
    return picked.empty() ? nullptr : picked.front();
}

std::vector<std::shared_ptr<OCCGeometry>> PickingService::pickAllGeometriesAtScreen(const wxPoint& screenPos) {
    std::vector<std::shared_ptr<OCCGeometry>> result;
    
    if (!m_enabled || !m_sceneManager || !m_geometries) {
        return result;
    }
    
    Canvas* canvas = m_sceneManager->getCanvas();
    if (!canvas) {
        LOG_WRN("Canvas not available for picking", "PickingService");
        return result;
    }
    
    try {
        // Get viewport dimensions
        int width, height;
        canvas->GetSize(&width, &height);
        
        if (width <= 0 || height <= 0) {
            LOG_WRN("Invalid viewport size for picking", "PickingService");
            return result;
        }
        
        // Convert screen coordinates to normalized device coordinates
        // Note: In wxWidgets, Y=0 is at top, but in OpenGL Y=0 is at bottom
        float normalizedX = (2.0f * screenPos.x) / width - 1.0f;
        float normalizedY = 1.0f - (2.0f * screenPos.y) / height;
        
        // Clamp to valid range
        normalizedX = std::max(-1.0f, std::min(1.0f, normalizedX));
        normalizedY = std::max(-1.0f, std::min(1.0f, normalizedY));
        
        // Create pick action
        SbViewportRegion viewport(width, height);
        SoRayPickAction pickAction(viewport);
        
        // Set pick point in normalized coordinates
        pickAction.setPoint(SbVec2s(static_cast<short>(screenPos.x), 
                                   static_cast<short>(height - screenPos.y))); // Flip Y coordinate
        
        // Get scene root for picking
        SoSeparator* sceneRoot = m_sceneManager->getObjectRoot();
        if (!sceneRoot) {
            LOG_WRN("Scene root not available for picking", "PickingService");
            return result;
        }
        
        // Perform picking
        pickAction.apply(sceneRoot);
        
        // Process picked points
        SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
        if (!pickedPoint) {
            return result; // Nothing picked
        }
        
        // Get the path to the picked object
        SoPath* path = pickedPoint->getPath();
        if (!path || path->getLength() == 0) {
            return result;
        }
        
        // Try to find the geometry that owns this scene node
        SoNode* pickedNode = path->getTail();
        if (!pickedNode) {
            return result;
        }
        
        // Search through our geometries to find the one that contains this node
        for (const auto& geometry : *m_geometries) {
            if (!geometry) continue;
            
            SoSeparator* geomNode = geometry->getCoinNode();
            if (!geomNode) continue;
            
            // Check if the picked node is within this geometry's scene graph
            if (isNodeInSubtree(pickedNode, geomNode)) {
                result.push_back(geometry);
                break; // For single pick, return first match
            }
        }
        
        if (!result.empty()) {
            LOG_DBG((std::string("Picked geometry: ") + result.front()->getName()).c_str(), "PickingService");
        }
        
    } catch (const std::exception& e) {
        LOG_ERR((std::string("Exception during picking: ") + e.what()).c_str(), "PickingService");
    }
    
    return result;
}

void PickingService::setEnabled(bool enabled) {
    m_enabled = enabled;
    LOG_DBG((std::string("Picking ") + (enabled ? "enabled" : "disabled")).c_str(), "PickingService");
}

bool PickingService::isNodeInSubtree(SoNode* targetNode, SoNode* rootNode) {
    if (!targetNode || !rootNode) {
        return false;
    }
    
    if (targetNode == rootNode) {
        return true;
    }
    
    // If root is a group, check its children recursively
    if (auto* group = dynamic_cast<SoGroup*>(rootNode)) {
        for (int i = 0; i < group->getNumChildren(); ++i) {
            if (isNodeInSubtree(targetNode, group->getChild(i))) {
                return true;
            }
        }
    }
    
    return false;
}