#pragma once
#include "CommandListener.h"
#include "CommandType.h"
#include <Inventor/SbVec3f.h>
class GeometryFactory;

class CreateWrenchListener : public CommandListener {
public:
    explicit CreateWrenchListener(GeometryFactory* factory);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "CreateWrenchListener"; }
private:
    GeometryFactory* m_factory;
}; 