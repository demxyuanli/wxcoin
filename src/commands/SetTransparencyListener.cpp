#include "SetTransparencyListener.h"
#include "CommandType.h"
#include "TransparencyDialog.h"
#include "logger/Logger.h"

SetTransparencyListener::SetTransparencyListener(wxFrame* frame, OCCViewer* viewer)
	: m_frame(frame), m_viewer(viewer)
{
}

CommandResult SetTransparencyListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	if (!m_frame || !m_viewer) {
		return CommandResult(false, "Frame or OCCViewer not available", commandType);
	}

	// Get selected geometries
	auto selectedGeometries = m_viewer->getSelectedGeometries();
	bool hasSelection = !selectedGeometries.empty();

	LOG_INF_S("SetTransparencyListener: " + std::to_string(selectedGeometries.size()) + " objects selected");

	if (hasSelection) {
		// Apply transparency to selected objects only
		LOG_INF_S("Applying transparency to " + std::to_string(selectedGeometries.size()) + " selected objects");

		// Create and show transparency dialog for selected geometries
		TransparencyDialog dialog(m_frame, m_viewer, selectedGeometries);
		if (dialog.ShowModal() == wxID_OK) {
			LOG_INF_S("Transparency settings applied to " +
				std::to_string(selectedGeometries.size()) + " selected geometries");

			// Add test feedback
			std::string feedbackMessage = "Transparency settings applied to " +
				std::to_string(selectedGeometries.size()) + " selected objects";

			// Show feedback to user
			wxMessageBox(feedbackMessage, "Transparency Applied", wxOK | wxICON_INFORMATION);

			// Show detailed test feedback in logs
			RenderingConfig& config = RenderingConfig::getInstance();
			config.showTestFeedback();

			return CommandResult(true, feedbackMessage, commandType);
		}
	}
	else {
		// Apply transparency to all objects
		LOG_INF_S("No objects selected, applying transparency to all objects");

		// Create and show transparency dialog for all geometries
		TransparencyDialog dialog(m_frame, m_viewer, m_viewer->getAllGeometry());
		if (dialog.ShowModal() == wxID_OK) {
			LOG_INF_S("Transparency settings applied to all geometries");

			// Add test feedback
			std::string feedbackMessage = "Transparency settings applied to all objects";

			// Show feedback to user
			wxMessageBox(feedbackMessage, "Transparency Applied", wxOK | wxICON_INFORMATION);

			// Show detailed test feedback in logs
			RenderingConfig& config = RenderingConfig::getInstance();
			config.showTestFeedback();

			return CommandResult(true, feedbackMessage, commandType);
		}
	}

	return CommandResult(false, "Transparency dialog cancelled", commandType);
}

bool SetTransparencyListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == cmd::to_string(cmd::CommandType::SetTransparency);
}

std::string SetTransparencyListener::getListenerName() const
{
	return "SetTransparencyListener";
}