#include "FileOpenListener.h"
#include "MainFrame.h"
#include <wx/filedlg.h>
#include "Logger.h"

FileOpenListener::FileOpenListener(MainFrame* mainFrame)
    : m_mainFrame(mainFrame) {}

CommandResult FileOpenListener::executeCommand(const std::string& commandType,
                                               const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mainFrame) {
        return CommandResult(false, "Main frame not available", commandType);
    }

    wxFileDialog openFileDialog(m_mainFrame, "Open Project", "", "", 
                                "Project files (*.proj)|*.proj", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        LOG_INF("Open file dialog cancelled");
        return CommandResult(false, "Open operation cancelled", commandType);
    }

    LOG_INF("Opening project: " + openFileDialog.GetPath().ToStdString());
    return CommandResult(true, "Opened: " + openFileDialog.GetFilename().ToStdString(), commandType);
}

bool FileOpenListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::FileOpen);
} 