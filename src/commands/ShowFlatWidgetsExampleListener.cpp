#include "ShowFlatWidgetsExampleListener.h"
#include "widgets/FlatWidgetsExampleDialog.h"
#include "logger/Logger.h"

ShowFlatWidgetsExampleListener::ShowFlatWidgetsExampleListener(wxWindow* parent)
	: m_parent(parent)
{
}

CommandResult ShowFlatWidgetsExampleListener::executeCommand(const std::string& commandType, const std::unordered_map<std::string, std::string>& parameters)
{
	LOG_INF_S("Opening Flat Widgets Example dialog.");
	FlatWidgetsExampleDialog dialog(m_parent, "Flat Widgets Example");
	dialog.ShowModal();
	return CommandResult(true, "Flat Widgets Example dialog opened.");
}

bool ShowFlatWidgetsExampleListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == cmd::to_string(cmd::CommandType::ShowFlatWidgetsExample);
}

std::string ShowFlatWidgetsExampleListener::getListenerName() const
{
	return "ShowFlatWidgetsExampleListener";
}