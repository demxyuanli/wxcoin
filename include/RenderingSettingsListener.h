#pragma once

#include "CommandListener.h"

class OCCViewer;
class RenderingEngine;

class RenderingSettingsListener : public CommandListener
{
public:
    RenderingSettingsListener(OCCViewer* occViewer, RenderingEngine* renderingEngine);
    virtual ~RenderingSettingsListener();

    CommandResult executeCommand(const std::string& commandType,
                                const std::unordered_map<std::string, std::string>& parameters) override;
    
    CommandResult executeCommand(cmd::CommandType commandType,
                                const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;

private:
    OCCViewer* m_occViewer;
    RenderingEngine* m_renderingEngine;
}; 