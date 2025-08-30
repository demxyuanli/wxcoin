#pragma once

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <memory>
#include "viewer/ImageOutlinePass.h"

class SceneManager;
class SoSeparator;
class SoCamera;
class SoPerspectiveCamera;
class ImageOutlinePass;

class OutlinePreviewCanvas : public wxGLCanvas {
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
    
    // Color configuration
    void setBackgroundColor(const wxColour& color) { m_bgColor = color; m_needsRedraw = true; }
    void setOutlineColor(const wxColour& color) { m_outlineColor = color; m_needsRedraw = true; }
    void setHoverColor(const wxColour& color) { m_hoverColor = color; m_needsRedraw = true; }
    void setGeometryColor(const wxColour& color);

private:
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
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<ImageOutlinePass> m_outlinePass;
    SoSeparator* m_sceneRoot{ nullptr };
    SoSeparator* m_modelRoot{ nullptr };
    SoCamera* m_camera{ nullptr };
    
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