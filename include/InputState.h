#pragma once

class wxMouseEvent;

class InputState {
public:
    virtual ~InputState() = default;

    virtual void onMouseButton(wxMouseEvent& event) = 0;
    virtual void onMouseMotion(wxMouseEvent& event) = 0;
    virtual void onMouseWheel(wxMouseEvent& event) = 0;
}; 