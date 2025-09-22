#include "NavigationModeManager.h"
#include "NavigationController.h"
#include "InventorNavigationController.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "logger/Logger.h"

NavigationModeManager::NavigationModeManager(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_sceneManager(sceneManager)
    , m_currentStyle(NavigationStyle::GESTURE)
{
    LOG_INF_S("NavigationModeManager initializing");
    initializeControllers();
}

NavigationModeManager::~NavigationModeManager() {
    LOG_INF_S("NavigationModeManager destroying");
}

void NavigationModeManager::initializeControllers() {
    m_gestureController = std::make_unique<NavigationController>(m_canvas, m_sceneManager);
    m_inventorController = std::make_unique<InventorNavigationController>(m_canvas, m_sceneManager);
    
    LOG_INF_S("Navigation controllers initialized");
}

void NavigationModeManager::setNavigationStyle(NavigationStyle style) {
    if (m_currentStyle != style) {
        m_currentStyle = style;
        LOG_INF_S("Navigation style changed to: " + std::to_string(static_cast<int>(style)));
    }
}

NavigationStyle NavigationModeManager::getNavigationStyle() const {
    return m_currentStyle;
}

void NavigationModeManager::handleMouseButton(wxMouseEvent& event) {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        m_gestureController->handleMouseButton(event);
        break;
    case NavigationStyle::INVENTOR:
        m_inventorController->handleMouseButton(event);
        break;
    }
}

void NavigationModeManager::handleMouseMotion(wxMouseEvent& event) {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        m_gestureController->handleMouseMotion(event);
        break;
    case NavigationStyle::INVENTOR:
        m_inventorController->handleMouseMotion(event);
        break;
    }
}

void NavigationModeManager::handleMouseWheel(wxMouseEvent& event) {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        m_gestureController->handleMouseWheel(event);
        break;
    case NavigationStyle::INVENTOR:
        m_inventorController->handleMouseWheel(event);
        break;
    }
}

void NavigationModeManager::viewAll() {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        m_gestureController->viewAll();
        break;
    case NavigationStyle::INVENTOR:
        m_inventorController->viewAll();
        break;
    }
}

void NavigationModeManager::viewTop() {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        m_gestureController->viewTop();
        break;
    case NavigationStyle::INVENTOR:
        m_inventorController->viewTop();
        break;
    }
}

void NavigationModeManager::viewFront() {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        m_gestureController->viewFront();
        break;
    case NavigationStyle::INVENTOR:
        m_inventorController->viewFront();
        break;
    }
}

void NavigationModeManager::viewRight() {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        m_gestureController->viewRight();
        break;
    case NavigationStyle::INVENTOR:
        m_inventorController->viewRight();
        break;
    }
}

void NavigationModeManager::viewIsometric() {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        m_gestureController->viewIsometric();
        break;
    case NavigationStyle::INVENTOR:
        m_inventorController->viewIsometric();
        break;
    }
}

void NavigationModeManager::setZoomSpeedFactor(float factor) {
    // Set zoom speed for all controllers
    m_gestureController->setZoomSpeedFactor(factor);
    m_inventorController->setZoomSpeedFactor(factor);
}

float NavigationModeManager::getZoomSpeedFactor() const {
    // Return zoom speed from current controller
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        return m_gestureController->getZoomSpeedFactor();
    case NavigationStyle::INVENTOR:
        return m_inventorController->getZoomSpeedFactor();
    }
    return 1.0f;
}

NavigationController* NavigationModeManager::getCurrentController() {
    switch (m_currentStyle) {
    case NavigationStyle::GESTURE:
        return m_gestureController.get();
    case NavigationStyle::INVENTOR:
        return nullptr; // Inventor controller is different type
    }
    return nullptr;
}

InventorNavigationController* NavigationModeManager::getInventorController() {
    return m_inventorController.get();
}
