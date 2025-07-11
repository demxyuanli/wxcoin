#include "ShowEdgesListener.h"
#include "OCCViewer.h"

ShowEdgesListener::ShowEdgesListener(OCCViewer* viewer): m_viewer(viewer) {}

CommandResult ShowEdgesListener::executeCommand(const std::string& commandType,
                                                const std::unordered_map<std::string, std::string>&) {
    if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
    bool show = !m_viewer->isShowEdges();
    m_viewer->setShowEdges(show);
    return CommandResult(true, show ? "Edges shown" : "Edges hidden", commandType);
}

bool ShowEdgesListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowEdges);
} 
