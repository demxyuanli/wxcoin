#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <wx/frame.h>

class SceneManager;

class CoordinateSystemVisibilityListener : public CommandListener
{
public:
	CoordinateSystemVisibilityListener(wxFrame* frame, SceneManager* sceneManager);
	~CoordinateSystemVisibilityListener();

	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "CoordinateSystemVisibilityListener"; }

private:
	wxFrame* m_frame;
	SceneManager* m_sceneManager;
};