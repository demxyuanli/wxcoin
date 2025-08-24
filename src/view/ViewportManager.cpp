#include "ViewportManager.h"
#include "RenderingEngine.h"
#include "NavigationCubeManager.h"
#include "DPIManager.h"
#include "logger/Logger.h"
#include <cmath>

ViewportManager::ViewportManager(wxGLCanvas* canvas)
    : m_canvas(canvas)
    , m_renderingEngine(nullptr)
    , m_navigationCubeManager(nullptr)
    , m_dpiScale(1.0f)
    , m_lastSize(-1, -1)
    , m_lastEventTime(0)
{
    LOG_INF_S("ViewportManager::ViewportManager: Initializing");
    
    if (m_canvas) {
        m_dpiScale = m_canvas->GetContentScaleFactor();
        LOG_INF_S("ViewportManager::ViewportManager: Initial DPI scale factor: " + std::to_string(m_dpiScale));
        
        // Initialize DPI manager with current scale
        DPIManager::getInstance().updateDPIScale(m_dpiScale);
    }
}

ViewportManager::~ViewportManager() {
    LOG_INF_S("ViewportManager::~ViewportManager: Destroying");
}

void ViewportManager::handleSizeChange(const wxSize& size) {
    if (!shouldProcessSizeEvent(size)) {
        return;
    }

    if (size.x > 0 && size.y > 0) {
        updateDPISettings();
        
        if (m_renderingEngine) {
            m_renderingEngine->handleResize(size);
        }
    } 
}

bool ViewportManager::shouldProcessSizeEvent(const wxSize& size) {
    wxLongLong currentTime = wxGetLocalTimeMillis();
    
    if (size == m_lastSize && (currentTime - m_lastEventTime) < 100) {
        LOG_DBG_S("ViewportManager::shouldProcessSizeEvent: Redundant size event ignored: " + 
               std::to_string(size.x) + "x" + std::to_string(size.y));
        return false;
    }

    m_lastSize = size;
    m_lastEventTime = currentTime;
    return true;
}

void ViewportManager::updateDPISettings() {
    if (!m_canvas) {
        return;
    }

    float newDpiScale = m_canvas->GetContentScaleFactor();
    if (std::abs(m_dpiScale - newDpiScale) > 0.01f) {
        LOG_INF_S("ViewportManager::updateDPISettings: DPI scale changed from " +
               std::to_string(m_dpiScale) + " to " + std::to_string(newDpiScale));

        m_dpiScale = newDpiScale;
        DPIManager::getInstance().updateDPIScale(m_dpiScale);

        // Apply DPI scaling to UI elements
        applyDPIScalingToUI();
    }
}

void ViewportManager::applyDPIScalingToUI() {
    if (!m_canvas) {
        return;
    }

    auto& dpiManager = DPIManager::getInstance();

    // Update canvas font
    wxFont currentFont = m_canvas->GetFont();
    if (currentFont.IsOk()) {
        wxFont scaledFont = dpiManager.getScaledFont(currentFont);
        m_canvas->SetFont(scaledFont);
        LOG_DBG_S("ViewportManager::applyDPIScalingToUI: Updated canvas font size to " +
               std::to_string(scaledFont.GetPointSize()) + " points");
    }

    // Notify navigation cube manager of DPI change
    if (m_navigationCubeManager) {
        m_navigationCubeManager->handleDPIChange();
    }

    LOG_INF_S("ViewportManager::applyDPIScalingToUI: Applied DPI scaling with factor " +
           std::to_string(m_dpiScale));
} 
