#include "SliceToggleListener.h"
#include "OCCViewer.h"

SliceToggleListener::SliceToggleListener(OCCViewer* viewer)
	: m_viewer(viewer) {
}

CommandResult SliceToggleListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
	bool enable = !m_viewer->isSliceEnabled();
	m_viewer->setSliceEnabled(enable);
	return CommandResult(true, enable ? "Slice enabled" : "Slice disabled", commandType);
}

bool SliceToggleListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::SliceToggle);
}