#include "ShowFeatureEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"

ShowFeatureEdgesListener::ShowFeatureEdgesListener(OCCViewer* viewer): m_viewer(viewer) {}

CommandResult ShowFeatureEdgesListener::executeCommand(const std::string& commandType,
                                                      const std::unordered_map<std::string, std::string>&) {
    if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
    bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::Feature);
    m_viewer->setShowFeatureEdges(show);
    return CommandResult(true, show ? "Feature edges shown" : "Feature edges hidden", commandType);
}

bool ShowFeatureEdgesListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowFeatureEdges);
}

std::string ShowFeatureEdgesListener::getListenerName() const {
    return "ShowFeatureEdgesListener";
} 