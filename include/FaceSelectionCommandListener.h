#pragma once

#include "CommandListener.h"
#include "InputManager.h"

/**
 * @brief Command listener for face selection tool activation
 */
class FaceSelectionCommandListener : public CommandListener
{
public:
	FaceSelectionCommandListener(InputManager* inputManager, class PickingService* pickingService, class OCCViewer* occViewer);

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override;

private:
	InputManager* m_inputManager;
	class PickingService* m_pickingService;
	class OCCViewer* m_occViewer;
	bool m_coordinateSystemVisibilitySaved;
	bool m_savedCoordinateSystemVisibility;
	bool m_originalEdgesStateSaved;
	bool m_savedOriginalEdgesState;
};


