#include "ShowNormalsListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "logger/Logger.h"
#include "CommandErrorHelper.h"

ShowNormalsListener::ShowNormalsListener(OCCViewer* viewer) : m_viewer(viewer) {}

CommandResult ShowNormalsListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {
    // Use unified error handling to check if OCCViewer is available
    CHECK_PTR_RETURN(m_viewer, "OCCViewer", commandType);

    // Use EXECUTE_SAFELY macro for automatic exception handling
    EXECUTE_SAFELY([&]() {
        // Toggle normal display based on current state
        bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::NormalLine);
        m_viewer->setShowNormals(show);

        LOG_INF_S("Normal display toggled to: " + std::string(show ? "ON" : "OFF"));
    }, commandType);

    // Return appropriate success result after macro execution
    bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::NormalLine);
    if (show) {
        RETURN_SUCCESS("Node normals shown", commandType);
    } else {
        RETURN_SUCCESS("Node normals hidden", commandType);
    }
}

bool ShowNormalsListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowNormals);
}
