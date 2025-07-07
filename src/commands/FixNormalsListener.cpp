#include "FixNormalsListener.h"
#include "OCCViewer.h"

FixNormalsListener::FixNormalsListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult FixNormalsListener::executeCommand(const std::string& commandType,
                                                 const std::unordered_map<std::string, std::string>&) {
    if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
    m_viewer->fixNormals();
    return CommandResult(true, "Face normals fixed", commandType);
}

bool FixNormalsListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::FixNormals);
} 