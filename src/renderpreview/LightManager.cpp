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
    m_lightContainer = new SoSeparator();
    m_lightContainer->ref();
    m_sceneRoot->addChild(m_lightContainer);
    
    m_indicatorContainer = new SoSeparator();
    m_indicatorContainer->ref();
    m_objectRoot->addChild(m_indicatorContainer);
    
    LOG_INF_S("LightManager: Initialized with containers");
}

LightManager::~LightManager()
{
    clearAllLights();
    
    if (m_lightContainer) {
        m_lightContainer->unref();
    }
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
    
    managedLight->lightGroup = new SoSeparator();
    managedLight->lightGroup->ref();
    
    if (settings.type != "directional") {
        SoTransform* lightTransform = new SoTransform();
        SbVec3f position(static_cast<float>(settings.positionX),
                        static_cast<float>(settings.positionY),
                        static_cast<float>(settings.positionZ));
        lightTransform->translation.setValue(position);
        managedLight->lightGroup->addChild(lightTransform);
    }
    
    managedLight->lightGroup->addChild(managedLight->lightNode);
    m_lightContainer->addChild(managedLight->lightGroup);
    
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
    
    if (managedLight->lightGroup) {
        int index = m_lightContainer->findChild(managedLight->lightGroup);
        if (index >= 0) {
            m_lightContainer->removeChild(index);
        }
        managedLight->lightGroup->unref();
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
        
        if (managedLight->lightGroup) {
            managedLight->lightGroup->unref();
        }
        if (managedLight->indicatorNode) {
            managedLight->indicatorNode->unref();
        }
    }
    
    m_lights.clear();
    
    m_lightContainer->removeAllChildren();
    m_indicatorContainer->removeAllChildren();
    
    LOG_INF_S("LightManager::clearAllLights: All lights cleared");
}

void LightManager::updateMultipleLights(const std::vector<RenderLightSettings>& lights)
{
    LOG_INF_S("LightManager::updateMultipleLights: Updating " + std::to_string(lights.size()) + " lights");
    
    clearAllLights();
    
    for (const auto& lightSettings : lights) {
        if (lightSettings.enabled) {
            addLight(lightSettings);
        }
    }
    
    // Add material update
    updateMaterialsForLighting();
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
    LOG_INF_S("LightManager::updateMaterialsForLighting: Updating materials based on current lighting");
    
    if (!m_objectRoot) {
        LOG_WRN_S("LightManager::updateMaterialsForLighting: Object root not available");
        return;
    }
    
    // Calculate combined lighting from all enabled lights
    float totalR = 0.0f, totalG = 0.0f, totalB = 0.0f;
    float totalIntensity = 0.0f;
    int enabledLightCount = 0;
    
    for (const auto& pair : m_lights) {
        const auto& settings = pair.second->settings;
        if (settings.enabled) {
            float r = settings.color.Red() / 255.0f;
            float g = settings.color.Green() / 255.0f;
            float b = settings.color.Blue() / 255.0f;
            float intensity = static_cast<float>(settings.intensity);
            
            totalR += r * intensity;
            totalG += g * intensity;
            totalB += b * intensity;
            totalIntensity += intensity;
            enabledLightCount++;
        }
    }
    
    // Use weighted average for better multi-light effect
    if (totalIntensity > 0.0f) {
        totalR /= totalIntensity;
        totalG /= totalIntensity;
        totalB /= totalIntensity;
    } else {
        // Default lighting if no lights are enabled
        totalR = totalG = totalB = 0.5f;
        totalIntensity = 1.0f;
    }
    
    // Update all materials in the scene
    updateSceneMaterials(SbColor(totalR, totalG, totalB), totalIntensity, enabledLightCount);
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
    // Search for all material nodes in the object root
    SoSearchAction searchAction;
    searchAction.setType(SoMaterial::getClassTypeId(), true);
    searchAction.setSearchingAll(true);
    searchAction.apply(m_objectRoot);
    
    for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
        SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
        if (!path) continue;
        
        SoNode* tailNode = path->getTail();
        if (!tailNode || !tailNode->isOfType(SoMaterial::getClassTypeId())) continue;
        
        SoMaterial* material = static_cast<SoMaterial*>(tailNode);
        if (!material) continue;
        
        // Get the base color from the material's current diffuse color
        SbColor baseDiffuse = material->diffuseColor[0];
        
        // Calculate lighting-adjusted colors
        float ambientStrength = 0.3f + (lightCount * 0.05f); // Increase ambient with more lights
        float diffuseStrength = 0.8f;
        float specularStrength = 0.6f;
        
        // Apply intensity scaling (cap at 2x intensity)
        float intensityScale = std::min(totalIntensity / static_cast<float>(lightCount > 0 ? lightCount : 1), 2.0f);
        
        // Calculate new material colors
        SbColor newAmbient(
            baseDiffuse[0] * lightColor[0] * ambientStrength * intensityScale,
            baseDiffuse[1] * lightColor[1] * ambientStrength * intensityScale,
            baseDiffuse[2] * lightColor[2] * ambientStrength * intensityScale
        );
        
        SbColor newDiffuse(
            baseDiffuse[0] * lightColor[0] * diffuseStrength * intensityScale,
            baseDiffuse[1] * lightColor[1] * diffuseStrength * intensityScale,
            baseDiffuse[2] * lightColor[2] * diffuseStrength * intensityScale
        );
        
        SbColor newSpecular(
            lightColor[0] * specularStrength * intensityScale,
            lightColor[1] * specularStrength * intensityScale,
            lightColor[2] * specularStrength * intensityScale
        );
        
        // Apply the new colors
        material->ambientColor.setValue(newAmbient);
        material->diffuseColor.setValue(newDiffuse);
        material->specularColor.setValue(newSpecular);
    }
    
    LOG_INF_S("LightManager::updateSceneMaterials: Updated materials for " + std::to_string(lightCount) + " lights");
} 