#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <wx/frame.h>

class OCCViewer;

class EdgeSettingsListener : public CommandListener
{
public:
    EdgeSettingsListener(wxFrame* frame, OCCViewer* viewer);
    virtual ~EdgeSettingsListener() = default;

    CommandResult executeCommand(const std::string& commandType,
                                const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "EdgeSettingsListener"; }

private:
    wxFrame* m_frame;
    OCCViewer* m_viewer;
}; 