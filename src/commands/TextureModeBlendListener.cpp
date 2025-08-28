#include "TextureModeBlendListener.h"
#include "OCCViewer.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>

TextureModeBlendListener::TextureModeBlendListener(wxFrame* frame, OCCViewer* viewer)
	: m_frame(frame)
	, m_viewer(viewer)
{
}

CommandResult TextureModeBlendListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	if (!m_viewer) {
		wxMessageBox("OCCViewer not available", "Error", wxOK | wxICON_ERROR);
		return CommandResult(false, "OCCViewer not available", commandType);
	}

	// Check if any objects are selected
	auto selectedGeometries = m_viewer->getSelectedGeometries();
	bool hasSelection = !selectedGeometries.empty();

	LOG_INF_S("TextureModeBlendListener: " + std::to_string(selectedGeometries.size()) + " objects selected");

	// Update RenderingConfig - this will trigger notification to update geometries
	RenderingConfig& config = RenderingConfig::getInstance();

	if (hasSelection) {
		// Apply settings to selected objects only
		LOG_INF_S("Applying Blend texture mode to " + std::to_string(selectedGeometries.size()) + " selected objects");

		// Enable texture and set a visible color texture for Blend mode
		config.setSelectedTextureEnabled(true);
		config.setSelectedTextureColor(Quantity_Color(1.0, 0.0, 1.0, Quantity_TOC_RGB)); // Bright Magenta
		config.setSelectedTextureIntensity(0.6); // Moderate intensity for blending effect
		config.setSelectedTextureMode(RenderingConfig::TextureMode::Blend);

		// Set material to a base color that will be blended with
		config.setSelectedMaterialDiffuseColor(Quantity_Color(0.2, 0.6, 0.2, Quantity_TOC_RGB)); // Green base
		config.setSelectedMaterialTransparency(0.2); // Some transparency for blend effect
	}
	else {
		// Apply settings to all objects
		LOG_INF_S("No objects selected, applying Blend texture mode to all objects");

		// Enable texture and set a visible color texture for Blend mode
		config.setTextureEnabled(true);
		config.setTextureColor(Quantity_Color(1.0, 0.0, 1.0, Quantity_TOC_RGB)); // Bright Magenta
		config.setTextureIntensity(0.6); // Moderate intensity for blending effect
		config.setTextureMode(RenderingConfig::TextureMode::Blend);

		// Set material to a base color that will be blended with
		config.setMaterialDiffuseColor(Quantity_Color(0.2, 0.6, 0.2, Quantity_TOC_RGB)); // Green base
		config.setMaterialTransparency(0.2); // Some transparency for blend effect
	}

	// Force notification to ensure geometries are updated
	config.notifySettingsChanged();

	// Also directly update geometries as a fallback
	if (m_viewer) {
		auto geometries = hasSelection ? selectedGeometries : m_viewer->getAllGeometry();
		LOG_INF_S("Directly updating " + std::to_string(geometries.size()) + " geometries for Blend mode");
		for (auto& geometry : geometries) {
			geometry->updateFromRenderingConfig();
		}

		// Force immediate refresh after direct update
		if (m_frame) {
			m_frame->CallAfter([this]() {
				m_frame->Refresh(true);
				m_frame->Update();
				LOG_INF_S("Forced delayed refresh for Blend mode");
				});
		}
	}

	LOG_INF_S("Texture mode set to Blend via RenderingConfig");

	// Add test feedback
	std::string feedbackMessage = "Blend texture mode applied to " +
		(hasSelection ? std::to_string(selectedGeometries.size()) + " selected objects" : "all objects") +
		" (Magenta texture blending with Green base)";

	// Show feedback to user
	if (m_frame) {
		wxMessageBox(feedbackMessage, "Texture Mode Applied", wxOK | wxICON_INFORMATION);
	}

	// Show detailed test feedback in logs
	config.showTestFeedback();

	return CommandResult(true, feedbackMessage, commandType);
}

bool TextureModeBlendListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == cmd::to_string(cmd::CommandType::TextureModeBlend);
}

std::string TextureModeBlendListener::getListenerName() const
{
	return "TextureModeBlendListener";
}