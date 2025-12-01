#pragma once

#include "CommandListener.h"
#include "CommandType.h"

class NavigationModeManager;

class ViewAllListener : public CommandListener {
public:
	explicit ViewAllListener(NavigationModeManager* nav);
	~ViewAllListener() override = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "ViewAllListener"; }

private:
	NavigationModeManager* m_nav;
};