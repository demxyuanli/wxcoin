#include "ShowNormalsListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "logger/Logger.h"
#include "CommandErrorHelper.h"

ShowNormalsListener::ShowNormalsListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowNormalsListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {
    LOG_INF_S("ShowNormalsListener::executeCommand called with commandType: " + commandType);
    
    // Use unified error handling to check if OCCViewer is available
    CHECK_PTR_RETURN(m_viewer, "OCCViewer", commandType);

    // Toggle normal display based on current state
    bool currentState = m_viewer->isEdgeTypeEnabled(EdgeType::NormalLine);
    bool newState = !currentState;

    LOG_INF_S("ShowNormalsListener::executeCommand - Current state: " + std::string(currentState ? "ON" : "OFF") + 
              ", New state: " + std::string(newState ? "ON" : "OFF"));

    // Call setShowNormals with error handling
    try {
        m_viewer->setShowNormals(newState);
        LOG_INF_S("Normal display toggled to: " + std::string(newState ? "ON" : "OFF"));
    } catch (const std::exception& e) {
        return CommandErrorHelper::error("Exception occurred: " + std::string(e.what()), commandType);
    } catch (...) {
        return CommandErrorHelper::error("Unknown exception occurred", commandType);
    }

    // Return appropriate success result
    if (newState) {
        RETURN_SUCCESS("Node normals shown", commandType);
    } else {
        RETURN_SUCCESS("Node normals hidden", commandType);
    }
}

bool ShowNormalsListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowNormals);
}
