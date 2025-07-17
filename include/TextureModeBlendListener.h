#pragma once

#include "CommandListener.h"
#include <wx/frame.h>

class OCCViewer;

class TextureModeBlendListener : public CommandListener {
public:
    TextureModeBlendListener(wxFrame* frame, OCCViewer* viewer);
    virtual ~TextureModeBlendListener() = default;

    virtual CommandResult executeCommand(const std::string& commandType,
                                       const std::unordered_map<std::string, std::string>& parameters) override;
    virtual bool canHandleCommand(const std::string& commandType) const override;
    virtual std::string getListenerName() const override;

private:
    wxFrame* m_frame;
    OCCViewer* m_viewer;
}; 