#pragma once

#include "CommandListener.h"
#include "MouseHandler.h"

class CreateTorusListener : public CommandListener {
public:
    explicit CreateTorusListener(MouseHandler* mouseHandler);
    ~CreateTorusListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "CreateTorusListener"; }

private:
    MouseHandler* m_mouseHandler;
}; 