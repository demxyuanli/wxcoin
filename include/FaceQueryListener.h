#pragma once

#include "InputState.h"
#include "viewer/PickingService.h"

/**
 * @brief Face query input state for handling face picking and information display
 */
class FaceQueryListener : public InputState
{
public:
	FaceQueryListener(class Canvas* canvas, PickingService* pickingService);

	virtual void onMouseButton(wxMouseEvent& event) override;
	virtual void onMouseMotion(wxMouseEvent& event) override;
	virtual void onMouseWheel(wxMouseEvent& event) override;

private:
	class Canvas* m_canvas;
	PickingService* m_pickingService;
};
