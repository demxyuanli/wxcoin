#include "InputManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include "NavigationModeManager.h"
#include "logger/Logger.h"
#include "PositionBasicDialog.h"
#include "PickingAidManager.h"
#include "DefaultInputState.h"
#include "PickingInputState.h"
#include "FaceSelectionListener.h"
#include "EdgeSelectionListener.h"
#include "VertexSelectionListener.h"
#include "FaceQueryListener.h"
#include "FlatFrame.h"

const int InputManager::MOTION_INTERVAL = 10; // Mouse motion throttling (milliseconds)

InputManager::InputManager(Canvas* canvas)
	: m_canvas(canvas)
	, m_mouseHandler(nullptr)
	, m_navigationController(nullptr)
	, m_navigationModeManager(nullptr)
	, m_currentState(nullptr)
	, m_stateChangeCallback(nullptr)
	, m_lastMotionTime(0)
{
	LOG_INF_S("InputManager initializing");
}

InputManager::~InputManager() {
	LOG_INF_S("InputManager destroying");
	// Clear dependencies to prevent access to destroyed objects
	clearDependencies();
}

void InputManager::setMouseHandler(MouseHandler* handler) {
	m_mouseHandler = handler;
	LOG_INF_S("MouseHandler set for InputManager");
}

void InputManager::setNavigationController(NavigationController* controller) {
	m_navigationController = controller;
	LOG_INF_S("NavigationController set for InputManager");
}

void InputManager::setNavigationModeManager(NavigationModeManager* manager) {
	m_navigationModeManager = manager;
	LOG_INF_S("NavigationModeManager set for InputManager");
}

void InputManager::initializeStates() {
	// Only initialize if dependencies are still valid
	if (!m_canvas) {
		LOG_ERR_S("InputManager: Cannot initialize states - Canvas is null");
		return;
	}
	
	// Prefer NavigationModeManager over NavigationController for DefaultInputState
	m_defaultState = std::make_unique<DefaultInputState>(m_mouseHandler, m_navigationModeManager);
	m_pickingState = std::make_unique<PickingInputState>(m_canvas);
	m_currentState = m_defaultState.get();
	LOG_INF_S("InputManager states initialized");
}

void InputManager::enterDefaultState() {
	if (m_currentState != m_defaultState.get()) {
		InputState* oldState = m_currentState;
		
		// Validate transition
		if (!canTransitionTo(m_defaultState.get())) {
			LOG_WRN_S("InputManager: Invalid state transition to DefaultState blocked");
			return;
		}
		
		// Cleanup old custom state if it was active
		if (m_customState && m_currentState == m_customState.get()) {
			LOG_INF_S("InputManager: Deactivating custom state before entering default state");
			m_customState->deactivate();
		}
		
		m_currentState = m_defaultState.get();
		
		// Log state transition
		logStateTransition(oldState, m_currentState);
		
		// Notify state change
		if (m_stateChangeCallback) {
			m_stateChangeCallback(oldState, m_currentState);
		}
	}
}

void InputManager::enterPickingState() {
	if (m_currentState != m_pickingState.get()) {
		InputState* oldState = m_currentState;
		
		// Validate transition
		if (!canTransitionTo(m_pickingState.get())) {
			LOG_WRN_S("InputManager: Invalid state transition to PickingState blocked");
			return;
		}
		
		// Cleanup old custom state if it was active
		if (m_customState && m_currentState == m_customState.get()) {
			LOG_INF_S("InputManager: Deactivating custom state before entering picking state");
			m_customState->deactivate();
		}
		
		m_currentState = m_pickingState.get();
		
		// Log state transition
		logStateTransition(oldState, m_currentState);
		
		// Notify state change
		if (m_stateChangeCallback) {
			m_stateChangeCallback(oldState, m_currentState);
		}
	}
}

void InputManager::setCustomInputState(std::unique_ptr<InputState> state) {
	if (state) {
		InputState* oldState = m_currentState;
		InputState* newState = state.get();
		
		// Validate transition
		if (!canTransitionTo(newState)) {
			LOG_WRN_S("InputManager: Invalid state transition to custom state blocked");
			return;
		}
		
		// Cleanup old custom state before replacing it
		if (m_customState && m_currentState == m_customState.get()) {
			LOG_INF_S("InputManager: Deactivating previous custom state before replacement");
			m_customState->deactivate();
		}
		
		m_customState = std::move(state);
		m_currentState = m_customState.get();
		
		// Log state transition
		logStateTransition(oldState, m_currentState);
		
		// Notify state change
		if (m_stateChangeCallback) {
			m_stateChangeCallback(oldState, m_currentState);
		}
	}
}

void InputManager::onMouseButton(wxMouseEvent& event) {
	if (!isValidState()) {
		LOG_WRN_S("InputManager: Invalid state or dependencies, skipping mouse button event");
		event.Skip();
		return;
	}
	
	if (m_currentState) {
		m_currentState->onMouseButton(event);
	}
	else {
		LOG_WRN_S("InputManager: No active state to handle mouse button event");
		event.Skip();
	}
}

void InputManager::onMouseMotion(wxMouseEvent& event) {
	if (!isValidState()) {
		LOG_WRN_S("InputManager: Invalid state or dependencies, skipping mouse motion event");
		event.Skip();
		return;
	}
	
	if (!m_currentState) {
		LOG_WRN_S("Mouse motion event skipped: No active state");
		event.Skip();
		return;
	}

	wxLongLong currentTime = wxGetLocalTimeMillis();
	if (currentTime - m_lastMotionTime >= MOTION_INTERVAL) {
		m_currentState->onMouseMotion(event);
		m_lastMotionTime = currentTime;
	}
	else {
		event.Skip();
	}
}

void InputManager::onMouseWheel(wxMouseEvent& event) {
	if (!isValidState()) {
		LOG_WRN_S("InputManager: Invalid state or dependencies, skipping mouse wheel event");
		event.Skip();
		return;
	}
	
	if (m_currentState) {
		m_currentState->onMouseWheel(event);
	}
	else {
		LOG_WRN_S("Mouse wheel event skipped: No active state");
		event.Skip();
	}
}

MouseHandler* InputManager::getMouseHandler() const {
	return m_mouseHandler;
}

NavigationController* InputManager::getNavigationController() const {
	return m_navigationController;
}

NavigationModeManager* InputManager::getNavigationModeManager() const {
	return m_navigationModeManager;
}

void InputManager::clearDependencies() {
	// Clear state to prevent access to potentially destroyed objects
	m_currentState = nullptr;
	
	// Clear custom state first to release ownership
	m_customState.reset();
	
	// Reset to default state (which may also have invalid pointers, but at least we're in a known state)
	// Actually, better to just clear everything
	m_defaultState.reset();
	m_pickingState.reset();
	
	// Clear raw pointer references (don't delete, just clear)
	m_mouseHandler = nullptr;
	m_navigationController = nullptr;
	m_navigationModeManager = nullptr;
	m_canvas = nullptr;
}

bool InputManager::isValidState() const {
	// Check if current state pointer is valid
	if (!m_currentState) {
		return false;
	}
	
	// Check if current state points to one of our managed states
	bool isManagedState = 
		(m_currentState == m_defaultState.get()) ||
		(m_currentState == m_pickingState.get()) ||
		(m_currentState == m_customState.get());
	
	if (!isManagedState) {
		LOG_WRN_S("InputManager: Current state is not a managed state");
		return false;
	}
	
	// For DefaultInputState, check if dependencies are still valid
	// Note: We can't easily check this without RTTI, but we can at least verify
	// that the state pointer itself is valid
	
	return true;
}

void InputManager::setStateChangeCallback(StateChangeCallback callback) {
	m_stateChangeCallback = callback;
	LOG_INF_S("InputManager: State change callback set");
}

void InputManager::clearStateChangeCallback() {
	m_stateChangeCallback = nullptr;
	LOG_INF_S("InputManager: State change callback cleared");
}

int InputManager::getCurrentToolId() const {
	// Return tool ID if custom state is active and is a known tool
	if (!m_currentState || m_currentState != m_customState.get()) {
		return -1; // No tool active
	}
	
	// Use RTTI to identify tool type and map to button ID
	if (dynamic_cast<FaceSelectionListener*>(m_currentState)) {
		return ID_FACE_SELECTION_TOOL;
	} else if (dynamic_cast<EdgeSelectionListener*>(m_currentState)) {
		return ID_EDGE_SELECTION_TOOL;
	} else if (dynamic_cast<VertexSelectionListener*>(m_currentState)) {
		return ID_VERTEX_SELECTION_TOOL;
	} else if (dynamic_cast<FaceQueryListener*>(m_currentState)) {
		return ID_FACE_QUERY_TOOL;
	}
	
	return -1; // Unknown tool type
}

bool InputManager::canTransitionTo(InputState* newState) const {
	// Basic validation: check if new state is valid
	if (!newState) {
		LOG_WRN_S("InputManager::canTransitionTo - New state is null");
		return false;
	}
	
	// Check if new state is one of our managed states
	// Note: For custom states, we check if it's the current custom state OR if it's being set
	// (in setCustomInputState, the new state hasn't been assigned to m_customState yet)
	bool isManagedState = 
		(newState == m_defaultState.get()) ||
		(newState == m_pickingState.get()) ||
		(newState == m_customState.get());
	
	// If not a managed state, it might be a new custom state being set
	// In that case, we allow it (validation happens in setCustomInputState)
	if (!isManagedState) {
		// Allow if it's a new custom state (not yet in m_customState)
		// This happens when setCustomInputState is called with a new state
		LOG_INF_S("InputManager::canTransitionTo - New state is not a current managed state, allowing (new custom state)");
	}
	
	// Additional validation: check if current state is valid before transition
	if (m_currentState && !isValidState()) {
		LOG_WRN_S("InputManager::canTransitionTo - Current state is invalid, cannot transition");
		return false;
	}
	
	// Check for state conflicts
	if (detectStateConflict(newState)) {
		LOG_WRN_S("InputManager::canTransitionTo - State conflict detected, transition blocked");
		return false;
	}
	
	return true;
}

bool InputManager::detectStateConflict(InputState* newState) const {
	// Detect if there's a conflict: new state is being set while another state is active
	// This is normal for state transitions, but we log it for analysis
	
	if (!m_currentState || !newState) {
		return false; // No conflict if no current state or new state is null
	}
	
	// Check if new state is different from current state
	if (m_currentState == newState) {
		return false; // No conflict if transitioning to same state
	}
	
	// Check if current state is a custom state and new state is also a custom state
	// This indicates a tool switch, which is normal but worth logging
	if (m_currentState == m_customState.get() && newState == m_customState.get()) {
		// This shouldn't happen (same pointer), but log if it does
		LOG_WRN_S("InputManager::detectStateConflict - Attempting to set same custom state");
		return false; // Not really a conflict, just redundant
	}
	
	// If we're switching from one custom state to another, this is normal
	// But if current state is active and we're trying to set a new custom state,
	// this is expected behavior (tool switching)
	
	// Log potential conflicts for analysis (but don't block)
	if (m_currentState != m_defaultState.get() && 
		m_currentState != m_pickingState.get() &&
		m_currentState == m_customState.get()) {
		// Current state is a custom state, new state is different
		// This is normal tool switching, but log for analysis
		LOG_INF_S("InputManager::detectStateConflict - Tool switch detected (normal operation)");
	}
	
	return false; // Don't block transitions, just log for analysis
}

void InputManager::logStateTransition(InputState* oldState, InputState* newState) const {
	// Get state type names for logging
	std::string oldStateName = "Unknown";
	std::string newStateName = "Unknown";
	
	if (oldState == m_defaultState.get()) {
		oldStateName = "DefaultInputState";
	} else if (oldState == m_pickingState.get()) {
		oldStateName = "PickingInputState";
	} else if (oldState == m_customState.get()) {
		// Try to identify custom state type
		if (dynamic_cast<FaceSelectionListener*>(oldState)) {
			oldStateName = "FaceSelectionListener";
		} else if (dynamic_cast<EdgeSelectionListener*>(oldState)) {
			oldStateName = "EdgeSelectionListener";
		} else if (dynamic_cast<VertexSelectionListener*>(oldState)) {
			oldStateName = "VertexSelectionListener";
		} else if (dynamic_cast<FaceQueryListener*>(oldState)) {
			oldStateName = "FaceQueryListener";
		} else {
			oldStateName = "CustomInputState";
		}
	} else if (oldState == nullptr) {
		oldStateName = "None";
	}
	
	if (newState == m_defaultState.get()) {
		newStateName = "DefaultInputState";
	} else if (newState == m_pickingState.get()) {
		newStateName = "PickingInputState";
	} else if (newState == m_customState.get()) {
		// Try to identify custom state type
		if (dynamic_cast<FaceSelectionListener*>(newState)) {
			newStateName = "FaceSelectionListener";
		} else if (dynamic_cast<EdgeSelectionListener*>(newState)) {
			newStateName = "EdgeSelectionListener";
		} else if (dynamic_cast<VertexSelectionListener*>(newState)) {
			newStateName = "VertexSelectionListener";
		} else if (dynamic_cast<FaceQueryListener*>(newState)) {
			newStateName = "FaceQueryListener";
		} else {
			newStateName = "CustomInputState";
		}
	}
	
	LOG_INF_S("InputManager state transition: " + oldStateName + " -> " + newStateName);
}