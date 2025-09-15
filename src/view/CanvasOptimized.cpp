#include "view/CanvasOptimized.h"
#include "RenderingEngine.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include <wx/dcclient.h>

wxBEGIN_EVENT_TABLE(CanvasOptimized, Canvas)
    EVT_TIMER(wxID_ANY, CanvasOptimized::onHighQualityTimer)
wxEND_EVENT_TABLE()

CanvasOptimized::CanvasOptimized(wxWindow* parent, wxWindowID id, 
                                 const wxPoint& pos, const wxSize& size,
                                 long style, const wxString& name)
    : Canvas(parent, id, pos, size)  // Canvas only takes 5 parameters
    , m_lastResizeTime(std::chrono::steady_clock::now())
{
    // Initialize high quality render timer
    m_highQualityTimer = new wxTimer(this);
    
    // Bind our own event handlers with higher priority
    Bind(wxEVT_SIZE, &CanvasOptimized::onSize, this);
    Bind(wxEVT_PAINT, &CanvasOptimized::onPaint, this);
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
    if (!m_progressiveRenderingEnabled || !m_isResizing) {
        // Use normal rendering
        wxPaintDC dc(this);
        render(false);
        return;
    }
    
    // Use fast rendering during resize
    wxPaintDC dc(this);
    
    auto start = std::chrono::high_resolution_clock::now();
    render(true);  // Fast mode
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_DBG_S("Fast render during resize: " + std::to_string(duration) + "ms");
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
    
    // Update size
    RenderingEngine* engine = getRenderingEngine();
    if (engine) {
        engine->handleResize(event.GetSize());
    }
    
    // Trigger repaint
    Refresh(false);
}

void CanvasOptimized::renderLowQuality() {
    // Simply use fast mode rendering
    render(true);
}

void CanvasOptimized::renderHighQuality() {
    // Use normal quality rendering
    render(false);
    
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