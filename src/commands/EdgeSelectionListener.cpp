#include "EdgeSelectionListener.h"
#include "BaseSelectionListener.h"
#include "mod/Selection.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "config/SelectionHighlightConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/SbVec3f.h>
#include <wx/event.h>
#include <wx/msgdlg.h>

EdgeSelectionListener::EdgeSelectionListener(Canvas* canvas, PickingService* pickingService, OCCViewer* occViewer)
	: BaseSelectionListener(canvas, pickingService, occViewer)
	, m_highlightedGeometry(nullptr), m_highlightedEdgeId(-1)
	, m_selectedGeometry(nullptr), m_selectedEdgeId(-1)
	, m_highlightNode(nullptr), m_selectedNode(nullptr)
{
	LOG_INF_S("EdgeSelectionListener created");
}

EdgeSelectionListener::~EdgeSelectionListener()
{
	clearHighlight();
	clearSelection();
}

void EdgeSelectionListener::onMouseButton(wxMouseEvent& event) {
	wxPoint mousePos = event.GetPosition();
	bool isLeftUp = event.LeftUp();

	if (isLeftUp) {
		// Left-click: select edge
		event.Skip(false);

		if (!m_pickingService) {
			LOG_WRN_S("EdgeSelectionListener::onMouseButton - PickingService not available");
			return;
		}

		PickingResult result = m_pickingService->pickDetailedAtScreen(mousePos);

		if (result.geometry && !result.subElementName.empty() && result.elementType == "Edge" && result.geometryEdgeId >= 0) {
			// Use Selection system to manage selection
			auto& selection = mod::Selection::getInstance();
			selection.setSelection(result.geometry->getName(), result.subElementName, 
				result.elementType, result.x, result.y, result.z);
			
			// Update local state
			selectEdge(result.geometry, result.geometryEdgeId);
			
			LOG_INF_S("EdgeSelectionListener::onMouseButton - Selected " + result.subElementName + 
				" in geometry " + result.geometry->getName());

			// Show selection result in floating info window (top-left of canvas)
			if (m_canvas && m_canvas->getSelectionInfoDialog()) {
				m_canvas->getSelectionInfoDialog()->SetPickingResult(result);
			}
		} else {
			// Clicked on empty space or non-edge element, clear selection
			auto& selection = mod::Selection::getInstance();
			selection.clearSelection();
			clearSelection();
			LOG_INF_S("EdgeSelectionListener::onMouseButton - Cleared selection");

			// Show picking failure in floating info window
			if (m_canvas && m_canvas->getSelectionInfoDialog()) {
				m_canvas->getSelectionInfoDialog()->SetPickingResult(result);
			}
		}

		return;
	}

	event.Skip();
}

void EdgeSelectionListener::onMouseMotion(wxMouseEvent& event) {
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
		
		// Only handle edge preselection
		if (result.elementType == "Edge" && result.geometryEdgeId >= 0) {
			// Check if this is a different edge than currently highlighted
			if (!m_highlightedGeometry || m_highlightedGeometry != result.geometry || 
				m_highlightedEdgeId != result.geometryEdgeId) {
				clearHighlight();
				highlightEdge(result.geometry, result.geometryEdgeId);

				LOG_INF_S("EdgeSelectionListener::onMouseMotion - Highlighting edge " +
					std::to_string(result.geometryEdgeId) + " in geometry " + result.geometry->getName());
			}
		}
	} else {
		// Not hovering over any edge, clear preselection
		auto& selection = mod::Selection::getInstance();
		selection.removePreselect();
		if (m_highlightedGeometry) {
			clearHighlight();
		}
	}

	event.Skip();
}

void EdgeSelectionListener::highlightEdge(std::shared_ptr<OCCGeometry> geometry, int edgeId) {
	if (!geometry || edgeId < 0) {
		LOG_WRN_S("EdgeSelectionListener::highlightEdge - Invalid parameters");
		return;
	}

	// Check if already highlighting the same edge
	if (m_highlightedGeometry == geometry && m_highlightedEdgeId == edgeId && m_highlightNode) {
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
		LOG_WRN_S("EdgeSelectionListener::highlightEdge - Geometry has no Coin3D node");
		return;
	}

	// Get or create highlight node (use cache)
	m_highlightNode = getOrCreateHighlightNode(geometry, edgeId, false);
	if (!m_highlightNode) {
		LOG_WRN_S("EdgeSelectionListener::highlightEdge - Failed to get/create highlight node");
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
	m_highlightedEdgeId = edgeId;

	LOG_INF_S("EdgeSelectionListener::highlightEdge - Highlighted edge " +
		std::to_string(edgeId) + " in geometry " + geometry->getName());

	if (m_canvas) {
		m_canvas->Refresh(false);
	}
}

void EdgeSelectionListener::clearHighlight() {
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
	m_highlightedEdgeId = -1;
}

void EdgeSelectionListener::selectEdge(std::shared_ptr<OCCGeometry> geometry, int edgeId) {
	if (!geometry || edgeId < 0) {
		LOG_WRN_S("EdgeSelectionListener::selectEdge - Invalid parameters");
		return;
	}

	LOG_INF_S("EdgeSelectionListener::selectEdge - Selecting edge " + std::to_string(edgeId) +
		" in geometry " + geometry->getName());

	// Clear previous selection
	clearSelection();

	// Get geometry's Coin3D node
	SoSeparator* geometryNode = geometry->getCoinNode();
	if (!geometryNode) {
		LOG_WRN_S("EdgeSelectionListener::selectEdge - Geometry has no Coin3D node");
		return;
	}

	// Get or create selection highlight node (use cache)
	m_selectedNode = getOrCreateHighlightNode(geometry, edgeId, true);
	if (!m_selectedNode) {
		LOG_WRN_S("EdgeSelectionListener::selectEdge - Failed to get/create selection node");
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
	m_selectedEdgeId = edgeId;

	LOG_INF_S("EdgeSelectionListener::selectEdge - Selected edge " +
		std::to_string(edgeId) + " in geometry " + geometry->getName());

	if (m_canvas) {
		m_canvas->Refresh(false);
	}
}

void EdgeSelectionListener::clearSelection() {
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
	m_selectedEdgeId = -1;
}

void EdgeSelectionListener::onSelectionChanged(const mod::SelectionChange& change) {
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

SoSwitch* EdgeSelectionListener::getOrCreateHighlightNode(std::shared_ptr<OCCGeometry> geometry, int edgeId, bool isSelection)
{
	// Generate cache key
	std::string cacheKey = getCacheKey(geometry, edgeId, isSelection);

	// Check cache
	auto it = m_highlightCache.find(cacheKey);
	if (it != m_highlightCache.end() && it->second) {
		return it->second;
	}

	// Create new highlight geometry
	SoSeparator* highlightGeometry = createHighlightGeometry(geometry, edgeId, isSelection);
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

SoSeparator* EdgeSelectionListener::createHighlightGeometry(std::shared_ptr<OCCGeometry> geometry, int edgeId, bool isSelection)
{
	// Extract edge data
	std::vector<gp_Pnt> edgePoints;
	if (!extractEdgeData(geometry, edgeId, edgePoints)) {
		LOG_WRN_S("EdgeSelectionListener::createHighlightGeometry - Failed to extract edge data");
		return nullptr;
	}

	if (edgePoints.empty()) {
		LOG_WRN_S("EdgeSelectionListener::createHighlightGeometry - Empty edge points");
		return nullptr;
	}

	// Create separator for highlight
	SoSeparator* highlightSeparator = new SoSeparator();
	highlightSeparator->ref();

	// Add draw style for line width - use configured values
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->style = SoDrawStyle::LINES;
	auto& configManager = SelectionHighlightConfigManager::getInstance();
	const auto& edgeHighlight = configManager.getEdgeHighlight();
	drawStyle->lineWidth = isSelection ? edgeHighlight.selectionLineWidth : edgeHighlight.lineWidth;
	highlightSeparator->addChild(drawStyle);

	// Add highlight material - use configured colors
	SoMaterial* material = new SoMaterial;
	if (isSelection) {
		// Selection: use configured selection colors
		const auto& sel = edgeHighlight.selectionDiffuse;
		const auto& amb = edgeHighlight.selectionAmbient;
		const auto& spec = edgeHighlight.selectionSpecular;
		const auto& emis = edgeHighlight.selectionEmissive;
		
		material->diffuseColor.setValue(sel.r, sel.g, sel.b);
		material->ambientColor.setValue(amb.r, amb.g, amb.b);
		material->specularColor.setValue(spec.r, spec.g, spec.b);
		material->emissiveColor.setValue(emis.r, emis.g, emis.b);
	} else {
		// Preselection (hover): use configured hover colors
		const auto& hover = edgeHighlight.hoverDiffuse;
		const auto& amb = edgeHighlight.hoverAmbient;
		const auto& spec = edgeHighlight.hoverSpecular;
		const auto& emis = edgeHighlight.hoverEmissive;
		
		material->diffuseColor.setValue(hover.r, hover.g, hover.b);
		material->ambientColor.setValue(amb.r, amb.g, amb.b);
		material->specularColor.setValue(spec.r, spec.g, spec.b);
		material->emissiveColor.setValue(emis.r, emis.g, emis.b);
	}
	highlightSeparator->addChild(material);

	// Add coordinate node
	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(static_cast<int>(edgePoints.size()));
	SbVec3f* points = coords->point.startEditing();
	for (size_t i = 0; i < edgePoints.size(); ++i) {
		const gp_Pnt& point = edgePoints[i];
		points[i].setValue(
			static_cast<float>(point.X()),
			static_cast<float>(point.Y()),
			static_cast<float>(point.Z())
		);
	}
	coords->point.finishEditing();
	highlightSeparator->addChild(coords);

	// Add indexed line set
	SoIndexedLineSet* lineSet = new SoIndexedLineSet;
	lineSet->coordIndex.setNum(static_cast<int>(edgePoints.size()));
	int32_t* indices = lineSet->coordIndex.startEditing();
	for (size_t i = 0; i < edgePoints.size(); ++i) {
		indices[i] = static_cast<int32_t>(i);
	}
	lineSet->coordIndex.finishEditing();
	highlightSeparator->addChild(lineSet);

	return highlightSeparator;
}

bool EdgeSelectionListener::extractEdgeData(std::shared_ptr<OCCGeometry> geometry, int edgeId, std::vector<gp_Pnt>& edgePoints)
{
	if (!geometry) {
		LOG_WRN_S("EdgeSelectionListener::extractEdgeData - Geometry is null");
		return false;
	}

	// Only extract from original edges, not mesh edges
	// Get the original edge node from ModularEdgeComponent
	if (!geometry->modularEdgeComponent) {
		LOG_WRN_S("EdgeSelectionListener::extractEdgeData - Geometry has no modular edge component");
		return false;
	}

	SoSeparator* originalEdgeNode = geometry->modularEdgeComponent->getEdgeNode(EdgeType::Original);
	if (!originalEdgeNode) {
		LOG_WRN_S("EdgeSelectionListener::extractEdgeData - No original edge node found");
		return false;
	}

	// Search for line set and coordinate nodes in the original edge node
	SoSearchAction searchLines;
	searchLines.setType(SoIndexedLineSet::getClassTypeId());
	searchLines.setInterest(SoSearchAction::ALL);
	searchLines.apply(originalEdgeNode);

	SoSearchAction searchCoords;
	searchCoords.setType(SoCoordinate3::getClassTypeId());
	searchCoords.setInterest(SoSearchAction::ALL);
	searchCoords.apply(originalEdgeNode);

	// Get line set path
	SoPathList& linePaths = searchLines.getPaths();
	if (linePaths.getLength() == 0) {
		LOG_WRN_S("EdgeSelectionListener::extractEdgeData - No line set nodes found in original edges");
		return false;
	}

	// Get coordinate path
	SoPathList& coordPaths = searchCoords.getPaths();
	if (coordPaths.getLength() == 0) {
		LOG_WRN_S("EdgeSelectionListener::extractEdgeData - No coordinate nodes found in original edges");
		return false;
	}

	// Use the first line set found (should be the only one for original edges)
	SoPath* linePath = linePaths[0];
	SoIndexedLineSet* lineSet = static_cast<SoIndexedLineSet*>(linePath->getTail());

	// Find the corresponding coordinate node (should be in the same path or nearby)
	SoCoordinate3* coords = nullptr;
	for (int i = 0; i < linePath->getLength(); ++i) {
		SoNode* node = linePath->getNode(i);
		if (node->isOfType(SoCoordinate3::getClassTypeId())) {
			coords = static_cast<SoCoordinate3*>(node);
			break;
		}
	}
	// If not found in path, use the first coordinate node
	if (!coords && coordPaths.getLength() > 0) {
		coords = static_cast<SoCoordinate3*>(coordPaths[0]->getTail());
	}

	if (!lineSet || !coords) {
		LOG_WRN_S("EdgeSelectionListener::extractEdgeData - Could not find line set or coordinate node in original edges");
		return false;
	}

	// Extract line indices
	const int32_t* indices = lineSet->coordIndex.getValues(0);
	int numIndices = lineSet->coordIndex.getNum();

	// Find the line segment for the given edgeId
	// lineId corresponds to a line index in the line set
	// Each line is defined by consecutive indices separated by -1
	int currentLineIndex = 0;
	for (int i = 0; i < numIndices; ) {
		if (currentLineIndex == edgeId) {
			// Found the line we're looking for
			// Extract points until we hit -1 or end
			while (i < numIndices && indices[i] != -1) {
				int coordIndex = indices[i];
				if (coordIndex >= 0 && coordIndex < coords->point.getNum()) {
					const SbVec3f& point = coords->point[coordIndex];
					edgePoints.emplace_back(point[0], point[1], point[2]);
				}
				++i;
			}
			if (!edgePoints.empty()) {
				LOG_INF_S("EdgeSelectionListener::extractEdgeData - Found edge " + std::to_string(edgeId) + 
					" with " + std::to_string(edgePoints.size()) + " points");
				return true;
			}
			break;
		}

		// Skip to next line (until -1 or end)
		while (i < numIndices && indices[i] != -1) {
			++i;
		}
		if (i < numIndices && indices[i] == -1) {
			++i; // Skip separator
		}
		++currentLineIndex;
	}

	LOG_WRN_S("EdgeSelectionListener::extractEdgeData - Edge ID " + std::to_string(edgeId) + " not found in line set");
	return false;
}

std::string EdgeSelectionListener::getCacheKey(std::shared_ptr<OCCGeometry> geometry, int edgeId, bool isSelection) const
{
	if (!geometry) {
		return "";
	}

	// Create unique cache key: geometry name + edgeId + selection flag
	std::string key = geometry->getName() + "_edge" + std::to_string(edgeId);
	if (isSelection) {
		key += "_selected";
	} else {
		key += "_preselected";
	}

	return key;
}

