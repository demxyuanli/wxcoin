#pragma once

#include <wx/event.h>
#include <wx/types.h>
#include <memory>
#include "InputState.h"

class Canvas;
class MouseHandler;
class NavigationController;

class InputManager {
public:
	static const int MOTION_INTERVAL;
	explicit InputManager(Canvas* canvas);
	~InputManager();

	void setMouseHandler(MouseHandler* handler);
	void setNavigationController(NavigationController* controller);
	void initializeStates();

	// State management
	void enterDefaultState();
	void enterPickingState();
	void setCustomInputState(std::unique_ptr<InputState> state);
	bool isCustomInputStateActive() const { return m_currentState == m_customState.get(); }

	// Event handlers
	void onMouseButton(wxMouseEvent& event);
	void onMouseMotion(wxMouseEvent& event);
	void onMouseWheel(wxMouseEvent& event);

	MouseHandler* getMouseHandler() const;
	NavigationController* getNavigationController() const;

private:
	Canvas* m_canvas;
	MouseHandler* m_mouseHandler;
	NavigationController* m_navigationController;

	std::unique_ptr<InputState> m_defaultState;
	std::unique_ptr<InputState> m_pickingState;
	std::unique_ptr<InputState> m_customState;
	InputState* m_currentState;

	wxLongLong m_lastMotionTime;
};