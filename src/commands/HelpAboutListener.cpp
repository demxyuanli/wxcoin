#include "HelpAboutListener.h"
#include "CommandDispatcher.h"
#include <wx/aboutdlg.h>
#include "logger/Logger.h"

HelpAboutListener::HelpAboutListener(wxFrame* frame)
    : m_frame(frame)
{
    if (!m_frame) {
        LOG_ERR_S("HelpAboutListener: frame pointer is null");
    }
}

CommandResult HelpAboutListener::executeCommand(const std::string& commandType,
                                               const std::unordered_map<std::string, std::string>&) {
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetName("wxCoin CAD Application");
    aboutInfo.SetVersion("1.0.0");
    aboutInfo.SetDescription("A 3D CAD application using wxWidgets and OpenCASCADE");
    aboutInfo.SetCopyright("(C) 2024");
    
    wxAboutBox(aboutInfo, m_frame);
    
    return CommandResult(true, "About dialog shown", commandType);
}

bool HelpAboutListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::HelpAbout);
} 
