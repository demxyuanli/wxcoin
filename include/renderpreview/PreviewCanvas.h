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
#include "SoFCBackgroundGradient.h"
#include "SoFCBackgroundImage.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include "renderpreview/RenderLightSettings.h"
#include "renderpreview/LightManager.h"
#include "renderpreview/AntiAliasingManager.h"
#include "renderpreview/RenderingManager.h"
#include "renderpreview/AntiAliasingSettings.h"
#include "renderpreview/RenderingSettings.h"
#include "renderpreview/ObjectManager.h"
#include "renderpreview/ObjectSettings.h"
#include "renderpreview/BackgroundManager.h"

// Forward declarations
class OCCBox;
class OCCSphere;
class OCCCone;
class BackgroundConfigListener;
class RenderingEngine;



class PreviewCanvas : public wxGLCanvas
{
public:
    PreviewCanvas(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    virtual ~PreviewCanvas();

    void render(bool fastMode = false);
    void resetView();
    
    // Unified light management interface
    int addLight(const RenderLightSettings& settings);
    bool removeLight(int lightId);
    bool updateLight(int lightId, const RenderLightSettings& settings);
    void updateMultipleLights(const std::vector<RenderLightSettings>& lights);
    void clearAllLights();
    void resetToDefaultLighting();
    std::vector<RenderLightSettings> getAllLights() const;
    bool hasLights() const;
    
    // Unified anti-aliasing management interface
    int addAntiAliasingConfig(const AntiAliasingSettings& settings);
    bool removeAntiAliasingConfig(int configId);
    bool updateAntiAliasingConfig(int configId, const AntiAliasingSettings& settings);
    bool setActiveAntiAliasingConfig(int configId);
    std::vector<AntiAliasingSettings> getAllAntiAliasingConfigs() const;
    
    // Unified rendering management interface
    int addRenderingConfig(const RenderingSettings& settings);
    bool removeRenderingConfig(int configId);
    bool updateRenderingConfig(int configId, const RenderingSettings& settings);
    bool setActiveRenderingConfig(int configId);
    std::vector<RenderingSettings> getAllRenderingConfigs() const;
    
    // Manager access methods
    AntiAliasingManager* getAntiAliasingManager() const { return m_antiAliasingManager.get(); }
    RenderingManager* getRenderingManager() const { return m_renderingManager.get(); }
    ObjectManager* getObjectManager() const { return m_objectManager.get(); }
    LightManager* getLightManager() const { return m_lightManager.get(); }
    BackgroundManager* getBackgroundManager() const { return m_backgroundManager.get(); }
    
    // Legacy methods (for backward compatibility)
    void updateLighting(float ambient, float diffuse, float specular, const wxColour& color, float intensity);
    void updateMultiLighting(const std::vector<RenderLightSettings>& lights);
    
    // Material and rendering methods
    void updateMaterial(float ambient, float diffuse, float specular, float shininess, float transparency);
    void updateTexture(bool enabled, int mode, float scale);
    void updateAntiAliasing(int method, int msaaSamples, bool fxaaEnabled);
    void updateRenderingMode(int mode);
    
    // Object management interface
    int addObject(const ObjectSettings& settings);
    bool removeObject(int objectId);
    bool updateObject(int objectId, const ObjectSettings& settings);
    void updateMultipleObjects(const std::vector<ObjectSettings>& objects);
    void clearAllObjects();
    std::vector<ObjectSettings> getAllObjects() const;

private:
    void initializeScene();
    void createDefaultScene();
    void createCheckerboardPlane();
    void createBasicGeometryObjects();
    void createCoordinateSystem();
    void setupDefaultCamera();
    void setupDefaultLighting();
    void renderGradientBackground(const wxColour& topColor, const wxColour& bottomColor);
    void renderGradientBackgroundFromConfig();
    void renderBackgroundDirectly(const wxSize& size);

public:
    // Configuration update methods
    void updateBackgroundFromConfig();
    void renderImageBackground(const std::string& imagePath, float opacity, int fit, bool maintainAspect);
    bool loadTexture(const std::string& imagePath, unsigned int& textureId);
    void drawFullscreenQuad();
    void applyRenderingModeSettings(const RenderingSettings& settings);
    
    // Apply main view background settings
    void applyMainView(RenderingEngine* mainViewEngine);
    
    // Apply preview canvas background settings to main view
    void applyToMainView(RenderingEngine* mainViewEngine);

    
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
    SoSeparator* m_objectRoot;
    SoSeparator* m_backgroundRoot;  // Background scene graph root
    SoMaterial* m_lightMaterial;
    SoFCBackgroundGradient* m_backgroundGradient;  // FreeCAD-style background gradient node
    SoFCBackgroundImage* m_backgroundImage;  // FreeCAD-style background image node
    
    // OCCGeometry objects for basic shapes
    std::unique_ptr<OCCBox> m_occBox;
    std::unique_ptr<OCCSphere> m_occSphere;
    std::unique_ptr<OCCCone> m_occCone;
    
    // Unified parameter management
    std::unique_ptr<LightManager> m_lightManager;
    std::unique_ptr<AntiAliasingManager> m_antiAliasingManager;
    std::unique_ptr<RenderingManager> m_renderingManager;
    std::unique_ptr<ObjectManager> m_objectManager;  // New object manager
    std::unique_ptr<BackgroundManager> m_backgroundManager; // New background manager

    // Configuration listener for dynamic background updates
    BackgroundConfigListener* m_backgroundConfigListener;

    // OpenGL context
    wxGLContext* m_glContext;
    bool m_initialized;

    // Mouse interaction state
    bool m_mouseDown;
    wxPoint m_lastMousePos;
    float m_cameraDistance;
    SbVec3f m_cameraCenter;

    DECLARE_EVENT_TABLE()
    
    // Runtime configuration management
    int m_runtimeConfigId = -1;  // ID for runtime rendering configuration

    // Configuration-based background settings
    int m_configBackgroundMode = 0;
    double m_configBackgroundColorR = 0.6;
    double m_configBackgroundColorG = 0.8;
    double m_configBackgroundColorB = 1.0;
    double m_configGradientTopR = 0.7;
    double m_configGradientTopG = 0.7;
    double m_configGradientTopB = 0.9;
    double m_configGradientBottomR = 0.5;
    double m_configGradientBottomG = 0.5;
    double m_configGradientBottomB = 0.8;
    std::string m_configBackgroundTexturePath = "";
    
    // Texture cache for background images
    std::unordered_map<std::string, unsigned int> m_textureCache;
    std::string m_currentBackgroundImagePath;
    unsigned int m_currentBackgroundTextureId = 0;
    
    // Helper methods for unified coordination
    RenderingSettings createRenderingSettingsForMode(int mode);
    int getRuntimeConfigurationId() const;
    void setRuntimeConfigurationId(int configId);
};