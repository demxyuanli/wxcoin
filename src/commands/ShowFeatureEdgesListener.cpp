#include "ShowFeatureEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "FeatureEdgesParamDialog.h"
#include "FlatFrame.h"
#include <wx/wx.h>

ShowFeatureEdgesListener::ShowFeatureEdgesListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowFeatureEdgesListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) {
		return CommandResult(false, "OCCViewer not available", commandType);
	}

	// Toggle logic: if currently enabled -> disable; if disabled -> open param dialog
	const bool currentlyEnabled = m_viewer->isEdgeTypeEnabled(EdgeType::Feature);
	if (!currentlyEnabled) {
		// Log dialog opening
		if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
			if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
				flatFrame->appendMessage("Opening Feature Edges Parameter Dialog...");
			}
		}

		FeatureEdgesParamDialog dialog(wxGetTopLevelParent(wxWindow::FindFocus()));
		if (dialog.ShowModal() == wxID_OK) {
			// Get parameters from dialog
			double angle = dialog.getAngle();
			double minLength = dialog.getMinLength();
			bool onlyConvex = dialog.getOnlyConvex();
			bool onlyConcave = dialog.getOnlyConcave();
			wxColour edgeCol = dialog.getEdgeColor();
			double edgeWidth = dialog.getEdgeWidth();
			int edgeStyle = dialog.getEdgeStyle();
			bool edgesOnly = dialog.getEdgesOnly();

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
					wxString paramMsg = wxString::Format("Feature edge parameters: Angle=%.1f°, MinLength=%.3f, OnlyConvex=%s, OnlyConcave=%s, EdgeWidth=%.1f, Style=%s",
						angle, minLength, onlyConvex ? "Yes" : "No", onlyConcave ? "Yes" : "No", edgeWidth, styleName.mb_str());
					flatFrame->appendMessage(paramMsg);
					flatFrame->appendMessage("Starting feature edge generation...");

					// Ensure progress gauge is visible immediately
					if (auto* bar = flatFrame->GetFlatUIStatusBar()) {
						bar->SetGaugeRange(100);
						bar->SetGaugeValue(0);
						bar->EnableProgressGauge(true);
					}
				}
			}

			// Enable feature edges with parameters and appearance
			Quantity_Color qcol(edgeCol.Red() / 255.0, edgeCol.Green() / 255.0, edgeCol.Blue() / 255.0, Quantity_TOC_RGB);
			m_viewer->setShowFeatureEdges(true, angle, minLength, onlyConvex, onlyConcave, qcol, edgeWidth);
			// Apply additional appearance settings (style, edges-only)
			m_viewer->applyFeatureEdgeAppearance(qcol, edgeWidth, edgeStyle, edgesOnly);
			return CommandResult(true, "Feature edges shown with custom parameters", commandType);
		}
		else {
			// User cancelled, don't enable feature edges
			if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
				if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
					flatFrame->appendMessage("Feature edges dialog cancelled by user");
				}
			}
			return CommandResult(true, "Feature edges display cancelled", commandType);
		}
	}
	else {
		// Disable feature edges
		if (auto* frame = wxGetTopLevelParent(wxWindow::FindFocus())) {
			if (auto* flatFrame = dynamic_cast<FlatFrame*>(frame)) {
				flatFrame->appendMessage("Feature edges disabled");
			}
		}
		m_viewer->setShowFeatureEdges(false);
		return CommandResult(true, "Feature edges hidden", commandType);
	}
}

bool ShowFeatureEdgesListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ShowFeatureEdges);
}

std::string ShowFeatureEdgesListener::getListenerName() const {
	return "ShowFeatureEdgesListener";
}