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
#include <Inventor/sensors/SoTimerSensor.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <cmath>
#include <algorithm>
#include <limits>
#include <cstdlib>

LightManager::LightManager(SoSeparator* sceneRoot, SoSeparator* objectRoot)
    : m_sceneRoot(sceneRoot), m_objectRoot(objectRoot), m_nextLightId(1)
    , m_animationRunning(false), m_animationRate(DEFAULT_UPDATE_RATE)
    , m_keyEventCallback(nullptr), m_mouseEventCallback(nullptr), m_eventRoot(nullptr)
    , m_maxLights(MAX_LIGHTS), m_cameraPosition(0.0f, 0.0f, 10.0f)
{
    // Create indicator container for light visualization
    m_indicatorContainer = new SoSeparator();
    m_indicatorContainer->ref();
    m_objectRoot->addChild(m_indicatorContainer);
    
    // Initialize animation timer
    m_animationTimer = std::make_unique<SoTimerSensor>(animationTimerCallback, this);
    m_animationTimer->setInterval(1.0f / m_animationRate);
    
    LOG_INF_S("LightManager: Initialized with animation and event support");
}

LightManager::~LightManager()
{
    stopAnimation();
    removeEventCallbacks();
    clearAllLights();
    
    if (m_indicatorContainer) {
        m_indicatorContainer->unref();
    }
}

int LightManager::addLight(const RenderLightSettings& settings)
{
    LOG_INF_S("LightManager::addLight: Adding light '" + settings.name + "' of type '" + settings.type + "'");
    
    // Check light limit
    if (getLightCount() >= m_maxLights) {
        LOG_WRN_S("LightManager::addLight: Light limit reached (" + std::to_string(m_maxLights) + "), enforcing limit");
        enforceLightLimit();
    }
    
    auto managedLight = std::make_unique<ManagedLight>();
    managedLight->settings = settings;
    managedLight->lightId = m_nextLightId++;
    managedLight->animationTime = 0.0;
    managedLight->needsUpdate = false;
    
    SoLight* lightNode = createLightNode(settings);
    if (!lightNode) {
        LOG_ERR_S("LightManager::addLight: Failed to create light node");
        return -1;
    }
    
    // Ref the light node to ensure it's not deleted when removed from scene
    lightNode->ref();
    
    // Create transform node for animated lights
    if (settings.animated) {
        managedLight->transformNode = new SoTransform();
        managedLight->transformNode->ref();
        
        // Add transform before light node
        SoSeparator* lightGroup = new SoSeparator();
        lightGroup->ref();
        lightGroup->addChild(managedLight->transformNode);
        lightGroup->addChild(managedLight->lightNode);
        
        // Add light group to scene root
        int objectRootIndex = -1;
        for (int i = 0; i < m_sceneRoot->getNumChildren(); ++i) {
            if (m_sceneRoot->getChild(i) == m_objectRoot) {
                objectRootIndex = i;
                break;
            }
        }
        
        if (objectRootIndex >= 0) {
            m_sceneRoot->insertChild(lightGroup, objectRootIndex);
        } else {
            m_sceneRoot->addChild(lightGroup);
        }
        
        managedLight->lightNode = lightGroup; // Store group instead of individual light
    } else {
        managedLight->transformNode = nullptr;
        managedLight->lightNode = lightNode;
        
        // Add light directly to scene root
        int objectRootIndex = -1;
        for (int i = 0; i < m_sceneRoot->getNumChildren(); ++i) {
            if (m_sceneRoot->getChild(i) == m_objectRoot) {
                objectRootIndex = i;
                break;
            }
        }
        
        if (objectRootIndex >= 0) {
            m_sceneRoot->insertChild(lightNode, objectRootIndex);
        } else {
            m_sceneRoot->addChild(lightNode);
        }
    }
    
    // Cast to SoLight* for indicator creation
    if (managedLight->lightNode->isOfType(SoLight::getClassTypeId())) {
        managedLight->indicatorNode = createLightIndicator(settings, static_cast<SoLight*>(managedLight->lightNode));
    } else {
        managedLight->indicatorNode = nullptr;
    }
    if (managedLight->indicatorNode) {
        m_indicatorContainer->addChild(managedLight->indicatorNode);
    }
    
    int lightId = managedLight->lightId;
    m_lights[lightId] = std::move(managedLight);
    
    // Add to priority queue
    m_lightPriorityQueue.push({settings.priority, lightId});
    
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
    
    // Remove from scene graph
    if (managedLight->lightNode) {
        int index = m_sceneRoot->findChild(managedLight->lightNode);
        if (index >= 0) {
            m_sceneRoot->removeChild(index);
        }
        managedLight->lightNode->unref();
    }
    
    if (managedLight->transformNode) {
        managedLight->transformNode->unref();
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
    
    // Cast to SoLight* for update
    if (managedLight->lightNode->isOfType(SoLight::getClassTypeId())) {
        updateLightNode(static_cast<SoLight*>(managedLight->lightNode), settings);
    }
    
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
        if (managedLight->transformNode) {
            managedLight->transformNode->unref();
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
    if (it != m_lights.end() && it->second->lightNode->isOfType(SoLight::getClassTypeId())) {
        SoLight* light = static_cast<SoLight*>(it->second->lightNode);
        light->on.setValue(enabled);
        it->second->settings.enabled = enabled;
        updateMaterialsForLighting();
    }
}

void LightManager::setLightIntensity(int lightId, float intensity)
{
    auto it = m_lights.find(lightId);
    if (it != m_lights.end() && it->second->lightNode->isOfType(SoLight::getClassTypeId())) {
        SoLight* light = static_cast<SoLight*>(it->second->lightNode);
        light->intensity.setValue(intensity);
        it->second->settings.intensity = intensity;
        updateMaterialsForLighting();
    }
}

void LightManager::setLightColor(int lightId, const wxColour& color)
{
    auto it = m_lights.find(lightId);
    if (it != m_lights.end() && it->second->lightNode->isOfType(SoLight::getClassTypeId())) {
        SoLight* light = static_cast<SoLight*>(it->second->lightNode);
        float r = color.Red() / 255.0f;
        float g = color.Green() / 255.0f;
        float b = color.Blue() / 255.0f;
        light->color.setValue(SbColor(r, g, b));
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

// Animation system methods
void LightManager::setLightAnimation(int lightId, bool animated, double speed, double radius)
{
    auto it = m_lights.find(lightId);
    if (it == m_lights.end()) {
        LOG_WRN_S("LightManager::setLightAnimation: Light with ID " + std::to_string(lightId) + " not found");
        return;
    }
    
    auto& managedLight = it->second;
    managedLight->settings.animated = animated;
    managedLight->settings.animationSpeed = speed;
    managedLight->settings.animationRadius = radius;
    
    if (animated && !m_animationRunning) {
        startAnimation();
    }
    
    LOG_INF_S("LightManager::setLightAnimation: Set animation for light " + std::to_string(lightId) + 
              " - animated: " + std::to_string(animated) + ", speed: " + std::to_string(speed));
}

void LightManager::startAnimation()
{
    if (!m_animationRunning) {
        m_animationRunning = true;
        m_animationTimer->schedule();
        LOG_INF_S("LightManager::startAnimation: Animation started");
    }
}

void LightManager::stopAnimation()
{
    if (m_animationRunning) {
        m_animationRunning = false;
        m_animationTimer->unschedule();
        LOG_INF_S("LightManager::stopAnimation: Animation stopped");
    }
}

void LightManager::setAnimationRate(int fps)
{
    m_animationRate = fps;
    m_animationTimer->setInterval(1.0f / m_animationRate);
    LOG_INF_S("LightManager::setAnimationRate: Animation rate set to " + std::to_string(fps) + " FPS");
}

bool LightManager::isAnimationRunning() const
{
    return m_animationRunning;
}

void LightManager::updateLightAnimation(int lightId, double time)
{
    auto it = m_lights.find(lightId);
    if (it == m_lights.end() || !it->second->settings.animated) {
        return;
    }
    
    auto& managedLight = it->second;
    const auto& settings = managedLight->settings;
    
    // Calculate orbital animation
    double angle = time * settings.animationSpeed * 2.0 * M_PI;
    double x = settings.animationRadius * cos(angle);
    double z = settings.animationRadius * sin(angle);
    double y = settings.animationHeight;
    
    // Update transform node
    if (managedLight->transformNode) {
        SbVec3f position(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        managedLight->transformNode->translation.setValue(position);
        
        // Update light direction to point towards origin
        SbVec3f direction = -position;
        direction.normalize();
        
        // Update light node direction
        if (managedLight->lightNode->isOfType(SoDirectionalLight::getClassTypeId())) {
            SoDirectionalLight* dirLight = static_cast<SoDirectionalLight*>(managedLight->lightNode);
            dirLight->direction.setValue(direction);
        } else if (managedLight->lightNode->isOfType(SoSpotLight::getClassTypeId())) {
            SoSpotLight* spotLight = static_cast<SoSpotLight*>(managedLight->lightNode);
            spotLight->direction.setValue(direction);
        }
        
        // Call animation callback if set
        if (m_animationCallback) {
            m_animationCallback(lightId, position, direction);
        }
    }
}

void LightManager::updateAllAnimations()
{
    static double lastTime = 0.0;
    double currentTime = lastTime + (1.0 / m_animationRate);
    lastTime = currentTime;
    
    for (auto& pair : m_lights) {
        if (pair.second->settings.animated) {
            updateLightAnimation(pair.first, currentTime);
        }
    }
}

void LightManager::animationTimerCallback(void* data, SoSensor* sensor)
{
    LightManager* manager = static_cast<LightManager*>(data);
    if (manager && manager->m_animationRunning) {
        manager->updateAllAnimations();
    }
}

// Event callback methods
void LightManager::setupEventCallbacks(SoSeparator* eventRoot)
{
    if (m_eventRoot) {
        removeEventCallbacks();
    }
    
    m_eventRoot = eventRoot;
    
    // Setup keyboard event callback
    m_keyEventCallback = new SoEventCallback();
    m_keyEventCallback->addEventCallback(SoKeyboardEvent::getClassTypeId(), keyEventCallback, this);
    m_eventRoot->addChild(m_keyEventCallback);
    
    // Setup mouse event callback
    m_mouseEventCallback = new SoEventCallback();
    m_mouseEventCallback->addEventCallback(SoMouseButtonEvent::getClassTypeId(), mouseEventCallback, this);
    m_eventRoot->addChild(m_mouseEventCallback);
    
    LOG_INF_S("LightManager::setupEventCallbacks: Event callbacks setup completed");
}

void LightManager::removeEventCallbacks()
{
    if (m_keyEventCallback) {
        m_keyEventCallback->removeEventCallback(SoKeyboardEvent::getClassTypeId(), keyEventCallback, this);
        m_keyEventCallback->unref();
        m_keyEventCallback = nullptr;
    }
    
    if (m_mouseEventCallback) {
        m_mouseEventCallback->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), mouseEventCallback, this);
        m_mouseEventCallback->unref();
        m_mouseEventCallback = nullptr;
    }
    
    m_eventRoot = nullptr;
    LOG_INF_S("LightManager::removeEventCallbacks: Event callbacks removed");
}

void LightManager::keyEventCallback(void* data, SoEventCallback* eventCB)
{
    LightManager* manager = static_cast<LightManager*>(data);
    const SoKeyboardEvent* keyEvent = dynamic_cast<const SoKeyboardEvent*>(eventCB->getEvent());
    
    if (keyEvent && keyEvent->getState() == SoButtonEvent::DOWN) {
        if (keyEvent->getKey() == SoKeyboardEvent::L) {
            // Add new light on 'L' key press
            RenderLightSettings newLight;
            newLight.name = "Dynamic Light " + std::to_string(manager->getLightCount() + 1);
            newLight.type = "point";
            newLight.positionX = (rand() % 100 - 50) / 10.0;
            newLight.positionY = (rand() % 100 - 50) / 10.0;
            newLight.positionZ = (rand() % 100 - 50) / 10.0;
            newLight.intensity = 1.0 + (rand() % 100) / 100.0;
            newLight.color = wxColour(rand() % 255, rand() % 255, rand() % 255);
            newLight.animated = true;
            newLight.animationSpeed = 0.5 + (rand() % 100) / 100.0;
            
            manager->addLight(newLight);
            LOG_INF_S("LightManager::keyEventCallback: Added dynamic light on 'L' key press");
        } else if (keyEvent->getKey() == SoKeyboardEvent::A) {
            // Toggle animation on 'A' key press
            if (manager->isAnimationRunning()) {
                manager->stopAnimation();
            } else {
                manager->startAnimation();
            }
            LOG_INF_S("LightManager::keyEventCallback: Toggled animation on 'A' key press");
        }
    }
    
    eventCB->setHandled();
}

void LightManager::mouseEventCallback(void* data, SoEventCallback* eventCB)
{
    LightManager* manager = static_cast<LightManager*>(data);
    const SoMouseButtonEvent* mouseEvent = dynamic_cast<const SoMouseButtonEvent*>(eventCB->getEvent());
    
    if (mouseEvent && mouseEvent->getState() == SoButtonEvent::DOWN) {
        if (mouseEvent->getButton() == SoMouseButtonEvent::BUTTON2) { // Right click
            // Remove last light on right click
            auto lightIds = manager->getAllLightIds();
            if (!lightIds.empty()) {
                manager->removeLight(lightIds.back());
                LOG_INF_S("LightManager::mouseEventCallback: Removed last light on right click");
            }
        }
    }
    
    eventCB->setHandled();
}

// Performance management methods
void LightManager::setMaxLights(int maxLights)
{
    m_maxLights = std::min(maxLights, MAX_LIGHTS);
    if (getLightCount() > m_maxLights) {
        enforceLightLimit();
    }
    LOG_INF_S("LightManager::setMaxLights: Max lights set to " + std::to_string(m_maxLights));
}

int LightManager::getMaxLights() const
{
    return m_maxLights;
}

void LightManager::enforceLightLimit()
{
    if (getLightCount() <= m_maxLights) {
        return;
    }
    
    // Remove lowest priority lights
    std::vector<std::pair<int, int>> lightPriorities; // priority, lightId
    for (const auto& pair : m_lights) {
        lightPriorities.push_back({pair.second->settings.priority, pair.first});
    }
    
    // Sort by priority (lowest first)
    std::sort(lightPriorities.begin(), lightPriorities.end());
    
    // Remove excess lights
    int lightsToRemove = getLightCount() - m_maxLights;
    for (int i = 0; i < lightsToRemove; ++i) {
        removeLight(lightPriorities[i].second);
    }
    
    LOG_INF_S("LightManager::enforceLightLimit: Removed " + std::to_string(lightsToRemove) + " lights to enforce limit");
}

double LightManager::calculateLightDistance(int lightId, const SbVec3f& cameraPosition)
{
    auto it = m_lights.find(lightId);
    if (it == m_lights.end()) {
        return std::numeric_limits<double>::max();
    }
    
    const auto& settings = it->second->settings;
    SbVec3f lightPosition(static_cast<float>(settings.positionX),
                         static_cast<float>(settings.positionY),
                         static_cast<float>(settings.positionZ));
    
    return (lightPosition - cameraPosition).length();
}

void LightManager::optimizeLightOrder()
{
    // Reorder lights by priority and distance to camera
    std::vector<std::pair<double, int>> lightScores;
    
    for (const auto& pair : m_lights) {
        int lightId = pair.first;
        const auto& settings = pair.second->settings;
        
        // Calculate score based on priority and distance
        double distance = calculateLightDistance(lightId, m_cameraPosition);
        double score = settings.priority * 1000.0 - distance; // Higher priority and closer = better score
        
        lightScores.push_back({score, lightId});
    }
    
    // Sort by score (highest first)
    std::sort(lightScores.begin(), lightScores.end(), std::greater<>());
    
    LOG_INF_S("LightManager::optimizeLightOrder: Light order optimized");
}

// Preset lighting methods
void LightManager::createThreePointLighting()
{
    clearAllLights();
    
    // Main light (key light)
    RenderLightSettings mainLight;
    mainLight.name = "Key Light";
    mainLight.type = "directional";
    mainLight.directionX = 0.0f;
    mainLight.directionY = -0.707f;
    mainLight.directionZ = -0.707f;
    mainLight.intensity = 1.5f;
    mainLight.priority = 3;
    addLight(mainLight);
    
    // Fill light
    RenderLightSettings fillLight;
    fillLight.name = "Fill Light";
    fillLight.type = "directional";
    fillLight.directionX = -1.0f;
    fillLight.directionY = 0.0f;
    fillLight.directionZ = 0.0f;
    fillLight.intensity = 0.6f;
    fillLight.priority = 2;
    addLight(fillLight);
    
    // Rim light
    RenderLightSettings rimLight;
    rimLight.name = "Rim Light";
    rimLight.type = "directional";
    rimLight.directionX = 0.0f;
    rimLight.directionY = 1.0f;
    rimLight.directionZ = 0.0f;
    rimLight.intensity = 0.8f;
    rimLight.priority = 1;
    addLight(rimLight);
    
    LOG_INF_S("LightManager::createThreePointLighting: Three-point lighting setup completed");
}

void LightManager::createStudioLighting()
{
    clearAllLights();
    
    // Main studio light
    RenderLightSettings mainLight;
    mainLight.name = "Studio Main";
    mainLight.type = "spot";
    mainLight.positionX = 0.0f;
    mainLight.positionY = 5.0f;
    mainLight.positionZ = 5.0f;
    mainLight.directionX = 0.0f;
    mainLight.directionY = -1.0f;
    mainLight.directionZ = -1.0f;
    mainLight.intensity = 2.0f;
    mainLight.spotAngle = 45.0f;
    mainLight.spotExponent = 2.0f;
    mainLight.priority = 3;
    addLight(mainLight);
    
    // Ambient fill
    RenderLightSettings ambientLight;
    ambientLight.name = "Studio Ambient";
    ambientLight.type = "point";
    ambientLight.positionX = 0.0f;
    ambientLight.positionY = 2.0f;
    ambientLight.positionZ = 0.0f;
    ambientLight.intensity = 0.3f;
    ambientLight.priority = 1;
    addLight(ambientLight);
    
    LOG_INF_S("LightManager::createStudioLighting: Studio lighting setup completed");
}

void LightManager::createOutdoorLighting()
{
    clearAllLights();
    
    // Sun light
    RenderLightSettings sunLight;
    sunLight.name = "Sun";
    sunLight.type = "directional";
    sunLight.directionX = 0.5f;
    sunLight.directionY = -0.8f;
    sunLight.directionZ = -0.3f;
    sunLight.intensity = 2.0f;
    sunLight.color = wxColour(255, 248, 220); // Warm sunlight
    sunLight.priority = 3;
    addLight(sunLight);
    
    // Sky light
    RenderLightSettings skyLight;
    skyLight.name = "Sky";
    skyLight.type = "directional";
    skyLight.directionX = 0.0f;
    skyLight.directionY = 1.0f;
    skyLight.directionZ = 0.0f;
    skyLight.intensity = 0.5f;
    skyLight.color = wxColour(173, 216, 230); // Light blue sky
    skyLight.priority = 2;
    addLight(skyLight);
    
    LOG_INF_S("LightManager::createOutdoorLighting: Outdoor lighting setup completed");
}

void LightManager::setAnimationCallback(LightAnimationCallback callback)
{
    m_animationCallback = callback;
} 