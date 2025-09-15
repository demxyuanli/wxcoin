#pragma once

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <memory>
#include <vector>

namespace ads {

/**
 * GPU-accelerated layout system using OpenGL
 * 
 * Benefits:
 * 1. Offload layout calculations to GPU
 * 2. Hardware-accelerated transformations
 * 3. Smooth animations during resize
 */
class GPUAcceleratedLayout : public wxGLCanvas {
public:
    GPUAcceleratedLayout(wxWindow* parent);
    virtual ~GPUAcceleratedLayout();
    
    // Add dock area to GPU layout
    void addDockArea(wxWindow* area, const wxRect& bounds);
    void removeDockArea(wxWindow* area);
    
    // Animate resize
    void animateResize(const wxSize& newSize, int durationMs = 150);
    
protected:
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    
private:
    struct AreaTexture {
        wxWindow* window;
        unsigned int textureId;
        wxRect currentBounds;
        wxRect targetBounds;
        float animationProgress;
    };
    
    std::unique_ptr<wxGLContext> m_glContext;
    std::vector<AreaTexture> m_areas;
    
    wxTimer* m_animationTimer;
    bool m_isAnimating{false};
    
    // OpenGL methods
    void initializeGL();
    void renderFrame();
    void updateTextures();
    void createTextureFromWindow(AreaTexture& area);
    
    // Animation
    void updateAnimation();
    wxRect interpolateRect(const wxRect& from, const wxRect& to, float t);
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads