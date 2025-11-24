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
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <wx/msgdlg.h>

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

	SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
	SoRayPickAction pick(viewport);
	
	int pickY = size.GetHeight() - screenPos.y; // Flip Y for OpenInventor
	pick.setPoint(SbVec2s(screenPos.x, pickY));
	pick.setRadius(3);

	// CRITICAL: Apply pick against the scene root (which includes camera), not just objectRoot
	// The camera is needed for ray calculation in SoRayPickAction
	SoSeparator* sceneRoot = m_sceneManager->getSceneRoot();
	SoSeparator* objectRoot = m_sceneManager->getObjectRoot();
	int occChildren = m_occRoot->getNumChildren();
	int objChildren = objectRoot->getNumChildren();
	int sceneChildren = sceneRoot ? sceneRoot->getNumChildren() : 0;
	int mapSize = m_nodeToGeom ? static_cast<int>(m_nodeToGeom->size()) : 0;
	
	// Apply pick to scene root which contains camera
	pick.apply(sceneRoot ? sceneRoot : objectRoot);

	SoPickedPoint* picked = pick.getPickedPoint();
	if (!picked) {
		return result;
	}


	SoPath* p = picked->getPath();
	if (!p) {
		LOG_WRN_S("PickingService - Picked point has null path");
		return result;
	}

	
	SoSeparator* sep = findTopLevelSeparatorInPath(p, m_occRoot);
	if (!sep) {
		LOG_WRN_S("PickingService - Could not find top-level separator in path");
		return result;
	}


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

	// Try to get element information from the picked point detail
	const SoDetail* detail = picked->getDetail();
	if (detail) {
		// Handle face picking
		if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
			const SoFaceDetail* faceDetail = static_cast<const SoFaceDetail*>(detail);

			// Get the face index (triangle index in the mesh)
			int triangleIndex = faceDetail->getFaceIndex();
			result.triangleIndex = triangleIndex;

			// Use face domain mapping to get geometry face ID
			if (result.geometry && result.geometry->hasFaceDomainMapping()) {
				int geometryFaceId = result.geometry->getGeometryFaceIdForTriangle(triangleIndex);
				result.geometryFaceId = geometryFaceId;

				// Get triangle count from FaceDomain
				const FaceDomain* domain = result.geometry->getFaceDomain(geometryFaceId);
				int triangleCount = domain ? domain->getTriangleCount() : 0;

			// Generate sub-element name in FreeCAD style: "Face5"
			if (geometryFaceId >= 0) {
				result.elementType = "Face";
				result.subElementName = "Face" + std::to_string(geometryFaceId);
				LOG_INF_S("PickingService - Successfully picked face " + std::to_string(geometryFaceId) +
					" (triangle " + std::to_string(triangleIndex) + ") with " + std::to_string(triangleCount) +
					" triangles in geometry " + result.geometry->getName());
			} else {
				LOG_WRN_S("PickingService - Invalid face ID returned from mapping for triangle " + std::to_string(triangleIndex));
				// Fallback: use triangle index as face ID for debugging
				result.elementType = "Face";
				result.subElementName = "Face" + std::to_string(triangleIndex);
				result.geometryFaceId = triangleIndex;
				LOG_WRN_S("PickingService - Using fallback face ID " + std::to_string(triangleIndex));
			}
			} else {
				LOG_WRN_S("PickingService - Geometry does not have face index mapping");
			}

		// Handle edge picking
		} else if (detail->isOfType(SoLineDetail::getClassTypeId())) {
			const SoLineDetail* lineDetail = static_cast<const SoLineDetail*>(detail);

			// Get the line index (edge index in the mesh)
			int lineIndex = lineDetail->getLineIndex();
			result.lineIndex = lineIndex;

			// Edge picking - use line index as edge ID (domain system doesn't support edge mapping)
			result.elementType = "Edge";
			result.subElementName = "Edge" + std::to_string(lineIndex);
			result.geometryEdgeId = lineIndex;
			LOG_INF_S("PickingService - Picked edge (line " + std::to_string(lineIndex) +
				") in geometry " + result.geometry->getName() + " (domain system)");

		// Handle vertex picking
		} else if (detail->isOfType(SoPointDetail::getClassTypeId())) {
			const SoPointDetail* pointDetail = static_cast<const SoPointDetail*>(detail);

			// Get the coordinate index (vertex index in the mesh)
			int coordinateIndex = pointDetail->getCoordinateIndex();
			result.vertexIndex = coordinateIndex;

			// Vertex picking - use coordinate index as vertex ID (domain system doesn't support vertex mapping)
			result.elementType = "Vertex";
			result.subElementName = "Vertex" + std::to_string(coordinateIndex);
			result.geometryVertexId = coordinateIndex;
			LOG_INF_S("PickingService - Picked vertex (coordinate " + std::to_string(coordinateIndex) +
				") in geometry " + result.geometry->getName() + " (domain system)");

		} else {
			LOG_WRN_S("PickingService - Unknown detail type: " + std::string(detail->getTypeId().getName().getString()));
		}

		// Get 3D coordinates from picked point (common for all types)
		const SbVec3f& point = picked->getPoint();
		result.x = point[0];
		result.y = point[1];
		result.z = point[2];

	} else {
		LOG_WRN_S("PickingService - No detail found in picked point");
	}

	return result;
}
