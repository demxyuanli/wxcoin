#pragma once

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include <memory>
#include "viewer/ImageOutlinePass.h"

class SceneManager;
class SoSeparator;
class SoCamera;
class SoPerspectiveCamera;

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

private:
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseEvent(wxMouseEvent& event);
    void onIdle(wxIdleEvent& event);
    
    void createBasicModels();
    void render();
    
    // FBO and shader management
    void initializeFBO(int width, int height);
    void cleanupFBO();
    void initializeShaders();
    void cleanupShaders();
    unsigned int compileShader(const char* source, unsigned int type);
    unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
    void createQuadVAO();
    
    wxGLContext* m_glContext{ nullptr };
    std::unique_ptr<SceneManager> m_sceneManager;
    SoSeparator* m_sceneRoot{ nullptr };
    SoSeparator* m_modelRoot{ nullptr };
    SoCamera* m_camera{ nullptr };
    
    ImageOutlineParams m_outlineParams;
    bool m_outlineEnabled{ true };
    
    // FBO and texture resources
    unsigned int m_fbo{ 0 };
    unsigned int m_colorTexture{ 0 };
    unsigned int m_depthTexture{ 0 };
    unsigned int m_normalTexture{ 0 };
    
    // Shader programs
    unsigned int m_normalShader{ 0 };
    unsigned int m_outlineShader{ 0 };
    unsigned int m_quadVAO{ 0 };
    unsigned int m_quadVBO{ 0 };
    
    // FBO size
    int m_fboWidth{ 0 };
    int m_fboHeight{ 0 };
    
    bool m_initialized{ false };
    bool m_needsRedraw{ true };
    
    // Mouse interaction
    wxPoint m_lastMousePos;
    bool m_mouseDown{ false };
    
    DECLARE_EVENT_TABLE()
};