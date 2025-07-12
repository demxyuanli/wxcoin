#pragma once
#include "CommandListener.h"
#include "CommandType.h"
#include <Inventor/SbVec3f.h>
#include "MouseHandler.h"
class GeometryFactory;

class CreateWrenchListener : public CommandListener {
public:
    CreateWrenchListener(MouseHandler* mouseHandler, GeometryFactory* factory);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "CreateWrenchListener"; }
private:
    MouseHandler* m_mouseHandler;
    GeometryFactory* m_factory;
}; 