#include "ShowEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"

ShowEdgesListener::ShowEdgesListener(OCCViewer* viewer): m_viewer(viewer) {}

CommandResult ShowEdgesListener::executeCommand(const std::string& commandType,
                                                const std::unordered_map<std::string, std::string>&) {
    if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
    bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::Feature);
    m_viewer->setShowFeatureEdges(show);
    return CommandResult(true, show ? "Feature edges shown" : "Feature edges hidden", commandType);
}

bool ShowEdgesListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowEdges);
} 
