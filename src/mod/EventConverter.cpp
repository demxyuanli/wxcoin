#include "mod/EventConverter.h"
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>

namespace mod {

SoEvent* EventConverter::convertMouseEvent(const wxMouseEvent& wxEvent, const wxSize& viewportSize)
{
    if (wxEvent.Moving() || wxEvent.Dragging() || wxEvent.Leaving()) {
        return convertLocation2Event(wxEvent, viewportSize);
    } else if (wxEvent.IsButton() || wxEvent.ButtonDown() || wxEvent.ButtonUp()) {
        return convertMouseButtonEvent(wxEvent, viewportSize);
    }
    return nullptr;
}

SoLocation2Event* EventConverter::convertLocation2Event(const wxMouseEvent& wxEvent, const wxSize& viewportSize)
{
    SoLocation2Event* event = new SoLocation2Event();
    
    SbVec2s pos = convertPosition(wxEvent.GetPosition(), viewportSize);
    event->setPosition(pos);
    
    // Set modifier keys
    event->setShiftDown(wxEvent.ShiftDown());
    event->setCtrlDown(wxEvent.ControlDown());
    event->setAltDown(wxEvent.AltDown());
    
    return event;
}

SoMouseButtonEvent* EventConverter::convertMouseButtonEvent(const wxMouseEvent& wxEvent, const wxSize& viewportSize)
{
    SoMouseButtonEvent* event = new SoMouseButtonEvent();
    
    SbVec2s pos = convertPosition(wxEvent.GetPosition(), viewportSize);
    event->setPosition(pos);
    
    // Convert button
    SoMouseButtonEvent::Button button = convertMouseButton(wxEvent.GetButton());
    event->setButton(button);
    
    // Set button state
    if (wxEvent.ButtonDown() || wxEvent.ButtonDClick()) {
        event->setState(SoButtonEvent::DOWN);
    } else if (wxEvent.ButtonUp()) {
        event->setState(SoButtonEvent::UP);
    }
    
    // Set modifier keys
    event->setShiftDown(wxEvent.ShiftDown());
    event->setCtrlDown(wxEvent.ControlDown());
    event->setAltDown(wxEvent.AltDown());
    
    return event;
}

SoKeyboardEvent* EventConverter::convertKeyEvent(const wxKeyEvent& wxEvent)
{
    SoKeyboardEvent* event = new SoKeyboardEvent();
    
    SoKeyboardEvent::Key key = convertKeyCode(wxEvent.GetKeyCode());
    event->setKey(key);
    
    // Set key state
    if (wxEvent.GetEventType() == wxEVT_KEY_DOWN) {
        event->setState(SoButtonEvent::DOWN);
    } else if (wxEvent.GetEventType() == wxEVT_KEY_UP) {
        event->setState(SoButtonEvent::UP);
    }
    
    // Set modifier keys
    event->setShiftDown(wxEvent.ShiftDown());
    event->setCtrlDown(wxEvent.ControlDown());
    event->setAltDown(wxEvent.AltDown());
    
    return event;
}

SbVec2s EventConverter::convertPosition(const wxPoint& wxPos, const wxSize& viewportSize)
{
    // Coin3D uses bottom-left origin, wxWidgets uses top-left
    int coinY = viewportSize.GetHeight() - wxPos.y - 1;
    return SbVec2s(wxPos.x, coinY);
}

SoMouseButtonEvent::Button EventConverter::convertMouseButton(int wxButton)
{
    switch (wxButton) {
    case wxMOUSE_BTN_LEFT:
        return SoMouseButtonEvent::BUTTON1;
    case wxMOUSE_BTN_MIDDLE:
        return SoMouseButtonEvent::BUTTON2;
    case wxMOUSE_BTN_RIGHT:
        return SoMouseButtonEvent::BUTTON3;
    default:
        return SoMouseButtonEvent::BUTTON1;
    }
}

SoKeyboardEvent::Key EventConverter::convertKeyCode(int wxKeyCode)
{
    // Map common wxWidgets key codes to Coin3D key codes
    switch (wxKeyCode) {
    case WXK_ESCAPE:
        return SoKeyboardEvent::ESCAPE;
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        return SoKeyboardEvent::RETURN;
    case WXK_SPACE:
        return SoKeyboardEvent::SPACE;
    case WXK_TAB:
        return SoKeyboardEvent::TAB;
    case WXK_BACK:
        return SoKeyboardEvent::BACKSPACE;
    case WXK_DELETE: {
        // Avoid Windows DELETE macro conflict
        #ifdef DELETE
        #undef DELETE
        #endif
        return SoKeyboardEvent::DELETE;
        }
    case WXK_UP:
        return SoKeyboardEvent::UP_ARROW;
    case WXK_DOWN:
        return SoKeyboardEvent::DOWN_ARROW;
    case WXK_LEFT:
        return SoKeyboardEvent::LEFT_ARROW;
    case WXK_RIGHT:
        return SoKeyboardEvent::RIGHT_ARROW;
    case WXK_HOME:
        return SoKeyboardEvent::HOME;
    case WXK_END:
        return SoKeyboardEvent::END;
    case WXK_PAGEUP:
        return SoKeyboardEvent::PAGE_UP;
    case WXK_PAGEDOWN:
        return SoKeyboardEvent::PAGE_DOWN;
    case WXK_F1:
        return SoKeyboardEvent::F1;
    case WXK_F2:
        return SoKeyboardEvent::F2;
    case WXK_F3:
        return SoKeyboardEvent::F3;
    case WXK_F4:
        return SoKeyboardEvent::F4;
    case WXK_F5:
        return SoKeyboardEvent::F5;
    case WXK_F6:
        return SoKeyboardEvent::F6;
    case WXK_F7:
        return SoKeyboardEvent::F7;
    case WXK_F8:
        return SoKeyboardEvent::F8;
    case WXK_F9:
        return SoKeyboardEvent::F9;
    case WXK_F10:
        return SoKeyboardEvent::F10;
    case WXK_F11:
        return SoKeyboardEvent::F11;
    case WXK_F12:
        return SoKeyboardEvent::F12;
    default:
        // For ASCII characters, use the character code directly
        if (wxKeyCode >= 32 && wxKeyCode <= 126) {
            return static_cast<SoKeyboardEvent::Key>(wxKeyCode);
        }
        return SoKeyboardEvent::ANY;
    }
}

void EventConverter::setupEventHandling(wxWindow* window)
{
    // Note: Event handling setup would typically be done in the wxWindow
    // subclass. This method provides a centralized place to configure
    // event routing if needed.
    // The actual event handling is done in Canvas.cpp or similar window classes.
}

bool EventConverter::handleEvent(wxEvent& wxEvent, SoNode* sceneRoot, const wxSize& viewportSize)
{
    if (!sceneRoot) {
        return false;
    }

    // Convert wxEvent to SoEvent
    SoEvent* coinEvent = nullptr;

    if (wxEvent.GetEventType() == wxEVT_LEFT_DOWN ||
        wxEvent.GetEventType() == wxEVT_LEFT_UP ||
        wxEvent.GetEventType() == wxEVT_RIGHT_DOWN ||
        wxEvent.GetEventType() == wxEVT_RIGHT_UP ||
        wxEvent.GetEventType() == wxEVT_MIDDLE_DOWN ||
        wxEvent.GetEventType() == wxEVT_MIDDLE_UP ||
        wxEvent.GetEventType() == wxEVT_LEFT_DCLICK ||
        wxEvent.GetEventType() == wxEVT_MOTION ||
        wxEvent.GetEventType() == wxEVT_MOUSEWHEEL) {

        wxMouseEvent& mouseEvent = static_cast<wxMouseEvent&>(wxEvent);
        coinEvent = convertMouseEvent(mouseEvent, viewportSize);

    } else if (wxEvent.GetEventType() == wxEVT_KEY_DOWN ||
               wxEvent.GetEventType() == wxEVT_KEY_UP) {

        wxKeyEvent& keyEvent = static_cast<wxKeyEvent&>(wxEvent);
        coinEvent = convertKeyEvent(keyEvent);
    }

    if (!coinEvent) {
        return false;
    }

    // Create and apply handle event action
    SoHandleEventAction* action = createHandleEventAction(coinEvent, viewportSize);

    if (action) {
        action->apply(sceneRoot);
        bool handled = action->isHandled();
        delete action;

        // Coin event is managed by the action, don't delete it here
        // delete coinEvent; // Commented out as action manages lifetime

        return handled;
    }

    delete coinEvent;
    return false;
}

SoHandleEventAction* EventConverter::createHandleEventAction(SoEvent* event, const wxSize& viewportSize)
{
    if (!event) {
        return nullptr;
    }

    SoHandleEventAction* action = new SoHandleEventAction(SbViewportRegion(viewportSize.GetWidth(), viewportSize.GetHeight()));
    action->setEvent(event);

    return action;
}

} // namespace mod

