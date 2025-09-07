#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class Canvas;
class NavCubeConfigListener : public CommandListener {
public:
	explicit NavCubeConfigListener(Canvas* canvas);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "NavCubeConfigListener"; }
private:
	Canvas* m_canvas;
};