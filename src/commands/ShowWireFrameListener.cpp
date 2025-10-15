#include "ShowWireFrameListener.h"
#include "OCCViewer.h"
#include "WireframeParamDialog.h"
#include "FlatFrame.h"
#include <wx/wx.h>

ShowWireFrameListener::ShowWireFrameListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowWireFrameListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);

	// Toggle logic: if currently enabled -> disable; if disabled -> open param dialog
	const bool currentlyEnabled = m_viewer->isWireframeMode();
	if (!currentlyEnabled) {
		// Log dialog opening
		if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
			if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
				flatFrame->appendMessage("Opening Wireframe Parameter Dialog...");
			}
		}

		WireframeParamDialog dialog(wxGetTopLevelParent(wxWindow::FindFocus()));
		if (dialog.ShowModal() == wxID_OK) {
			// Get parameters from dialog
			wxColour edgeCol = dialog.getEdgeColor();
			double edgeWidth = dialog.getEdgeWidth();
			int edgeStyle = dialog.getEdgeStyle();
			bool showOnlyNew = dialog.getShowOnlyNew();

			// Log parameters
			if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
				if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
					wxString styleName;
					switch (edgeStyle) {
						case 0: styleName = "Solid"; break;
						case 1: styleName = "Dashed"; break;
						case 2: styleName = "Dotted"; break;
						case 3: styleName = "Dash-Dot"; break;
						default: styleName = "Unknown"; break;
					}
					wxString paramMsg = wxString::Format("Wireframe parameters: EdgeWidth=%.1f, Style=%s, ShowOnlyNew=%s",
						edgeWidth, styleName.mb_str(), showOnlyNew ? "Yes" : "No");
					flatFrame->appendMessage(paramMsg);
					flatFrame->appendMessage("Starting wireframe mode with custom parameters...");
				}
			}

			// Enable wireframe with parameters
			Quantity_Color qcol(edgeCol.Red() / 255.0, edgeCol.Green() / 255.0, edgeCol.Blue() / 255.0, Quantity_TOC_RGB);
			m_viewer->setWireframeMode(true);
			m_viewer->applyWireframeAppearance(qcol, edgeWidth, edgeStyle, showOnlyNew);
			return CommandResult(true, "Wireframe mode enabled with custom parameters", commandType);
		}
		else {
			// User cancelled, don't enable wireframe
			if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
				if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
					flatFrame->appendMessage("Wireframe mode dialog cancelled by user");
				}
			}
			return CommandResult(true, "Wireframe mode display cancelled", commandType);
		}
	}
	else {
		// Disable wireframe
		if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
			if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
				flatFrame->appendMessage("Wireframe mode disabled");
			}
		}
		m_viewer->setWireframeMode(false);
		return CommandResult(true, "Wireframe mode disabled", commandType);
	}
}

bool ShowWireFrameListener::canHandleCommand(const std::string& commandType) const {
	// Ensure only wireframe toggling is handled here
	return commandType == cmd::to_string(cmd::CommandType::ToggleWireframe);
}