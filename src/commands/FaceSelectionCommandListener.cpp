#include "FaceSelectionCommandListener.h"
#include "FaceSelectionListener.h"
#include "viewer/PickingService.h"
#include "Canvas.h"
#include "MouseHandler.h"
#include "SceneManager.h"
#include "OCCViewer.h"
#include "logger/Logger.h"

FaceSelectionCommandListener::FaceSelectionCommandListener(InputManager* inputManager,
	PickingService* pickingService, OCCViewer* occViewer)
	: m_inputManager(inputManager), m_pickingService(pickingService), m_occViewer(occViewer)
	, m_coordinateSystemVisibilitySaved(false), m_savedCoordinateSystemVisibility(false)
	, m_originalEdgesStateSaved(false), m_savedOriginalEdgesState(false)
{
	LOG_INF_S("FaceSelectionCommandListener created");
}

CommandResult FaceSelectionCommandListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters) {

	LOG_INF_S("FaceSelectionCommandListener::executeCommand - Command received: " + commandType);

	if (commandType != "FACE_SELECTION_TOOL") {
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

		// Create face selection input state
		auto faceSelectionState = std::make_unique<FaceSelectionListener>(canvas, m_pickingService, m_occViewer);

		// Switch to face selection mode
		m_inputManager->setCustomInputState(std::move(faceSelectionState));

		// Verify activation
		bool nowActive = m_inputManager->isCustomInputStateActive();
		if (nowActive) {
			LOG_INF_S("FaceSelectionCommandListener::executeCommand - Tool successfully activated");
			return CommandResult(true, "Face selection tool activated - hover to highlight, click to select, right-click for menu", commandType);
		} else {
			LOG_ERR_S("FaceSelectionCommandListener::executeCommand - Tool activation failed");
			return CommandResult(false, "Failed to activate face selection tool", commandType);
		}
	}
}

bool FaceSelectionCommandListener::canHandleCommand(const std::string& commandType) const {
	return commandType == "FACE_SELECTION_TOOL";
}

std::string FaceSelectionCommandListener::getListenerName() const {
	return "FaceSelectionCommandListener";
}


