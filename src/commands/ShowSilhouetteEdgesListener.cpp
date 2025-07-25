#include "ShowSilhouetteEdgesListener.h"
#include "OCCViewer.h"
#include "EdgeTypes.h"
#include "CommandDispatcher.h"
#include "logger/Logger.h"

ShowSilhouetteEdgesListener::ShowSilhouetteEdgesListener(OCCViewer* viewer): m_viewer(viewer) {}

CommandResult ShowSilhouetteEdgesListener::executeCommand(const std::string& commandType,
                                                const std::unordered_map<std::string, std::string>&) {
    LOG_INF_S("[ShowSilhouetteEdgesDebug] ShowSilhouetteEdgesListener::executeCommand called");
    if (!m_viewer) {
        LOG_ERR_S("[ShowSilhouetteEdgesDebug] OCCViewer not available");
        return CommandResult(false, "OCCViewer not available", commandType);
    }
    
    // Check current silhouette edge display state and toggle
    bool show = !m_viewer->isEdgeTypeEnabled(EdgeType::Silhouette);
    LOG_INF_S("[ShowSilhouetteEdgesDebug] Setting silhouette edges to: " + std::string(show ? "true" : "false"));
    m_viewer->setShowSilhouetteEdges(show);
    return CommandResult(true, show ? "Silhouette edges shown" : "Silhouette edges hidden", commandType);
}

bool ShowSilhouetteEdgesListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ShowSilhouetteEdges);
} 