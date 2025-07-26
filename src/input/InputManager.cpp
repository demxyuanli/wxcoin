#include "InputManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include "logger/Logger.h"
#include "PositionBasicDialog.h"
#include "PickingAidManager.h"
#include "DefaultInputState.h"
#include "PickingInputState.h"

const int InputManager::MOTION_INTERVAL = 10; // Mouse motion throttling (milliseconds)

InputManager::InputManager(Canvas* canvas)
    : m_canvas(canvas)
    , m_mouseHandler(nullptr)
    , m_navigationController(nullptr)
    , m_currentState(nullptr)
    , m_lastMotionTime(0)
{
    LOG_INF_S("InputManager initializing");
}

InputManager::~InputManager() {
    LOG_INF_S("InputManager destroying");
}

void InputManager::setMouseHandler(MouseHandler* handler) {
    m_mouseHandler = handler;
    LOG_INF_S("MouseHandler set for InputManager");
}

void InputManager::setNavigationController(NavigationController* controller) {
    m_navigationController = controller;
    LOG_INF_S("NavigationController set for InputManager");
}

void InputManager::initializeStates() {
    m_defaultState = std::make_unique<DefaultInputState>(m_mouseHandler, m_navigationController);
    m_pickingState = std::make_unique<PickingInputState>(m_canvas);
    m_currentState = m_defaultState.get();
    LOG_INF_S("InputManager states initialized");
}

void InputManager::enterDefaultState() {
    if (m_currentState != m_defaultState.get()) {
        m_currentState = m_defaultState.get();
        LOG_INF_S("InputManager entered DefaultState");
    }
}

void InputManager::enterPickingState() {
    if (m_currentState != m_pickingState.get()) {
        m_currentState = m_pickingState.get();
        LOG_INF_S("InputManager entered PickingState");
    }
}

void InputManager::onMouseButton(wxMouseEvent& event) {
    if (m_currentState) {
        m_currentState->onMouseButton(event);
    } else {
        LOG_WRN_S("InputManager: No active state to handle mouse button event");
        event.Skip();
    }
}

void InputManager::onMouseMotion(wxMouseEvent& event) {
    if (!m_currentState) {
        LOG_WRN_S("Mouse motion event skipped: No active state");
        event.Skip();
        return;
    }

    wxLongLong currentTime = wxGetLocalTimeMillis();
    if (currentTime - m_lastMotionTime >= MOTION_INTERVAL) {
        m_currentState->onMouseMotion(event);
        m_lastMotionTime = currentTime;
    } else {
        event.Skip();
    }
}

void InputManager::onMouseWheel(wxMouseEvent& event) {
    if (m_currentState) {
        m_currentState->onMouseWheel(event);
    } else {
        LOG_WRN_S("Mouse wheel event skipped: No active state");
        event.Skip();
    }
}

MouseHandler* InputManager::getMouseHandler() const {
    return m_mouseHandler;
}

NavigationController* InputManager::getNavigationController() const {
    return m_navigationController;
}
