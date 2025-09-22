#pragma once

#include "CommandListener.h"
#include "NavigationModeManager.h"

class NavigationModeListener : public CommandListener {
public:
    NavigationModeListener();
    ~NavigationModeListener();

    bool canHandleCommand(const std::string& commandType) const override;
    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    std::string getListenerName() const override;

private:
    void showNavigationModeDialog(NavigationModeManager* navManager);
};
