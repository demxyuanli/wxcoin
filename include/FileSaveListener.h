#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <memory>
#include <wx/frame.h>

class FileSaveListener : public CommandListener {
public:
	explicit FileSaveListener(wxFrame* frame);
	~FileSaveListener() override = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "FileSaveListener"; }

private:
	wxFrame* m_frame;
};