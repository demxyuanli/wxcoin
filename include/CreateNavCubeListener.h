#pragma once
#include "CommandListener.h"
#include <memory>

class MouseHandler;

class CreateNavCubeListener : public CommandListener {
public:
	explicit CreateNavCubeListener(MouseHandler* mouseHandler);
	virtual ~CreateNavCubeListener() = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters = {}) override;

	bool canHandleCommand(const std::string& commandType) const override;

	std::string getListenerName() const override { return "CreateNavCubeListener"; }

private:
	MouseHandler* m_mouseHandler;
};
