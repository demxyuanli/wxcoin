#include "RenderModeListener.h"
#include "CommandType.h"
#include "logger/Logger.h"
#include "OCCViewer.h"
#include "config/RenderingConfig.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/GeometryRenderContext.h"
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

	// Preserve existing display settings (like GlobalSettingsPanel::OnMainApply)
	// Only update display mode and mode-specific settings, keep other settings intact
	auto getDisplaySettingsForMode = [&](RenderingConfig::DisplayMode targetMode) {
		// Start with current settings to preserve all existing states
		RenderingConfig::DisplaySettings result = baseConfigSettings;
		result.displayMode = targetMode;
		
		// Try to get configuration from DisplayModeConfigFactory
		// If failed, use default values
		bool configLoaded = false;
		try {
			// Create default context for config retrieval
			GeometryRenderContext defaultContext;
			defaultContext.material.ambientColor = Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB);
			defaultContext.material.diffuseColor = Quantity_Color(0.95, 0.95, 0.95, Quantity_TOC_RGB);
			defaultContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
			defaultContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
			defaultContext.material.shininess = 50.0;
			defaultContext.material.transparency = 0.0;
			defaultContext.display.wireframeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
			defaultContext.display.wireframeWidth = 1.0;
			// For Points mode, default to points only (no surface)
			if (targetMode == RenderingConfig::DisplayMode::Points) {
				defaultContext.display.showSolidWithPointView = false;
			}
			
			// Get config from factory
			DisplayModeConfig config = DisplayModeConfigFactory::getConfig(targetMode, defaultContext);
			configLoaded = true;
			
			// Map config node requirements to DisplaySettings flags
			// Geometry data types: surface, original edge, mesh edge, point
			result.showSurface = config.nodes.requireSurface;
			result.showOriginalEdges = config.nodes.requireOriginalEdges;
			result.showMeshEdges = config.nodes.requireMeshEdges;
			result.showPointView = config.nodes.requirePoints;
		} catch (...) {
			// If config loading fails, use default values based on mode
			configLoaded = false;
		}
		
		// If config not loaded, use default values
		if (!configLoaded) {
			switch (targetMode) {
			case RenderingConfig::DisplayMode::NoShading:
				result.showSurface = true;
				result.showOriginalEdges = true;
				result.showMeshEdges = false;
				result.showPointView = false;
				break;
			case RenderingConfig::DisplayMode::Points:
				result.showSurface = false;  // Default: points only
				result.showOriginalEdges = false;
				result.showMeshEdges = false;
				result.showPointView = true;
				break;
			case RenderingConfig::DisplayMode::Wireframe:
				result.showSurface = false;
				result.showOriginalEdges = true;
				result.showMeshEdges = false;
				result.showPointView = false;
				break;
			case RenderingConfig::DisplayMode::FlatLines:
				result.showSurface = true;
				result.showOriginalEdges = true;
				result.showMeshEdges = false;
				result.showPointView = false;
				break;
			case RenderingConfig::DisplayMode::Solid:
				result.showSurface = true;
				result.showOriginalEdges = false;
				result.showMeshEdges = false;
				result.showPointView = false;
				break;
			case RenderingConfig::DisplayMode::Transparent:
				result.showSurface = true;
				result.showOriginalEdges = false;
				result.showMeshEdges = false;
				result.showPointView = false;
				break;
			case RenderingConfig::DisplayMode::HiddenLine:
				result.showSurface = true;
				result.showOriginalEdges = false;
				result.showMeshEdges = true;
				result.showPointView = false;
				break;
			default:
				result.showSurface = true;
				result.showOriginalEdges = false;
				result.showMeshEdges = false;
				result.showPointView = false;
				break;
			}
		}
		
		return result;
	};

	// Apply surface visibility to all geometries
	auto applySurfaceVisibility = [&](bool showSurface) {
		auto geometries = m_viewer->getAllGeometry();
		for (auto& geometry : geometries) {
			if (geometry) {
				geometry->setFacesVisible(showSurface);
			}
		}
	};
	
	auto applyDisplaySettings = [&](const RenderingConfig::DisplaySettings& newSettings, const char* modeLabel) {
		m_viewer->setDisplaySettings(newSettings);
		renderingConfig.setDisplaySettings(newSettings);
		// Explicitly set surface visibility from DisplaySettings
		applySurfaceVisibility(newSettings.showSurface);
		LOG_INF_S(std::string("RenderModeListener: Set to ") + modeLabel);
	};

	// Apply shading mode and ensure it's properly set
	auto applyShadingMode = [&](RenderingConfig::ShadingMode mode, bool smoothNormals) {
		shadingSettings.shadingMode = mode;
		shadingSettings.smoothNormals = smoothNormals;
		renderingConfig.setShadingSettings(shadingSettings);
	};

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeNoShading)) {
		auto settings = getDisplaySettingsForMode(RenderingConfig::DisplayMode::NoShading);
		applyShadingMode(RenderingConfig::ShadingMode::Flat, false);
		applyDisplaySettings(settings, "NoShading mode");
		// Set original edges parameters and enable display
		// requireOriginalEdges=true: showOriginalEdges=true
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
		auto settings = getDisplaySettingsForMode(RenderingConfig::DisplayMode::Points);
		applyDisplaySettings(settings, "Points mode");
		// requireOriginalEdges=false: showOriginalEdges=false (already set in getDisplaySettingsForMode)
		m_viewer->setShowOriginalEdges(false);
		return CommandResult(true, "Points mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeWireframe)) {
		auto settings = getDisplaySettingsForMode(RenderingConfig::DisplayMode::Wireframe);
		applyDisplaySettings(settings, "Wireframe mode");
		// requireOriginalEdges=true: showOriginalEdges=true
		m_viewer->setShowOriginalEdges(true);
		return CommandResult(true, "Wireframe mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeFlatLines)) {
		auto settings = getDisplaySettingsForMode(RenderingConfig::DisplayMode::FlatLines);
		applyShadingMode(RenderingConfig::ShadingMode::Flat, false);
		applyDisplaySettings(settings, "Flat Lines mode");
		// requireOriginalEdges=true: showOriginalEdges=true
		m_viewer->setShowOriginalEdges(true);
		return CommandResult(true, "Flat Lines mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeShaded)) {
		auto settings = getDisplaySettingsForMode(RenderingConfig::DisplayMode::Solid);
		applyShadingMode(RenderingConfig::ShadingMode::Smooth, true);
		applyDisplaySettings(settings, "Shaded mode");
		// requireOriginalEdges=false (default): showOriginalEdges=false (already set in getDisplaySettingsForMode)
		m_viewer->setShowOriginalEdges(false);
		return CommandResult(true, "Shaded mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeTransparency)) {
		auto settings = getDisplaySettingsForMode(RenderingConfig::DisplayMode::Transparent);
		// Ensure surface is visible for Transparent mode
		settings.showSurface = true;
		applyShadingMode(RenderingConfig::ShadingMode::Smooth, true);
		
		RenderingConfig::MaterialSettings materialSettings = renderingConfig.getMaterialSettings();
		if (materialSettings.transparency <= 0.0) {
			materialSettings.transparency = 0.5;
			renderingConfig.setMaterialSettings(materialSettings);
		}
		
		RenderingConfig::BlendSettings blendSettings = renderingConfig.getBlendSettings();
		blendSettings.blendMode = RenderingConfig::BlendMode::Alpha;
		renderingConfig.setBlendSettings(blendSettings);
		
		applyDisplaySettings(settings, "Transparent mode");
		// requireOriginalEdges=false: showOriginalEdges=false (already set in getDisplaySettingsForMode)
		m_viewer->setShowOriginalEdges(false);
		// Explicitly ensure surface visibility is set
		applySurfaceVisibility(true);
		return CommandResult(true, "Transparent mode enabled", commandType);
	}

	if (commandType == cmd::to_string(cmd::CommandType::RenderModeHiddenLine)) {
		auto settings = getDisplaySettingsForMode(RenderingConfig::DisplayMode::HiddenLine);
		applyShadingMode(RenderingConfig::ShadingMode::Flat, false);
		applyDisplaySettings(settings, "Hidden Line mode");
		// requireOriginalEdges=false: showOriginalEdges=false (already set in getDisplaySettingsForMode)
		m_viewer->setShowOriginalEdges(false);
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
		commandType == cmd::to_string(cmd::CommandType::RenderModeTransparency) ||
		commandType == cmd::to_string(cmd::CommandType::RenderModeHiddenLine);
}

std::string RenderModeListener::getListenerName() const
{
	return "RenderModeListener";
}
