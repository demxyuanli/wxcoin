#include "ShowOriginalEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "OriginalEdgesParamDialog.h"
#include "logger/Logger.h"

ShowOriginalEdgesListener::ShowOriginalEdgesListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowOriginalEdgesListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>&) {
	if (!m_viewer) {
		return CommandResult(false, "OCCViewer not available", commandType);
	}

	// Toggle logic: if currently enabled -> disable; if disabled -> open param dialog
	const bool currentlyEnabled = m_viewer->isEdgeTypeEnabled(EdgeType::Original);
	if (!currentlyEnabled) {
		// Open parameter dialog to get parameters
		OriginalEdgesParamDialog dialog(nullptr);
		if (dialog.ShowModal() == wxID_OK) {
		// Get parameters from dialog
		double samplingDensity = dialog.getSamplingDensity();
		double minLength = dialog.getMinLength();
		bool showLinesOnly = dialog.getShowLinesOnly();
		wxColour edgeColor = dialog.getEdgeColor();
		double edgeWidth = dialog.getEdgeWidth();
		bool highlightIntersectionNodes = dialog.getHighlightIntersectionNodes();
		wxColour intersectionNodeColor = dialog.getIntersectionNodeColor();
		double intersectionNodeSize = dialog.getIntersectionNodeSize();

		LOG_INF_S("Original edges parameters: density=" + std::to_string(samplingDensity) +
			", minLength=" + std::to_string(minLength) +
			", linesOnly=" + std::string(showLinesOnly ? "true" : "false") +
			", width=" + std::to_string(edgeWidth) +
			", highlightNodes=" + std::string(highlightIntersectionNodes ? "true" : "false") +
			", nodeSize=" + std::to_string(intersectionNodeSize));

		// Apply parameters to viewer
		m_viewer->setOriginalEdgesParameters(samplingDensity, minLength, showLinesOnly, edgeColor, edgeWidth, 
			highlightIntersectionNodes, intersectionNodeColor, intersectionNodeSize);

			// Enable original edges display
			m_viewer->setShowOriginalEdges(true);

			return CommandResult(true, "Original edges shown with new parameters", commandType);
		}
		else {
			// User cancelled, don't enable original edges
			return CommandResult(true, "Original edges display cancelled", commandType);
		}
	}
	else {
		// Disable original edges
		m_viewer->setShowOriginalEdges(false);
		return CommandResult(true, "Original edges hidden", commandType);
	}
}

bool ShowOriginalEdgesListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ShowOriginalEdges);
}