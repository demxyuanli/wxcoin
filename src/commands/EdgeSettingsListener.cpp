#include "EdgeSettingsListener.h"
#include "EdgeSettingsDialog.h"
#include "OCCViewer.h"
#include "config/EdgeSettingsConfig.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/frame.h>
#include <string>

EdgeSettingsListener::EdgeSettingsListener(wxFrame* frame, OCCViewer* viewer)
	: m_frame(frame)
	, m_viewer(viewer)
{
}

CommandResult EdgeSettingsListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	if (!m_frame || !m_viewer) {
		wxMessageBox("Frame or OCCViewer not available", "Error", wxOK | wxICON_ERROR);
		return CommandResult(false, "Frame or OCCViewer not available", commandType);
	}

	// Check if any objects are selected
	auto selectedGeometries = m_viewer->getSelectedGeometries();
	bool hasSelection = !selectedGeometries.empty();

	LOG_INF_S("EdgeSettingsListener: " + std::to_string(selectedGeometries.size()) + " objects selected");

	// Create and show edge settings dialog
	EdgeSettingsDialog dialog(m_frame, m_viewer);

	if (dialog.ShowModal() == wxID_OK) {
		// Get settings from dialog
		const auto& globalSettings = dialog.getGlobalSettings();
		const auto& selectedSettings = dialog.getSelectedSettings();
		const auto& hoverSettings = dialog.getHoverSettings();

		// Show feedback
		std::ostringstream oss;
		oss << "Edge settings applied to all objects\n\n"
			<< "Global Settings:\n"
			<< "  Show Edges: " << (globalSettings.showEdges ? "Yes" : "No") << "\n"
			<< "  Edge Width: " << globalSettings.edgeWidth << "\n"
			<< "  Edge Color: " << (globalSettings.edgeColorEnabled ? "Enabled" : "Disabled") << "\n"
			<< "  Edge Style: " << globalSettings.edgeStyle << "\n"
			<< "  Edge Opacity: " << globalSettings.edgeOpacity << "\n\n"
			<< "Selected Settings:\n"
			<< "  Show Edges: " << (selectedSettings.showEdges ? "Yes" : "No") << "\n"
			<< "  Edge Width: " << selectedSettings.edgeWidth << "\n"
			<< "  Edge Color: " << (selectedSettings.edgeColorEnabled ? "Enabled" : "Disabled") << "\n"
			<< "  Edge Style: " << selectedSettings.edgeStyle << "\n"
			<< "  Edge Opacity: " << selectedSettings.edgeOpacity << "\n\n"
			<< "Hover Settings:\n"
			<< "  Show Edges: " << (hoverSettings.showEdges ? "Yes" : "No") << "\n"
			<< "  Edge Width: " << hoverSettings.edgeWidth << "\n"
			<< "  Edge Color: " << (hoverSettings.edgeColorEnabled ? "Enabled" : "Disabled") << "\n"
			<< "  Edge Style: " << hoverSettings.edgeStyle << "\n"
			<< "  Edge Opacity: " << hoverSettings.edgeOpacity;

		std::string feedbackMessage = oss.str();
		wxMessageBox(feedbackMessage, "Edge Settings Applied", wxOK | wxICON_INFORMATION);

		return CommandResult(true, "Edge settings applied to all objects", commandType);
	}

	return CommandResult(false, "Edge settings dialog cancelled", commandType);
}

bool EdgeSettingsListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == cmd::to_string(cmd::CommandType::EdgeSettings);
}