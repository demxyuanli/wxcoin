#include "FaceSelectionCommandListener.h"
#include "FaceSelectionListener.h"
#include "EdgeSelectionListener.h"
#include "VertexSelectionListener.h"
#include "viewer/PickingService.h"
#include "Canvas.h"
#include "MouseHandler.h"
#include "SceneManager.h"
#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"

FaceSelectionCommandListener::FaceSelectionCommandListener(InputManager* inputManager,
	PickingService* pickingService, OCCViewer* occViewer)
	: m_inputManager(inputManager), m_pickingService(pickingService), m_occViewer(occViewer)
	, m_coordinateSystemVisibilitySaved(false), m_savedCoordinateSystemVisibility(false)
	, m_originalEdgesStateSaved(false), m_savedOriginalEdgesState(false)
	, m_geometryDisplayStatesSaved(false)
{
	LOG_INF_S("FaceSelectionCommandListener created");
}

CommandResult FaceSelectionCommandListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters) {

	LOG_INF_S("FaceSelectionCommandListener::executeCommand - Command received: " + commandType);

	// Support three selection modes: face, edge, vertex
	std::string selectionMode = "Face";
	if (commandType == "FACE_SELECTION_TOOL") {
		selectionMode = "Face";
	} else if (commandType == "EDGE_SELECTION_TOOL") {
		selectionMode = "Edge";
	} else if (commandType == "VERTEX_SELECTION_TOOL") {
		selectionMode = "Vertex";
	} else {
		LOG_WRN_S("FaceSelectionCommandListener::executeCommand - Unknown command: " + commandType);
		return CommandResult(false, "Unknown command: " + commandType, commandType);
	}

	if (!m_inputManager) {
		LOG_ERR_S("FaceSelectionCommandListener::executeCommand - InputManager not available");
		return CommandResult(false, "Required services not available", commandType);
	}

	if (!m_pickingService) {
		LOG_ERR_S("FaceSelectionCommandListener::executeCommand - PickingService not available");
		return CommandResult(false, "Required services not available", commandType);
	}

	if (!m_occViewer) {
		LOG_ERR_S("FaceSelectionCommandListener::executeCommand - OCCViewer not available");
		return CommandResult(false, "Required services not available", commandType);
	}

	// Check if face selection is currently active
	bool isActive = m_inputManager->isCustomInputStateActive();
	LOG_INF_S("FaceSelectionCommandListener::executeCommand - Current tool state: " +
		std::string(isActive ? "ACTIVE" : "INACTIVE"));

	if (isActive) {
		// Deactivate face selection tool
		LOG_INF_S("FaceSelectionCommandListener::executeCommand - Deactivating face selection tool");
		m_inputManager->enterDefaultState();

		// Restore coordinate system visibility if it was saved
		MouseHandler* mouseHandler = m_inputManager->getMouseHandler();
		if (mouseHandler) {
			Canvas* canvas = mouseHandler->getCanvas();
			if (m_coordinateSystemVisibilitySaved && canvas && canvas->getSceneManager()) {
				canvas->getSceneManager()->setCoordinateSystemVisible(m_savedCoordinateSystemVisibility);
				LOG_INF_S("FaceSelectionCommandListener::executeCommand - Restored coordinate system visibility: " + 
					std::string(m_savedCoordinateSystemVisibility ? "visible" : "hidden"));
				m_coordinateSystemVisibilitySaved = false;
			}

			// Restore original edges state if it was saved
			if (m_originalEdgesStateSaved) {
				const auto& flags = m_occViewer->getEdgeDisplayFlags();
				if (flags.showOriginalEdges != m_savedOriginalEdgesState) {
					m_occViewer->setShowOriginalEdges(m_savedOriginalEdgesState);
					LOG_INF_S("FaceSelectionCommandListener::executeCommand - Restored original edges state: " + 
						std::string(m_savedOriginalEdgesState ? "shown" : "hidden"));
				}
				m_originalEdgesStateSaved = false;
			}

		// Restore geometry display states if they were saved
		if (m_geometryDisplayStatesSaved) {
			restoreGeometryDisplayStates();
		}

		// Hide selection info dialog when deactivating
		if (canvas && canvas->getSelectionInfoDialog()) {
			canvas->getSelectionInfoDialog()->Hide();
		}
	}

	// Verify deactivation
	bool stillActive = m_inputManager->isCustomInputStateActive();
	if (stillActive) {
		LOG_WRN_S("FaceSelectionCommandListener::executeCommand - Tool deactivation may have failed");
	} else {
		LOG_INF_S("FaceSelectionCommandListener::executeCommand - Tool successfully deactivated");
	}

	return CommandResult(true, "Face selection tool deactivated", commandType);
	} else {
		// Get canvas from input manager's mouse handler
		MouseHandler* mouseHandler = m_inputManager->getMouseHandler();
		if (!mouseHandler) {
			LOG_ERR_S("FaceSelectionCommandListener::executeCommand - MouseHandler not available");
			return CommandResult(false, "MouseHandler not available", commandType);
		}

		Canvas* canvas = mouseHandler->getCanvas();
		if (!canvas) {
			LOG_ERR_S("FaceSelectionCommandListener::executeCommand - Canvas not available");
			return CommandResult(false, "Canvas not available", commandType);
		}

		LOG_INF_S("FaceSelectionCommandListener::executeCommand - Activating face selection tool");

		// Save and hide coordinate system to avoid interference with face picking
		if (canvas && canvas->getSceneManager()) {
			m_savedCoordinateSystemVisibility = canvas->getSceneManager()->isCoordinateSystemVisible();
			m_coordinateSystemVisibilitySaved = true;
			canvas->getSceneManager()->setCoordinateSystemVisible(false);
			LOG_INF_S("FaceSelectionCommandListener::executeCommand - Saved and hidden coordinate system (was: " + 
				std::string(m_savedCoordinateSystemVisibility ? "visible" : "hidden") + ")");
		}

		// Check if original edges are shown, if not, show them
		const auto& flags = m_occViewer->getEdgeDisplayFlags();
		m_savedOriginalEdgesState = flags.showOriginalEdges;
		m_originalEdgesStateSaved = true;
		if (!flags.showOriginalEdges) {
			m_occViewer->setShowOriginalEdges(true);
			LOG_INF_S("FaceSelectionCommandListener::executeCommand - Enabled original edges display");
		} else {
			LOG_INF_S("FaceSelectionCommandListener::executeCommand - Original edges already shown");
		}

		// Save geometry display states before modifying them
		saveGeometryDisplayStates();

		// Set display states based on selection mode
		if (selectionMode == "Edge") {
			setGeometryDisplayForEdgeSelection();
		} else if (selectionMode == "Vertex") {
			setGeometryDisplayForVertexSelection();
		}
		// Face mode: keep current display states

		// Create selection input state based on mode
		std::unique_ptr<InputState> selectionState;
		std::string toolName;
		if (selectionMode == "Face") {
			selectionState = std::make_unique<FaceSelectionListener>(canvas, m_pickingService, m_occViewer);
			toolName = "Face selection";
		} else if (selectionMode == "Edge") {
			selectionState = std::make_unique<EdgeSelectionListener>(canvas, m_pickingService, m_occViewer);
			toolName = "Edge selection";
		} else if (selectionMode == "Vertex") {
			selectionState = std::make_unique<VertexSelectionListener>(canvas, m_pickingService, m_occViewer);
			toolName = "Vertex selection";
		}

		// Switch to selection mode
		m_inputManager->setCustomInputState(std::move(selectionState));

		// Verify activation
		bool nowActive = m_inputManager->isCustomInputStateActive();
		if (nowActive) {
			LOG_INF_S("FaceSelectionCommandListener::executeCommand - " + toolName + " tool successfully activated");
			return CommandResult(true, toolName + " tool activated - hover to highlight, click to select", commandType);
		} else {
			LOG_ERR_S("FaceSelectionCommandListener::executeCommand - " + toolName + " tool activation failed");
			return CommandResult(false, "Failed to activate " + toolName + " tool", commandType);
		}
	}
}

bool FaceSelectionCommandListener::canHandleCommand(const std::string& commandType) const {
	return commandType == "FACE_SELECTION_TOOL" || 
		   commandType == "EDGE_SELECTION_TOOL" || 
		   commandType == "VERTEX_SELECTION_TOOL";
}

std::string FaceSelectionCommandListener::getListenerName() const {
	return "FaceSelectionCommandListener";
}

void FaceSelectionCommandListener::saveGeometryDisplayStates() {
	if (!m_occViewer || m_geometryDisplayStatesSaved) {
		return;
	}

	m_savedFacesVisibleStates.clear();
	m_savedVerticesVisibleStates.clear();
	m_savedPointViewStates.clear();

	auto geometries = m_occViewer->getAllGeometry();
	for (const auto& geometry : geometries) {
		if (geometry) {
			std::string name = geometry->getName();
			// Save current face visibility state
			m_savedFacesVisibleStates[name] = geometry->isFacesVisible();
			
			// Save vertices visibility state
			m_savedVerticesVisibleStates[name] = geometry->isShowVerticesEnabled();
			
			// Save point view state
			m_savedPointViewStates[name] = geometry->isShowPointViewEnabled();
		}
	}

	m_geometryDisplayStatesSaved = true;
	LOG_INF_S("FaceSelectionCommandListener::saveGeometryDisplayStates - Saved display states for " + 
		std::to_string(geometries.size()) + " geometries");
}

void FaceSelectionCommandListener::restoreGeometryDisplayStates() {
	if (!m_occViewer || !m_geometryDisplayStatesSaved) {
		return;
	}

	auto geometries = m_occViewer->getAllGeometry();
	MeshParameters params = m_occViewer->getMeshParameters();
	
	for (const auto& geometry : geometries) {
		if (geometry) {
			std::string name = geometry->getName();
			
			// Restore face visibility
			auto itFaces = m_savedFacesVisibleStates.find(name);
			if (itFaces != m_savedFacesVisibleStates.end()) {
				geometry->setFacesVisible(itFaces->second);
			}
			
			// Restore vertices visibility
			auto itVertices = m_savedVerticesVisibleStates.find(name);
			if (itVertices != m_savedVerticesVisibleStates.end()) {
				geometry->setShowVertices(itVertices->second);
			}
			
			// Restore point view state
			auto itPointView = m_savedPointViewStates.find(name);
			if (itPointView != m_savedPointViewStates.end()) {
				geometry->setShowPointView(itPointView->second);
			}
			
			// Force rebuild to apply changes
			geometry->forceCoinRepresentationRebuild(params);
		}
	}

	// Clear saved states
	m_savedFacesVisibleStates.clear();
	m_savedVerticesVisibleStates.clear();
	m_savedPointViewStates.clear();
	m_geometryDisplayStatesSaved = false;

	// Request view refresh
	MouseHandler* mouseHandler = m_inputManager ? m_inputManager->getMouseHandler() : nullptr;
	if (mouseHandler) {
		Canvas* canvas = mouseHandler->getCanvas();
		if (canvas) {
			canvas->Refresh(false);
		}
	}

	LOG_INF_S("FaceSelectionCommandListener::restoreGeometryDisplayStates - Restored display states");
}

void FaceSelectionCommandListener::setGeometryDisplayForEdgeSelection() {
	if (!m_occViewer) {
		return;
	}

	auto geometries = m_occViewer->getAllGeometry();
	for (const auto& geometry : geometries) {
		if (geometry) {
			// Hide faces, show only original edges
			geometry->setFacesVisible(false);
			geometry->setShowVertices(false);
			geometry->setShowPointView(false);
			// Force rebuild to apply changes
			MeshParameters params = m_occViewer->getMeshParameters();
			geometry->forceCoinRepresentationRebuild(params);
		}
	}

	// Request view refresh
	MouseHandler* mouseHandler = m_inputManager ? m_inputManager->getMouseHandler() : nullptr;
	if (mouseHandler) {
		Canvas* canvas = mouseHandler->getCanvas();
		if (canvas) {
			canvas->Refresh(false);
		}
	}

	LOG_INF_S("FaceSelectionCommandListener::setGeometryDisplayForEdgeSelection - Set display for edge selection mode");
}

void FaceSelectionCommandListener::setGeometryDisplayForVertexSelection() {
	if (!m_occViewer) {
		return;
	}

	auto geometries = m_occViewer->getAllGeometry();
	for (const auto& geometry : geometries) {
		if (geometry) {
			// Hide faces, show original edges and points
			geometry->setFacesVisible(false);
			geometry->setShowVertices(true);
			geometry->setShowPointView(true);
			// Force rebuild to apply changes
			MeshParameters params = m_occViewer->getMeshParameters();
			geometry->forceCoinRepresentationRebuild(params);
		}
	}

	// Request view refresh
	MouseHandler* mouseHandler = m_inputManager ? m_inputManager->getMouseHandler() : nullptr;
	if (mouseHandler) {
		Canvas* canvas = mouseHandler->getCanvas();
		if (canvas) {
			canvas->Refresh(false);
		}
	}

	LOG_INF_S("FaceSelectionCommandListener::setGeometryDisplayForVertexSelection - Set display for vertex selection mode");
}





