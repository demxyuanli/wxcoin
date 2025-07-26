#include "PickingInputState.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "PositionBasicDialog.h"
#include "MouseHandler.h"
#include "InputManager.h"
#include "logger/Logger.h"
#include <wx/wx.h>

PickingInputState::PickingInputState(Canvas* canvas)
    : m_canvas(canvas) {}

void PickingInputState::onMouseButton(wxMouseEvent& event) {
    if (!m_canvas || !event.LeftDown()) {
        event.Skip();
        return;
    }

    LOG_INF_S("Picking position with mouse click (PickingInputState)");
    SbVec3f worldPos;
    if (m_canvas->getSceneManager()->screenToWorld(event.GetPosition(), worldPos)) {
        LOG_INF_S("[PickingDebug] Picked position: " + std::to_string(worldPos[0]) + ", " + std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]));

        // Get the MouseHandler through InputManager
        InputManager* inputManager = m_canvas->getInputManager();
        if (inputManager) {
            MouseHandler* mouseHandler = inputManager->getMouseHandler();
            if (mouseHandler) {
                // Call the position picked handler
                mouseHandler->OnPositionPicked(worldPos);
                LOG_INF_S("Position picked and sent to MouseHandler");
            } else {
                LOG_ERR_S("MouseHandler is null in PickingInputState");
            }
        } else {
            LOG_ERR_S("InputManager is null in PickingInputState");
        }
    } else {
        LOG_WRN_S("Failed to convert screen position to world coordinates");
    }
}

void PickingInputState::onMouseMotion(wxMouseEvent& event) {
    if (!m_canvas) {
        event.Skip();
        return;
    }

    SbVec3f worldPos;
    if (m_canvas->getSceneManager()->screenToWorld(event.GetPosition(), worldPos)) {
        m_canvas->getSceneManager()->getPickingAidManager()->showPickingAidLines(worldPos);
    }
    event.Skip();
}

void PickingInputState::onMouseWheel(wxMouseEvent& event) {
    // Do nothing during picking
    event.Skip();
} 
