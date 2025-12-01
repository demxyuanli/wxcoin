#pragma once

#include "InputState.h"

class MouseHandler;
class NavigationModeManager;

class DefaultInputState : public InputState {
public:
	DefaultInputState(MouseHandler* mouseHandler, NavigationModeManager* navigationModeManager);

	void onMouseButton(wxMouseEvent& event) override;
	void onMouseMotion(wxMouseEvent& event) override;
	void onMouseWheel(wxMouseEvent& event) override;

private:
	MouseHandler* m_mouseHandler;
	NavigationModeManager* m_navigationModeManager;
};