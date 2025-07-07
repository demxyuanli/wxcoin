#include "ZoomSpeedListener.h"
#include "MainFrame.h"
#include "Canvas.h"
#include "InputManager.h"
#include "NavigationController.h"
#include <wx/textdlg.h>

ZoomSpeedListener::ZoomSpeedListener(MainFrame* mainFrame, Canvas* canvas)
    : m_mainFrame(mainFrame), m_canvas(canvas) {}

CommandResult ZoomSpeedListener::executeCommand(const std::string& commandType,
                                                const std::unordered_map<std::string, std::string>&) {
    if (!m_canvas) return CommandResult(false, "Canvas not available", commandType);
    auto inputManager = m_canvas->getInputManager();
    if (!inputManager) return CommandResult(false, "Input manager not available", commandType);
    auto nav = inputManager->getNavigationController();
    if (!nav) return CommandResult(false, "Navigation controller not available", commandType);
    float currentSpeed = nav->getZoomSpeedFactor();
    wxTextEntryDialog dlg(m_mainFrame, "Enter zoom speed multiplier:", "Zoom Speed", wxString::Format("%f", currentSpeed));
    if (dlg.ShowModal() == wxID_OK) {
        double value;
        if (dlg.GetValue().ToDouble(&value) && value > 0) {
            nav->setZoomSpeedFactor(static_cast<float>(value));
            return CommandResult(true, "Zoom speed updated", commandType);
        } else {
            return CommandResult(false, "Invalid speed value", commandType);
        }
    }
    return CommandResult(false, "Zoom speed dialog cancelled", commandType);
}

bool ZoomSpeedListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ZoomSpeed);
} 