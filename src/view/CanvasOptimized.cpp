#include "view/CanvasOptimized.h"
#include "RenderingEngine.h"
#include "logger/Logger.h"
#include <wx/dcclient.h>

wxBEGIN_EVENT_TABLE(CanvasOptimized, Canvas)
    EVT_PAINT(CanvasOptimized::onPaint)
    EVT_SIZE(CanvasOptimized::onSize)
    EVT_TIMER(wxID_ANY, CanvasOptimized::onHighQualityTimer)
wxEND_EVENT_TABLE()

CanvasOptimized::CanvasOptimized(wxWindow* parent, wxWindowID id, 
                                 const wxPoint& pos, const wxSize& size,
                                 long style, const wxString& name)
    : Canvas(parent, id, pos, size, style, name)
    , m_lastResizeTime(std::chrono::steady_clock::now())
{
    // Initialize high quality render timer
    m_highQualityTimer = new wxTimer(this);
    
    // Store original antialiasing setting
    if (m_renderingEngine) {
        m_highQualityAntiAliasing = m_renderingEngine->getAntiAliasing();
    }
}

CanvasOptimized::~CanvasOptimized() {
    if (m_highQualityTimer) {
        if (m_highQualityTimer->IsRunning()) {
            m_highQualityTimer->Stop();
        }
        delete m_highQualityTimer;
    }
}

void CanvasOptimized::onPaint(wxPaintEvent& event) {
    if (!m_progressiveRenderingEnabled) {
        // Use standard painting if progressive rendering is disabled
        Canvas::onPaint(event);
        return;
    }
    
    // Progressive rendering logic
    if (m_isResizing) {
        renderLowQuality();
    } else {
        renderHighQuality();
    }
}

void CanvasOptimized::onSize(wxSizeEvent& event) {
    // Mark as resizing
    m_isResizing = true;
    m_needsHighQualityRender = true;
    m_lastResizeTime = std::chrono::steady_clock::now();
    
    // Cancel any pending high quality render
    if (m_highQualityTimer->IsRunning()) {
        m_highQualityTimer->Stop();
    }
    
    // Schedule high quality render after resize stabilizes
    scheduleHighQualityRender();
    
    // Call base class handler
    Canvas::onSize(event);
}

void CanvasOptimized::renderLowQuality() {
    wxPaintDC dc(this);
    
    if (!m_renderingEngine || !m_sceneManager) {
        return;
    }
    
    // Temporarily disable antialiasing for fast rendering
    int originalAA = m_renderingEngine->getAntiAliasing();
    m_renderingEngine->setAntiAliasing(m_lowQualityAntiAliasing);
    
    // Disable expensive features during resize
    bool originalShadows = m_renderingEngine->getShadowsEnabled();
    bool originalReflections = m_renderingEngine->getReflectionsEnabled();
    
    m_renderingEngine->setShadowsEnabled(false);
    m_renderingEngine->setReflectionsEnabled(false);
    
    // Render with reduced quality
    auto start = std::chrono::high_resolution_clock::now();
    
    m_renderingEngine->beginFrame();
    m_sceneManager->render();
    m_renderingEngine->endFrame();
    SwapBuffers();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Log performance
    LOG_DBG_S("Low quality render: " + std::to_string(duration) + "ms");
    
    // Restore settings
    m_renderingEngine->setAntiAliasing(originalAA);
    m_renderingEngine->setShadowsEnabled(originalShadows);
    m_renderingEngine->setReflectionsEnabled(originalReflections);
}

void CanvasOptimized::renderHighQuality() {
    wxPaintDC dc(this);
    
    if (!m_renderingEngine || !m_sceneManager) {
        return;
    }
    
    // Ensure high quality settings
    m_renderingEngine->setAntiAliasing(m_highQualityAntiAliasing);
    
    // Standard high quality render
    auto start = std::chrono::high_resolution_clock::now();
    
    m_renderingEngine->beginFrame();
    m_sceneManager->render();
    m_renderingEngine->endFrame();
    SwapBuffers();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Log performance
    LOG_DBG_S("High quality render: " + std::to_string(duration) + "ms");
    
    // Clear flags
    m_isResizing = false;
    m_needsHighQualityRender = false;
}

void CanvasOptimized::scheduleHighQualityRender() {
    // Schedule high quality render 100ms after last resize
    m_highQualityTimer->Start(100, wxTIMER_ONE_SHOT);
}

void CanvasOptimized::onHighQualityTimer(wxTimerEvent& event) {
    // Check if enough time has passed since last resize
    auto now = std::chrono::steady_clock::now();
    auto timeSinceResize = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastResizeTime).count();
    
    if (timeSinceResize >= 100) {
        // Resize has stabilized, render in high quality
        m_isResizing = false;
        
        if (m_needsHighQualityRender) {
            Refresh(false);
        }
    } else {
        // Still resizing, reschedule
        scheduleHighQualityRender();
    }
}