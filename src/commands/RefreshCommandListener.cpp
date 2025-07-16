#include "RefreshCommandListener.h"
#include "Canvas.h"
#include "OCCViewer.h"
#include "SceneManager.h"
#include "CommandType.h"
#include "logger/Logger.h"

RefreshCommandListener::RefreshCommandListener(Canvas* canvas, 
                                             OCCViewer* occViewer, 
                                             SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_occViewer(occViewer)
    , m_sceneManager(sceneManager)
{
    LOG_INF_S("RefreshCommandListener created");
}

CommandResult RefreshCommandListener::executeCommand(const std::string& commandType,
                                                   const std::unordered_map<std::string, std::string>& parameters)
{
    LOG_INF_S("RefreshCommandListener: Received command: " + commandType);
    
    // Check if we can handle this command
    if (!canHandleCommand(commandType)) {
        return CommandResult(false, "RefreshCommandListener cannot handle command: " + commandType, commandType);
    }
    
    // Create refresh command using factory
    auto command = RefreshCommandFactory::createCommand(commandType, parameters);
    if (command) {
        executeRefreshCommand(command);
        return CommandResult(true, "Refresh command executed successfully", commandType);
    } else {
        LOG_WRN_S("RefreshCommandListener: Failed to create command for: " + commandType);
        return CommandResult(false, "Failed to create refresh command", commandType);
    }
}

bool RefreshCommandListener::canHandleCommand(const std::string& commandType) const
{
    try {
        cmd::CommandType type = cmd::from_string(commandType);
        return (type == cmd::CommandType::RefreshView ||
                type == cmd::CommandType::RefreshScene ||
                type == cmd::CommandType::RefreshObject ||
                type == cmd::CommandType::RefreshMaterial ||
                type == cmd::CommandType::RefreshGeometry ||
                type == cmd::CommandType::RefreshUI);
    } catch (...) {
        // Handle potential static map access issues during shutdown
        return false;
    }
}

std::string RefreshCommandListener::getListenerName() const
{
    return "RefreshCommandListener";
}

void RefreshCommandListener::executeRefreshCommand(std::shared_ptr<RefreshCommand> command)
{
    if (!command) {
        return;
    }
    
    // Set appropriate context based on command type
    cmd::CommandType type = command->getCommandType();
    
    switch (type) {
        case cmd::CommandType::RefreshView:
        case cmd::CommandType::RefreshUI: {
            auto viewCommand = std::dynamic_pointer_cast<RefreshViewCommand>(command);
            auto uiCommand = std::dynamic_pointer_cast<RefreshUICommand>(command);
            
            if (viewCommand && m_canvas) {
                viewCommand->setCanvas(m_canvas);
            } else if (uiCommand && m_canvas) {
                uiCommand->setCanvas(m_canvas);
            }
            break;
        }
        
        case cmd::CommandType::RefreshScene: {
            auto sceneCommand = std::dynamic_pointer_cast<RefreshSceneCommand>(command);
            if (sceneCommand && m_sceneManager) {
                sceneCommand->setSceneManager(m_sceneManager);
            }
            break;
        }
        
        case cmd::CommandType::RefreshObject:
        case cmd::CommandType::RefreshMaterial:
        case cmd::CommandType::RefreshGeometry: {
            auto objectCommand = std::dynamic_pointer_cast<RefreshObjectCommand>(command);
            auto materialCommand = std::dynamic_pointer_cast<RefreshMaterialCommand>(command);
            auto geometryCommand = std::dynamic_pointer_cast<RefreshGeometryCommand>(command);
            
            if (objectCommand && m_occViewer) {
                objectCommand->setOCCViewer(m_occViewer);
            } else if (materialCommand && m_occViewer) {
                materialCommand->setOCCViewer(m_occViewer);
            } else if (geometryCommand && m_occViewer) {
                geometryCommand->setOCCViewer(m_occViewer);
            }
            break;
        }
        
        default:
            LOG_WRN_S("RefreshCommandListener: Unknown command type for execution");
            return;
    }
    
    // Execute the command
    try {
        command->execute();
        LOG_INF_S("RefreshCommandListener: Successfully executed " + command->getDescription());
    } catch (const std::exception& e) {
        LOG_ERR_S("RefreshCommandListener: Exception executing command: " + std::string(e.what()));
    }
} 