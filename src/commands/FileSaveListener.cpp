#include "FileSaveListener.h"
#include "MainFrame.h"
#include <wx/filedlg.h>
#include "Logger.h"

FileSaveListener::FileSaveListener(MainFrame* mainFrame) : m_mainFrame(mainFrame) {}

CommandResult FileSaveListener::executeCommand(const std::string& commandType,
                                               const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_mainFrame) {
        return CommandResult(false, "Main frame not available", commandType);
    }

    wxFileDialog saveFileDialog(m_mainFrame, "Save Project", "", "", 
                                "Project files (*.proj)|*.proj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL) {
        LOG_INF("Save file dialog cancelled");
        return CommandResult(false, "Save operation cancelled", commandType);
    }

    LOG_INF("Saving project: " + saveFileDialog.GetPath().ToStdString());
    return CommandResult(true, "Saved: " + saveFileDialog.GetFilename().ToStdString(), commandType);
}

bool FileSaveListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::FileSave);
} 