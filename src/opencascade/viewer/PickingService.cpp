#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/PickingService.h"
#include "OCCGeometry.h"
#include "SceneManager.h"
#include "Canvas.h"

#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SoPickedPoint.h>

PickingService::PickingService(SceneManager* sceneManager,
                               SoSeparator* occRoot,
                               const std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* nodeToGeom)
    : m_sceneManager(sceneManager), m_occRoot(occRoot), m_nodeToGeom(nodeToGeom) {}

void PickingService::setRoot(SoSeparator* occRoot) {
    m_occRoot = occRoot;
}

SoSeparator* PickingService::findTopLevelSeparatorInPath(SoPath* path, SoSeparator* occRoot) {
    if (!path || !occRoot) return nullptr;
    for (int i = 0; i < path->getLength(); ++i) {
        SoNode* node = path->getNode(i);
        if (node == occRoot) {
            for (int j = i + 1; j < path->getLength(); ++j) {
                SoSeparator* sep = dynamic_cast<SoSeparator*>(path->getNode(j));
                if (sep) return sep;
            }
            break;
        }
    }
    return nullptr;
}

std::shared_ptr<OCCGeometry> PickingService::pickGeometryAtScreen(const wxPoint& screenPos) const {
    if (!m_sceneManager || !m_occRoot) return nullptr;
    wxSize size = m_sceneManager->getCanvas() ? m_sceneManager->getCanvas()->GetClientSize() : wxSize(0, 0);
    if (size.x <= 0 || size.y <= 0) return nullptr;
    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    SoRayPickAction pick(viewport);
    int pickY = size.GetHeight() - screenPos.y; // Flip Y for OpenInventor
    pick.setPoint(SbVec2s(screenPos.x, pickY));
    pick.setRadius(3);
    // Apply pick against the full object root so auxiliary visuals (grids/aid lines) are accounted for in the path,
    // but final geometry resolution still relies on the node-to-geometry map.
    SoSeparator* objectRoot = m_sceneManager ? m_sceneManager->getObjectRoot() : m_occRoot;
    pick.apply(objectRoot);
    SoPickedPoint* picked = pick.getPickedPoint();
    if (!picked) return nullptr;
    SoPath* p = picked->getPath();
    SoSeparator* sep = findTopLevelSeparatorInPath(p, m_occRoot);
    if (!sep) return nullptr;
    auto it = m_nodeToGeom->find(sep);
    if (it != m_nodeToGeom->end()) return it->second;
    return nullptr;
}


