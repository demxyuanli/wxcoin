#include "LightingSettingsListener.h"
#include "LightingSettingsDialog.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/frame.h>

LightingSettingsListener::LightingSettingsListener(wxFrame* frame)
	: m_frame(frame)
{
}

CommandResult LightingSettingsListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	LOG_INF_S("LightingSettingsListener: Executing lighting settings command");
	showLightingSettingsDialog();

	CommandResult result;
	result.success = true;
	result.message = "Lighting settings dialog opened";
	return result;
}

bool LightingSettingsListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == "LIGHTING_SETTINGS";
}

void LightingSettingsListener::showLightingSettingsDialog()
{
	if (!m_frame) {
		LOG_ERR_S("LightingSettingsListener: Frame not available");
		return;
	}

	LightingSettingsDialog dialog(m_frame, wxID_ANY, "Lighting Settings", wxDefaultPosition, wxDefaultSize);

	int result = dialog.ShowModal();
	if (result == wxID_OK) {
		LOG_INF_S("LightingSettingsListener: Lighting settings applied and saved");
		wxMessageBox("Lighting settings have been applied and saved to configuration file.",
			"Settings Saved", wxOK | wxICON_INFORMATION);
	}
	else {
		LOG_INF_S("LightingSettingsListener: Lighting settings dialog cancelled");
	}
}