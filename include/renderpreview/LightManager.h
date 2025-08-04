#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <wx/colour.h>
#include <memory>
#include <map>
#include <vector>
#include <string>

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

class LightManager
{
public:
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

    // Presets and utilities
    void applyLightPreset(const std::string& presetName);

    // Material update based on lighting
    void updateMaterialsForLighting();

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

    // Internal structure for managing lights
    struct ManagedLight
    {
        int lightId;
        RenderLightSettings settings;
        SoLight* lightNode;
        SoSeparator* lightGroup;
        SoSeparator* indicatorNode;
    };

    // Member variables
    SoSeparator* m_sceneRoot;
    SoSeparator* m_objectRoot;
    SoSeparator* m_lightContainer;
    SoSeparator* m_indicatorContainer;
    std::map<int, std::unique_ptr<ManagedLight>> m_lights;
    int m_nextLightId;
}; 