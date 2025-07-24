#include "ShowMeshEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"

ShowMeshEdgesListener::ShowMeshEdgesListener(OCCViewer* viewer): m_viewer(viewer) {}

CommandResult ShowMeshEdgesListener::executeCommand(const std::string& commandType,
                                                    const std::unordered_map<std::string, std::string>&) {
    if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
    bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::Mesh);
    m_viewer->setShowMeshEdges(show);
    return CommandResult(true, show ? "Mesh edges shown" : "Mesh edges hidden", commandType);
}

bool ShowMeshEdgesListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowMeshEdges);
} 