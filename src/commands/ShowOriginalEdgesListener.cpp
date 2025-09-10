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

	// Open parameter dialog
	OriginalEdgesParamDialog dialog(nullptr);
	if (dialog.ShowModal() == wxID_OK) {
		// Get parameters from dialog
		double samplingDensity = dialog.getSamplingDensity();
		double minLength = dialog.getMinLength();
		bool showLinesOnly = dialog.getShowLinesOnly();
		wxColour edgeColor = dialog.getEdgeColor();
		double edgeWidth = dialog.getEdgeWidth();

		LOG_INF_S("Original edges parameters: density=" + std::to_string(samplingDensity) + 
			", minLength=" + std::to_string(minLength) + 
			", linesOnly=" + std::string(showLinesOnly ? "true" : "false") +
			", width=" + std::to_string(edgeWidth));

		// Apply parameters to viewer
		m_viewer->setOriginalEdgesParameters(samplingDensity, minLength, showLinesOnly, edgeColor, edgeWidth);
		
		// Toggle original edges display
		const bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::Original);
		m_viewer->setShowOriginalEdges(show);
		
		return CommandResult(true, show ? "Original edges shown with new parameters" : "Original edges hidden", commandType);
	}
	
	return CommandResult(false, "Original edges dialog cancelled", commandType);
}

bool ShowOriginalEdgesListener::canHandleCommand(const std::string& commandType) const {
	return commandType == cmd::to_string(cmd::CommandType::ShowOriginalEdges);
}