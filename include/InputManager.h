#pragma once

#include <wx/event.h>

class Canvas;
class MouseHandler;
class NavigationController;

class InputManager {
public:
    static const int MOTION_INTERVAL;
    InputManager(Canvas* canvas);
    ~InputManager();

    void setMouseHandler(MouseHandler* handler);
    void setNavigationController(NavigationController* controller);
    MouseHandler* getMouseHandler() const { return m_mouseHandler; } // Added
    NavigationController* getNavigationController() const { return m_navigationController; } // Added

    void onMouseButton(wxMouseEvent& event);
    void onMouseMotion(wxMouseEvent& event);
    void onMouseWheel(wxMouseEvent& event);

private:
    Canvas* m_canvas;
    MouseHandler* m_mouseHandler;
    NavigationController* m_navigationController;
    wxLongLong m_lastMotionTime;
};