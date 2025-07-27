#pragma once

#include "CommandListener.h"
#include "renderpreview/RenderPreviewDialog.h"
#include <wx/wx.h>

class RenderPreviewSystemListener : public CommandListener
{
public:
    RenderPreviewSystemListener(wxWindow* parent);
    CommandResult executeCommand(const std::string& commandType, const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;

private:
    wxWindow* m_parent;
};