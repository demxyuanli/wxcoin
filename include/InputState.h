#pragma once

class wxMouseEvent;

class InputState {
public:
	virtual ~InputState() = default;

	virtual void onMouseButton(wxMouseEvent& event) = 0;
	virtual void onMouseMotion(wxMouseEvent& event) = 0;
	virtual void onMouseWheel(wxMouseEvent& event) = 0;
	
	// Optional cleanup method called before state is replaced
	// Override in derived classes to clean up resources, hide overlays, etc.
	virtual void deactivate() {
		// Default implementation does nothing
		// Derived classes can override to perform cleanup
	}
};