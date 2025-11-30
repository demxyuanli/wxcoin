#include "FaceQueryCommandListener.h"
#include "FaceQueryListener.h"
#include "PropertyPanel.h"
#include "viewer/PickingService.h"
#include "Canvas.h"
#include "MouseHandler.h"
#include "SceneManager.h"
#include "logger/Logger.h"

FaceQueryCommandListener::FaceQueryCommandListener(InputManager* inputManager,
	PickingService* pickingService)
	: m_inputManager(inputManager), m_pickingService(pickingService)
	, m_coordinateSystemVisibilitySaved(false), m_savedCoordinateSystemVisibility(false)
{
	LOG_INF_S("FaceQueryCommandListener created");
}

CommandResult FaceQueryCommandListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters) {

	LOG_INF_S("FaceQueryCommandListener::executeCommand - Command received: " + commandType);

	if (commandType != "FACE_QUERY_TOOL") {
		LOG_WRN_S("FaceQueryCommandListener::executeCommand - Unknown command: " + commandType);
		return CommandResult(false, "Unknown command: " + commandType, commandType);
	}

	if (!m_inputManager) {
		LOG_ERR_S("FaceQueryCommandListener::executeCommand - InputManager not available");
		return CommandResult(false, "Required services not available", commandType);
	}

	if (!m_pickingService) {
		LOG_ERR_S("FaceQueryCommandListener::executeCommand - PickingService not available");
		return CommandResult(false, "Required services not available", commandType);
	}

	// Check if face query is currently active
	// Need to check if the current active state is actually FaceQueryListener
	// not just any custom input state (could be selection tool)
	bool isActive = false;
	if (m_inputManager->isCustomInputStateActive()) {
		InputState* currentState = m_inputManager->getCurrentInputState();
		if (currentState) {
			// Check if current state is FaceQueryListener using dynamic_cast
			FaceQueryListener* faceQueryState = dynamic_cast<FaceQueryListener*>(currentState);
			isActive = (faceQueryState != nullptr);
		}
	}
	
	LOG_INF_S("FaceQueryCommandListener::executeCommand - Current tool state: " +
		std::string(isActive ? "ACTIVE (FaceQuery)" : "INACTIVE or other tool"));

	if (isActive) {
		// Deactivate face query tool
		LOG_INF_S("FaceQueryCommandListener::executeCommand - Deactivating face query tool");
		m_inputManager->enterDefaultState();

		// Clear face info overlay when tool is deactivated
		MouseHandler* mouseHandler = m_inputManager->getMouseHandler();
		if (mouseHandler) {
			Canvas* canvas = mouseHandler->getCanvas();
			if (canvas && canvas->getFaceInfoOverlay()) {
				canvas->getFaceInfoOverlay()->clear();
				canvas->Refresh(false); // Refresh to remove overlay display
				LOG_INF_S("FaceQueryCommandListener::executeCommand - Cleared face info overlay");
			}

			// Restore coordinate system visibility if it was saved
			if (m_coordinateSystemVisibilitySaved && canvas && canvas->getSceneManager()) {
				canvas->getSceneManager()->setCoordinateSystemVisible(m_savedCoordinateSystemVisibility);
				LOG_INF_S("FaceQueryCommandListener::executeCommand - Restored coordinate system visibility: " + 
					std::string(m_savedCoordinateSystemVisibility ? "visible" : "hidden"));
				m_coordinateSystemVisibilitySaved = false;
			}
		}

		// Verify deactivation
		bool stillActive = m_inputManager->isCustomInputStateActive();
		if (stillActive) {
			LOG_WRN_S("FaceQueryCommandListener::executeCommand - Tool deactivation may have failed");
		} else {
			LOG_INF_S("FaceQueryCommandListener::executeCommand - Tool successfully deactivated");
		}

		return CommandResult(true, "Face query tool deactivated", commandType);
	} else {
		// Get canvas from input manager's mouse handler
		MouseHandler* mouseHandler = m_inputManager->getMouseHandler();
		if (!mouseHandler) {
			LOG_ERR_S("FaceQueryCommandListener::executeCommand - MouseHandler not available");
			return CommandResult(false, "MouseHandler not available", commandType);
		}

		Canvas* canvas = mouseHandler->getCanvas();
		if (!canvas) {
			LOG_ERR_S("FaceQueryCommandListener::executeCommand - Canvas not available");
			return CommandResult(false, "Canvas not available", commandType);
		}

		LOG_INF_S("FaceQueryCommandListener::executeCommand - Activating face query tool");

		// Clear any previous face info overlay when activating tool
		if (canvas->getFaceInfoOverlay()) {
			canvas->getFaceInfoOverlay()->clear();
			LOG_INF_S("FaceQueryCommandListener::executeCommand - Cleared previous face info overlay");
		}

		// Save and hide coordinate system to avoid interference with face picking
		if (canvas && canvas->getSceneManager()) {
			m_savedCoordinateSystemVisibility = canvas->getSceneManager()->isCoordinateSystemVisible();
			m_coordinateSystemVisibilitySaved = true;
			canvas->getSceneManager()->setCoordinateSystemVisible(false);
			LOG_INF_S("FaceQueryCommandListener::executeCommand - Saved and hidden coordinate system (was: " + 
				std::string(m_savedCoordinateSystemVisibility ? "visible" : "hidden") + ")");
		}

		// Create face query input state
		auto faceQueryState = std::make_unique<FaceQueryListener>(canvas, m_pickingService);

		// Switch to face query mode
		m_inputManager->setCustomInputState(std::move(faceQueryState));

		// Verify activation
		bool nowActive = m_inputManager->isCustomInputStateActive();
		if (nowActive) {
			LOG_INF_S("FaceQueryCommandListener::executeCommand - Tool successfully activated");
			return CommandResult(true, "Face query tool activated - left-click or middle-click on faces to view information", commandType);
		} else {
			LOG_ERR_S("FaceQueryCommandListener::executeCommand - Tool activation failed");
			return CommandResult(false, "Failed to activate face query tool", commandType);
		}
	}
}

bool FaceQueryCommandListener::canHandleCommand(const std::string& commandType) const {
	return commandType == "FACE_QUERY_TOOL";
}

std::string FaceQueryCommandListener::getListenerName() const {
	return "FaceQueryCommandListener";
}
