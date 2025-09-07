#include "RenderingSettingsListener.h"
#include "RenderingSettingsDialog.h"
#include "OCCViewer.h"
#include "RenderingEngine.h"
#include "CommandDispatcher.h"
#include <wx/wx.h>

RenderingSettingsListener::RenderingSettingsListener(OCCViewer* occViewer, RenderingEngine* renderingEngine)
	: m_occViewer(occViewer)
	, m_renderingEngine(renderingEngine)
{
}

RenderingSettingsListener::~RenderingSettingsListener()
{
}

CommandResult RenderingSettingsListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	if (commandType == "RENDERING_SETTINGS")
	{
		// Get the parent window (main frame)
		wxWindow* parent = wxGetActiveWindow();
		if (!parent) {
			parent = wxTheApp->GetTopWindow();
		}

		// Create and show the rendering settings dialog
		RenderingSettingsDialog dialog(parent, m_occViewer, m_renderingEngine);
		dialog.ShowModal();

		return CommandResult(true, "Rendering settings dialog opened", "RENDERING_SETTINGS");
	}

	return CommandResult(false, "Unknown command type", "RENDERING_SETTINGS");
}

CommandResult RenderingSettingsListener::executeCommand(cmd::CommandType commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	return executeCommand(cmd::to_string(commandType), parameters);
}

bool RenderingSettingsListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == "RENDERING_SETTINGS";
}

std::string RenderingSettingsListener::getListenerName() const
{
	return "RenderingSettingsListener";
}