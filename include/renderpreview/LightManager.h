#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/sensors/SoTimerSensor.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <wx/colour.h>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <queue>

#include "renderpreview/RenderLightSettings.h"

// Forward declarations
class SoMaterial;
class SoDirectionalLight;
class SoPointLight;
class SoSpotLight;
class SoTransform;
class SoSphere;
class SoCone;
class SoCoordinate3;
class SoIndexedFaceSet;
class SoSearchAction;
class SoFullPath;

// Light animation callback function type
using LightAnimationCallback = std::function<void(int lightId, const SbVec3f& newPosition, const SbVec3f& newDirection)>;

class LightManager
{
public:
    // Constants
    static const int MAX_LIGHTS = 8;  // OpenGL limit
    static const int DEFAULT_UPDATE_RATE = 60;  // Hz
    
    LightManager(SoSeparator* sceneRoot, SoSeparator* objectRoot);
    ~LightManager();

    // Light management
    int addLight(const RenderLightSettings& settings);
    bool removeLight(int lightId);
    bool updateLight(int lightId, const RenderLightSettings& settings);
    void clearAllLights();
    void updateMultipleLights(const std::vector<RenderLightSettings>& lights);

    // Light queries
    std::vector<RenderLightSettings> getAllLightSettings() const;
    std::vector<int> getAllLightIds() const;
    RenderLightSettings getLightSettings(int lightId) const;
    bool hasLight(int lightId) const;
    int getLightCount() const;

    // Individual light property setters
    void setLightEnabled(int lightId, bool enabled);
    void setLightIntensity(int lightId, float intensity);
    void setLightColor(int lightId, const wxColour& color);
    void setLightPosition(int lightId, float x, float y, float z);
    void setLightDirection(int lightId, float x, float y, float z);
    void setLightAnimation(int lightId, bool animated, double speed = 1.0, double radius = 5.0);

    // Animation control
    void startAnimation();
    void stopAnimation();
    void setAnimationRate(int fps);
    bool isAnimationRunning() const;
    
    // Event handling
    void setupEventCallbacks(SoSeparator* eventRoot);
    void removeEventCallbacks();
    
    // Performance management
    void setMaxLights(int maxLights);
    int getMaxLights() const;
    void optimizeLightOrder();  // Reorder lights by priority and distance
    
    // Presets and utilities
    void applyLightPreset(const std::string& presetName);
    void createThreePointLighting();
    void createStudioLighting();
    void createOutdoorLighting();

    // Material update based on lighting
    void updateMaterialsForLighting();
    
    // Callback registration
    void setAnimationCallback(LightAnimationCallback callback);

private:
    // Helper method for updating scene materials
    void updateSceneMaterials(const SbColor& lightColor, float totalIntensity, int lightCount);

    // Light creation and update helpers
    SoLight* createLightNode(const RenderLightSettings& settings);
    void updateLightNode(SoLight* lightNode, const RenderLightSettings& settings);

    // Indicator creation and update helpers
    SoSeparator* createLightIndicator(const RenderLightSettings& settings, SoLight* lightNode);
    void updateLightIndicator(SoSeparator* indicator, const RenderLightSettings& settings);
    void createDirectionalLightIndicator(SoSeparator* indicator, const SbVec3f& lightDirection, float lightIntensity);
    void createPointLightIndicator(SoSeparator* indicator, float lightIntensity);
    void createSpotLightIndicator(SoSeparator* indicator, const SbVec3f& lightDirection, float lightIntensity);

    // Animation helpers
    void updateLightAnimation(int lightId, double time);
    void updateAllAnimations();
    static void animationTimerCallback(void* data, SoSensor* sensor);
    
    // Event callback helpers
    static void keyEventCallback(void* data, SoEventCallback* eventCB);
    static void mouseEventCallback(void* data, SoEventCallback* eventCB);
    
    // Performance helpers
    void enforceLightLimit();
    double calculateLightDistance(int lightId, const SbVec3f& cameraPosition);
    
    // Internal structure for managing lights
    struct ManagedLight
    {
        int lightId;
        RenderLightSettings settings;
        SoNode* lightNode;          // Can be SoLight* or SoSeparator* for animated lights
        SoSeparator* indicatorNode;
        SoTransform* transformNode;  // For animated lights
        double animationTime;        // Current animation time
        bool needsUpdate;            // Flag for optimization
    };

    // Member variables
    SoSeparator* m_sceneRoot;
    SoSeparator* m_objectRoot;
    SoSeparator* m_indicatorContainer;
    std::map<int, std::unique_ptr<ManagedLight>> m_lights;
    int m_nextLightId;
    
    // Animation system
    std::unique_ptr<SoTimerSensor> m_animationTimer;
    bool m_animationRunning;
    int m_animationRate;
    
    // Event system
    SoEventCallback* m_keyEventCallback;
    SoEventCallback* m_mouseEventCallback;
    SoSeparator* m_eventRoot;
    
    // Performance management
    int m_maxLights;
    std::priority_queue<std::pair<int, int>> m_lightPriorityQueue;  // priority, lightId
    
    // Callbacks
    LightAnimationCallback m_animationCallback;
    
    // Camera position for distance calculations
    SbVec3f m_cameraPosition;
}; 