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

#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/details/SoFaceDetail.h>

PickingService::PickingService(SceneManager* sceneManager,
	SoSeparator* occRoot,
	const std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* nodeToGeom)
	: m_sceneManager(sceneManager), m_occRoot(occRoot), m_nodeToGeom(nodeToGeom) {
}

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

PickingResult PickingService::pickDetailedAtScreen(const wxPoint& screenPos) const {
	PickingResult result;

	if (!m_sceneManager || !m_occRoot) {
		LOG_WRN_S("PickingService - SceneManager or OCC root is null");
		return result;
	}

	wxSize size = m_sceneManager->getCanvas() ? m_sceneManager->getCanvas()->GetClientSize() : wxSize(0, 0);
	if (size.x <= 0 || size.y <= 0) {
		LOG_WRN_S("PickingService - Invalid viewport size");
		return result;
	}

	LOG_INF_S("PickingService - Screen pos: (" + std::to_string(screenPos.x) + ", " + std::to_string(screenPos.y) + 
		"), viewport: " + std::to_string(size.GetWidth()) + "x" + std::to_string(size.GetHeight()));

	SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
	SoRayPickAction pick(viewport);
	
	int pickY = size.GetHeight() - screenPos.y; // Flip Y for OpenInventor
	pick.setPoint(SbVec2s(screenPos.x, pickY));
	pick.setRadius(3);

	LOG_INF_S("PickingService - Coin3D pick point: (" + std::to_string(screenPos.x) + ", " + std::to_string(pickY) + 
		"), radius: 3");

	// CRITICAL: Apply pick against the scene root (which includes camera), not just objectRoot
	// The camera is needed for ray calculation in SoRayPickAction
	SoSeparator* sceneRoot = m_sceneManager->getSceneRoot();
	SoSeparator* objectRoot = m_sceneManager->getObjectRoot();
	int occChildren = m_occRoot->getNumChildren();
	int objChildren = objectRoot->getNumChildren();
	int sceneChildren = sceneRoot ? sceneRoot->getNumChildren() : 0;
	int mapSize = m_nodeToGeom ? static_cast<int>(m_nodeToGeom->size()) : 0;
	
	LOG_INF_S("PickingService - occRoot children: " + std::to_string(occChildren) + 
		", objectRoot children: " + std::to_string(objChildren) + 
		", sceneRoot children: " + std::to_string(sceneChildren) + 
		", nodeToGeom map size: " + std::to_string(mapSize));
	
	// Apply pick to scene root which contains camera
	pick.apply(sceneRoot ? sceneRoot : objectRoot);

	SoPickedPoint* picked = pick.getPickedPoint();
	if (!picked) {
		LOG_INF_S("PickingService - No picked point found by ray pick action");
		return result;
	}

	LOG_INF_S("PickingService - Picked point found");

	SoPath* p = picked->getPath();
	if (!p) {
		LOG_WRN_S("PickingService - Picked point has null path");
		return result;
	}

	LOG_INF_S("PickingService - Path length: " + std::to_string(p->getLength()));
	
	SoSeparator* sep = findTopLevelSeparatorInPath(p, m_occRoot);
	if (!sep) {
		LOG_WRN_S("PickingService - Could not find top-level separator in path");
		return result;
	}

	LOG_INF_S("PickingService - Found top-level separator");

	if (!m_nodeToGeom) {
		LOG_WRN_S("PickingService - nodeToGeom map is null");
		return result;
	}

	auto it = m_nodeToGeom->find(sep);
	if (it == m_nodeToGeom->end()) {
		LOG_WRN_S("PickingService - Separator not found in nodeToGeom map");
		return result;
	}

	result.geometry = it->second;
	LOG_INF_S("PickingService - Found geometry: " + result.geometry->getName());

	// Try to get triangle index from the picked point detail
	const SoDetail* detail = picked->getDetail();
	if (detail && detail->isOfType(SoFaceDetail::getClassTypeId())) {
		const SoFaceDetail* faceDetail = static_cast<const SoFaceDetail*>(detail);

		// Get the face index (triangle index in the mesh)
		int triangleIndex = faceDetail->getFaceIndex();
		result.triangleIndex = triangleIndex;
		LOG_INF_S("PickingService - Triangle index: " + std::to_string(triangleIndex));

		// Use face index mapping to get geometry face ID
		if (result.geometry && result.geometry->hasFaceIndexMapping()) {
			int geometryFaceId = result.geometry->getGeometryFaceIdForTriangle(triangleIndex);
			result.geometryFaceId = geometryFaceId;
			LOG_INF_S("PickingService - Geometry face ID: " + std::to_string(geometryFaceId));
		} else {
			LOG_WRN_S("PickingService - Geometry does not have face index mapping");
		}
	} else {
		LOG_WRN_S("PickingService - No face detail found in picked point");
	}

	return result;
}
