#pragma once

#include "CommandListener.h"

class FlatFrame;

class SelectionHighlightConfigListener : public CommandListener
{
public:
	SelectionHighlightConfigListener(FlatFrame* frame = nullptr);
	virtual ~SelectionHighlightConfigListener();

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;

	CommandResult executeCommand(cmd::CommandType commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;

	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override;

private:
	FlatFrame* m_frame;
};

