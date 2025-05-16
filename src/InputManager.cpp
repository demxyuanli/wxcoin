#include "InputManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "NavigationCube.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include "Logger.h"
#include "PositionDialog.h"

const int InputManager::MOTION_INTERVAL = 10; // Mouse motion throttling (milliseconds)

InputManager::InputManager(Canvas* canvas)
    : m_canvas(canvas)
    , m_mouseHandler(nullptr)
    , m_navigationController(nullptr)
    , m_lastMotionTime(0)
{
    LOG_INF("InputManager initializing");
}

InputManager::~InputManager() {
    LOG_INF("InputManager destroying");
}

void InputManager::setMouseHandler(MouseHandler* handler) {
    m_mouseHandler = handler;
    LOG_INF("MouseHandler set for InputManager");
}

void InputManager::setNavigationController(NavigationController* controller) {
    m_navigationController = controller;
    LOG_INF("NavigationController set for InputManager");
}

void InputManager::onMouseButton(wxMouseEvent& event) {
    if (!m_mouseHandler) {
        LOG_WAR("Mouse button event skipped: Invalid handler");
        event.Skip();
        return;
    }

    // Check if clicking on navigation cube
    if (event.LeftDown() && m_canvas->getNavigationCube() && m_canvas->getNavigationCube()->isEnabled()) {
        m_canvas->getNavigationCube()->handleMouseClick(event, m_canvas->GetClientSize());
    }

    if (g_isPickingPosition && event.LeftDown()) {
        LOG_INF("Picking position with mouse click");
        SbVec3f worldPos;
        if (m_canvas->getSceneManager()->screenToWorld(event.GetPosition(), worldPos)) {
            LOG_INF("Picked position: " + std::to_string(worldPos[0]) + ", " + std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]));

            wxWindow* dialog = wxWindow::FindWindowByName("PositionDialog");
            if (dialog) {
                PositionDialog* posDialog = dynamic_cast<PositionDialog*>(dialog);
                if (posDialog) {
                    posDialog->SetPosition(worldPos);
                    posDialog->Show(true);
                    LOG_INF("Position dialog updated and shown");
                    g_isPickingPosition = false;
                }
                else {
                    LOG_ERR("Failed to cast dialog to PositionDialog");
                }
            }
            else {
                LOG_ERR("PositionDialog not found");
            }
        }
        else {
            LOG_WAR("Failed to convert screen position to world coordinates");
        }
    }
    else {
        m_mouseHandler->handleMouseButton(event);
    }
    event.Skip();
}

void InputManager::onMouseMotion(wxMouseEvent& event) {
    if (!m_mouseHandler || !m_navigationController) {
        LOG_WAR("Mouse motion event skipped: Invalid handler or controller");
        event.Skip();
        return;
    }

    wxLongLong currentTime = wxGetLocalTimeMillis();
    if (currentTime - m_lastMotionTime >= MOTION_INTERVAL) {
        m_mouseHandler->handleMouseMotion(event);
        m_lastMotionTime = currentTime;
    }
    event.Skip();
}

void InputManager::onMouseWheel(wxMouseEvent& event) {
    if (!m_navigationController) {
        LOG_WAR("Mouse wheel event skipped: Invalid controller");
        event.Skip();
        return;
    }

    static float accumulatedDelta = 0.0f;
    accumulatedDelta += event.GetWheelRotation();

    if (std::abs(accumulatedDelta) >= event.GetWheelDelta()) {
        m_navigationController->handleMouseWheel(event);
        accumulatedDelta -= std::copysign(event.GetWheelDelta(), accumulatedDelta);
    }
    event.Skip();
}