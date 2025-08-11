#include "ShowOriginalEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"

ShowOriginalEdgesListener::ShowOriginalEdgesListener(OCCViewer* viewer): m_viewer(viewer) {}

CommandResult ShowOriginalEdgesListener::executeCommand(const std::string& commandType,
                                                        const std::unordered_map<std::string, std::string>&) {
    if (!m_viewer) {
        return CommandResult(false, "OCCViewer not available", commandType);
    }
    const bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::Original);
    m_viewer->setShowOriginalEdges(show);
    return CommandResult(true, show ? "Original edges shown" : "Original edges hidden", commandType);
}

bool ShowOriginalEdgesListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowOriginalEdges);
} 