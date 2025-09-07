#pragma once

#include "InputState.h"

class Canvas;

class PickingInputState : public InputState {
public:
	explicit PickingInputState(Canvas* canvas);

	void onMouseButton(wxMouseEvent& event) override;
	void onMouseMotion(wxMouseEvent& event) override;
	void onMouseWheel(wxMouseEvent& event) override;

private:
	Canvas* m_canvas;
};