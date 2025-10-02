#include "ShowNormalsListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "logger/Logger.h"

ShowNormalsListener::ShowNormalsListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowNormalsListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {
    if (!m_viewer) {
        return CommandResult(false, "OCCViewer not available", commandType);
    }
    
    try {
        // Toggle normal display based on current state
        bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::NormalLine);
        m_viewer->setShowNormals(show);
        
        LOG_INF_S("Normal display toggled to: " + std::string(show ? "ON" : "OFF"));
        return CommandResult(true, show ? "Node normals shown" : "Node normals hidden", commandType);
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception during normal display toggle: " + std::string(e.what()));
        return CommandResult(false, "Error toggling normal display: " + std::string(e.what()), commandType);
    }
}

bool ShowNormalsListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowNormals);
}
