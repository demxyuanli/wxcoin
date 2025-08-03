#pragma once

#include <wx/glcanvas.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/actions/SoSearchAction.h>
#include <memory>
#include <vector>
#include "renderpreview/RenderLightSettings.h"

// Forward declarations
class OCCBox;
class OCCSphere;
class OCCCone;

class PreviewCanvas : public wxGLCanvas
{
public:
    PreviewCanvas(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    virtual ~PreviewCanvas();

    void render(bool fastMode = false);
    void resetView();
    void updateLighting(float ambient, float diffuse, float specular, const wxColour& color, float intensity);
    void updateMultiLighting(const std::vector<RenderLightSettings>& lights);
    SoLight* createLightByType(const RenderLightSettings& lightSettings);
    void createLightIndicator(SoLight* light, int lightIndex, const std::string& lightName, SoSeparator* container, const SbVec3f& lightPosition);
    void updateMaterial(float ambient, float diffuse, float specular, float shininess, float transparency);
    void updateObjectMaterial(SoNode* node, float ambient, float diffuse, float specular, float shininess, float transparency);
    void updateGeometryMaterialsForLighting(float lightR, float lightG, float lightB, float totalIntensity);
    void updateObjectMaterialForLighting(SoNode* node, const SbColor& baseColor, float lightR, float lightG, float lightB, float totalIntensity);
    void updateTexture(bool enabled, int mode, float scale);
    void updateAntiAliasing(int method, int msaaSamples, bool fxaaEnabled);
    void updateRenderingMode(int mode);

private:
    void initializeScene();
    void createDefaultScene();
    void createCheckerboardPlane();
    void createBasicGeometryObjects();
    void createLightIndicator();
    void updateLightIndicator(const wxColour& color, float intensity);
    void createCoordinateSystem();
    void setupDefaultCamera();
    void setupLighting();
    void clearAllLights();

    // Event handlers
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    void onMouseDown(wxMouseEvent& event);
    void onMouseUp(wxMouseEvent& event);
    void onMouseMove(wxMouseEvent& event);
    void onMouseWheel(wxMouseEvent& event);

    static const int s_canvasAttribs[];

    // Coin3D scene graph
    SoSeparator* m_sceneRoot;
    SoCamera* m_camera;
    SoDirectionalLight* m_light;
    SoSeparator* m_objectRoot;
    SoMaterial* m_lightMaterial;
    
    // OCCGeometry objects for basic shapes
    std::unique_ptr<OCCBox> m_occBox;
    std::unique_ptr<OCCSphere> m_occSphere;
    std::unique_ptr<OCCCone> m_occCone;
    
    // Light indicator visualization
    SoSeparator* m_lightIndicator;
    SoSeparator* m_lightIndicatorsContainer;

    // OpenGL context
    wxGLContext* m_glContext;
    bool m_initialized;

    // Mouse interaction state
    bool m_mouseDown;
    wxPoint m_lastMousePos;
    float m_cameraDistance;
    SbVec3f m_cameraCenter;

    DECLARE_EVENT_TABLE()
}; 