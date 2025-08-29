#pragma once

#include "CommandListener.h"

// Forward declarations
class OCCViewer;

/**
 * @brief Command listener for opening outline settings dialog
 * 
 * Handles the "OutlineSettings" command type to open the outline
 * configuration dialog for real-time parameter adjustment.
 */
class OutlineSettingsListener : public CommandListener {
public:
    /**
     * @brief Constructor
     * @param viewer OCC viewer to configure
     */
    explicit OutlineSettingsListener(OCCViewer* viewer) : m_viewer(viewer) {}

    /**
     * @brief Execute outline settings command
     * @param commandType Command type identifier
     * @param parameters Command parameters
     * @return Command execution result
     */
    CommandResult executeCommand(const std::string& commandType,
                               const std::unordered_map<std::string, std::string>& parameters) override;

    /**
     * @brief Check if this listener can handle the command
     * @param commandType Command type to check
     * @return True if can handle the command
     */
    bool canHandleCommand(const std::string& commandType) const override;

private:
    OCCViewer* m_viewer;
};