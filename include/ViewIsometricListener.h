#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class NavigationController;
class ViewIsometricListener : public CommandListener {
public:
    explicit ViewIsometricListener(NavigationController* nav);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ViewIsometricListener"; }
private:
    NavigationController* m_nav;
}; 