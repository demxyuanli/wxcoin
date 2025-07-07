#pragma once

#include "CommandListener.h"
#include "CommandType.h"

class MouseHandler;

class CreateSphereListener : public CommandListener {
public:
    explicit CreateSphereListener(MouseHandler* mouseHandler);
    ~CreateSphereListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "CreateSphereListener"; }

private:
    MouseHandler* m_mouseHandler;
}; 