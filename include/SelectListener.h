#pragma once

#include "CommandListener.h"
#include "MouseHandler.h"
#include <unordered_map>

class SelectListener : public CommandListener
{
public:
    SelectListener(MouseHandler* mouseHandler);
    ~SelectListener() override = default;

    CommandResult executeCommand(const std::string& commandType, 
                               const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "SelectListener"; }

private:
    MouseHandler* m_mouseHandler;
}; 