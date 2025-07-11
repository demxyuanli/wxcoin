#include "FileOpenListener.h"
#include "CommandDispatcher.h"
#include <wx/filedlg.h>
#include "logger/Logger.h"

FileOpenListener::FileOpenListener(wxFrame* frame)
    : m_frame(frame)
{
    if (!m_frame) {
        LOG_ERR_S("FileOpenListener: frame pointer is null");
    }
}

CommandResult FileOpenListener::executeCommand(const std::string& commandType,
                                             const std::unordered_map<std::string, std::string>&) {
    wxFileDialog openFileDialog(m_frame, "Open Project File", "", "",
                               "Project files (*.prj)|*.prj|All files (*.*)|*.*",
                               wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return CommandResult(false, "File open cancelled", commandType);
    
    wxString selectedPath = openFileDialog.GetPath();
    LOG_INF_S("File selected for opening: " + selectedPath.ToStdString());
    
    // TODO: Implement actual file loading logic
    return CommandResult(true, "File opened: " + selectedPath.ToStdString(), commandType);
}

bool FileOpenListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::FileOpen);
} 
