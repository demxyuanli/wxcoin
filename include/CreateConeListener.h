#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class MouseHandler;
class CreateConeListener : public CommandListener {
public:
	explicit CreateConeListener(MouseHandler* mouseHandler);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "CreateConeListener"; }
private:
	MouseHandler* m_mouseHandler;
};