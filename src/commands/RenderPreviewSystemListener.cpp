#include "RenderPreviewSystemListener.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "logger/Logger.h"

RenderPreviewSystemListener::RenderPreviewSystemListener(wxWindow* parent)
    : m_parent(parent)
{
}

CommandResult RenderPreviewSystemListener::executeCommand(const std::string& commandType, const std::unordered_map<std::string, std::string>& parameters)
{
    LOG_INF_S("Opening Render Preview System dialog.");
    RenderPreviewDialog dialog(m_parent);
    dialog.ShowModal();
    return CommandResult(true, "Render Preview System dialog opened.");
}

bool RenderPreviewSystemListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::RenderPreviewSystem);
}

std::string RenderPreviewSystemListener::getListenerName() const
{
    return "RenderPreviewSystemListener";
}
