#pragma once

#include "CommandListener.h"
#include <wx/frame.h>

class OCCViewer;

class MeshQualityDialogListener : public CommandListener {
public:
    MeshQualityDialogListener(wxFrame* frame, OCCViewer* occViewer);
    ~MeshQualityDialogListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "MeshQualityDialogListener"; }

private:
    wxFrame* m_frame;
    OCCViewer* m_occViewer;
}; 