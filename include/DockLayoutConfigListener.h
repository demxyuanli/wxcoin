#pragma once

#include "CommandListener.h"

namespace ads {
    class DockManager;
}

/**
 * @brief Command listener for dock layout configuration
 * 
 * Handles the DOCK_LAYOUT_CONFIG command to show the dock layout
 * configuration dialog.
 */
class DockLayoutConfigListener : public CommandListener
{
public:
    explicit DockLayoutConfigListener(ads::DockManager* dockManager);
    virtual ~DockLayoutConfigListener();

    virtual CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;

    virtual CommandResult executeCommand(cmd::CommandType commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;

    virtual bool canHandleCommand(const std::string& commandType) const override;

    virtual std::string getListenerName() const override;

private:
    ads::DockManager* m_dockManager;
};