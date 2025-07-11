#include "MeshQualityDialogListener.h"
#include "CommandDispatcher.h"
#include "MeshQualityDialog.h"
#include "OCCViewer.h"
#include "logger/Logger.h"

MeshQualityDialogListener::MeshQualityDialogListener(wxFrame* frame, OCCViewer* occViewer)
    : m_frame(frame), m_occViewer(occViewer)
{
    if (!m_frame) {
        LOG_ERR_S("MeshQualityDialogListener: frame pointer is null");
    }
}

CommandResult MeshQualityDialogListener::executeCommand(const std::string& commandType,
                                                       const std::unordered_map<std::string, std::string>&) {
    if (!m_frame || !m_occViewer) {
        return CommandResult(false, "Frame or OCCViewer not available", commandType);
    }
    
    MeshQualityDialog dialog(m_frame, m_occViewer);
    if (dialog.ShowModal() == wxID_OK) {
        LOG_INF_S("Mesh quality settings applied");
        return CommandResult(true, "Mesh quality settings updated", commandType);
    }
    
    return CommandResult(false, "Mesh quality dialog cancelled", commandType);
}

bool MeshQualityDialogListener::canHandleCommand(const std::string& commandType) const {
    return commandType == "MESH_QUALITY_DIALOG";
} 
