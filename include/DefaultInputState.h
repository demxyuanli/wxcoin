#pragma once

#include "InputState.h"

class MouseHandler;
class NavigationController;

class DefaultInputState : public InputState {
public:
    DefaultInputState(MouseHandler* mouseHandler, NavigationController* navigationController);

    void onMouseButton(wxMouseEvent& event) override;
    void onMouseMotion(wxMouseEvent& event) override;
    void onMouseWheel(wxMouseEvent& event) override;

private:
    MouseHandler* m_mouseHandler;
    NavigationController* m_navigationController;
}; 