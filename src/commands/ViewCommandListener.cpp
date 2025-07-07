#include "ViewCommandListener.h"
#include "CommandDispatcher.h"
#include "NavigationController.h"
#include "OCCViewer.h"
#include "Logger.h"

ViewCommandListener::ViewCommandListener(NavigationController* navController, OCCViewer* occViewer)
    : m_navigationController(navController), m_occViewer(occViewer)
{
    initializeSupportedCommands();
    LOG_INF("ViewCommandListener initialized");
}

ViewCommandListener::~ViewCommandListener()
{
    LOG_INF("ViewCommandListener destroyed");
}

void ViewCommandListener::initializeSupportedCommands()
{
    m_supportedCommands.insert("VIEW_ALL");
    m_supportedCommands.insert("VIEW_TOP");
    m_supportedCommands.insert("VIEW_FRONT");
    m_supportedCommands.insert("VIEW_RIGHT");
    m_supportedCommands.insert("VIEW_ISOMETRIC");
    m_supportedCommands.insert("SHOW_NORMALS");
    m_supportedCommands.insert("FIX_NORMALS");
    m_supportedCommands.insert("SHOW_EDGES");
}

CommandResult ViewCommandListener::executeCommand(const std::string& commandType, 
                                                const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_navigationController) {
        return CommandResult(false, "Navigation controller not available", commandType);
    }
    
    try {
        if (commandType == "VIEW_ALL") {
            m_navigationController->viewAll();
            return CommandResult(true, "Fit all view applied", commandType);
        }
        else if (commandType == "VIEW_TOP") {
            m_navigationController->viewTop();
            return CommandResult(true, "Top view applied", commandType);
        }
        else if (commandType == "VIEW_FRONT") {
            m_navigationController->viewFront();
            return CommandResult(true, "Front view applied", commandType);
        }
        else if (commandType == "VIEW_RIGHT") {
            m_navigationController->viewRight();
            return CommandResult(true, "Right view applied", commandType);
        }
        else if (commandType == "VIEW_ISOMETRIC") {
            m_navigationController->viewIsometric();
            return CommandResult(true, "Isometric view applied", commandType);
        }
        else if (commandType == "SHOW_NORMALS" && m_occViewer) {
            bool showNormals = !m_occViewer->isShowNormals();
            m_occViewer->setShowNormals(showNormals);
            return CommandResult(true, showNormals ? "Normals shown" : "Normals hidden", commandType);
        }
        else if (commandType == "FIX_NORMALS" && m_occViewer) {
            m_occViewer->fixNormals();
            return CommandResult(true, "Face normals fixed", commandType);
        }
        else if (commandType == "SHOW_EDGES" && m_occViewer) {
            bool showEdges = !m_occViewer->isShowingEdges();
            m_occViewer->setShowEdges(showEdges);
            return CommandResult(true, showEdges ? "Edges shown" : "Edges hidden", commandType);
        }
        
        return CommandResult(false, "Unknown view command or missing OCC viewer: " + commandType, commandType);
    }
    catch (const std::exception& e) {
        return CommandResult(false, "Error executing view command: " + std::string(e.what()), commandType);
    }
}

bool ViewCommandListener::canHandleCommand(const std::string& commandType) const
{
    return m_supportedCommands.find(commandType) != m_supportedCommands.end();
}

std::string ViewCommandListener::getListenerName() const
{
    return "ViewCommandListener";
}