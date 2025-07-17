#pragma once

#include "CommandListener.h"
#include <wx/frame.h>

class OCCViewer;

class TextureModeModulateListener : public CommandListener {
public:
    TextureModeModulateListener(wxFrame* frame, OCCViewer* viewer);
    virtual ~TextureModeModulateListener() = default;

    virtual CommandResult executeCommand(const std::string& commandType,
                                       const std::unordered_map<std::string, std::string>& parameters) override;
    virtual bool canHandleCommand(const std::string& commandType) const override;
    virtual std::string getListenerName() const override;

private:
    wxFrame* m_frame;
    OCCViewer* m_viewer;
}; 