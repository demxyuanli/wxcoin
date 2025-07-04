#pragma once

#include <wx/event.h>

// Forward declarations
class NavigationCubeManager;
class InputManager;

class EventCoordinator {
public:
    EventCoordinator();
    ~EventCoordinator();

    void setNavigationCubeManager(NavigationCubeManager* navCubeManager) { m_navigationCubeManager = navCubeManager; }
    void setInputManager(InputManager* inputManager) { m_inputManager = inputManager; }

    bool handleMouseEvent(wxMouseEvent& event);
    void handleSizeEvent(wxSizeEvent& event);
    void handlePaintEvent(wxPaintEvent& event);

private:
    NavigationCubeManager* m_navigationCubeManager;
    InputManager* m_inputManager;
}; 