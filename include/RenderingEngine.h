#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <wx/longlong.h>
#include <OpenCASCADE/Quantity_Color.hxx>

// Forward declarations
class SceneManager;
class NavigationCubeManager;

class RenderingEngine {
public:
    static const int RENDER_INTERVAL;
    
    explicit RenderingEngine(wxGLCanvas* canvas);
    ~RenderingEngine();

    bool initialize();
    void render(bool fastMode = false);
    void renderWithoutSwap(bool fastMode = false);
    void swapBuffers();
    void handleResize(const wxSize& size);
    void updateViewport(const wxSize& size, float dpiScale);
    
    bool isInitialized() const { return m_isInitialized; }
    bool isRendering() const { return m_isRendering; }
    
    void setSceneManager(SceneManager* sceneManager) { m_sceneManager = sceneManager; }
    void setNavigationCubeManager(NavigationCubeManager* navCubeManager) { m_navigationCubeManager = navCubeManager; }
    
    // Lighting settings
    void setLightAmbientColor(const Quantity_Color& color);
    void setLightDiffuseColor(const Quantity_Color& color);
    void setLightSpecularColor(const Quantity_Color& color);
    void setLightIntensity(double intensity);
    void setLightAmbientIntensity(double intensity);

private:
    void setupGLContext();
    void clearBuffers();
    void presentFrame();
    
    wxGLCanvas* m_canvas;
    std::unique_ptr<wxGLContext> m_glContext;
    SceneManager* m_sceneManager;
    NavigationCubeManager* m_navigationCubeManager;
    
    bool m_isInitialized;
    bool m_isRendering;
    wxLongLong m_lastRenderTime;
    
    // Lighting settings
    Quantity_Color m_lightAmbientColor;
    Quantity_Color m_lightDiffuseColor;
    Quantity_Color m_lightSpecularColor;
    double m_lightIntensity;
    double m_lightAmbientIntensity;
}; 