#include "DefaultInputState.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include <wx/event.h>

DefaultInputState::DefaultInputState(MouseHandler* mouseHandler, NavigationController* navigationController)
	: m_mouseHandler(mouseHandler), m_navigationController(navigationController) {
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
	if (m_navigationController) {
		m_navigationController->handleMouseWheel(event);
	}
	else {
		event.Skip();
	}
}