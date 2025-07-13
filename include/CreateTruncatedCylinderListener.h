#pragma once

#include "CommandListener.h"
#include "MouseHandler.h"

class CreateTruncatedCylinderListener : public CommandListener {
public:
    explicit CreateTruncatedCylinderListener(MouseHandler* mouseHandler);
    ~CreateTruncatedCylinderListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "CreateTruncatedCylinderListener"; }

private:
    MouseHandler* m_mouseHandler;
}; 