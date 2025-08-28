#pragma once

#include "CommandListener.h"
#include <wx/frame.h>

class FileExitListener : public CommandListener {
public:
	FileExitListener(wxFrame* frame);
	~FileExitListener() override = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "FileExitListener"; }

private:
	wxFrame* m_frame;
};