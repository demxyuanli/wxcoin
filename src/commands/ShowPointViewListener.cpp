#include "ShowPointViewListener.h"
#include "PointViewDialog.h"
#include "OCCViewer.h"
#include "RenderingEngine.h"
#include "CommandDispatcher.h"
#include <wx/wx.h>

ShowPointViewListener::ShowPointViewListener(OCCViewer* occViewer, RenderingEngine* renderingEngine)
    : m_occViewer(occViewer)
    , m_renderingEngine(renderingEngine)
{
}

ShowPointViewListener::~ShowPointViewListener()
{
}

CommandResult ShowPointViewListener::executeCommand(const std::string& commandType,
                                                  const std::unordered_map<std::string, std::string>& parameters)
{
    if (commandType == "SHOW_POINT_VIEW")
    {
        if (!m_occViewer) {
            return CommandResult(false, "OCCViewer not available", commandType);
        }

        // Toggle logic: if currently enabled -> disable; if disabled -> open param dialog
        const bool currentlyEnabled = m_occViewer->isPointViewEnabled();
        if (!currentlyEnabled) {
            // Open parameter dialog to get parameters
            wxWindow* parent = wxGetActiveWindow();
            if (!parent) {
                parent = wxTheApp->GetTopWindow();
            }

            // Create and show the point view dialog
            PointViewDialog dialog(parent, m_occViewer, m_renderingEngine);
            if (dialog.ShowModal() == wxID_OK) {
                // Settings are applied in the dialog's applySettings method
                return CommandResult(true, "Point view enabled", commandType);
            } else {
                // User cancelled, don't enable point view
                return CommandResult(true, "Point view display cancelled", commandType);
            }
        }
        else {
            // Disable point view
            auto displaySettings = m_occViewer->getDisplaySettings();
            displaySettings.showPointView = false;
            m_occViewer->setDisplaySettings(displaySettings);
            return CommandResult(true, "Point view disabled", commandType);
        }
    }

    return CommandResult(false, "Unknown command type", "SHOW_POINT_VIEW");
}

CommandResult ShowPointViewListener::executeCommand(cmd::CommandType commandType,
                                                  const std::unordered_map<std::string, std::string>& parameters)
{
    return executeCommand(cmd::to_string(commandType), parameters);
}

bool ShowPointViewListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == "SHOW_POINT_VIEW";
}

std::string ShowPointViewListener::getListenerName() const
{
    return "ShowPointViewListener";
}
