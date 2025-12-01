#include "DefaultInputState.h"
#include "MouseHandler.h"
#include "NavigationModeManager.h"
#include <wx/event.h>

DefaultInputState::DefaultInputState(MouseHandler* mouseHandler, NavigationModeManager* navigationModeManager)
	: m_mouseHandler(mouseHandler), m_navigationModeManager(navigationModeManager) {
}

void DefaultInputState::onMouseButton(wxMouseEvent& event) {
	if (m_mouseHandler) {
		m_mouseHandler->handleMouseButton(event);
	}
	else {
		event.Skip();
	}
}

void DefaultInputState::onMouseMotion(wxMouseEvent& event) {
	if (m_mouseHandler) {
		m_mouseHandler->handleMouseMotion(event);
	}
	else {
		event.Skip();
	}
}

void DefaultInputState::onMouseWheel(wxMouseEvent& event) {
	if (m_navigationModeManager) {
		m_navigationModeManager->handleMouseWheel(event);
	}
	else {
		event.Skip();
	}
}