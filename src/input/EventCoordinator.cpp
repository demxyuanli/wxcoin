#include "EventCoordinator.h"
#include "NavigationCubeManager.h"
#include "InputManager.h"
#include "Logger.h"

EventCoordinator::EventCoordinator()
    : m_navigationCubeManager(nullptr)
    , m_inputManager(nullptr)
{
    LOG_INF("EventCoordinator::EventCoordinator: Initializing");
}

EventCoordinator::~EventCoordinator() {
    LOG_INF("EventCoordinator::~EventCoordinator: Destroying");
}

bool EventCoordinator::handleMouseEvent(wxMouseEvent& event) {
    if (!m_inputManager) {
        LOG_WRN("EventCoordinator::handleMouseEvent: InputManager is null");
        return false;
    }

    // Give navigation cube manager first chance to handle event
    if (m_navigationCubeManager && m_navigationCubeManager->handleMouseEvent(event)) {
        return true; // Event was handled by the cube
    }

    // Forward other events to InputManager
    if (event.GetEventType() == wxEVT_LEFT_DOWN ||
        event.GetEventType() == wxEVT_LEFT_UP ||
        event.GetEventType() == wxEVT_RIGHT_DOWN ||
        event.GetEventType() == wxEVT_RIGHT_UP) {
        m_inputManager->onMouseButton(event);
        return true;
    }
    else if (event.GetEventType() == wxEVT_MOTION) {
        m_inputManager->onMouseMotion(event);
        return true;
    }
    else if (event.GetEventType() == wxEVT_MOUSEWHEEL) {
        m_inputManager->onMouseWheel(event);
        return true;
    }

    return false; // Event not handled
}

void EventCoordinator::handleSizeEvent(wxSizeEvent& event) {
    // Size events are handled by ViewportManager
    // This method is here for potential future coordination needs
    //LOG_DBG("EventCoordinator::handleSizeEvent: Size event coordinated");
}

void EventCoordinator::handlePaintEvent(wxPaintEvent& event) {
    // Paint events are handled by RenderingEngine
    // This method is here for potential future coordination needs
    //LOG_DBG("EventCoordinator::handlePaintEvent: Paint event coordinated");
} 