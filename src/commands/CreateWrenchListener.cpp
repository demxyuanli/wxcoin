#include "CreateWrenchListener.h"
#include "GeometryFactory.h"
#include "Logger.h"

CreateWrenchListener::CreateWrenchListener(GeometryFactory* factory) : m_factory(factory) {}

CommandResult CreateWrenchListener::executeCommand(const std::string& commandType,
                                                   const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_factory) {
        return CommandResult(false, "Geometry factory not available", commandType);
    }
    m_factory->createGeometry("Wrench", SbVec3f(0.0f, 0.0f, 0.0f), GeometryType::OPENCASCADE);
    return CommandResult(true, "Wrench created successfully", commandType);
}

bool CreateWrenchListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::CreateWrench);
} 