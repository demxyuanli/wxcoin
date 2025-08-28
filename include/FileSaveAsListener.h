#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <memory>
#include <wx/frame.h>

class FileSaveAsListener : public CommandListener {
public:
	explicit FileSaveAsListener(wxFrame* frame);
	~FileSaveAsListener() override = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "FileSaveAsListener"; }

private:
	wxFrame* m_frame;
};