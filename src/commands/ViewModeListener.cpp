#include "ViewModeListener.h"
#include "CommandType.h"
#include "logger/Logger.h"
#include "OCCViewer.h"

ViewModeListener::ViewModeListener(OCCViewer* viewer)
	: m_viewer(viewer)
{
}

CommandResult ViewModeListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	if (!m_viewer) {
		return CommandResult(false, "OCCViewer not available", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::ToggleWireframe)) {
		const bool toWire = !m_viewer->isWireframeMode();
		m_viewer->setWireframeMode(toWire);
		std::string msg = std::string(toWire ? "Wireframe enabled" : "Wireframe disabled");
		LOG_INF_S(msg);
		return CommandResult(true, msg, commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::ToggleEdges)) {
		const bool toShow = !m_viewer->isShowEdges();
		m_viewer->setShowEdges(toShow);
		std::string msg = std::string(toShow ? "Edges enabled" : "Edges disabled");
		LOG_INF_S(msg);
		return CommandResult(true, msg, commandType);
	}
	// Removed ToggleShading command handling - functionality not needed

	return CommandResult(false, "Unknown command type", commandType);
}

bool ViewModeListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == cmd::to_string(cmd::CommandType::ToggleWireframe) ||
		commandType == cmd::to_string(cmd::CommandType::ToggleEdges);
}

std::string ViewModeListener::getListenerName() const
{
	return "ViewModeListener";
}