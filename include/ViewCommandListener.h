#pragma once

#include "CommandListener.h"
#include <memory>
#include <set>

class NavigationController;
class OCCViewer;

/**
 * @brief Handles view-related commands
 */
class ViewCommandListener : public CommandListener {
public:
    /**
     * @brief Constructor
     * @param navController Navigation controller for view operations
     * @param occViewer OCC viewer for display settings
     */
    ViewCommandListener(NavigationController* navController, OCCViewer* occViewer = nullptr);
    ~ViewCommandListener() override;
    
    CommandResult executeCommand(const std::string& commandType, 
                               const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;
    
private:
    NavigationController* m_navigationController;
    OCCViewer* m_occViewer;
    std::set<std::string> m_supportedCommands;
    
    /**
     * @brief Initialize supported command types
     */
    void initializeSupportedCommands();
};