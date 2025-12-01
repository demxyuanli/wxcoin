#pragma once

#include <wx/event.h>
#include <wx/types.h>
#include <memory>
#include <functional>
#include "InputState.h"

class Canvas;
class MouseHandler;
class NavigationController;
class NavigationModeManager;

class InputManager {
public:
	static const int MOTION_INTERVAL;
	explicit InputManager(Canvas* canvas);
	~InputManager();

	void setMouseHandler(MouseHandler* handler);
	void setNavigationController(NavigationController* controller);
	void setNavigationModeManager(NavigationModeManager* manager);
	void initializeStates();

	// State management
	void enterDefaultState();
	void enterPickingState();
	void setCustomInputState(std::unique_ptr<InputState> state);
	bool isCustomInputStateActive() const { return m_currentState == m_customState.get(); }
	InputState* getCurrentInputState() const { return m_currentState; }

	// Event handlers
	void onMouseButton(wxMouseEvent& event);
	void onMouseMotion(wxMouseEvent& event);
	void onMouseWheel(wxMouseEvent& event);

	MouseHandler* getMouseHandler() const;
	NavigationController* getNavigationController() const;
	NavigationModeManager* getNavigationModeManager() const;
	
	// Lifecycle management
	void clearDependencies(); // Clear all raw pointer dependencies before destruction
	
	// State change notifications for synchronization
	using StateChangeCallback = std::function<void(InputState* oldState, InputState* newState)>;
	void setStateChangeCallback(StateChangeCallback callback);
	void clearStateChangeCallback();
	
	// Get current tool ID if custom state is active (for ButtonGroup sync)
	int getCurrentToolId() const;
	
	// State transition validation and logging
	bool canTransitionTo(InputState* newState) const;
	void logStateTransition(InputState* oldState, InputState* newState) const;
	
	// State conflict detection
	bool detectStateConflict(InputState* newState) const;

private:
	Canvas* m_canvas;
	MouseHandler* m_mouseHandler;
	NavigationController* m_navigationController;
	NavigationModeManager* m_navigationModeManager;
	
	// Safety checks
	bool isValidState() const; // Check if current state and dependencies are valid

	std::unique_ptr<InputState> m_defaultState;
	std::unique_ptr<InputState> m_pickingState;
	std::unique_ptr<InputState> m_customState;
	InputState* m_currentState;
	
	// State change notification
	StateChangeCallback m_stateChangeCallback;

	wxLongLong m_lastMotionTime;
};