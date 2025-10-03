#include "param/ParameterUpdateManager.h"
#include "OCCGeometry.h"
#include "config/RenderingConfig.h"
#include <algorithm>
#include <iostream>
#include <thread>

// ParameterUpdateMapping 实现
std::map<std::string, UpdateType> ParameterUpdateMapping::s_parameterToUpdateType;
std::map<std::string, UpdatePriority> ParameterUpdateMapping::s_parameterToPriority;
std::map<std::string, std::vector<UpdateType>> ParameterUpdateMapping::s_parameterToAffectedTypes;

void ParameterUpdateMapping::initializeMappings() {
    // 几何变换参数
    s_parameterToUpdateType["geometry/transform/position/x"] = UpdateType::Transform;
    s_parameterToUpdateType["geometry/transform/position/y"] = UpdateType::Transform;
    s_parameterToUpdateType["geometry/transform/position/z"] = UpdateType::Transform;
    s_parameterToUpdateType["geometry/transform/rotation/axis/x"] = UpdateType::Transform;
    s_parameterToUpdateType["geometry/transform/rotation/axis/y"] = UpdateType::Transform;
    s_parameterToUpdateType["geometry/transform/rotation/axis/z"] = UpdateType::Transform;
    s_parameterToUpdateType["geometry/transform/rotation/angle"] = UpdateType::Transform;
    s_parameterToUpdateType["geometry/transform/scale"] = UpdateType::Transform;
    
    // 几何显示参数
    s_parameterToUpdateType["geometry/display/visible"] = UpdateType::Display;
    s_parameterToUpdateType["geometry/display/selected"] = UpdateType::Display;
    s_parameterToUpdateType["geometry/display/wireframe_mode"] = UpdateType::Display;
    s_parameterToUpdateType["geometry/display/show_wireframe"] = UpdateType::Display;
    s_parameterToUpdateType["geometry/display/faces_visible"] = UpdateType::Display;
    s_parameterToUpdateType["geometry/display/wireframe_overlay"] = UpdateType::Display;
    
    // 几何颜色参数
    s_parameterToUpdateType["geometry/color/main"] = UpdateType::Color;
    s_parameterToUpdateType["geometry/color/edge"] = UpdateType::Color;
    s_parameterToUpdateType["geometry/color/vertex"] = UpdateType::Color;
    s_parameterToUpdateType["geometry/transparency"] = UpdateType::Material;
    
    // 材质参数
    s_parameterToUpdateType["material/color/ambient"] = UpdateType::Material;
    s_parameterToUpdateType["material/color/diffuse"] = UpdateType::Material;
    s_parameterToUpdateType["material/color/specular"] = UpdateType::Material;
    s_parameterToUpdateType["material/color/emissive"] = UpdateType::Material;
    s_parameterToUpdateType["material/properties/shininess"] = UpdateType::Material;
    s_parameterToUpdateType["material/properties/transparency"] = UpdateType::Material;
    s_parameterToUpdateType["material/properties/metallic"] = UpdateType::Material;
    s_parameterToUpdateType["material/properties/roughness"] = UpdateType::Material;
    
    // 纹理参数
    s_parameterToUpdateType["texture/enabled"] = UpdateType::Texture;
    s_parameterToUpdateType["texture/image_path"] = UpdateType::Texture;
    s_parameterToUpdateType["texture/color/main"] = UpdateType::Texture;
    s_parameterToUpdateType["texture/intensity"] = UpdateType::Texture;
    s_parameterToUpdateType["texture/mode/texture_mode"] = UpdateType::Texture;
    
    // 光照参数
    s_parameterToUpdateType["lighting/model/lighting_model"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/model/roughness"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/model/metallic"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/model/fresnel"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/ambient/color"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/ambient/intensity"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/diffuse/color"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/diffuse/intensity"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/specular/color"] = UpdateType::Lighting;
    s_parameterToUpdateType["lighting/specular/intensity"] = UpdateType::Lighting;
    
    // 阴影参数
    s_parameterToUpdateType["shadow/mode/shadow_mode"] = UpdateType::Shadow;
    s_parameterToUpdateType["shadow/mode/enabled"] = UpdateType::Shadow;
    s_parameterToUpdateType["shadow/intensity/shadow_intensity"] = UpdateType::Shadow;
    s_parameterToUpdateType["shadow/intensity/shadow_softness"] = UpdateType::Shadow;
    s_parameterToUpdateType["shadow/quality/shadow_map_size"] = UpdateType::Shadow;
    s_parameterToUpdateType["shadow/quality/shadow_bias"] = UpdateType::Shadow;
    
    // 质量参数
    s_parameterToUpdateType["quality/level/rendering_quality"] = UpdateType::Quality;
    s_parameterToUpdateType["quality/level/tessellation_level"] = UpdateType::Quality;
    s_parameterToUpdateType["quality/antialiasing/samples"] = UpdateType::Quality;
    s_parameterToUpdateType["quality/antialiasing/enabled"] = UpdateType::Quality;
    s_parameterToUpdateType["quality/lod/enabled"] = UpdateType::Quality;
    s_parameterToUpdateType["quality/lod/distance"] = UpdateType::Quality;
    
    // 渲染参数
    s_parameterToUpdateType["rendering/mode/display_mode"] = UpdateType::Rendering;
    s_parameterToUpdateType["rendering/mode/shading_mode"] = UpdateType::Rendering;
    s_parameterToUpdateType["rendering/mode/rendering_quality"] = UpdateType::Rendering;
    s_parameterToUpdateType["rendering/features/show_edges"] = UpdateType::Rendering;
    s_parameterToUpdateType["rendering/features/show_vertices"] = UpdateType::Rendering;
    s_parameterToUpdateType["rendering/features/smooth_normals"] = UpdateType::Rendering;
    
    // 设置优先级
    for (auto& pair : s_parameterToUpdateType) {
        const std::string& path = pair.first;
        UpdateType type = pair.second;
        
        if (path.find("transform") != std::string::npos) {
            s_parameterToPriority[path] = UpdatePriority::High;
        } else if (path.find("color") != std::string::npos || path.find("material") != std::string::npos) {
            s_parameterToPriority[path] = UpdatePriority::Normal;
        } else if (path.find("quality") != std::string::npos || path.find("shadow") != std::string::npos) {
            s_parameterToPriority[path] = UpdatePriority::Low;
        } else {
            s_parameterToPriority[path] = UpdatePriority::Normal;
        }
    }
    
    // 设置影响类型
    for (auto& pair : s_parameterToUpdateType) {
        const std::string& path = pair.first;
        UpdateType type = pair.second;
        
        s_parameterToAffectedTypes[path] = {type};
        
        // 某些参数可能影响多个更新类型
        if (path.find("material") != std::string::npos) {
            s_parameterToAffectedTypes[path].push_back(UpdateType::Rendering);
        }
        if (path.find("lighting") != std::string::npos) {
            s_parameterToAffectedTypes[path].push_back(UpdateType::Rendering);
        }
        if (path.find("quality") != std::string::npos) {
            s_parameterToAffectedTypes[path].push_back(UpdateType::Rendering);
        }
    }
}

UpdateType ParameterUpdateMapping::getUpdateType(const std::string& parameterPath) {
    auto it = s_parameterToUpdateType.find(parameterPath);
    return (it != s_parameterToUpdateType.end()) ? it->second : UpdateType::FullRefresh;
}

UpdatePriority ParameterUpdateMapping::getUpdatePriority(const std::string& parameterPath) {
    auto it = s_parameterToPriority.find(parameterPath);
    return (it != s_parameterToPriority.end()) ? it->second : UpdatePriority::Normal;
}

std::vector<UpdateType> ParameterUpdateMapping::getAffectedUpdateTypes(const std::string& parameterPath) {
    auto it = s_parameterToAffectedTypes.find(parameterPath);
    return (it != s_parameterToAffectedTypes.end()) ? it->second : std::vector<UpdateType>{UpdateType::FullRefresh};
}

// ParameterUpdateManager 实现
ParameterUpdateManager& ParameterUpdateManager::getInstance() {
    static ParameterUpdateManager instance;
    return instance;
}

ParameterUpdateManager::ParameterUpdateManager()
    : m_inBatchUpdate(false)
    , m_batchUpdateThreshold(10)
    , m_updateDelay(std::chrono::milliseconds(16)) // 60 FPS
    , m_optimizationEnabled(true)
    , m_maxUpdatesPerSecond(60)
    , m_debugMode(false) {
}

void ParameterUpdateManager::registerUpdateInterface(std::shared_ptr<IUpdateInterface> interface) {
    std::lock_guard<std::mutex> lock(m_interfacesMutex);
    m_updateInterfaces.push_back(interface);
}

void ParameterUpdateManager::unregisterUpdateInterface(std::shared_ptr<IUpdateInterface> interface) {
    std::lock_guard<std::mutex> lock(m_interfacesMutex);
    m_updateInterfaces.erase(
        std::remove_if(m_updateInterfaces.begin(), m_updateInterfaces.end(),
            [&interface](const std::shared_ptr<IUpdateInterface>& ptr) {
                return ptr == interface;
            }),
        m_updateInterfaces.end()
    );
}

void ParameterUpdateManager::onParameterChanged(const std::string& path, const ParameterValue& value) {
    if (m_debugMode) {
        std::cout << "Parameter changed: " << path << std::endl;
    }
    
    if (m_inBatchUpdate) {
        std::lock_guard<std::mutex> lock(m_batchMutex);
        m_batchChangedPaths.push_back(path);
    } else {
        scheduleUpdate(path, value);
    }
}

void ParameterUpdateManager::onBatchUpdate(const std::vector<std::string>& changedPaths) {
    if (m_debugMode) {
        std::cout << "Batch update with " << changedPaths.size() << " parameters" << std::endl;
    }
    
    // 优化批量更新
    if (m_optimizationEnabled) {
        optimizeUpdateTasks();
    }
    
    // 执行更新任务
    processUpdateTasks();
}

void ParameterUpdateManager::addUpdateTask(const UpdateTask& task) {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_updateTasks.push_back(task);
}

void ParameterUpdateManager::processUpdateTasks() {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    
    if (m_updateTasks.empty()) {
        return;
    }
    
    // 按优先级排序
    std::sort(m_updateTasks.begin(), m_updateTasks.end(),
        [](const UpdateTask& a, const UpdateTask& b) {
            return static_cast<int>(a.priority) > static_cast<int>(b.priority);
        });
    
    // 合并相同类型的更新任务
    if (m_optimizationEnabled) {
        mergeUpdateTasks();
    }
    
    // 执行更新任务
    std::set<UpdateType> executedTypes;
    for (const auto& task : m_updateTasks) {
        if (executedTypes.find(task.type) == executedTypes.end()) {
            executeUpdate(task.type);
            executedTypes.insert(task.type);
        }
    }
    
    m_updateTasks.clear();
}

void ParameterUpdateManager::clearUpdateTasks() {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_updateTasks.clear();
}

void ParameterUpdateManager::beginBatchUpdate() {
    m_inBatchUpdate = true;
    std::lock_guard<std::mutex> lock(m_batchMutex);
    m_batchChangedPaths.clear();
}

void ParameterUpdateManager::endBatchUpdate() {
    m_inBatchUpdate = false;
    
    std::lock_guard<std::mutex> lock(m_batchMutex);
    if (!m_batchChangedPaths.empty()) {
        onBatchUpdate(m_batchChangedPaths);
        m_batchChangedPaths.clear();
    }
}

void ParameterUpdateManager::setUpdateStrategy(UpdateType type, std::function<void()> strategy) {
    m_updateStrategies[type] = strategy;
}

void ParameterUpdateManager::setUpdateFrequencyLimit(int maxUpdatesPerSecond) {
    m_maxUpdatesPerSecond = maxUpdatesPerSecond;
}

size_t ParameterUpdateManager::getPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    return m_updateTasks.size();
}

std::vector<std::string> ParameterUpdateManager::getPendingParameterPaths() const {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    std::vector<std::string> paths;
    for (const auto& task : m_updateTasks) {
        paths.push_back(task.parameterPath);
    }
    return paths;
}

void ParameterUpdateManager::scheduleUpdate(const std::string& parameterPath, const ParameterValue& value) {
    if (shouldSkipUpdate(parameterPath)) {
        return;
    }
    
    UpdateType type = ParameterUpdateMapping::getUpdateType(parameterPath);
    UpdatePriority priority = ParameterUpdateMapping::getUpdatePriority(parameterPath);
    
    auto task = UpdateTask(type, priority, parameterPath, value, nullptr);
    addUpdateTask(task);
    
    // 检查是否需要立即执行
    if (priority >= UpdatePriority::High) {
        processUpdateTasks();
    }
}

void ParameterUpdateManager::executeUpdate(UpdateType type) {
    std::lock_guard<std::mutex> lock(m_interfacesMutex);
    
    for (auto& interface : m_updateInterfaces) {
        switch (type) {
            case UpdateType::Geometry:
                interface->updateGeometry();
                break;
            case UpdateType::Rendering:
                interface->updateRendering();
                break;
            case UpdateType::Display:
                interface->updateDisplay();
                break;
            case UpdateType::Lighting:
                interface->updateLighting();
                break;
            case UpdateType::Material:
                interface->updateMaterial();
                break;
            case UpdateType::Texture:
                interface->updateTexture();
                break;
            case UpdateType::Shadow:
                interface->updateShadow();
                break;
            case UpdateType::Quality:
                interface->updateQuality();
                break;
            case UpdateType::Transform:
                interface->updateTransform();
                break;
            case UpdateType::Color:
                interface->updateColor();
                break;
            case UpdateType::FullRefresh:
                interface->fullRefresh();
                break;
        }
    }
}

void ParameterUpdateManager::optimizeUpdateTasks() {
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    
    // 移除重复的更新任务
    std::map<UpdateType, UpdateTask> latestTasks;
    for (const auto& task : m_updateTasks) {
        auto it = latestTasks.find(task.type);
        if (it == latestTasks.end() || task.timestamp > it->second.timestamp) {
            latestTasks[task.type] = task;
        }
    }
    
    m_updateTasks.clear();
    for (const auto& pair : latestTasks) {
        m_updateTasks.push_back(pair.second);
    }
}

void ParameterUpdateManager::mergeUpdateTasks() {
    // 合并相同类型的更新任务，只保留最新的
    std::map<UpdateType, UpdateTask> mergedTasks;
    
    for (const auto& task : m_updateTasks) {
        auto it = mergedTasks.find(task.type);
        if (it == mergedTasks.end() || task.timestamp > it->second.timestamp) {
            mergedTasks[task.type] = task;
        }
    }
    
    m_updateTasks.clear();
    for (const auto& pair : mergedTasks) {
        m_updateTasks.push_back(pair.second);
    }
}

bool ParameterUpdateManager::shouldSkipUpdate(const std::string& parameterPath) const {
    if (!m_optimizationEnabled) {
        return false;
    }
    
    // 检查更新频率限制
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastUpdateTime).count();
    
    if (timeSinceLastUpdate < (1000 / m_maxUpdatesPerSecond)) {
        return true;
    }
    
    // 检查是否最近已更新
    if (m_recentlyUpdatedPaths.find(parameterPath) != m_recentlyUpdatedPaths.end()) {
        return true;
    }
    
    return false;
}

// GeometryUpdateInterface 实现
GeometryUpdateInterface::GeometryUpdateInterface(std::shared_ptr<OCCGeometry> geometry)
    : m_geometry(geometry) {
}

void GeometryUpdateInterface::updateGeometry() {
    if (m_geometry) {
        m_geometry->setMeshRegenerationNeeded(true);
    }
}

void GeometryUpdateInterface::updateRendering() {
    if (m_geometry) {
        m_geometry->updateFromRenderingConfig();
    }
}

void GeometryUpdateInterface::updateDisplay() {
    if (m_geometry) {
        m_geometry->buildCoinRepresentation();
    }
}

void GeometryUpdateInterface::updateLighting() {
    if (m_geometry) {
        m_geometry->updateMaterialForLighting();
    }
}

void GeometryUpdateInterface::updateMaterial() {
    if (m_geometry) {
        m_geometry->updateMaterialForLighting();
    }
}

void GeometryUpdateInterface::updateTexture() {
    if (m_geometry) {
        m_geometry->forceTextureUpdate();
    }
}

void GeometryUpdateInterface::updateShadow() {
    if (m_geometry) {
        m_geometry->updateFromRenderingConfig();
    }
}

void GeometryUpdateInterface::updateQuality() {
    if (m_geometry) {
        m_geometry->setMeshRegenerationNeeded(true);
    }
}

void GeometryUpdateInterface::updateTransform() {
    if (m_geometry) {
        m_geometry->buildCoinRepresentation();
    }
}

void GeometryUpdateInterface::updateColor() {
    if (m_geometry) {
        m_geometry->updateFromRenderingConfig();
    }
}

void GeometryUpdateInterface::fullRefresh() {
    if (m_geometry) {
        m_geometry->setMeshRegenerationNeeded(true);
        m_geometry->buildCoinRepresentation();
    }
}

// RenderingConfigUpdateInterface 实现
RenderingConfigUpdateInterface::RenderingConfigUpdateInterface(RenderingConfig* config)
    : m_config(config) {
}

void RenderingConfigUpdateInterface::updateGeometry() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateRendering() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateDisplay() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateLighting() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateMaterial() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateTexture() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateShadow() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateQuality() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateTransform() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::updateColor() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

void RenderingConfigUpdateInterface::fullRefresh() {
    if (m_config) {
        m_config->notifySettingsChanged();
    }
}

// ParameterUpdateManagerInitializer 实现
void ParameterUpdateManagerInitializer::initialize() {
    initializeParameterMappings();
    initializeUpdateStrategies();
    initializeDefaultInterfaces();
}

void ParameterUpdateManagerInitializer::initializeParameterMappings() {
    ParameterUpdateMapping::initializeMappings();
}

void ParameterUpdateManagerInitializer::initializeUpdateStrategies() {
    auto& manager = ParameterUpdateManager::getInstance();
    
    // 设置默认更新策略
    manager.setUpdateStrategy(UpdateType::Geometry, []() {
        // 几何更新策略
    });
    
    manager.setUpdateStrategy(UpdateType::Rendering, []() {
        // 渲染更新策略
    });
    
    manager.setUpdateStrategy(UpdateType::Display, []() {
        // 显示更新策略
    });
}

void ParameterUpdateManagerInitializer::initializeDefaultInterfaces() {
    // 初始化默认的更新接口
    // 这里可以根据需要添加默认的更新接口
}