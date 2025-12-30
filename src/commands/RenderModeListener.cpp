#include "RenderModeListener.h"
#include "CommandType.h"
#include "logger/Logger.h"
#include "OCCViewer.h"
#include "config/RenderingConfig.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/GeometryRenderContext.h"
#include "opencascade/geometry/helper/DisplayModeConfigDialog.h"
#include "widgets/FlatProgressBar.h"
#include "config/ThemeManager.h"
#include <wx/colour.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/app.h>
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

	if (commandType == cmd::to_string(cmd::CommandType::DisplayModeConfig)) {
		LOG_INF_S("RenderModeListener: DisplayModeConfig command received");
		// Get parent window for progress dialog and config dialog
		wxWindow* parentWindow = wxGetActiveWindow();
		if (!parentWindow) {
			parentWindow = wxTheApp->GetTopWindow();
		}
		if (!parentWindow) {
			LOG_ERR_S("RenderModeListener: Parent window not available");
			return CommandResult(false, "Parent window not available", commandType);
		}
		LOG_INF_S("RenderModeListener: Parent window found, creating progress dialog");

		// Define all display modes to load
		static const RenderingConfig::DisplayMode modes[] = {
			RenderingConfig::DisplayMode::NoShading,
			RenderingConfig::DisplayMode::Points,
			RenderingConfig::DisplayMode::Wireframe,
			RenderingConfig::DisplayMode::Solid,
			RenderingConfig::DisplayMode::FlatLines,
			RenderingConfig::DisplayMode::Transparent,
			RenderingConfig::DisplayMode::HiddenLine,
			RenderingConfig::DisplayMode::Custom
		};
		const int modeCount = 8;

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
		defaultContext.display.showSolidWithPointView = true;

		// Create flat style progress dialog with theme-adapted colors
		wxDialog* progressDialog = new wxDialog(parentWindow, wxID_ANY, "Loading Display Modes", 
		                                        wxDefaultPosition, wxSize(400, 150),
		                                        wxNO_BORDER | wxFRAME_SHAPED);
		
		// Use PanelDialogBgColour for dialog background, with fallback chain
		wxColour bgColor = CFG_COLOUR("PanelDialogBgColour");
		if (!bgColor.IsOk() || (bgColor.Red() == 255 && bgColor.Green() == 0 && bgColor.Blue() == 0)) {
			bgColor = CFG_COLOUR("PanelPopupBgColour");
			if (!bgColor.IsOk() || (bgColor.Red() == 255 && bgColor.Green() == 0 && bgColor.Blue() == 0)) {
				bgColor = CFG_COLOUR("SecondaryBackgroundColour");
				if (!bgColor.IsOk() || (bgColor.Red() == 255 && bgColor.Green() == 0 && bgColor.Blue() == 0)) {
					bgColor = wxColour(250, 250, 250);
				}
			}
		}
		
		wxPanel* contentPanel = new wxPanel(progressDialog, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
		contentPanel->SetBackgroundColour(bgColor);
		contentPanel->SetDoubleBuffered(true);
		
		progressDialog->SetBackgroundColour(bgColor);
		progressDialog->SetDoubleBuffered(true);
		
		wxBoxSizer* progressSizer = new wxBoxSizer(wxVERTICAL);
		
		wxStaticText* progressLabel = new wxStaticText(contentPanel, wxID_ANY, 
		                                               "Loading all display mode configurations...");
		wxColour textColor = CFG_COLOUR("PrimaryTextColour");
		if (!textColor.IsOk() || (textColor.Red() == 255 && textColor.Green() == 0 && textColor.Blue() == 0)) {
			textColor = wxColour(100, 100, 100);
		}
		progressLabel->SetForegroundColour(textColor);
		progressSizer->Add(progressLabel, 0, wxALL | wxALIGN_CENTER, 15);
		
		// Total steps: modeCount (config loading) + 6 (dialog initialization)
		static const int dialogInitSteps = 6;
		int totalSteps = modeCount + dialogInitSteps;
		
		FlatProgressBar* progressBar = new FlatProgressBar(contentPanel, wxID_ANY, 0, 0, 
		                                                    totalSteps,
		                                                    wxDefaultPosition, wxSize(350, 25),
		                                                    FlatProgressBar::ProgressBarStyle::MODERN_LINEAR);
		progressBar->SetShowPercentage(true);
		progressBar->SetTextFollowProgress(true);
		progressBar->SetCornerRadius(12);
		progressSizer->Add(progressBar, 0, wxALL | wxALIGN_CENTER, 15);
		
		wxStaticText* statusLabel = new wxStaticText(contentPanel, wxID_ANY, "");
		statusLabel->SetForegroundColour(textColor);
		progressSizer->Add(statusLabel, 0, wxALL | wxALIGN_LEFT | wxLEFT | wxRIGHT, 10);
		
		contentPanel->SetSizer(progressSizer);
		
		wxBoxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);
		dialogSizer->Add(contentPanel, 1, wxEXPAND);
		progressDialog->SetSizer(dialogSizer);
		progressDialog->Layout();
		progressDialog->CentreOnParent();
		progressDialog->Show();
		wxSafeYield();
		LOG_INF_S("RenderModeListener: Progress dialog shown");

		// Load all display mode configurations
		std::map<RenderingConfig::DisplayMode, DisplayModeConfig> loadedConfigs;
		int currentProgress = 0;
		
		for (int i = 0; i < modeCount; ++i) {
			RenderingConfig::DisplayMode mode = modes[i];
			
			progressBar->SetValue(currentProgress);
			
			wxString modeName;
			switch (mode) {
			case RenderingConfig::DisplayMode::NoShading:
				modeName = "No Shading";
				break;
			case RenderingConfig::DisplayMode::Points:
				modeName = "Points";
				break;
			case RenderingConfig::DisplayMode::Wireframe:
				modeName = "Wireframe";
				break;
			case RenderingConfig::DisplayMode::Solid:
				modeName = "Solid";
				break;
			case RenderingConfig::DisplayMode::FlatLines:
				modeName = "Flat Lines";
				break;
			case RenderingConfig::DisplayMode::Transparent:
				modeName = "Transparent";
				break;
			case RenderingConfig::DisplayMode::HiddenLine:
				modeName = "Hidden Line";
				break;
			case RenderingConfig::DisplayMode::Custom:
				modeName = "Custom";
				break;
			}
			
			statusLabel->SetLabel("Loading: " + modeName);
			progressDialog->Refresh();
			wxSafeYield();

			loadedConfigs[mode] = DisplayModeConfigFactory::getConfig(mode, defaultContext);

			currentProgress++;
		}

		progressBar->SetValue(modeCount);
		statusLabel->SetLabel("Display mode configurations loaded successfully");
		progressDialog->Refresh();
		wxSafeYield();
		
		LOG_INF_S("RenderModeListener: Configurations loaded, creating config dialog with progress callback");

		// Get current display mode
		const RenderingConfig& renderingConfig = RenderingConfig::getInstance();
		const auto displaySettings = renderingConfig.getDisplaySettings();
		
		// Create progress callback to update progress dialog during config dialog initialization
		DisplayModeConfigDialog::ProgressCallback progressCallback = 
			[progressBar, statusLabel, progressDialog, modeCount, totalSteps](int current, int total, const wxString& message) {
				if (progressBar && statusLabel && progressDialog) {
					// Map config dialog progress (1-6) to progress bar (modeCount to modeCount+6)
					// current is 1-based, so we add modeCount to get the absolute position
					int currentStep = modeCount + current;
					
					progressBar->SetValue(currentStep);
					statusLabel->SetLabel(message);
					progressDialog->Refresh();
					wxSafeYield();
				}
			};
		
		// Create and show config dialog with pre-loaded configurations and progress callback
		try {
			DisplayModeConfigDialog dialog(parentWindow, displaySettings.displayMode, loadedConfigs, progressCallback);
			LOG_INF_S("RenderModeListener: Config dialog created, showing modal");
			
			// Close progress dialog after config dialog is fully initialized
			wxMilliSleep(200);
			progressDialog->Destroy();
			LOG_INF_S("RenderModeListener: Progress dialog destroyed after config dialog initialization");
			
			dialog.ShowModal();
			LOG_INF_S("RenderModeListener: Config dialog closed");
		} catch (const std::exception& e) {
			if (progressDialog) {
				progressDialog->Destroy();
			}
			LOG_ERR_S("RenderModeListener: Exception creating/config dialog: " + std::string(e.what()));
			return CommandResult(false, "Exception: " + std::string(e.what()), commandType);
		} catch (...) {
			if (progressDialog) {
				progressDialog->Destroy();
			}
			LOG_ERR_S("RenderModeListener: Unknown exception creating/config dialog");
			return CommandResult(false, "Unknown exception", commandType);
		}
		
		return CommandResult(true, "Display mode config dialog opened", commandType);
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
		commandType == cmd::to_string(cmd::CommandType::RenderModeHiddenLine) ||
		commandType == cmd::to_string(cmd::CommandType::DisplayModeConfig);
}

std::string RenderModeListener::getListenerName() const
{
	return "RenderModeListener";
}
