#pragma once

#include <wx/glcanvas.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/colour.h>
#include <memory>
#include "viewer/ImageOutlinePass.h" // For ImageOutlineParams

class SoSeparator;
class SoCamera;

class OutlinePreviewCanvas : public wxGLCanvas {
public:
    OutlinePreviewCanvas(wxWindow* parent, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize);
    ~OutlinePreviewCanvas();
    
    void updateOutlineParams(const ImageOutlineParams& params);
    ImageOutlineParams getOutlineParams() const;
    void setOutlineEnabled(bool enabled);
    
    // Color setters
    void setBackgroundColor(const wxColour& color) { m_bgColor = color; Refresh(); }
    void setOutlineColor(const wxColour& color) { m_outlineColor = color; Refresh(); }
    void setHoverColor(const wxColour& color) { m_hoverColor = color; Refresh(); }
    void setGeometryColor(const wxColour& color);
    
private:
    void initializeScene();
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& event);
    void onIdle(wxIdleEvent& event);
    
    void createBasicModels();
    void render();
    int getObjectAtPosition(const wxPoint& pos);
    
    wxGLContext* m_glContext{ nullptr };
    SoSeparator* m_sceneRoot{ nullptr };
    SoSeparator* m_modelRoot{ nullptr };
    SoCamera* m_camera{ nullptr };
    
    ImageOutlineParams m_outlineParams;
    bool m_outlineEnabled{ true };
    bool m_initialized{ false };
    bool m_needsRedraw{ true };
    
    // Mouse interaction
    bool m_mouseDown{ false };
    wxPoint m_lastMousePos;
    int m_hoveredObjectIndex{ -1 };
    
    // Colors
    wxColour m_bgColor{ 128, 128, 128 };
    wxColour m_outlineColor{ 0, 0, 0 };
    wxColour m_hoverColor{ 255, 128, 0 };
    wxColour m_geomColor{ 200, 200, 200 };
    
    DECLARE_EVENT_TABLE()
};