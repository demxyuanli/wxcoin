#include "FileSaveListener.h"
#include "CommandDispatcher.h"
#include <wx/filedlg.h>
#include "logger/Logger.h"

FileSaveListener::FileSaveListener(wxFrame* frame)
    : m_frame(frame)
{
    if (!m_frame) {
        LOG_ERR_S("FileSaveListener: frame pointer is null");
    }
}

CommandResult FileSaveListener::executeCommand(const std::string& commandType,
                                             const std::unordered_map<std::string, std::string>&) {
    wxFileDialog saveFileDialog(m_frame, "Save Project File", "", "",
                               "Project files (*.prj)|*.prj|All files (*.*)|*.*",
                               wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return CommandResult(false, "File save cancelled", commandType);
    
    wxString selectedPath = saveFileDialog.GetPath();
    LOG_INF_S("File selected for saving: " + selectedPath.ToStdString());
    
    // TODO: Implement actual file saving logic
    return CommandResult(true, "File saved: " + selectedPath.ToStdString(), commandType);
}

bool FileSaveListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::FileSave);
} 
