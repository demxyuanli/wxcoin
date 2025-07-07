#include "GeometryCommandListener.h"
#include "CommandDispatcher.h"
#include "GeometryFactory.h"
#include "MouseHandler.h"
#include "Logger.h"
#include <Inventor/SbVec3f.h>

GeometryCommandListener::GeometryCommandListener(GeometryFactory* factory, MouseHandler* mouseHandler)
    : m_geometryFactory(factory), m_mouseHandler(mouseHandler)
{
    initializeSupportedCommands();
    LOG_INF("GeometryCommandListener initialized");
}

GeometryCommandListener::~GeometryCommandListener()
{
    LOG_INF("GeometryCommandListener destroyed");
}

void GeometryCommandListener::initializeSupportedCommands()
{
    m_supportedCommands.insert("CREATE_BOX");
    m_supportedCommands.insert("CREATE_SPHERE");
    m_supportedCommands.insert("CREATE_CYLINDER");
    m_supportedCommands.insert("CREATE_CONE");
    m_supportedCommands.insert("CREATE_WRENCH");
}

CommandResult GeometryCommandListener::executeCommand(const std::string& commandType, 
                                                    const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_geometryFactory || !m_mouseHandler) {
        return CommandResult(false, "Geometry factory or mouse handler not available", commandType);
    }
    
    try {
        if (commandType == "CREATE_BOX") {
            m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
            m_mouseHandler->setCreationGeometryType("Box");
            return CommandResult(true, "Box creation mode activated", commandType);
        }
        else if (commandType == "CREATE_SPHERE") {
            m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
            m_mouseHandler->setCreationGeometryType("Sphere");
            return CommandResult(true, "Sphere creation mode activated", commandType);
        }
        else if (commandType == "CREATE_CYLINDER") {
            m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
            m_mouseHandler->setCreationGeometryType("Cylinder");
            return CommandResult(true, "Cylinder creation mode activated", commandType);
        }
        else if (commandType == "CREATE_CONE") {
            m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
            m_mouseHandler->setCreationGeometryType("Cone");
            return CommandResult(true, "Cone creation mode activated", commandType);
        }
        else if (commandType == "CREATE_WRENCH") {
            // Create wrench directly at origin
            m_geometryFactory->createGeometry("Wrench", SbVec3f(0.0f, 0.0f, 0.0f), GeometryType::OPENCASCADE);
            return CommandResult(true, "Wrench created successfully", commandType);
        }
        
        return CommandResult(false, "Unknown geometry command: " + commandType, commandType);
    }
    catch (const std::exception& e) {
        return CommandResult(false, "Error executing geometry command: " + std::string(e.what()), commandType);
    }
}

bool GeometryCommandListener::canHandleCommand(const std::string& commandType) const
{
    return m_supportedCommands.find(commandType) != m_supportedCommands.end();
}

std::string GeometryCommandListener::getListenerName() const
{
    return "GeometryCommandListener";
}