#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class MouseHandler;
class CreateCylinderListener : public CommandListener {
public:
	explicit CreateCylinderListener(MouseHandler* mouseHandler);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "CreateCylinderListener"; }
private:
	MouseHandler* m_mouseHandler;
};