#pragma once

#include "CommandListener.h"
#include "CommandType.h"

class MouseHandler;

class CreateBoxListener : public CommandListener {
public:
	explicit CreateBoxListener(MouseHandler* mouseHandler);
	~CreateBoxListener() override = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "CreateBoxListener"; }

private:
	MouseHandler* m_mouseHandler;
};