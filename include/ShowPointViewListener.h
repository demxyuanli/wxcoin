#pragma once

#include "CommandListener.h"

class OCCViewer;
class RenderingEngine;

/**
 * @brief Listener for showing point view dialog
 *
 * Handles the SHOW_POINT_VIEW command by opening the point view settings dialog
 */
class ShowPointViewListener : public CommandListener
{
public:
    ShowPointViewListener(OCCViewer* occViewer, RenderingEngine* renderingEngine);
    virtual ~ShowPointViewListener();

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
