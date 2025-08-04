#include "renderpreview/LightManager.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbRotation.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/SoFullPath.h>
#include <cmath>

LightManager::LightManager(SoSeparator* sceneRoot, SoSeparator* objectRoot)
    : m_sceneRoot(sceneRoot), m_objectRoot(objectRoot), m_nextLightId(1)
{
    // Create indicator container for light visualization
    m_indicatorContainer = new SoSeparator();
    m_indicatorContainer->ref();
    m_objectRoot->addChild(m_indicatorContainer);
    
    LOG_INF_S("LightManager: Initialized. Lights will be added directly to scene root");
}

LightManager::~LightManager()
{
    clearAllLights();
    
    if (m_indicatorContainer) {
        m_indicatorContainer->unref();
    }
}

int LightManager::addLight(const RenderLightSettings& settings)
{
    LOG_INF_S("LightManager::addLight: Adding light '" + settings.name + "' of type '" + settings.type + "'");
    
    auto managedLight = std::make_unique<ManagedLight>();
    managedLight->settings = settings;
    managedLight->lightId = m_nextLightId++;
    
    managedLight->lightNode = createLightNode(settings);
    if (!managedLight->lightNode) {
        LOG_ERR_S("LightManager::addLight: Failed to create light node");
        return -1;
    }
    
    // Ref the light node to ensure it's not deleted when removed from scene
    managedLight->lightNode->ref();
    
    // Add light directly to scene root (after camera but before object root)
    // Find the index of object root to insert before it
    int objectRootIndex = -1;
    for (int i = 0; i < m_sceneRoot->getNumChildren(); ++i) {
        if (m_sceneRoot->getChild(i) == m_objectRoot) {
            objectRootIndex = i;
            break;
        }
    }
    
    if (objectRootIndex >= 0) {
        m_sceneRoot->insertChild(managedLight->lightNode, objectRootIndex);
        LOG_INF_S("LightManager::addLight: Added light directly to scene root at index " + std::to_string(objectRootIndex));
    } else {
        // Fallback: add to end
        m_sceneRoot->addChild(managedLight->lightNode);
        LOG_INF_S("LightManager::addLight: Added light to end of scene root");
    }
    
    managedLight->indicatorNode = createLightIndicator(settings, managedLight->lightNode);
    if (managedLight->indicatorNode) {
        m_indicatorContainer->addChild(managedLight->indicatorNode);
    }
    
    int lightId = managedLight->lightId;
    m_lights[lightId] = std::move(managedLight);
    
    LOG_INF_S("LightManager::addLight: Successfully added light with ID " + std::to_string(lightId));
    return lightId;
}

bool LightManager::removeLight(int lightId)
{
    auto it = m_lights.find(lightId);
    if (it == m_lights.end()) {
        LOG_WRN_S("LightManager::removeLight: Light with ID " + std::to_string(lightId) + " not found");
        return false;
    }
    
    auto& managedLight = it->second;
    
    if (managedLight->lightNode) {
        int index = m_sceneRoot->findChild(managedLight->lightNode);
        if (index >= 0) {
            m_sceneRoot->removeChild(index);
        }
        managedLight->lightNode->unref();
    }
    
    if (managedLight->indicatorNode) {
        int index = m_indicatorContainer->findChild(managedLight->indicatorNode);
        if (index >= 0) {
            m_indicatorContainer->removeChild(index);
        }
        managedLight->indicatorNode->unref();
    }
    
    m_lights.erase(it);
    
    LOG_INF_S("LightManager::removeLight: Successfully removed light with ID " + std::to_string(lightId));
    return true;
}

bool LightManager::updateLight(int lightId, const RenderLightSettings& settings)
{
    auto it = m_lights.find(lightId);
    if (it == m_lights.end()) {
        LOG_WRN_S("LightManager::updateLight: Light with ID " + std::to_string(lightId) + " not found");
        return false;
    }
    
    auto& managedLight = it->second;
    
    updateLightNode(managedLight->lightNode, settings);
    
    if (managedLight->indicatorNode) {
        updateLightIndicator(managedLight->indicatorNode, settings);
    }
    
    managedLight->settings = settings;
    
    // Add material update
    updateMaterialsForLighting();
    
    LOG_INF_S("LightManager::updateLight: Successfully updated light with ID " + std::to_string(lightId));
    return true;
}

void LightManager::clearAllLights()
{
    LOG_INF_S("LightManager::clearAllLights: Clearing all managed lights");
    
    for (auto& pair : m_lights) {
        auto& managedLight = pair.second;
        
        if (managedLight->lightNode) {
            int index = m_sceneRoot->findChild(managedLight->lightNode);
            if (index >= 0) {
                m_sceneRoot->removeChild(index);
            }
            managedLight->lightNode->unref();
        }
        if (managedLight->indicatorNode) {
            managedLight->indicatorNode->unref();
        }
    }
    
    m_lights.clear();
    
    m_indicatorContainer->removeAllChildren();
    
    LOG_INF_S("LightManager::clearAllLights: All lights cleared");
}

void LightManager::updateMultipleLights(const std::vector<RenderLightSettings>& lights)
{
    LOG_INF_S("LightManager::updateMultipleLights: Updating " + std::to_string(lights.size()) + " lights");
    
    clearAllLights();
    
    for (const auto& lightSettings : lights) {
        if (lightSettings.enabled) {
            int lightId = addLight(lightSettings);
            LOG_INF_S("LightManager::updateMultipleLights: Added light '" + lightSettings.name + "' with ID " + std::to_string(lightId));
        } else {
            LOG_INF_S("LightManager::updateMultipleLights: Skipping disabled light '" + lightSettings.name + "'");
        }
    }
    
    // Add material update
    updateMaterialsForLighting();
    
    LOG_INF_S("LightManager::updateMultipleLights: Final light count: " + std::to_string(getLightCount()));
}

std::vector<RenderLightSettings> LightManager::getAllLightSettings() const
{
    std::vector<RenderLightSettings> settings;
    for (const auto& pair : m_lights) {
        settings.push_back(pair.second->settings);
    }
    return settings;
}

std::vector<int> LightManager::getAllLightIds() const
{
    std::vector<int> ids;
    for (const auto& pair : m_lights) {
        ids.push_back(pair.first);
    }
    return ids;
}

RenderLightSettings LightManager::getLightSettings(int lightId) const
{
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        return it->second->settings;
    }
    return RenderLightSettings();
}

bool LightManager::hasLight(int lightId) const
{
    return m_lights.find(lightId) != m_lights.end();
}

int LightManager::getLightCount() const
{
    return static_cast<int>(m_lights.size());
}

void LightManager::setLightEnabled(int lightId, bool enabled)
{
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second->lightNode->on.setValue(enabled);
        it->second->settings.enabled = enabled;
        updateMaterialsForLighting();
    }
}

void LightManager::setLightIntensity(int lightId, float intensity)
{
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second->lightNode->intensity.setValue(intensity);
        it->second->settings.intensity = intensity;
        updateMaterialsForLighting();
    }
}

void LightManager::setLightColor(int lightId, const wxColour& color)
{
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        float r = color.Red() / 255.0f;
        float g = color.Green() / 255.0f;
        float b = color.Blue() / 255.0f;
        it->second->lightNode->color.setValue(SbColor(r, g, b));
        it->second->settings.color = color;
        updateMaterialsForLighting();
    }
}

void LightManager::setLightPosition(int lightId, float x, float y, float z)
{
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        if (it->second->lightNode->isOfType(SoPointLight::getClassTypeId())) {
            SoPointLight* pointLight = static_cast<SoPointLight*>(it->second->lightNode);
            pointLight->location.setValue(SbVec3f(x, y, z));
        } else if (it->second->lightNode->isOfType(SoSpotLight::getClassTypeId())) {
            SoSpotLight* spotLight = static_cast<SoSpotLight*>(it->second->lightNode);
            spotLight->location.setValue(SbVec3f(x, y, z));
        }
        it->second->settings.positionX = x;
        it->second->settings.positionY = y;
        it->second->settings.positionZ = z;
        updateMaterialsForLighting();
    }
}

void LightManager::setLightDirection(int lightId, float x, float y, float z)
{
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        SbVec3f direction(x, y, z);
        direction.normalize();
        
        if (it->second->lightNode->isOfType(SoDirectionalLight::getClassTypeId())) {
            SoDirectionalLight* dirLight = static_cast<SoDirectionalLight*>(it->second->lightNode);
            dirLight->direction.setValue(direction);
        } else if (it->second->lightNode->isOfType(SoSpotLight::getClassTypeId())) {
            SoSpotLight* spotLight = static_cast<SoSpotLight*>(it->second->lightNode);
            spotLight->direction.setValue(direction);
        }
        it->second->settings.directionX = x;
        it->second->settings.directionY = y;
        it->second->settings.directionZ = z;
        updateMaterialsForLighting();
    }
}

void LightManager::applyLightPreset(const std::string& presetName)
{
    // TODO: Implement light presets
    LOG_INF_S("LightManager::applyLightPreset: Preset '" + presetName + "' not implemented yet");
}

void LightManager::updateMaterialsForLighting()
{
    LOG_INF_S("LightManager::updateMaterialsForLighting: Coin3D lighting system handles materials automatically");
    
    // Coin3D's lighting system automatically handles material lighting
    // We don't need to manually adjust material colors
    // The SoLightModel and SoLight nodes work together to provide proper lighting
    
    if (!m_objectRoot) {
        LOG_WRN_S("LightManager::updateMaterialsForLighting: Object root not available");
        return;
    }
    
    // Just log the current lighting state for debugging
    int enabledLightCount = 0;
    for (const auto& pair : m_lights) {
        if (pair.second->settings.enabled) {
            enabledLightCount++;
        }
    }
    
    LOG_INF_S("LightManager::updateMaterialsForLighting: " + std::to_string(enabledLightCount) + " lights enabled");
}

SoLight* LightManager::createLightNode(const RenderLightSettings& settings)
{
    if (settings.type == "directional") {
        SoDirectionalLight* light = new SoDirectionalLight();
        SbVec3f direction(static_cast<float>(settings.directionX),
                          static_cast<float>(settings.directionY),
                          static_cast<float>(settings.directionZ));
        direction.normalize();
        light->direction.setValue(direction);
        
        float r = settings.color.Red() / 255.0f;
        float g = settings.color.Green() / 255.0f;
        float b = settings.color.Blue() / 255.0f;
        light->color.setValue(SbColor(r, g, b));
        light->intensity.setValue(static_cast<float>(settings.intensity));
        light->on.setValue(settings.enabled);
        
        return light;
    }
    else if (settings.type == "point") {
        SoPointLight* light = new SoPointLight();
        SbVec3f position(static_cast<float>(settings.positionX),
                         static_cast<float>(settings.positionY),
                         static_cast<float>(settings.positionZ));
        light->location.setValue(position);
        
        float r = settings.color.Red() / 255.0f;
        float g = settings.color.Green() / 255.0f;
        float b = settings.color.Blue() / 255.0f;
        light->color.setValue(SbColor(r, g, b));
        light->intensity.setValue(static_cast<float>(settings.intensity));
        light->on.setValue(settings.enabled);
        
        return light;
    }
    else if (settings.type == "spot") {
        SoSpotLight* light = new SoSpotLight();
        SbVec3f position(static_cast<float>(settings.positionX),
                         static_cast<float>(settings.positionY),
                         static_cast<float>(settings.positionZ));
        light->location.setValue(position);
        
        SbVec3f direction(static_cast<float>(settings.directionX),
                          static_cast<float>(settings.directionY),
                          static_cast<float>(settings.directionZ));
        direction.normalize();
        light->direction.setValue(direction);
        
        float r = settings.color.Red() / 255.0f;
        float g = settings.color.Green() / 255.0f;
        float b = settings.color.Blue() / 255.0f;
        light->color.setValue(SbColor(r, g, b));
        light->intensity.setValue(static_cast<float>(settings.intensity));
        light->on.setValue(settings.enabled);
        
        float spotAngle = static_cast<float>(settings.spotAngle) * M_PI / 180.0f;
        light->cutOffAngle.setValue(spotAngle);
        light->dropOffRate.setValue(static_cast<float>(settings.spotExponent));
        
        return light;
    }
    
    return nullptr;
}

void LightManager::updateLightNode(SoLight* lightNode, const RenderLightSettings& settings)
{
    if (!lightNode) return;
    
    float r = settings.color.Red() / 255.0f;
    float g = settings.color.Green() / 255.0f;
    float b = settings.color.Blue() / 255.0f;
    lightNode->color.setValue(SbColor(r, g, b));
    lightNode->intensity.setValue(static_cast<float>(settings.intensity));
    lightNode->on.setValue(settings.enabled);
    
    if (lightNode->isOfType(SoDirectionalLight::getClassTypeId())) {
        SoDirectionalLight* dirLight = static_cast<SoDirectionalLight*>(lightNode);
        SbVec3f direction(static_cast<float>(settings.directionX),
                          static_cast<float>(settings.directionY),
                          static_cast<float>(settings.directionZ));
        direction.normalize();
        dirLight->direction.setValue(direction);
    }
    else if (lightNode->isOfType(SoPointLight::getClassTypeId())) {
        SoPointLight* pointLight = static_cast<SoPointLight*>(lightNode);
        SbVec3f position(static_cast<float>(settings.positionX),
                         static_cast<float>(settings.positionY),
                         static_cast<float>(settings.positionZ));
        pointLight->location.setValue(position);
    }
    else if (lightNode->isOfType(SoSpotLight::getClassTypeId())) {
        SoSpotLight* spotLight = static_cast<SoSpotLight*>(lightNode);
        SbVec3f position(static_cast<float>(settings.positionX),
                         static_cast<float>(settings.positionY),
                         static_cast<float>(settings.positionZ));
        spotLight->location.setValue(position);
        
        SbVec3f direction(static_cast<float>(settings.directionX),
                          static_cast<float>(settings.directionY),
                          static_cast<float>(settings.directionZ));
        direction.normalize();
        spotLight->direction.setValue(direction);
        
        float spotAngle = static_cast<float>(settings.spotAngle) * M_PI / 180.0f;
        spotLight->cutOffAngle.setValue(spotAngle);
        spotLight->dropOffRate.setValue(static_cast<float>(settings.spotExponent));
    }
}

SoSeparator* LightManager::createLightIndicator(const RenderLightSettings& settings, SoLight* lightNode)
{
    SoSeparator* indicator = new SoSeparator();
    indicator->ref();
    
    SbColor lightColor = lightNode->color.getValue();
    float lightIntensity = lightNode->intensity.getValue();
    
    SbVec3f indicatorPosition(static_cast<float>(settings.positionX),
                              static_cast<float>(settings.positionY),
                              static_cast<float>(settings.positionZ));
    
    if (settings.type == "directional") {
        SbVec3f lightDirection(static_cast<float>(settings.directionX),
                               static_cast<float>(settings.directionY),
                               static_cast<float>(settings.directionZ));
        indicatorPosition = lightDirection * -8.0f;
    }
    
    SoTransform* transform = new SoTransform();
    transform->translation.setValue(indicatorPosition);
    indicator->addChild(transform);
    
    SoMaterial* indicatorMaterial = new SoMaterial();
    indicatorMaterial->diffuseColor.setValue(lightColor);
    indicatorMaterial->emissiveColor.setValue(lightColor * 0.8f);
    indicatorMaterial->ambientColor.setValue(lightColor * 0.3f);
    indicatorMaterial->transparency.setValue(0.2f);
    indicator->addChild(indicatorMaterial);
    
    if (settings.type == "directional") {
        SbVec3f lightDirection(static_cast<float>(settings.directionX),
                               static_cast<float>(settings.directionY),
                               static_cast<float>(settings.directionZ));
        createDirectionalLightIndicator(indicator, lightDirection, lightIntensity);
    } else if (settings.type == "point") {
        createPointLightIndicator(indicator, lightIntensity);
    } else if (settings.type == "spot") {
        SbVec3f lightDirection(static_cast<float>(settings.directionX),
                               static_cast<float>(settings.directionY),
                               static_cast<float>(settings.directionZ));
        createSpotLightIndicator(indicator, lightDirection, lightIntensity);
    }
    
    return indicator;
}

void LightManager::createDirectionalLightIndicator(SoSeparator* indicator, const SbVec3f& lightDirection, float lightIntensity)
{
    float planeSize = 0.2f + lightIntensity * 0.3f;
    
    // Create a transform to orient the plane perpendicular to light direction
    SoTransform* planeTransform = new SoTransform();
    if (planeTransform) {
        // Calculate rotation to make plane perpendicular to light direction
        SbVec3f defaultNormal(0.0f, 0.0f, 1.0f); // Default plane normal
        SbVec3f targetNormal = -lightDirection; // Plane normal should be opposite to light direction
        
        // Create rotation from default normal to target normal
        SbRotation rotation(defaultNormal, targetNormal);
        planeTransform->rotation.setValue(rotation);
        indicator->addChild(planeTransform);
    }
    
    SoCoordinate3* coords = new SoCoordinate3();
    coords->point.set1Value(0, SbVec3f(-planeSize, -planeSize, 0.0f));
    coords->point.set1Value(1, SbVec3f(planeSize, -planeSize, 0.0f));
    coords->point.set1Value(2, SbVec3f(planeSize, planeSize, 0.0f));
    coords->point.set1Value(3, SbVec3f(-planeSize, planeSize, 0.0f));
    indicator->addChild(coords);
    
    SoIndexedFaceSet* faceSet = new SoIndexedFaceSet();
    faceSet->coordIndex.set1Value(0, 0);
    faceSet->coordIndex.set1Value(1, 1);
    faceSet->coordIndex.set1Value(2, 2);
    faceSet->coordIndex.set1Value(3, 3);
    faceSet->coordIndex.set1Value(4, -1);
    indicator->addChild(faceSet);
}

void LightManager::createPointLightIndicator(SoSeparator* indicator, float lightIntensity)
{
    SoSphere* sphere = new SoSphere();
    float sphereRadius = 0.2f + lightIntensity * 0.3f;
    sphere->radius.setValue(sphereRadius);
    indicator->addChild(sphere);
}

void LightManager::createSpotLightIndicator(SoSeparator* indicator, const SbVec3f& lightDirection, float lightIntensity)
{
    float coneHeight = 0.4f + lightIntensity * 0.6f;
    float coneRadius = 0.15f + lightIntensity * 0.2f;
    
    // Create a transform to orient the cone
    SoTransform* coneTransform = new SoTransform();
    if (coneTransform) {
        // Calculate the base normal direction (from origin outward)
        SbVec3f baseNormal = lightDirection; // Direction from origin to light position
        baseNormal.normalize();
        
        // Position the cone so its base is away from origin and tip points to origin
        // Move the cone along the base normal by half its height so tip is closer to origin
        SbVec3f conePosition = baseNormal * (coneHeight * 0.5f);
        coneTransform->translation.setValue(conePosition);
        
        // Default cone axis is along Y-axis (0, 1, 0) with tip pointing up
        // We need to rotate it so the tip points towards origin (opposite to base normal)
        SbVec3f defaultAxis(0.0f, 1.0f, 0.0f);
        SbVec3f targetAxis = -baseNormal; // Tip should point towards origin (inward)
        
        // Create rotation to align cone axis with target direction
        SbRotation rotation(defaultAxis, targetAxis);
        coneTransform->rotation.setValue(rotation);
        
        indicator->addChild(coneTransform);
    }
    
    SoCone* cone = new SoCone();
    cone->height.setValue(coneHeight);
    cone->bottomRadius.setValue(coneRadius);
    indicator->addChild(cone);
}

void LightManager::updateLightIndicator(SoSeparator* indicator, const RenderLightSettings& settings)
{
    // For simplicity, we'll recreate the indicator
    // In a more sophisticated implementation, we could update in place
}

void LightManager::updateSceneMaterials(const SbColor& lightColor, float totalIntensity, int lightCount)
{
    // Coin3D automatically handles material lighting when SoLightModel is present
    // No need to manually adjust material colors
    LOG_INF_S("LightManager::updateSceneMaterials: Coin3D handles material lighting automatically");
} 