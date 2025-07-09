#include "MeshQualityDialogListener.h"
#include "MeshQualityDialog.h"
#include "OCCViewer.h"
#include "Logger.h"

MeshQualityDialogListener::MeshQualityDialogListener(wxWindow* parent, OCCViewer* occViewer)
    : m_parent(parent)
    , m_occViewer(occViewer)
{
}

CommandResult MeshQualityDialogListener::execute(const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_parent || !m_occViewer) {
        return CommandResult(false, "MESH_QUALITY_DIALOG", "Parent window or OCCViewer is null");
    }
    
    try {
        MeshQualityDialog dialog(m_parent, m_occViewer);
        int result = dialog.ShowModal();
        
        if (result == wxID_OK || result == wxID_APPLY) {
            return CommandResult(true, "MESH_QUALITY_DIALOG", "Mesh quality settings applied successfully");
        } else {
            return CommandResult(true, "MESH_QUALITY_DIALOG", "Mesh quality dialog cancelled");
        }
    } catch (const std::exception& e) {
        LOG_ERR("Exception in mesh quality dialog: " + std::string(e.what()));
        return CommandResult(false, "MESH_QUALITY_DIALOG", "Exception: " + std::string(e.what()));
    }
}

CommandResult MeshQualityDialogListener::executeCommand(const std::string& commandType, const std::unordered_map<std::string, std::string>& parameters)
{
    return execute(parameters);
}

bool MeshQualityDialogListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == "MESH_QUALITY_DIALOG";
}

std::string MeshQualityDialogListener::getListenerName() const
{
    return "MeshQualityDialogListener";
} 