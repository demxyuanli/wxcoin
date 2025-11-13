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

	RenderingConfig& renderingConfig = RenderingConfig::getInstance();
	RenderingConfig::DisplaySettings baseConfigSettings = renderingConfig.getDisplaySettings();
	RenderingConfig::ShadingSettings shadingSettings = renderingConfig.getShadingSettings();

	auto makeBaseDisplaySettings = [&]() {
		RenderingConfig::DisplaySettings result = baseConfigSettings;
		result.displayMode = RenderingConfig::DisplayMode::Solid;
		result.showEdges = false;
		result.showVertices = false;
		result.showPointView = false;
		result.showSolidWithPointView = true;
		result.pointSize = baseConfigSettings.pointSize;
		result.showPointView = false;
		return result;
	};

	auto applyDisplaySettings = [&](const RenderingConfig::DisplaySettings& newSettings, const char* modeLabel) {
		m_viewer->setDisplaySettings(newSettings);
		renderingConfig.setDisplaySettings(newSettings);
		LOG_INF_S(std::string("RenderModeListener: Set to ") + modeLabel);
	};

	auto applyShadingMode = [&](RenderingConfig::ShadingMode mode, bool smoothNormals) {
		if (shadingSettings.shadingMode != mode || shadingSettings.smoothNormals != smoothNormals) {
			shadingSettings.shadingMode = mode;
			shadingSettings.smoothNormals = smoothNormals;
			renderingConfig.setShadingSettings(shadingSettings);
		}
	};

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeNoShading)) {
		auto settings = makeBaseDisplaySettings();
		settings.displayMode = RenderingConfig::DisplayMode::NoShading;
		applyShadingMode(RenderingConfig::ShadingMode::Flat, false);
		applyDisplaySettings(settings, "NoShading mode");
		return CommandResult(true, "NoShading mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModePoints)) {
		auto settings = makeBaseDisplaySettings();
		settings.displayMode = RenderingConfig::DisplayMode::Points;
		settings.showPointView = true;
		settings.showSolidWithPointView = false;
		applyDisplaySettings(settings, "Points mode");
		return CommandResult(true, "Points mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeWireframe)) {
		auto settings = makeBaseDisplaySettings();
		settings.displayMode = RenderingConfig::DisplayMode::Wireframe;
		// Wireframe mode only shows edges, no faces to shade
		applyDisplaySettings(settings, "Wireframe mode");
		return CommandResult(true, "Wireframe mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeFlatLines)) {
		auto settings = makeBaseDisplaySettings();
		settings.displayMode = RenderingConfig::DisplayMode::SolidWireframe;
		settings.showEdges = true;
		applyShadingMode(RenderingConfig::ShadingMode::Flat, false);
		applyDisplaySettings(settings, "Flat Lines mode");
		return CommandResult(true, "Flat Lines mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeShaded)) {
		auto settings = makeBaseDisplaySettings();
		settings.displayMode = RenderingConfig::DisplayMode::Solid;
		applyShadingMode(RenderingConfig::ShadingMode::Smooth, true);
		applyDisplaySettings(settings, "Shaded mode");
		return CommandResult(true, "Shaded mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeShadedWireframe)) {
		auto settings = makeBaseDisplaySettings();
		settings.displayMode = RenderingConfig::DisplayMode::SolidWireframe;
		settings.showEdges = true;
		applyShadingMode(RenderingConfig::ShadingMode::Smooth, true);
		applyDisplaySettings(settings, "Shaded+Wireframe mode");
		return CommandResult(true, "Shaded+Wireframe mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeHiddenLine)) {
		auto settings = makeBaseDisplaySettings();
		settings.displayMode = RenderingConfig::DisplayMode::HiddenLine;
		applyShadingMode(RenderingConfig::ShadingMode::Flat, false);
		applyDisplaySettings(settings, "Hidden Line mode");
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
