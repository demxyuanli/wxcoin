#pragma once

#include "CommandListener.h"
#include <memory>
#include <set>

class MainFrame;
class Canvas;
class CommandManager;

/**
 * @brief Handles file operations and project management commands
 */
class FileCommandListener : public CommandListener {
public:
    /**
     * @brief Constructor
     * @param mainFrame Main application frame
     * @param canvas 3D canvas
     * @param commandManager Command manager for undo/redo
     */
    FileCommandListener(MainFrame* mainFrame, Canvas* canvas, CommandManager* commandManager);
    ~FileCommandListener() override;
    
    CommandResult executeCommand(const std::string& commandType, 
                               const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;
    
private:
    MainFrame* m_mainFrame;
    Canvas* m_canvas;
    CommandManager* m_commandManager;
    std::set<std::string> m_supportedCommands;
    
    /**
     * @brief Initialize supported command types
     */
    void initializeSupportedCommands();
    
    // Individual command execution methods
    CommandResult executeNewCommand();
    CommandResult executeOpenCommand();
    CommandResult executeSaveCommand();
    CommandResult executeImportSTEPCommand();
    CommandResult executeExitCommand();
    CommandResult executeUndoCommand();
    CommandResult executeRedoCommand();
    CommandResult executeAboutCommand();
    CommandResult executeNavCubeConfigCommand();
    CommandResult executeZoomSpeedCommand();
};