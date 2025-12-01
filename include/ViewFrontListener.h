#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class NavigationModeManager;
class ViewFrontListener : public CommandListener {
public:
	explicit ViewFrontListener(NavigationModeManager* nav);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "ViewFrontListener"; }
private:
	NavigationModeManager* m_nav;
};