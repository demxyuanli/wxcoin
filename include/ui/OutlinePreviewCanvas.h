#pragma once

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <memory>
#include "viewer/ImageOutlinePass.h"
#include "viewer/IOutlineRenderer.h"

class SoSeparator;
class SoCamera;
class SoPerspectiveCamera;
class ImageOutlinePass2;

class OutlinePreviewCanvas : public wxGLCanvas, public IOutlineRenderer {
public:
    OutlinePreviewCanvas(wxWindow* parent, 
                        wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize);
    ~OutlinePreviewCanvas();

    // Initialize the preview scene
    void initializeScene();
    
    // Update outline parameters
    void updateOutlineParams(const ImageOutlineParams& params);
    
    // Get current parameters
    ImageOutlineParams getOutlineParams() const;
    
    // Enable/disable outline
    void setOutlineEnabled(bool enabled);
    
    // Color configuration
    void setBackgroundColor(const wxColour& color) { m_bgColor = color; m_needsRedraw = true; }
    void setOutlineColor(const wxColour& color) { m_outlineColor = color; m_needsRedraw = true; }
    void setHoverColor(const wxColour& color) { m_hoverColor = color; m_needsRedraw = true; }
    void setGeometryColor(const wxColour& color);
    
    // IOutlineRenderer implementation
    wxGLCanvas* getGLCanvas() const override { return const_cast<OutlinePreviewCanvas*>(this); }
    SoCamera* getCamera() const override { return m_camera; }
    SoSeparator* getSceneRoot() const override { return m_sceneRoot; }
    void requestRedraw() override { m_needsRedraw = true; Refresh(false); }

private:
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void onIdle(wxIdleEvent& event);
    
    void createBasicModels();
    void render();
    void renderSimpleOutline();
    int getObjectAtPosition(const wxPoint& pos);
    
    wxGLContext* m_glContext{ nullptr };
    SoSeparator* m_sceneRoot{ nullptr };
    SoSeparator* m_modelRoot{ nullptr };
    SoCamera* m_camera{ nullptr };
    std::unique_ptr<ImageOutlinePass2> m_outlinePass;
    
    ImageOutlineParams m_outlineParams;
    bool m_outlineEnabled{ true };
    
    bool m_initialized{ false };
    bool m_needsRedraw{ true };
    
    // Mouse interaction
    wxPoint m_lastMousePos;
    bool m_mouseDown{ false };
    int m_hoveredObjectIndex{ -1 };  // Index of hovered object
    
    // Color settings
    wxColour m_bgColor{ 51, 51, 51 };      // Dark gray background
    wxColour m_outlineColor{ 0, 0, 0 };    // Black outline
    wxColour m_hoverColor{ 255, 128, 0 };  // Orange hover
    wxColour m_geomColor{ 200, 200, 200 }; // Light gray geometry
    
    DECLARE_EVENT_TABLE()
};