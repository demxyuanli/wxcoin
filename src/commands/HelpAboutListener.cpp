#include "HelpAboutListener.h"
#include "MainFrame.h"
#include <wx/aboutdlg.h>
#include "Logger.h"

HelpAboutListener::HelpAboutListener(MainFrame* mainFrame) : m_mainFrame(mainFrame) {}

CommandResult HelpAboutListener::executeCommand(const std::string& commandType,
                                                const std::unordered_map<std::string, std::string>&) {
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetName("FreeCAD Navigation");
    aboutInfo.SetVersion("1.0");
    aboutInfo.SetDescription("A 3D CAD application with navigation and geometry creation");
    aboutInfo.SetCopyright("(C) 2025 Your Name");
    wxAboutBox(aboutInfo, m_mainFrame);
    LOG_INF("About dialog shown");
    return CommandResult(true, "About dialog displayed", commandType);
}

bool HelpAboutListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::HelpAbout);
} 