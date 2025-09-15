#include "ExplodeConfigListener.h"
#include "OCCViewer.h"
#include <wx/wx.h>
#include <wx/radiobox.h>
#include <wx/spinctrl.h>
#include <memory>
#include "ExplodeConfigDialog.h"

// Forward declare dialog
class ExplodeConfigDialog;

ExplodeConfigListener::ExplodeConfigListener(wxFrame* frame, OCCViewer* viewer)
    : m_frame(frame), m_viewer(viewer) {}

CommandResult ExplodeConfigListener::executeCommand(const std::string& commandType,
                                                  const std::unordered_map<std::string, std::string>&) {
    if (!m_frame || !m_viewer) return CommandResult(false, "Viewer not available", commandType);

    // Use dedicated dialog
    OCCViewer::ExplodeMode mode; double factor; m_viewer->getExplodeParams(mode, factor);
    ExplodeConfigDialog dlg(m_frame, mode, factor);
    if (dlg.ShowModal() == wxID_OK) {
        m_viewer->setExplodeParams(dlg.getMode(), dlg.getFactor());
        return CommandResult(true, "Explode parameters updated", commandType);
    }
    return CommandResult(false, "Explode configuration cancelled", commandType);
}

bool ExplodeConfigListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ExplodeConfig);
}


