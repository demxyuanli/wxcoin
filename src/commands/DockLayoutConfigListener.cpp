#include "DockLayoutConfigListener.h"
#include "docking/DockManager.h"
#include "docking/DockLayoutConfig.h"
#include "docking/DockContainerWidget.h"
#include <wx/wx.h>

using namespace ads;

DockLayoutConfigListener::DockLayoutConfigListener(DockManager* dockManager)
    : m_dockManager(dockManager)
{
}

DockLayoutConfigListener::~DockLayoutConfigListener()
{
}

CommandResult DockLayoutConfigListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters)
{
    if (commandType == "DOCK_LAYOUT_CONFIG")
    {
        // Get the parent window (main frame)
        wxWindow* parent = wxGetActiveWindow();
        if (!parent) {
            parent = wxTheApp->GetTopWindow();
        }

        // Get current configuration
        const DockLayoutConfig& currentConfig = m_dockManager->getLayoutConfig();
        DockLayoutConfig config = currentConfig;  // Make a copy
        
        // Create and show the dock layout configuration dialog
        DockLayoutConfigDialog dialog(parent, config, m_dockManager);
        
        if (dialog.ShowModal() == wxID_OK)
        {
            config = dialog.GetConfig();
            m_dockManager->setLayoutConfig(config);
            
            // Apply the configuration immediately
            if (wxWindow* containerWidget = m_dockManager->containerWidget()) {
                if (DockContainerWidget* container = dynamic_cast<DockContainerWidget*>(containerWidget)) {
                    container->applyLayoutConfig();
                }
            }
            
            return CommandResult(true, "Dock layout configuration applied", "DOCK_LAYOUT_CONFIG");
        }
        
        return CommandResult(true, "Dock layout configuration cancelled", "DOCK_LAYOUT_CONFIG");
    }

    return CommandResult(false, "Unknown command type", "DOCK_LAYOUT_CONFIG");
}

CommandResult DockLayoutConfigListener::executeCommand(cmd::CommandType commandType,
    const std::unordered_map<std::string, std::string>& parameters)
{
    return executeCommand(cmd::to_string(commandType), parameters);
}

bool DockLayoutConfigListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == "DOCK_LAYOUT_CONFIG";
}

std::string DockLayoutConfigListener::getListenerName() const
{
    return "DockLayoutConfigListener";
}