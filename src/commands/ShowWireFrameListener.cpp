#include "ShowWireFrameListener.h"
#include "OCCViewer.h"

ShowWireFrameListener::ShowWireFrameListener(OCCViewer* viewer): m_viewer(viewer) {}

CommandResult ShowWireFrameListener::executeCommand(const std::string& commandType,
                                                    const std::unordered_map<std::string, std::string>&) {
    if (!m_viewer) return CommandResult(false, "OCCViewer not available", commandType);
    bool show = !m_viewer->isWireframeMode();
    m_viewer->setWireframeMode(show);
    return CommandResult(true, show ? "Wireframe mode enabled" : "Wireframe mode disabled", commandType);
}

bool ShowWireFrameListener::canHandleCommand(const std::string& commandType) const {
    // Ensure only wireframe toggling is handled here
    return commandType == cmd::to_string(cmd::CommandType::ToggleWireframe);
} 