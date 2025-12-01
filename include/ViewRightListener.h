#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class NavigationModeManager;
class ViewRightListener : public CommandListener {
public:
	explicit ViewRightListener(NavigationModeManager* nav);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "ViewRightListener"; }
private:
	NavigationModeManager* m_nav;
};