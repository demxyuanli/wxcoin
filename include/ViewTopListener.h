#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class NavigationModeManager;
class ViewTopListener : public CommandListener {
public:
	explicit ViewTopListener(NavigationModeManager* nav);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "ViewTopListener"; }
private:
	NavigationModeManager* m_nav;
};