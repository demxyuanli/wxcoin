#include "VertexSelectionListener.h"
#include "BaseSelectionListener.h"
#include "mod/Selection.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "config/SelectionHighlightConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/SbVec3f.h>
#include <wx/event.h>
#include <wx/msgdlg.h>

VertexSelectionListener::VertexSelectionListener(Canvas* canvas, PickingService* pickingService, OCCViewer* occViewer)
	: BaseSelectionListener(canvas, pickingService, occViewer)
	, m_highlightedGeometry(nullptr), m_highlightedVertexId(-1)
	, m_selectedGeometry(nullptr), m_selectedVertexId(-1)
	, m_highlightNode(nullptr), m_selectedNode(nullptr)
{
	LOG_INF_S("VertexSelectionListener created");
}

VertexSelectionListener::~VertexSelectionListener()
{
	clearHighlight();
	clearSelection();
}

void VertexSelectionListener::onMouseButton(wxMouseEvent& event) {
	wxPoint mousePos = event.GetPosition();
	bool isLeftUp = event.LeftUp();

	if (isLeftUp) {
		// Left-click: select vertex
		event.Skip(false);

		if (!m_pickingService) {
			LOG_WRN_S("VertexSelectionListener::onMouseButton - PickingService not available");
			return;
		}

		PickingResult result = m_pickingService->pickDetailedAtScreen(mousePos);

		if (result.geometry && !result.subElementName.empty() && result.elementType == "Vertex" && result.geometryVertexId >= 0) {
			// Use Selection system to manage selection
			auto& selection = mod::Selection::getInstance();
			selection.setSelection(result.geometry->getName(), result.subElementName, 
				result.elementType, result.x, result.y, result.z);
			
			// Update local state
			selectVertex(result.geometry, result.geometryVertexId);
			
			LOG_INF_S("VertexSelectionListener::onMouseButton - Selected " + result.subElementName + 
				" in geometry " + result.geometry->getName());
			
			// Show information message for selection result
			if (m_canvas) {
				wxString msg = wxString::Format(
					"Vertex Selection:\n\n"
					"Geometry: %s\n"
					"Vertex: %s\n"
					"Vertex Index: %d\n"
					"Position: (%.3f, %.3f, %.3f)",
					result.geometry->getName(),
					result.subElementName,
					result.vertexIndex,
					result.x, result.y, result.z
				);
				wxMessageBox(msg, "Vertex Selection Result", wxOK | wxICON_INFORMATION, m_canvas);
			}
		} else {
			// Clicked on empty space or non-vertex element, clear selection
			auto& selection = mod::Selection::getInstance();
			selection.clearSelection();
			clearSelection();
			LOG_INF_S("VertexSelectionListener::onMouseButton - Cleared selection");
			
			// Show information message for picking failure
			if (m_canvas && (!result.geometry || result.elementType != "Vertex")) {
				wxMessageBox("No vertex picked at this position.\n\n"
					"Please click on a visible vertex to select it.",
					"Picking Info", wxOK | wxICON_INFORMATION, m_canvas);
			}
		}

		return;
	}

	event.Skip();
}

void VertexSelectionListener::onMouseMotion(wxMouseEvent& event) {
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
		
		// Only handle vertex preselection
		if (result.elementType == "Vertex" && result.geometryVertexId >= 0) {
			// Check if this is a different vertex than currently highlighted
			if (!m_highlightedGeometry || m_highlightedGeometry != result.geometry || 
				m_highlightedVertexId != result.geometryVertexId) {
				clearHighlight();
				highlightVertex(result.geometry, result.geometryVertexId);

				LOG_INF_S("VertexSelectionListener::onMouseMotion - Highlighting vertex " +
					std::to_string(result.geometryVertexId) + " in geometry " + result.geometry->getName());
			}
		}
	} else {
		// Not hovering over any vertex, clear preselection
		auto& selection = mod::Selection::getInstance();
		selection.removePreselect();
		if (m_highlightedGeometry) {
			clearHighlight();
		}
	}

	event.Skip();
}

void VertexSelectionListener::highlightVertex(std::shared_ptr<OCCGeometry> geometry, int vertexId) {
	if (!geometry || vertexId < 0) {
		LOG_WRN_S("VertexSelectionListener::highlightVertex - Invalid parameters");
		return;
	}

	// Check if already highlighting the same vertex
	if (m_highlightedGeometry == geometry && m_highlightedVertexId == vertexId && m_highlightNode) {
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
		LOG_WRN_S("VertexSelectionListener::highlightVertex - Geometry has no Coin3D node");
		return;
	}

	// Get or create highlight node (use cache)
	m_highlightNode = getOrCreateHighlightNode(geometry, vertexId, false);
	if (!m_highlightNode) {
		LOG_WRN_S("VertexSelectionListener::highlightVertex - Failed to get/create highlight node");
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
	m_highlightedVertexId = vertexId;

	LOG_INF_S("VertexSelectionListener::highlightVertex - Highlighted vertex " +
		std::to_string(vertexId) + " in geometry " + geometry->getName());

	if (m_canvas) {
		m_canvas->Refresh(false);
	}
}

void VertexSelectionListener::clearHighlight() {
	// Safety check: ensure object is still valid
	if (!m_isAlive || !*m_isAlive) {
		return;
	}

	if (m_highlightNode) {
		// Hide highlight by setting switch to NONE (don't remove from scene graph)
		m_highlightNode->whichChild.setValue(SO_SWITCH_NONE);
		m_highlightNode = nullptr;
	}

	m_highlightGeometryRoot = nullptr;
	m_highlightedGeometry = nullptr;
	m_highlightedVertexId = -1;
}

void VertexSelectionListener::selectVertex(std::shared_ptr<OCCGeometry> geometry, int vertexId) {
	if (!geometry || vertexId < 0) {
		LOG_WRN_S("VertexSelectionListener::selectVertex - Invalid parameters");
		return;
	}

	LOG_INF_S("VertexSelectionListener::selectVertex - Selecting vertex " + std::to_string(vertexId) +
		" in geometry " + geometry->getName());

	// Clear previous selection
	clearSelection();

	// Get geometry's Coin3D node
	SoSeparator* geometryNode = geometry->getCoinNode();
	if (!geometryNode) {
		LOG_WRN_S("VertexSelectionListener::selectVertex - Geometry has no Coin3D node");
		return;
	}

	// Get or create selection highlight node (use cache)
	m_selectedNode = getOrCreateHighlightNode(geometry, vertexId, true);
	if (!m_selectedNode) {
		LOG_WRN_S("VertexSelectionListener::selectVertex - Failed to get/create selection node");
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
	m_selectedVertexId = vertexId;

	LOG_INF_S("VertexSelectionListener::selectVertex - Selected vertex " +
		std::to_string(vertexId) + " in geometry " + geometry->getName());

	if (m_canvas) {
		m_canvas->Refresh(false);
	}
}

void VertexSelectionListener::clearSelection() {
	// Safety check: ensure object is still valid
	if (!m_isAlive || !*m_isAlive) {
		return;
	}

	if (m_selectedNode) {
		// Hide selection by setting switch to NONE (don't remove from scene graph)
		m_selectedNode->whichChild.setValue(SO_SWITCH_NONE);
		m_selectedNode = nullptr;
	}

	m_selectedGeometryRoot = nullptr;
	m_selectedGeometry = nullptr;
	m_selectedVertexId = -1;
}

void VertexSelectionListener::onSelectionChanged(const mod::SelectionChange& change) {
	// Handle selection changes from Selection system
	if (change.type == mod::SelectionChangeType::SetPreselect ||
		change.type == mod::SelectionChangeType::MovePreselect) {
		// Preselection (hover) - handled by ViewProvider
	} else if (change.type == mod::SelectionChangeType::RemovePreselect) {
		// Clear preselection
		clearHighlight();
	} else if (change.type == mod::SelectionChangeType::AddSelection ||
			   change.type == mod::SelectionChangeType::SetSelection) {
		// Selection - handled by ViewProvider
	} else if (change.type == mod::SelectionChangeType::ClearSelection ||
			   change.type == mod::SelectionChangeType::RemoveSelection) {
		// Clear selection
		clearSelection();
	}
}

SoSwitch* VertexSelectionListener::getOrCreateHighlightNode(std::shared_ptr<OCCGeometry> geometry, int vertexId, bool isSelection)
{
	// Generate cache key
	std::string cacheKey = getCacheKey(geometry, vertexId, isSelection);

	// Check cache
	auto it = m_highlightCache.find(cacheKey);
	if (it != m_highlightCache.end() && it->second) {
		return it->second;
	}

	// Create new highlight geometry
	SoSeparator* highlightGeometry = createHighlightGeometry(geometry, vertexId, isSelection);
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

SoSeparator* VertexSelectionListener::createHighlightGeometry(std::shared_ptr<OCCGeometry> geometry, int vertexId, bool isSelection)
{
	// Extract vertex data
	gp_Pnt vertexPoint;
	if (!extractVertexData(geometry, vertexId, vertexPoint)) {
		LOG_WRN_S("VertexSelectionListener::createHighlightGeometry - Failed to extract vertex data");
		return nullptr;
	}

	// Create separator for highlight
	SoSeparator* highlightSeparator = new SoSeparator();
	highlightSeparator->ref();

	// Add draw style for point size - use configured values
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->style = SoDrawStyle::POINTS;
	auto& configManager = SelectionHighlightConfigManager::getInstance();
	const auto& vertexHighlight = configManager.getVertexHighlight();
	drawStyle->pointSize = isSelection ? vertexHighlight.selectionPointSize : vertexHighlight.pointSize;
	highlightSeparator->addChild(drawStyle);

	// Add highlight material - use configured colors
	SoMaterial* material = new SoMaterial;
	if (isSelection) {
		// Selection: use configured selection colors
		const auto& sel = vertexHighlight.selectionDiffuse;
		const auto& amb = vertexHighlight.selectionAmbient;
		const auto& spec = vertexHighlight.selectionSpecular;
		const auto& emis = vertexHighlight.selectionEmissive;
		
		material->diffuseColor.setValue(sel.r, sel.g, sel.b);
		material->ambientColor.setValue(amb.r, amb.g, amb.b);
		material->specularColor.setValue(spec.r, spec.g, spec.b);
		material->emissiveColor.setValue(emis.r, emis.g, emis.b);
	} else {
		// Preselection (hover): use configured hover colors
		const auto& hover = vertexHighlight.hoverDiffuse;
		const auto& amb = vertexHighlight.hoverAmbient;
		const auto& spec = vertexHighlight.hoverSpecular;
		const auto& emis = vertexHighlight.hoverEmissive;
		
		material->diffuseColor.setValue(hover.r, hover.g, hover.b);
		material->ambientColor.setValue(amb.r, amb.g, amb.b);
		material->specularColor.setValue(spec.r, spec.g, spec.b);
		material->emissiveColor.setValue(emis.r, emis.g, emis.b);
	}
	highlightSeparator->addChild(material);

	// Add coordinate node
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.set1Value(0,
		static_cast<float>(vertexPoint.X()),
		static_cast<float>(vertexPoint.Y()),
		static_cast<float>(vertexPoint.Z())
	);
	highlightSeparator->addChild(coords);

	// Add point set
	SoPointSet* pointSet = new SoPointSet;
	pointSet->numPoints.setValue(1);
	highlightSeparator->addChild(pointSet);

	return highlightSeparator;
}

bool VertexSelectionListener::extractVertexData(std::shared_ptr<OCCGeometry> geometry, int vertexId, gp_Pnt& vertexPoint)
{
	if (!geometry) {
		LOG_WRN_S("VertexSelectionListener::extractVertexData - Geometry is null");
		return false;
	}

	SoSeparator* geometryNode = geometry->getCoinNode();
	if (!geometryNode) {
		LOG_WRN_S("VertexSelectionListener::extractVertexData - Geometry has no Coin3D node");
		return false;
	}

	// Search for coordinate node
	SoSearchAction searchCoords;
	searchCoords.setType(SoCoordinate3::getClassTypeId());
	searchCoords.setInterest(SoSearchAction::ALL);
	searchCoords.apply(geometryNode);

	// Get coordinate node
	SoPathList& coordPaths = searchCoords.getPaths();
	if (coordPaths.getLength() == 0) {
		LOG_WRN_S("VertexSelectionListener::extractVertexData - No coordinate nodes found");
		return false;
	}

	// Use the first coordinate node found
	SoPath* coordPath = coordPaths[0];
	SoCoordinate3* coords = static_cast<SoCoordinate3*>(coordPath->getTail());

	// Check if vertexId is valid
	if (vertexId < 0 || vertexId >= coords->point.getNum()) {
		LOG_WRN_S("VertexSelectionListener::extractVertexData - Invalid vertex ID " + std::to_string(vertexId));
		return false;
	}

	// Extract vertex point
	const SbVec3f& point = coords->point[vertexId];
	vertexPoint = gp_Pnt(point[0], point[1], point[2]);

	return true;
}

std::string VertexSelectionListener::getCacheKey(std::shared_ptr<OCCGeometry> geometry, int vertexId, bool isSelection) const
{
	if (!geometry) {
		return "";
	}

	// Create unique cache key: geometry name + vertexId + selection flag
	std::string key = geometry->getName() + "_vertex" + std::to_string(vertexId);
	if (isSelection) {
		key += "_selected";
	} else {
		key += "_preselected";
	}

	return key;
}

