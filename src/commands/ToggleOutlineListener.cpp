#include "ToggleOutlineListener.h"
#include "OCCViewer.h"
#include "CommandType.h"
#include "logger/Logger.h"

CommandResult ToggleOutlineListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters) {
	if (!m_viewer) return CommandResult(false, "Viewer not available", commandType);
	bool enable = false;
	auto it = parameters.find("toggle");
	if (it != parameters.end()) {
		// If UI passes a toggle intent, invert current state
		enable = !m_viewer->isOutlineEnabled();
	}
	else {
		// Or explicit value via parameters["value"] == "true"/"false"
		auto it2 = parameters.find("value");
		if (it2 != parameters.end()) enable = (it2->second == "true");
		else enable = !m_viewer->isOutlineEnabled();
	}
	m_viewer->setOutlineEnabled(enable);
	LOG_INF_S(std::string("Outline ") + (enable ? "enabled" : "disabled"));
	return CommandResult(true, enable ? "Outline enabled" : "Outline disabled", commandType);
}

bool ToggleOutlineListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ToggleOutline);
}