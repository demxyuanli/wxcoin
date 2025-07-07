#include "ShowNormalsListener.h"
#include "OCCViewer.h"

ShowNormalsListener::ShowNormalsListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowNormalsListener::executeCommand(const std::string& commandType,
                                                  const std::unordered_map<std::string, std::string>&) {
    if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
    bool show = !m_viewer->isShowNormals();
    m_viewer->setShowNormals(show);
    return CommandResult(true, show ? "Normals shown" : "Normals hidden", commandType);
}

bool ShowNormalsListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowNormals);
} 