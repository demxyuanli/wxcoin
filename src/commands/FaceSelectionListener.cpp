#include "FaceSelectionListener.h"
#include "mod/FaceHighlightManager.h"
#include "mod/Selection.h"
#include "viewer/PickingService.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "rendering/GeometryProcessor.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/SbVec3f.h>
#include <wx/menu.h>
#include <wx/event.h>
#include <wx/msgdlg.h>
#include <unordered_map>

FaceSelectionListener::FaceSelectionListener(Canvas* canvas, PickingService* pickingService, OCCViewer* occViewer)
	: m_canvas(canvas), m_pickingService(pickingService), m_occViewer(occViewer)
	, m_highlightedGeometry(nullptr), m_highlightedFaceId(-1)
	, m_selectedGeometry(nullptr), m_selectedFaceId(-1)
	, m_highlightNode(nullptr), m_selectedNode(nullptr)
{
	LOG_INF_S("FaceSelectionListener created");

	// Register selection observer to handle preselection and selection changes
	auto& selection = mod::Selection::getInstance();
	selection.addObserver([this](const mod::SelectionChange& change) {
		this->onSelectionChanged(change);
	});
}

FaceSelectionListener::~FaceSelectionListener()
{
	clearHighlight();
	clearSelection();

	// Clean up cache
	for (auto& pair : m_highlightCache) {
		if (pair.second) {
			pair.second->unref();
		}
	}
	m_highlightCache.clear();
}

void FaceSelectionListener::onMouseButton(wxMouseEvent& event) {
	wxPoint mousePos = event.GetPosition();
	bool isLeftDown = event.LeftDown();
	bool isLeftUp = event.LeftUp();
	bool isRightDown = event.RightDown();
	bool isRightUp = event.RightUp();

	if (isRightUp) {
		// Right-click: show context menu if face is selected
		if (m_selectedGeometry && m_selectedFaceId >= 0) {
			wxPoint screenPos = m_canvas->ClientToScreen(mousePos);
			showContextMenu(screenPos, m_selectedGeometry, m_selectedFaceId);
			event.Skip(false);
			return;
		}
		event.Skip();
		return;
	}

	if (isLeftUp) {
		// Left-click: select face
		event.Skip(false);

		if (!m_pickingService) {
			LOG_WRN_S("FaceSelectionListener::onMouseButton - PickingService not available");
			return;
		}

	PickingResult result = m_pickingService->pickDetailedAtScreen(mousePos);

	if (result.geometry && !result.subElementName.empty()) {
		// Use Selection system to manage selection
		auto& selection = mod::Selection::getInstance();
		selection.setSelection(result.geometry->getName(), result.subElementName, 
			result.elementType, result.x, result.y, result.z);
		bool selectionSuccess = true; // setSelection always succeeds (it clears and sets)
		
		// Also update local state for backward compatibility
		// Note: Currently only Face highlighting is implemented
		bool highlightSuccess = false;
		if (result.elementType == "Face" && result.geometryFaceId >= 0) {
			selectFace(result.geometry, result.geometryFaceId);
			highlightSuccess = true;
		} else if (result.elementType == "Edge" || result.elementType == "Vertex") {
			// TODO: Implement edge and vertex highlighting
			LOG_INF_S("FaceSelectionListener::onMouseButton - " + result.elementType +
				" selection not yet implemented for highlighting");
		}
		
		LOG_INF_S("FaceSelectionListener::onMouseButton - Selected " + result.subElementName + 
			" in geometry " + result.geometry->getName());
		
		// Show information message for selection result
		if (m_canvas) {
			wxString elementInfo;
			if (result.elementType == "Face") {
				elementInfo = wxString::Format("Face ID: %d", result.geometryFaceId);
			} else if (result.elementType == "Edge") {
				elementInfo = wxString::Format("Edge Index: %d", result.lineIndex);
			} else if (result.elementType == "Vertex") {
				elementInfo = wxString::Format("Vertex Index: %d", result.vertexIndex);
			} else {
				elementInfo = "Unknown element";
			}

			wxString msg = wxString::Format(
				"Selection Operation:\n\n"
				"Status: %s\n"
				"Geometry: %s\n"
				"Element: %s\n"
				"Type: %s\n"
				"%s\n"
				"Position: (%.3f, %.3f, %.3f)\n\n"
				"Selection System: %s\n"
				"Highlight Manager: %s",
				(selectionSuccess && (highlightSuccess || result.elementType != "Face")) ? "SUCCESS" : "PARTIAL",
				result.geometry->getName(),
				result.subElementName,
				result.elementType.empty() ? "Unknown" : result.elementType,
				elementInfo,
				result.x, result.y, result.z,
				selectionSuccess ? "OK" : "FAILED",
				(highlightSuccess || result.elementType != "Face") ? "OK" : "FAILED"
			);
			wxMessageBox(msg, "Element Selection Result", wxOK | wxICON_INFORMATION, m_canvas);
		}
	} else {
		// Clicked on empty space, clear selection
		auto& selection = mod::Selection::getInstance();
		selection.clearSelection();
		clearSelection();
		LOG_INF_S("FaceSelectionListener::onMouseButton - Cleared selection");
		
		// Show information message for picking failure
		if (m_canvas) {
			wxMessageBox("No geometry element picked at this position.\n\n"
				"Please click on a visible face to select it.",
				"Picking Info", wxOK | wxICON_INFORMATION, m_canvas);
		}
	}

		return;
	}

	event.Skip();
}

void FaceSelectionListener::onMouseMotion(wxMouseEvent& event) {
	wxPoint mousePos = event.GetPosition();

	if (!m_pickingService) {
		event.Skip();
		return;
	}

	PickingResult result = m_pickingService->pickDetailedAtScreen(mousePos);

	if (result.geometry && !result.subElementName.empty()) {
		// Use Selection system to manage preselection (hover)
		auto& selection = mod::Selection::getInstance();
		int changed = selection.setPreselect(result.geometry->getName(), result.subElementName,
			result.elementType, result.x, result.y, result.z);
		
		// Also update local state for backward compatibility
		bool highlightSuccess = false;
		if (result.geometryFaceId >= 0) {
			// Check if this is a different face than currently highlighted
			if (!m_highlightedGeometry || m_highlightedGeometry != result.geometry || 
				m_highlightedFaceId != result.geometryFaceId) {
				highlightFace(result.geometry, result.geometryFaceId);
				highlightSuccess = true;
			}
		}
		
		// Show information message for preselection (only on change, to avoid spam)
		if (changed > 0 && m_canvas) {
			wxString msg = wxString::Format(
				"Preselection (Hover):\n\n"
				"Status: %s\n"
				"Geometry: %s\n"
				"Element: %s\n"
				"Type: %s\n"
				"Face ID: %d\n"
				"Position: (%.3f, %.3f, %.3f)\n\n"
				"Selection System: OK\n"
				"Highlight Manager: %s",
				highlightSuccess ? "SUCCESS" : "FAILED",
				result.geometry->getName(),
				result.subElementName,
				result.elementType.empty() ? "Unknown" : result.elementType,
				result.geometryFaceId,
				result.x, result.y, result.z,
				highlightSuccess ? "OK" : "FAILED"
			);
			// Use a less intrusive notification for hover (could be a tooltip or status bar instead)
			// For now, we'll log it but not show a message box to avoid spam
			LOG_INF_S("FaceSelectionListener::onMouseMotion - Preselected: " + result.subElementName);
		}
	} else {
		// Not hovering over any face, clear preselection
		auto& selection = mod::Selection::getInstance();
		selection.removePreselect();
		if (m_highlightedGeometry) {
			clearHighlight();
		}
	}

	event.Skip();
}

void FaceSelectionListener::onMouseWheel(wxMouseEvent& event) {
	event.Skip();
}

void FaceSelectionListener::highlightFace(std::shared_ptr<OCCGeometry> geometry, int faceId) {
	if (!geometry || faceId < 0) {
		LOG_WRN_S("FaceSelectionListener::highlightFace - Invalid parameters");
		return;
	}

	// Check if already highlighting the same face
	if (m_highlightedGeometry == geometry && m_highlightedFaceId == faceId && m_highlightNode) {
		// Already highlighted, just ensure it's visible
		if (m_highlightNode->whichChild.getValue() != SO_SWITCH_ALL) {
			m_highlightNode->whichChild.setValue(SO_SWITCH_ALL);
		}
		return;
	}

	// Clear previous highlight
	clearHighlight();

	// Get geometry's Coin3D node
	SoSeparator* geometryNode = geometry->getCoinNode();
	if (!geometryNode) {
		LOG_WRN_S("FaceSelectionListener::highlightFace - Geometry has no Coin3D node");
		return;
	}

	// Get or create highlight node (use cache)
	m_highlightNode = getOrCreateHighlightNode(geometry, faceId, false);
	if (!m_highlightNode) {
		LOG_WRN_S("FaceSelectionListener::highlightFace - Failed to get/create highlight node");
		return;
	}

	// Add highlight node to geometry root if not already added
	int index = geometryNode->findChild(m_highlightNode);
	if (index < 0) {
		geometryNode->addChild(m_highlightNode);
	}

	// Make highlight visible
	m_highlightNode->whichChild.setValue(SO_SWITCH_ALL);

	m_highlightGeometryRoot = geometryNode;
	m_highlightedGeometry = geometry;
	m_highlightedFaceId = faceId;

	LOG_INF_S("FaceSelectionListener::highlightFace - Highlighted face " +
		std::to_string(faceId) + " in geometry " + geometry->getName());

	if (m_canvas) {
		m_canvas->Refresh(false);
	}
}

void FaceSelectionListener::clearHighlight() {
	if (m_highlightNode) {
		// Hide highlight by setting switch to NONE (don't remove from scene graph)
		m_highlightNode->whichChild.setValue(SO_SWITCH_NONE);
		m_highlightNode = nullptr;
	}

	m_highlightGeometryRoot = nullptr;
	m_highlightedGeometry = nullptr;
	m_highlightedFaceId = -1;
}

void FaceSelectionListener::selectFace(std::shared_ptr<OCCGeometry> geometry, int faceId) {
	if (!geometry || faceId < 0) {
		LOG_WRN_S("FaceSelectionListener::selectFace - Invalid parameters");
		return;
	}

	// Clear previous selection
	clearSelection();

	// Get geometry's Coin3D node
	SoSeparator* geometryNode = geometry->getCoinNode();
	if (!geometryNode) {
		LOG_WRN_S("FaceSelectionListener::selectFace - Geometry has no Coin3D node");
		return;
	}

	// Get or create selection highlight node (use cache)
	m_selectedNode = getOrCreateHighlightNode(geometry, faceId, true);
	if (!m_selectedNode) {
		LOG_WRN_S("FaceSelectionListener::selectFace - Failed to get/create selection node");
		return;
	}

	// Add selection node to geometry root if not already added
	int index = geometryNode->findChild(m_selectedNode);
	if (index < 0) {
		geometryNode->addChild(m_selectedNode);
	}

	// Make selection visible
	m_selectedNode->whichChild.setValue(SO_SWITCH_ALL);

	m_selectedGeometryRoot = geometryNode;
	m_selectedGeometry = geometry;
	m_selectedFaceId = faceId;

	LOG_INF_S("FaceSelectionListener::selectFace - Selected face " +
		std::to_string(faceId) + " in geometry " + geometry->getName());

	if (m_canvas) {
		m_canvas->Refresh(false);
	}
}

void FaceSelectionListener::clearSelection() {
	if (m_selectedNode) {
		// Hide selection by setting switch to NONE (don't remove from scene graph)
		m_selectedNode->whichChild.setValue(SO_SWITCH_NONE);
		m_selectedNode = nullptr;
	}

	m_selectedGeometryRoot = nullptr;
	m_selectedGeometry = nullptr;
	m_selectedFaceId = -1;
}

void FaceSelectionListener::showContextMenu(const wxPoint& screenPos, std::shared_ptr<OCCGeometry> geometry, int faceId) {
	if (!m_canvas) {
		return;
	}

	enum {
		ID_MENU_SHOW_INFO = wxID_HIGHEST + 1,
		ID_MENU_EDIT_INFO,
		ID_MENU_CHANGE_COLOR,
		ID_MENU_CHANGE_PROPERTIES
	};

	wxMenu contextMenu;

	contextMenu.Append(ID_MENU_SHOW_INFO, "Show Information", "Display face information");
	contextMenu.Append(ID_MENU_EDIT_INFO, "Edit Information", "Edit face information");
	contextMenu.AppendSeparator();
	contextMenu.Append(ID_MENU_CHANGE_COLOR, "Change Color", "Change face color");
	contextMenu.Append(ID_MENU_CHANGE_PROPERTIES, "Change Properties", "Modify face properties");

	// Show popup menu and get selection
	int selectedId = m_canvas->GetPopupMenuSelectionFromUser(contextMenu, m_canvas->ScreenToClient(screenPos));
	
	// Handle menu selection
	if (selectedId == ID_MENU_SHOW_INFO) {
		LOG_INF_S("FaceSelectionListener::showContextMenu - Show Information clicked for face " + 
			std::to_string(faceId) + " in geometry " + geometry->getName());
		// TODO: Show face information dialog
	} else if (selectedId == ID_MENU_EDIT_INFO) {
		LOG_INF_S("FaceSelectionListener::showContextMenu - Edit Information clicked for face " + 
			std::to_string(faceId) + " in geometry " + geometry->getName());
		// TODO: Show face edit dialog
	} else if (selectedId == ID_MENU_CHANGE_COLOR) {
		LOG_INF_S("FaceSelectionListener::showContextMenu - Change Color clicked for face " + 
			std::to_string(faceId) + " in geometry " + geometry->getName());
		// TODO: Show color picker dialog
	} else if (selectedId == ID_MENU_CHANGE_PROPERTIES) {
		LOG_INF_S("FaceSelectionListener::showContextMenu - Change Properties clicked for face " + 
			std::to_string(faceId) + " in geometry " + geometry->getName());
		// TODO: Show properties dialog
	}
}

// Face highlighting implementation methods

SoSwitch* FaceSelectionListener::getOrCreateHighlightNode(std::shared_ptr<OCCGeometry> geometry, int faceId, bool isSelection)
{
	// Generate cache key
	std::string cacheKey = getCacheKey(geometry, faceId, isSelection);

	// Check cache
	auto it = m_highlightCache.find(cacheKey);
	if (it != m_highlightCache.end() && it->second) {
		return it->second;
	}

	// Create new highlight geometry
	SoSeparator* highlightGeometry = createHighlightGeometry(geometry, faceId, isSelection);
	if (!highlightGeometry) {
		return nullptr;
	}

	// Create switch node to control visibility
	SoSwitch* switchNode = new SoSwitch();
	switchNode->ref();
	switchNode->whichChild.setValue(SO_SWITCH_NONE); // Initially hidden
	switchNode->addChild(highlightGeometry);

	// Add to cache
	m_highlightCache[cacheKey] = switchNode;

	return switchNode;
}

SoSeparator* FaceSelectionListener::createHighlightGeometry(std::shared_ptr<OCCGeometry> geometry, int faceId, bool isSelection)
{
	// Extract face mesh data
	TriangleMesh faceMesh;
	if (!extractFaceMesh(geometry, faceId, faceMesh)) {
		LOG_WRN_S("FaceSelectionListener::createHighlightNode - Failed to extract face mesh");
		return nullptr;
	}

	if (faceMesh.vertices.empty() || faceMesh.triangles.empty()) {
		LOG_WRN_S("FaceSelectionListener::createHighlightNode - Empty face mesh");
		return nullptr;
	}

	// Create separator for highlight
	SoSeparator* highlightSeparator = new SoSeparator();
	highlightSeparator->ref();

	// Add shape hints
	SoShapeHints* hints = new SoShapeHints;
	hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
	hints->shapeType = SoShapeHints::SOLID;
	hints->faceType = SoShapeHints::CONVEX;
	highlightSeparator->addChild(hints);

	// Add draw style for transparency
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->style = SoDrawStyle::FILLED;
	drawStyle->lineWidth = 1.0f;
	highlightSeparator->addChild(drawStyle);

	// Add highlight material
	SoMaterial* material = new SoMaterial;
	if (isSelection) {
		// Selection: brighter, more opaque blue-green
		material->diffuseColor.setValue(0.2f, 0.8f, 1.0f);  // Cyan-blue
		material->ambientColor.setValue(0.1f, 0.4f, 0.5f);
		material->specularColor.setValue(0.3f, 0.9f, 1.0f);
		material->emissiveColor.setValue(0.1f, 0.3f, 0.4f);
		material->shininess.setValue(0.8f);
		material->transparency.setValue(0.2f);  // Less transparent for selection
	} else {
		// Preselection (hover): semi-transparent yellow/orange
		material->diffuseColor.setValue(1.0f, 0.8f, 0.2f);  // Yellow-orange
		material->ambientColor.setValue(0.5f, 0.4f, 0.1f);
		material->specularColor.setValue(1.0f, 0.9f, 0.3f);
		material->emissiveColor.setValue(0.3f, 0.2f, 0.05f);  // Slight glow
		material->shininess.setValue(0.7f);
		material->transparency.setValue(0.3f);  // Semi-transparent
	}
	highlightSeparator->addChild(material);

	// Add coordinate node
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(static_cast<int>(faceMesh.vertices.size()));
	SbVec3f* points = coords->point.startEditing();
	for (size_t i = 0; i < faceMesh.vertices.size(); ++i) {
		const gp_Pnt& vertex = faceMesh.vertices[i];
		points[i].setValue(
			static_cast<float>(vertex.X()),
			static_cast<float>(vertex.Y()),
			static_cast<float>(vertex.Z())
		);
	}
	coords->point.finishEditing();
	highlightSeparator->addChild(coords);

	// Add normal node if available
	if (!faceMesh.normals.empty() && faceMesh.normals.size() == faceMesh.vertices.size()) {
		SoNormal* normals = new SoNormal;
		normals->vector.setNum(static_cast<int>(faceMesh.normals.size()));
		SbVec3f* normalVecs = normals->vector.startEditing();
		for (size_t i = 0; i < faceMesh.normals.size(); ++i) {
			const gp_Vec& normal = faceMesh.normals[i];
			normalVecs[i].setValue(
				static_cast<float>(normal.X()),
				static_cast<float>(normal.Y()),
				static_cast<float>(normal.Z())
			);
		}
		normals->vector.finishEditing();
		highlightSeparator->addChild(normals);

		SoNormalBinding* binding = new SoNormalBinding;
		binding->value = SoNormalBinding::PER_VERTEX_INDEXED;
		highlightSeparator->addChild(binding);
	}

	// Add indexed face set
	SoIndexedFaceSet* faceSet = new SoIndexedFaceSet;
	// Each triangle needs 3 indices + 1 separator (-1)
	int numTriangles = static_cast<int>(faceMesh.triangles.size() / 3);
	int numIndices = static_cast<int>(faceMesh.triangles.size()) + numTriangles;
	faceSet->coordIndex.setNum(numIndices);

	int32_t* indices = faceSet->coordIndex.startEditing();
	int indexPos = 0;

	// Triangle indices are already remapped to local vertex indices (0-based)
	// Just add them with separators
	for (size_t i = 0; i < faceMesh.triangles.size(); i += 3) {
		if (i + 2 < faceMesh.triangles.size()) {
			indices[indexPos++] = faceMesh.triangles[i];
			indices[indexPos++] = faceMesh.triangles[i + 1];
			indices[indexPos++] = faceMesh.triangles[i + 2];
			indices[indexPos++] = -1; // Triangle separator
		}
	}

	faceSet->coordIndex.finishEditing();
	highlightSeparator->addChild(faceSet);

	return highlightSeparator;
}

bool FaceSelectionListener::extractFaceMesh(std::shared_ptr<OCCGeometry> geometry, int faceId, TriangleMesh& faceMesh)
{
	if (!geometry) {
		LOG_WRN_S("FaceSelectionListener::extractFaceMesh - Geometry is null");
		return false;
	}

	if (!geometry->hasFaceIndexMapping()) {
		LOG_WRN_S("FaceSelectionListener::extractFaceMesh - Geometry '" + geometry->getName() + "' has no face mapping");
		return false;
	}

	// Get triangle indices for this face
	std::vector<int> triangleIndices = geometry->getTrianglesForGeometryFace(faceId);
	if (triangleIndices.empty()) {
		LOG_WRN_S("FaceSelectionListener::extractFaceMesh - No triangles found for face " + std::to_string(faceId) +
			" in geometry '" + geometry->getName() + "'");
		return false;
	}

	LOG_INF_S("FaceSelectionListener::extractFaceMesh - Found " + std::to_string(triangleIndices.size()) +
		" triangles for face " + std::to_string(faceId) + " in geometry '" + geometry->getName() + "'");

	// CRITICAL BUG FIX: Extract mesh data directly from geometry's Coin3D node
	// This ensures we use the exact same mesh data used for rendering
	SoSeparator* geometryNode = geometry->getCoinNode();
	if (!geometryNode) {
		LOG_WRN_S("FaceSelectionListener::extractFaceMesh - Geometry has no Coin3D node");
		return false;
	}

	TriangleMesh fullMesh;
	if (!extractMeshFromCoinNode(geometryNode, fullMesh)) {
		LOG_WRN_S("FaceSelectionListener::extractFaceMesh - Failed to extract mesh from Coin node");
		return false;
	}

	if (fullMesh.vertices.empty() || fullMesh.triangles.empty()) {
		LOG_WRN_S("FaceSelectionListener::extractFaceMesh - Extracted mesh is empty");
		return false;
	}

	LOG_INF_S("FaceSelectionListener::extractFaceMesh - Extracted mesh with " +
		std::to_string(fullMesh.vertices.size()) + " vertices, " +
		std::to_string(fullMesh.triangles.size() / 3) + " triangles");

	// Extract vertices and triangles for this face
	std::set<int> usedVertexIndices;

	// Collect all vertex indices used by face triangles
	// triangleIndices contains triangle indices (not vertex indices)
	for (int triIdx : triangleIndices) {
		// Each triangle has 3 vertex indices in the triangles array
		int baseIdx = triIdx * 3;
		if (baseIdx + 2 < static_cast<int>(fullMesh.triangles.size())) {
			usedVertexIndices.insert(fullMesh.triangles[baseIdx]);
			usedVertexIndices.insert(fullMesh.triangles[baseIdx + 1]);
			usedVertexIndices.insert(fullMesh.triangles[baseIdx + 2]);
		}
	}

	// Create vertex mapping: original index -> new index
	std::map<int, int> vertexMapping;
	int newIndex = 0;
	for (int origIdx : usedVertexIndices) {
		vertexMapping[origIdx] = newIndex++;
	}

	// Extract vertices
	faceMesh.vertices.reserve(usedVertexIndices.size());
	for (int origIdx : usedVertexIndices) {
		if (origIdx >= 0 && origIdx < static_cast<int>(fullMesh.vertices.size())) {
			faceMesh.vertices.push_back(fullMesh.vertices[origIdx]);
		}
	}

	// Extract normals if available
	if (!fullMesh.normals.empty() && fullMesh.normals.size() == fullMesh.vertices.size()) {
		faceMesh.normals.reserve(usedVertexIndices.size());
		for (int origIdx : usedVertexIndices) {
			if (origIdx >= 0 && origIdx < static_cast<int>(fullMesh.normals.size())) {
				faceMesh.normals.push_back(fullMesh.normals[origIdx]);
			}
		}
	}

	// Extract and remap triangles
	faceMesh.triangles.reserve(triangleIndices.size() * 3);
	for (int triIdx : triangleIndices) {
		int baseIdx = triIdx * 3;
		if (baseIdx + 2 < static_cast<int>(fullMesh.triangles.size())) {
			int v0 = fullMesh.triangles[baseIdx];
			int v1 = fullMesh.triangles[baseIdx + 1];
			int v2 = fullMesh.triangles[baseIdx + 2];

			auto it0 = vertexMapping.find(v0);
			auto it1 = vertexMapping.find(v1);
			auto it2 = vertexMapping.find(v2);

			if (it0 != vertexMapping.end() && it1 != vertexMapping.end() && it2 != vertexMapping.end()) {
				faceMesh.triangles.push_back(it0->second);
				faceMesh.triangles.push_back(it1->second);
				faceMesh.triangles.push_back(it2->second);
			}
		}
	}

	return true;
}

bool FaceSelectionListener::extractMeshFromCoinNode(SoSeparator* rootNode, TriangleMesh& mesh)
{
	if (!rootNode) {
		return false;
	}

	// Search for coordinate and face set nodes
	SoSearchAction searchCoords;
	searchCoords.setType(SoCoordinate3::getClassTypeId());
	searchCoords.setInterest(SoSearchAction::ALL);
	searchCoords.apply(rootNode);

	SoSearchAction searchFaces;
	searchFaces.setType(SoIndexedFaceSet::getClassTypeId());
	searchFaces.setInterest(SoSearchAction::ALL);
	searchFaces.apply(rootNode);

	// Get coordinate node
	SoPathList& coordPaths = searchCoords.getPaths();
	if (coordPaths.getLength() == 0) {
		LOG_WRN_S("FaceSelectionListener::extractMeshFromCoinNode - No coordinate nodes found");
		return false;
	}

	// Use the first coordinate node found
	SoPath* coordPath = coordPaths[0];
	SoCoordinate3* coords = static_cast<SoCoordinate3*>(coordPath->getTail());

	// Extract vertices
	int numPoints = coords->point.getNum();
	mesh.vertices.resize(numPoints);
	for (int i = 0; i < numPoints; ++i) {
		const SbVec3f& point = coords->point[i];
		mesh.vertices[i] = gp_Pnt(point[0], point[1], point[2]);
	}

	// Get face set node
	SoPathList& facePaths = searchFaces.getPaths();
	if (facePaths.getLength() == 0) {
		LOG_WRN_S("FaceSelectionListener::extractMeshFromCoinNode - No face set nodes found");
		return false;
	}

	// Use the first face set found
	SoPath* facePath = facePaths[0];
	SoIndexedFaceSet* faceSet = static_cast<SoIndexedFaceSet*>(facePath->getTail());

	// Extract triangles
	const int32_t* indices = faceSet->coordIndex.getValues(0);
	int numIndices = faceSet->coordIndex.getNum();

	// Convert face indices to triangle list
	for (int i = 0; i < numIndices; ) {
		// Collect vertices for this face until -1 separator
		std::vector<int> faceVertices;
		while (i < numIndices && indices[i] != -1) {
			faceVertices.push_back(indices[i]);
			i++;
		}
		if (indices[i] == -1) i++; // Skip separator

		// Triangulate the face (simple fan triangulation for convex faces)
		if (faceVertices.size() >= 3) {
			for (size_t j = 1; j < faceVertices.size() - 1; ++j) {
				mesh.triangles.push_back(faceVertices[0]);
				mesh.triangles.push_back(faceVertices[j]);
				mesh.triangles.push_back(faceVertices[j + 1]);
			}
		}
	}

	// Try to extract normals if available
	SoSearchAction searchNormals;
	searchNormals.setType(SoNormal::getClassTypeId());
	searchNormals.setInterest(SoSearchAction::ALL);
	searchNormals.apply(rootNode);

	SoPathList& normalPaths = searchNormals.getPaths();
	if (normalPaths.getLength() > 0) {
		SoPath* normalPath = normalPaths[0];
		SoNormal* normals = static_cast<SoNormal*>(normalPath->getTail());

		int numNormals = normals->vector.getNum();
		mesh.normals.resize(numNormals);
		for (int i = 0; i < numNormals; ++i) {
			const SbVec3f& normal = normals->vector[i];
			mesh.normals[i] = gp_Vec(normal[0], normal[1], normal[2]);
		}
	}

	return true;
}

std::string FaceSelectionListener::getCacheKey(std::shared_ptr<OCCGeometry> geometry, int faceId, bool isSelection) const
{
	if (!geometry) {
		return "";
	}

	// Create unique cache key: geometry name + faceId + selection flag
	std::string key = geometry->getName() + "_face" + std::to_string(faceId);
	if (isSelection) {
		key += "_selected";
	} else {
		key += "_preselected";
	}

	return key;
}

void FaceSelectionListener::onSelectionChanged(const mod::SelectionChange& change) {
	// Handle selection changes from Selection system
	// This allows other parts of the system to trigger selection changes

	if (change.type == mod::SelectionChangeType::SetPreselect ||
		change.type == mod::SelectionChangeType::MovePreselect) {
		// Preselection (hover) - handled by ViewProvider
		// The ViewProvider will be notified and will handle highlighting
	} else if (change.type == mod::SelectionChangeType::RemovePreselect) {
		// Clear preselection
		clearHighlight();
	} else if (change.type == mod::SelectionChangeType::AddSelection ||
			   change.type == mod::SelectionChangeType::SetSelection) {
		// Selection - handled by ViewProvider
		// The ViewProvider will be notified and will handle selection highlighting
	} else if (change.type == mod::SelectionChangeType::ClearSelection ||
			   change.type == mod::SelectionChangeType::RemoveSelection) {
		// Clear selection
		clearSelection();
	}
}

