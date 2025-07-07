#include "FileCommandListener.h"
#include "MainFrame.h"
#include "Canvas.h"
#include "Command.h"
#include "CommandListener.h"
#include "CommandDispatcher.h"
#include "Logger.h"
#include "InputManager.h"
#include "NavigationController.h"
#include "SceneManager.h"
#include <wx/filedlg.h>
#include <wx/aboutdlg.h>
#include <wx/textdlg.h>
#include <stdexcept>

FileCommandListener::FileCommandListener(MainFrame* mainFrame, Canvas* canvas, CommandManager* commandManager)
    : m_mainFrame(mainFrame), m_canvas(canvas), m_commandManager(commandManager)
{
    initializeSupportedCommands();
    LOG_INF("FileCommandListener initialized");
}

FileCommandListener::~FileCommandListener()
{
    LOG_INF("FileCommandListener destroyed");
}

void FileCommandListener::initializeSupportedCommands()
{
    m_supportedCommands.insert("FILE_NEW");
    m_supportedCommands.insert("FILE_OPEN");
    m_supportedCommands.insert("FILE_SAVE");
    m_supportedCommands.insert("IMPORT_STEP");
    m_supportedCommands.insert("FILE_EXIT");
    m_supportedCommands.insert("UNDO");
    m_supportedCommands.insert("REDO");
    m_supportedCommands.insert("HELP_ABOUT");
    m_supportedCommands.insert("NAV_CUBE_CONFIG");
    m_supportedCommands.insert("ZOOM_SPEED");
}

CommandResult FileCommandListener::executeCommand(const std::string& commandType, 
                                                const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_mainFrame || !m_canvas) {
        return CommandResult(false, "Main frame or canvas not available", commandType);
    }
    
    try {
        if (commandType == "FILE_NEW") {
            return executeNewCommand();
        }
        else if (commandType == "FILE_OPEN") {
            return executeOpenCommand();
        }
        else if (commandType == "FILE_SAVE") {
            return executeSaveCommand();
        }
        else if (commandType == "IMPORT_STEP") {
            return executeImportSTEPCommand();
        }
        else if (commandType == "FILE_EXIT") {
            return executeExitCommand();
        }
        else if (commandType == "UNDO") {
            return executeUndoCommand();
        }
        else if (commandType == "REDO") {
            return executeRedoCommand();
        }
        else if (commandType == "HELP_ABOUT") {
            return executeAboutCommand();
        }
        else if (commandType == "NAV_CUBE_CONFIG") {
            return executeNavCubeConfigCommand();
        }
        else if (commandType == "ZOOM_SPEED") {
            return executeZoomSpeedCommand();
        }
        
        return CommandResult(false, "Unknown file command: " + commandType, commandType);
    }
    catch (const std::exception& e) {
        return CommandResult(false, "Error executing file command: " + std::string(e.what()), commandType);
    }
}

bool FileCommandListener::canHandleCommand(const std::string& commandType) const
{
    return m_supportedCommands.find(commandType) != m_supportedCommands.end();
}

std::string FileCommandListener::getListenerName() const
{
    return "FileCommandListener";
}

CommandResult FileCommandListener::executeNewCommand()
{
    LOG_INF("Creating new project");
    m_canvas->getSceneManager()->cleanup();
    m_canvas->getSceneManager()->initScene();
    if (m_commandManager) {
        m_commandManager->clearHistory();
    }
    return CommandResult(true, "New project created", "FILE_NEW");
}

CommandResult FileCommandListener::executeOpenCommand()
{
    wxFileDialog openFileDialog(m_mainFrame, "Open Project", "", "", 
                               "Project files (*.proj)|*.proj", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        LOG_INF("Open file dialog cancelled");
        return CommandResult(false, "Open operation cancelled", "FILE_OPEN");
    }

    LOG_INF("Opening project: " + openFileDialog.GetPath().ToStdString());
    return CommandResult(true, "Opened: " + openFileDialog.GetFilename().ToStdString(), "FILE_OPEN");
}

CommandResult FileCommandListener::executeSaveCommand()
{
    wxFileDialog saveFileDialog(m_mainFrame, "Save Project", "", "", 
                               "Project files (*.proj)|*.proj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL) {
        LOG_INF("Save file dialog cancelled");
        return CommandResult(false, "Save operation cancelled", "FILE_SAVE");
    }

    LOG_INF("Saving project: " + saveFileDialog.GetPath().ToStdString());
    return CommandResult(true, "Saved: " + saveFileDialog.GetFilename().ToStdString(), "FILE_SAVE");
}

CommandResult FileCommandListener::executeImportSTEPCommand()
{
    // This would need access to OCCViewer, which should be passed in constructor or as parameter
    // For now, return a placeholder implementation
    wxString wildcard = "STEP files (*.step;*.stp)|*.step;*.stp|All files (*.*)|*.*";
    wxFileDialog openFileDialog(m_mainFrame, "Import STEP File", "", "", wildcard, 
                               wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        LOG_INF("STEP import dialog cancelled");
        return CommandResult(false, "Import operation cancelled", "IMPORT_STEP");
    }
    
    std::string filePath = openFileDialog.GetPath().ToStdString();
    LOG_INF("Importing STEP file: " + filePath);
    
    // TODO: Implement actual STEP import logic
    return CommandResult(true, "STEP file import initiated", "IMPORT_STEP");
}

CommandResult FileCommandListener::executeExitCommand()
{
    LOG_INF("Application exit requested");
    m_mainFrame->Close(true);
    return CommandResult(true, "Application closing", "FILE_EXIT");
}

CommandResult FileCommandListener::executeUndoCommand()
{
    if (!m_commandManager) {
        return CommandResult(false, "Command manager not available", "UNDO");
    }
    
    if (!m_commandManager->canUndo()) {
        return CommandResult(false, "Nothing to undo", "UNDO");
    }
    
    LOG_INF("Undoing last command");
    m_commandManager->undo();
    m_canvas->Refresh();
    return CommandResult(true, "Undo completed", "UNDO");
}

CommandResult FileCommandListener::executeRedoCommand()
{
    if (!m_commandManager) {
        return CommandResult(false, "Command manager not available", "REDO");
    }
    
    if (!m_commandManager->canRedo()) {
        return CommandResult(false, "Nothing to redo", "REDO");
    }
    
    LOG_INF("Redoing last undone command");
    m_commandManager->redo();
    m_canvas->Refresh();
    return CommandResult(true, "Redo completed", "REDO");
}

CommandResult FileCommandListener::executeAboutCommand()
{
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetName("FreeCAD Navigation");
    aboutInfo.SetVersion("1.0");
    aboutInfo.SetDescription("A 3D CAD application with navigation and geometry creation");
    aboutInfo.SetCopyright("(C) 2025 Your Name");
    wxAboutBox(aboutInfo, m_mainFrame);
    LOG_INF("About dialog shown");
    return CommandResult(true, "About dialog displayed", "HELP_ABOUT");
}

CommandResult FileCommandListener::executeNavCubeConfigCommand()
{
    LOG_INF("Opening navigation cube configuration dialog");
    m_canvas->ShowNavigationCubeConfigDialog();
    return CommandResult(true, "Navigation cube configuration opened", "NAV_CUBE_CONFIG");
}

CommandResult FileCommandListener::executeZoomSpeedCommand()
{
    auto inputManager = m_canvas->getInputManager();
    if (!inputManager) {
        return CommandResult(false, "Input manager not available", "ZOOM_SPEED");
    }
    
    auto nav = inputManager->getNavigationController();
    if (!nav) {
        return CommandResult(false, "Navigation controller not available", "ZOOM_SPEED");
    }
    
    float currentSpeed = nav->getZoomSpeedFactor();
    wxTextEntryDialog dlg(m_mainFrame, "Enter zoom speed multiplier:", "Zoom Speed", 
                         wxString::Format("%f", currentSpeed));
    
    if (dlg.ShowModal() == wxID_OK) {
        double value;
        if (dlg.GetValue().ToDouble(&value) && value > 0) {
            nav->setZoomSpeedFactor(static_cast<float>(value));
            return CommandResult(true, "Zoom speed updated", "ZOOM_SPEED");
        } else {
            return CommandResult(false, "Invalid speed value", "ZOOM_SPEED");
        }
    }
    
    return CommandResult(false, "Zoom speed dialog cancelled", "ZOOM_SPEED");
}