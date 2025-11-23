#pragma once

#include <Inventor/events/SoEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <wx/event.h>
#include <wx/window.h>

namespace mod {

/**
 * @brief Converts wxWidgets events to Coin3D events and manages event handling
 *
 * This class provides a comprehensive bridge between wxWidgets event system
 * and Coin3D's event system, allowing Coin3D nodes to handle events directly
 * from wxWidgets windows with proper event routing and coordinate conversion.
 */
class EventConverter {
public:
    /**
     * Initialize event conversion for a wxWidgets window
     * @param window The wxWidgets window to handle events for
     */
    static void setupEventHandling(wxWindow* window);

    /**
     * Convert wxMouseEvent to Coin3D SoEvent
     * @param wxEvent The wxWidgets mouse event
     * @param viewportSize The viewport size (for coordinate conversion)
     * @return A new SoEvent instance (caller must delete)
     */
    static SoEvent* convertMouseEvent(const wxMouseEvent& wxEvent, const wxSize& viewportSize);

    /**
     * Convert wxKeyEvent to Coin3D SoKeyboardEvent
     * @param wxEvent The wxWidgets keyboard event
     * @return A new SoKeyboardEvent instance (caller must delete)
     */
    static SoKeyboardEvent* convertKeyEvent(const wxKeyEvent& wxEvent);

    /**
     * Convert wxMouseEvent to SoLocation2Event (for mouse motion)
     * @param wxEvent The wxWidgets mouse event
     * @param viewportSize The viewport size
     * @return A new SoLocation2Event instance (caller must delete)
     */
    static SoLocation2Event* convertLocation2Event(const wxMouseEvent& wxEvent, const wxSize& viewportSize);

    /**
     * Convert wxMouseEvent to SoMouseButtonEvent
     * @param wxEvent The wxWidgets mouse event
     * @param viewportSize The viewport size
     * @return A new SoMouseButtonEvent instance (caller must delete)
     */
    static SoMouseButtonEvent* convertMouseButtonEvent(const wxMouseEvent& wxEvent, const wxSize& viewportSize);

    /**
     * Convert wxWidgets key code to Coin3D key code
     */
    static SoKeyboardEvent::Key convertKeyCode(int wxKeyCode);

    /**
     * Convert wxWidgets mouse button to Coin3D mouse button
     */
    static SoMouseButtonEvent::Button convertMouseButton(int wxButton);

    /**
     * Convert screen coordinates to Coin3D viewport coordinates
     * Coin3D uses bottom-left origin, wxWidgets uses top-left
     */
    static SbVec2s convertPosition(const wxPoint& wxPos, const wxSize& viewportSize);

    /**
     * Handle wxWidgets event and route it to Coin3D scene graph
     * @param wxEvent The wxWidgets event
     * @param sceneRoot The root of the Coin3D scene graph
     * @param viewportSize The current viewport size
     * @return true if event was handled by Coin3D
     */
    static bool handleEvent(wxEvent& wxEvent, SoNode* sceneRoot, const wxSize& viewportSize);

    /**
     * Create a handle event action for the given event
     * @param event The Coin3D event
     * @param viewportSize The viewport size
     * @return A configured SoHandleEventAction (caller must delete)
     */
    static SoHandleEventAction* createHandleEventAction(SoEvent* event, const wxSize& viewportSize);
};

} // namespace mod

