#include "ZoomSpeedListener.h"
#include "CommandDispatcher.h"
#include "Canvas.h"
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include "logger/Logger.h"

ZoomSpeedListener::ZoomSpeedListener(wxFrame* frame, Canvas* canvas)
    : m_frame(frame), m_canvas(canvas)
{
    if (!m_frame) {
        LOG_ERR_S("ZoomSpeedListener: frame pointer is null");
    }
}

CommandResult ZoomSpeedListener::executeCommand(const std::string& commandType,
                                              const std::unordered_map<std::string, std::string>&) {
    if (!m_canvas) {
        return CommandResult(false, "Canvas not available", commandType);
    }
    
    wxTextEntryDialog dialog(m_frame, 
                           "Enter zoom speed multiplier (0.1 - 10.0):", 
                           "Zoom Speed Configuration", 
                           "1.0");
    
    if (dialog.ShowModal() == wxID_OK) {
        wxString value = dialog.GetValue();
        double speed;
        if (value.ToDouble(&speed) && speed >= 0.1 && speed <= 10.0) {
            // TODO: Set zoom speed on canvas/navigation controller
            LOG_INF_S("Zoom speed set to: " + std::to_string(speed));
            return CommandResult(true, "Zoom speed updated to " + std::to_string(speed), commandType);
        } else {
            wxMessageBox("Invalid zoom speed value. Please enter a number between 0.1 and 10.0", 
                        "Error", wxOK | wxICON_ERROR, m_frame);
            return CommandResult(false, "Invalid zoom speed value", commandType);
        }
    }
    
    return CommandResult(false, "Zoom speed configuration cancelled", commandType);
}

bool ZoomSpeedListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ZoomSpeed);
} 
