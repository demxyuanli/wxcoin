#include "RenderModeListener.h"
#include "CommandType.h"
#include "logger/Logger.h"
#include "OCCViewer.h"
#include "config/RenderingConfig.h"

RenderModeListener::RenderModeListener(OCCViewer* viewer)
	: m_viewer(viewer)
{
}

CommandResult RenderModeListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	if (!m_viewer) {
		return CommandResult(false, "OCCViewer not available", commandType);
	}

	// Get current display settings
	RenderingConfig::DisplaySettings settings = m_viewer->getDisplaySettings();

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeNoShading)) {
		settings.displayMode = RenderingConfig::DisplayMode::NoShading;
		settings.showEdges = false;
		m_viewer->setDisplaySettings(settings);
		LOG_INF_S("RenderModeListener: Set to NoShading mode");
		return CommandResult(true, "NoShading mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModePoints)) {
		settings.displayMode = RenderingConfig::DisplayMode::Points;
		settings.showEdges = false;
		m_viewer->setDisplaySettings(settings);
		LOG_INF_S("RenderModeListener: Set to Points mode");
		return CommandResult(true, "Points mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeWireframe)) {
		settings.displayMode = RenderingConfig::DisplayMode::Wireframe;
		settings.showEdges = false;
		m_viewer->setDisplaySettings(settings);
		LOG_INF_S("RenderModeListener: Set to Wireframe mode");
		return CommandResult(true, "Wireframe mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeFlatLines)) {
		settings.displayMode = RenderingConfig::DisplayMode::Solid;
		settings.showEdges = true;
		m_viewer->setDisplaySettings(settings);
		LOG_INF_S("RenderModeListener: Set to Flat Lines mode");
		return CommandResult(true, "Flat Lines mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeShaded)) {
		settings.displayMode = RenderingConfig::DisplayMode::Solid;
		settings.showEdges = false;
		m_viewer->setDisplaySettings(settings);
		LOG_INF_S("RenderModeListener: Set to Shaded mode");
		return CommandResult(true, "Shaded mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeShadedWireframe)) {
		settings.displayMode = RenderingConfig::DisplayMode::SolidWireframe;
		settings.showEdges = true;
		m_viewer->setDisplaySettings(settings);
		LOG_INF_S("RenderModeListener: Set to Shaded+Wireframe mode");
		return CommandResult(true, "Shaded+Wireframe mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeHiddenLine)) {
		settings.displayMode = RenderingConfig::DisplayMode::HiddenLine;
		settings.showEdges = true;
		m_viewer->setDisplaySettings(settings);
		LOG_INF_S("RenderModeListener: Set to Hidden Line mode");
		return CommandResult(true, "Hidden Line mode enabled", commandType);
	}

	return CommandResult(false, "Unknown command type", commandType);
}

bool RenderModeListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == cmd::to_string(cmd::CommandType::RenderModeNoShading) ||
		commandType == cmd::to_string(cmd::CommandType::RenderModePoints) ||
		commandType == cmd::to_string(cmd::CommandType::RenderModeWireframe) ||
		commandType == cmd::to_string(cmd::CommandType::RenderModeFlatLines) ||
		commandType == cmd::to_string(cmd::CommandType::RenderModeShaded) ||
		commandType == cmd::to_string(cmd::CommandType::RenderModeShadedWireframe) ||
		commandType == cmd::to_string(cmd::CommandType::RenderModeHiddenLine);
}

std::string RenderModeListener::getListenerName() const
{
	return "RenderModeListener";
}
