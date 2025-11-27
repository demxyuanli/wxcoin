#include "RenderModeListener.h"
#include "CommandType.h"
#include "logger/Logger.h"
#include "OCCViewer.h"
#include "config/RenderingConfig.h"
#include <wx/colour.h>
#include "EdgeTypes.h"

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

	// Create a clean base display settings that clears all mode-specific states
	// This ensures only one render mode is active at a time
	auto makeBaseDisplaySettings = [&]() {
		// Start with a copy of base configuration to ensure all fields are initialized
		RenderingConfig::DisplaySettings result = baseConfigSettings;
		// Clear all mode-specific states to ensure mutual exclusivity
		result.displayMode = RenderingConfig::DisplayMode::Solid;
		result.showEdges = false;
		result.showVertices = false;
		result.showPointView = false;
		result.showSolidWithPointView = true;
		return result;
	};

	auto applyDisplaySettings = [&](const RenderingConfig::DisplaySettings& newSettings, const char* modeLabel) {
		m_viewer->setDisplaySettings(newSettings);
		renderingConfig.setDisplaySettings(newSettings);
		LOG_INF_S(std::string("RenderModeListener: Set to ") + modeLabel);
	};

	// Apply shading mode and ensure it's properly set
	auto applyShadingMode = [&](RenderingConfig::ShadingMode mode, bool smoothNormals) {
		shadingSettings.shadingMode = mode;
		shadingSettings.smoothNormals = smoothNormals;
		renderingConfig.setShadingSettings(shadingSettings);
	};

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeNoShading)) {
		auto settings = makeBaseDisplaySettings();
		settings.displayMode = RenderingConfig::DisplayMode::NoShading;
		settings.showEdges = true;  // Enable original edges display for No Shading mode
		applyShadingMode(RenderingConfig::ShadingMode::Flat, false);
		applyDisplaySettings(settings, "NoShading mode");
		// Explicitly enable original edges display with default parameters
		// Use default parameters: samplingDensity=80.0, minLength=0.01, black color for edges
		m_viewer->setOriginalEdgesParameters(
			80.0,  // samplingDensity
			0.01,  // minLength
			false, // showLinesOnly
			wxColour(0, 0, 0), // black color for edges
			1.0,   // width
			false, // highlightIntersectionNodes
			wxColour(255, 0, 0), // intersectionNodeColor (not used)
			3.0,   // intersectionNodeSize (not used)
			IntersectionNodeShape::Point // intersectionNodeShape (not used)
		);
		m_viewer->setShowOriginalEdges(true);
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
