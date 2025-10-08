#pragma once

#include "CommandListener.h"
#include "InputManager.h"

/**
 * @brief Command listener for face query tool activation
 */
class FaceQueryCommandListener : public CommandListener
{
public:
	FaceQueryCommandListener(InputManager* inputManager, class PickingService* pickingService);

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override;

private:
	InputManager* m_inputManager;
	class PickingService* m_pickingService;
};
