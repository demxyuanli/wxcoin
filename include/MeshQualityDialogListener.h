#pragma once

#include "CommandListener.h"
#include <memory>

class OCCViewer;
class wxWindow;

class MeshQualityDialogListener : public CommandListener
{
public:
    MeshQualityDialogListener(wxWindow* parent, OCCViewer* occViewer);
    virtual ~MeshQualityDialogListener() = default;

    CommandResult executeCommand(const std::string& commandType, const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;

    CommandResult execute(const std::unordered_map<std::string, std::string>& parameters);

private:
    wxWindow* m_parent;
    OCCViewer* m_occViewer;
}; 