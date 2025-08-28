#pragma once

#include "CommandListener.h"
#include "LightingSettingsDialog.h"

class LightingSettingsListener : public CommandListener
{
public:
	LightingSettingsListener(wxFrame* frame);
	virtual ~LightingSettingsListener() = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "LightingSettingsListener"; }

private:
	void showLightingSettingsDialog();

	wxFrame* m_frame;
};