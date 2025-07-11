#include "PickingInputState.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "PositionDialog.h"
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
        LOG_INF_S("Picked position: " + std::to_string(worldPos[0]) + ", " + std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]));

        wxWindow* dialog = wxWindow::FindWindowByName("PositionDialog");
        if (dialog) {
            PositionDialog* posDialog = dynamic_cast<PositionDialog*>(dialog);
            if (posDialog) {
                posDialog->SetPosition(worldPos);
                posDialog->Show(true);
                LOG_INF_S("Position dialog updated and shown");
                // The dialog itself will call stopPicking, which will trigger the state change
            } else {
                LOG_ERR_S("Failed to cast dialog to PositionDialog");
            }
        } else {
            LOG_ERR_S("PositionDialog not found");
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
