#pragma once

#include "CommandListener.h"
#include <wx/frame.h>

class Canvas;

class ZoomSpeedListener : public CommandListener {
public:
	ZoomSpeedListener(wxFrame* frame, Canvas* canvas);
	~ZoomSpeedListener() override = default;

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "ZoomSpeedListener"; }

private:
	wxFrame* m_frame;
	Canvas* m_canvas;
};