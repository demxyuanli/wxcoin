#include "ShowMeshEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "MeshEdgesParamDialog.h"
#include "FlatFrame.h"
#include <wx/wx.h>

ShowMeshEdgesListener::ShowMeshEdgesListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowMeshEdgesListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) {
		return CommandResult(false, "OCCViewer not available", commandType);
	}

	// Toggle logic: if currently enabled -> disable; if disabled -> open param dialog
	const bool currentlyEnabled = m_viewer->isEdgeTypeEnabled(EdgeType::Mesh);
	if (!currentlyEnabled) {
		// Log dialog opening
		if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
			if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
				flatFrame->appendMessage("Opening Mesh Edges Parameter Dialog...");
			}
		}

		MeshEdgesParamDialog dialog(wxGetTopLevelParent(wxWindow::FindFocus()));
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
					wxString paramMsg = wxString::Format("Mesh edge parameters: EdgeWidth=%.1f, Style=%s, ShowOnlyNew=%s",
						edgeWidth, styleName.mb_str(), showOnlyNew ? "Yes" : "No");
					flatFrame->appendMessage(paramMsg);
					flatFrame->appendMessage("Starting mesh edges display with custom parameters...");
				}
			}

			// Enable mesh edges with parameters
			Quantity_Color qcol(edgeCol.Red() / 255.0, edgeCol.Green() / 255.0, edgeCol.Blue() / 255.0, Quantity_TOC_RGB);
			m_viewer->setShowMeshEdges(true);
			m_viewer->applyMeshEdgeAppearance(qcol, edgeWidth, edgeStyle, showOnlyNew);
			return CommandResult(true, "Mesh edges shown with custom parameters", commandType);
		}
		else {
			// User cancelled, don't enable mesh edges
			if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
				if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
					flatFrame->appendMessage("Mesh edges dialog cancelled by user");
				}
			}
			return CommandResult(true, "Mesh edges display cancelled", commandType);
		}
	}
	else {
		// Disable mesh edges
		if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
			if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
				flatFrame->appendMessage("Mesh edges disabled");
			}
		}
		m_viewer->setShowMeshEdges(false);
		return CommandResult(true, "Mesh edges hidden", commandType);
	}
}

bool ShowMeshEdgesListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ShowMeshEdges);
}