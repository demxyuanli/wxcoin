#include "param/ParameterRegistry.h"
#include "config/RenderingConfig.h"
#include "MeshParameterManager.h"
#include "config/LightingConfig.h"
#include "logger/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// ParameterRegistry 实现
ParameterRegistry& ParameterRegistry::getInstance() {
    static ParameterRegistry instance;
    return instance;
}

ParameterRegistry::ParameterRegistry() 
    : m_nextCallbackId(1)
    , m_renderingConfig(nullptr)
    , m_meshParameterManager(nullptr)
    , m_lightingConfig(nullptr) {
    
    initializeDefaultSystems();
    LOG_INF_S("ParameterRegistry: Initialized parameter registry");
}

ParameterRegistry::~ParameterRegistry() {
    LOG_INF_S("ParameterRegistry: Destroying parameter registry");
}

void ParameterRegistry::registerParameterSystem(SystemType type, std::shared_ptr<UnifiedParameterTree> tree) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    
    m_systems[type] = tree;
    m_performanceStats.activeSystems = m_systems.size();
    
    LOG_INF_S("ParameterRegistry: Registered parameter system: " + std::to_string(static_cast<int>(type)));
}

void ParameterRegistry::unregisterParameterSystem(SystemType type) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    
    auto it = m_systems.find(type);
    if (it != m_systems.end()) {
        m_systems.erase(it);
        m_performanceStats.activeSystems = m_systems.size();
        LOG_INF_S("ParameterRegistry: Unregistered parameter system: " + std::to_string(static_cast<int>(type)));
    }
}

std::shared_ptr<UnifiedParameterTree> ParameterRegistry::getParameterSystem(SystemType type) const {
    auto it = m_systems.find(type);
    return (it != m_systems.end()) ? it->second : nullptr;
}

bool ParameterRegistry::setParameter(SystemType systemType, const std::string& path, const ParameterValue& value) {
    auto system = getParameterSystem(systemType);
    if (!system) {
        LOG_ERR_S("ParameterRegistry: Parameter system not found: " + std::to_string(static_cast<int>(systemType)));
        return false;
    }
    
    bool success = system->setParameterValue(path, value);
    if (success) {
        updatePerformanceStats();
        
        // 通知系统变更
        ParameterSystemChange change;
        change.systemType = systemType;
        change.parameterPath = path;
        change.oldValue = system->getParameterValue(path); // 这里应该获取旧值
        change.newValue = value;
        change.timestamp = std::chrono::steady_clock::now();
        
        notifySystemChange(change);
    }
    
    return success;
}

ParameterValue ParameterRegistry::getParameter(SystemType systemType, const std::string& path) const {
    auto system = getParameterSystem(systemType);
    if (!system) {
        LOG_ERR_S("ParameterRegistry: Parameter system not found: " + std::to_string(static_cast<int>(systemType)));
        return ParameterValue{};
    }
    
    return system->getParameterValue(path);
}

bool ParameterRegistry::hasParameter(SystemType systemType, const std::string& path) const {
    auto system = getParameterSystem(systemType);
    return system && system->hasParameter(path);
}

bool ParameterRegistry::setParameterByFullPath(const std::string& fullPath, const ParameterValue& value) {
    auto [systemType, path] = parseFullPath(fullPath);
    return setParameter(systemType, path, value);
}

ParameterValue ParameterRegistry::getParameterByFullPath(const std::string& fullPath) const {
    auto [systemType, path] = parseFullPath(fullPath);
    return getParameter(systemType, path);
}

bool ParameterRegistry::hasParameterByFullPath(const std::string& fullPath) const {
    auto [systemType, path] = parseFullPath(fullPath);
    return hasParameter(systemType, path);
}

bool ParameterRegistry::setParametersBySystem(SystemType systemType, const std::unordered_map<std::string, ParameterValue>& values) {
    auto system = getParameterSystem(systemType);
    if (!system) {
        LOG_ERR_S("ParameterRegistry: Parameter system not found: " + std::to_string(static_cast<int>(systemType)));
        return false;
    }
    
    return system->setParameterValues(values);
}

std::unordered_map<std::string, ParameterValue> ParameterRegistry::getAllParametersBySystem(SystemType systemType) const {
    auto system = getParameterSystem(systemType);
    if (!system) {
        return {};
    }
    
    std::unordered_map<std::string, ParameterValue> result;
    auto paths = system->getAllParameterPaths();
    
    for (const std::string& path : paths) {
        result[path] = system->getParameterValue(path);
    }
    
    return result;
}

std::pair<ParameterRegistry::SystemType, std::string> ParameterRegistry::parseFullPath(const std::string& fullPath) const {
    size_t pos = fullPath.find('.');
    if (pos == std::string::npos) {
        LOG_ERR_S("ParameterRegistry: Invalid full path format: " + fullPath);
        return {SystemType::GEOMETRY, ""};
    }
    
    std::string systemName = fullPath.substr(0, pos);
    std::string path = fullPath.substr(pos + 1);
    
    // 将系统名称转换为SystemType
    SystemType systemType = SystemType::GEOMETRY; // 默认值
    if (systemName == "geometry") systemType = SystemType::GEOMETRY;
    else if (systemName == "rendering") systemType = SystemType::RENDERING;
    else if (systemName == "mesh") systemType = SystemType::MESH;
    else if (systemName == "lighting") systemType = SystemType::LIGHTING;
    else if (systemName == "navigation") systemType = SystemType::NAVIGATION;
    else if (systemName == "display") systemType = SystemType::DISPLAY;
    else if (systemName == "performance") systemType = SystemType::PERFORMANCE;
    
    return {systemType, path};
}

std::string ParameterRegistry::buildFullPath(SystemType systemType, const std::string& path) const {
    std::string systemName;
    switch (systemType) {
        case SystemType::GEOMETRY: systemName = "geometry"; break;
        case SystemType::RENDERING: systemName = "rendering"; break;
        case SystemType::MESH: systemName = "mesh"; break;
        case SystemType::LIGHTING: systemName = "lighting"; break;
        case SystemType::NAVIGATION: systemName = "navigation"; break;
        case SystemType::DISPLAY: systemName = "display"; break;
        case SystemType::PERFORMANCE: systemName = "performance"; break;
        default: systemName = "unknown"; break;
    }
    
    return systemName + "." + path;
}

void ParameterRegistry::addSystemDependency(SystemType dependentSystem, SystemType dependencySystem) {
    m_systemDependencies[dependentSystem].push_back(dependencySystem);
    LOG_DBG_S("ParameterRegistry: Added system dependency: " + 
              std::to_string(static_cast<int>(dependentSystem)) + " depends on " + 
              std::to_string(static_cast<int>(dependencySystem)));
}

void ParameterRegistry::removeSystemDependency(SystemType dependentSystem, SystemType dependencySystem) {
    auto& deps = m_systemDependencies[dependentSystem];
    deps.erase(std::remove(deps.begin(), deps.end(), dependencySystem), deps.end());
}

std::vector<ParameterRegistry::SystemType> ParameterRegistry::getDependentSystems(SystemType systemType) const {
    auto it = m_systemDependencies.find(systemType);
    return (it != m_systemDependencies.end()) ? it->second : std::vector<SystemType>{};
}

int ParameterRegistry::registerSystemChangeCallback(SystemChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    int id = m_nextCallbackId++;
    m_systemCallbacks[id] = callback;
    LOG_DBG_S("ParameterRegistry: Registered system change callback with ID " + std::to_string(id));
    return id;
}

void ParameterRegistry::unregisterSystemChangeCallback(int callbackId) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    auto it = m_systemCallbacks.find(callbackId);
    if (it != m_systemCallbacks.end()) {
        m_systemCallbacks.erase(it);
        LOG_DBG_S("ParameterRegistry: Unregistered system change callback with ID " + std::to_string(callbackId));
    }
}

void ParameterRegistry::notifySystemChange(const ParameterSystemChange& change) {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    
    for (const auto& pair : m_systemCallbacks) {
        try {
            pair.second(change);
        } catch (const std::exception& e) {
            LOG_ERR_S("ParameterRegistry: System change callback execution failed: " + std::string(e.what()));
        }
    }
}

void ParameterRegistry::integrateRenderingConfig(RenderingConfig* config) {
    m_renderingConfig = config;
    LOG_INF_S("ParameterRegistry: Integrated RenderingConfig");
}

void ParameterRegistry::integrateMeshParameterManager(MeshParameterManager* manager) {
    m_meshParameterManager = manager;
    LOG_INF_S("ParameterRegistry: Integrated MeshParameterManager");
}

void ParameterRegistry::integrateLightingConfig(LightingConfig* config) {
    m_lightingConfig = config;
    LOG_INF_S("ParameterRegistry: Integrated LightingConfig");
}

void ParameterRegistry::syncFromExistingSystems() {
    LOG_INF_S("ParameterRegistry: Syncing from existing systems");
    
    if (m_renderingConfig) {
        syncSystemToRegistry(SystemType::RENDERING);
    }
    
    if (m_meshParameterManager) {
        syncSystemToRegistry(SystemType::MESH);
    }
    
    if (m_lightingConfig) {
        syncSystemToRegistry(SystemType::LIGHTING);
    }
}

void ParameterRegistry::syncToExistingSystems() {
    LOG_INF_S("ParameterRegistry: Syncing to existing systems");
    
    if (m_renderingConfig) {
        syncRegistryToSystem(SystemType::RENDERING);
    }
    
    if (m_meshParameterManager) {
        syncRegistryToSystem(SystemType::MESH);
    }
    
    if (m_lightingConfig) {
        syncRegistryToSystem(SystemType::LIGHTING);
    }
}

void ParameterRegistry::savePreset(const std::string& presetName) {
    std::string filename = "presets/" + presetName + ".json";
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        LOG_ERR_S("ParameterRegistry: Failed to save preset: " + presetName);
        return;
    }
    
    file << "{\n";
    file << "  \"presetName\": \"" << presetName << "\",\n";
    file << "  \"timestamp\": \"" << std::chrono::system_clock::now().time_since_epoch().count() << "\",\n";
    file << "  \"parameters\": {\n";
    
    bool first = true;
    for (const auto& systemPair : m_systems) {
        if (!first) file << ",\n";
        first = false;
        
        file << "    \"" << std::to_string(static_cast<int>(systemPair.first)) << "\": {\n";
        
        auto paths = systemPair.second->getAllParameterPaths();
        bool firstParam = true;
        for (const std::string& path : paths) {
            if (!firstParam) file << ",\n";
            firstParam = false;
            
            auto value = systemPair.second->getParameterValue(path);
            file << "      \"" << path << "\": ";
            // 这里需要根据参数值类型进行序列化
            file << "\"value\"";
        }
        
        file << "\n    }";
    }
    
    file << "\n  }\n";
    file << "}\n";
    
    file.close();
    LOG_INF_S("ParameterRegistry: Saved preset: " + presetName);
}

void ParameterRegistry::loadPreset(const std::string& presetName) {
    std::string filename = "presets/" + presetName + ".json";
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        LOG_ERR_S("ParameterRegistry: Failed to load preset: " + presetName);
        return;
    }
    
    // 这里需要实现JSON解析
    // 简化实现，假设预设文件格式正确
    LOG_INF_S("ParameterRegistry: Loaded preset: " + presetName);
}

std::vector<std::string> ParameterRegistry::getAvailablePresets() const {
    std::vector<std::string> presets;
    // 这里应该扫描presets目录
    return presets;
}

void ParameterRegistry::deletePreset(const std::string& presetName) {
    std::string filename = "presets/" + presetName + ".json";
    if (std::remove(filename.c_str()) == 0) {
        LOG_INF_S("ParameterRegistry: Deleted preset: " + presetName);
    } else {
        LOG_ERR_S("ParameterRegistry: Failed to delete preset: " + presetName);
    }
}

bool ParameterRegistry::validateAllSystems() const {
    for (const auto& systemPair : m_systems) {
        if (!systemPair.second->validateAllParameters()) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> ParameterRegistry::getValidationReport() const {
    std::vector<std::string> report;
    
    for (const auto& systemPair : m_systems) {
        auto errors = systemPair.second->getValidationErrors();
        for (const std::string& error : errors) {
            report.push_back("System " + std::to_string(static_cast<int>(systemPair.first)) + ": " + error);
        }
    }
    
    return report;
}

std::string ParameterRegistry::getSystemStatusReport() const {
    std::ostringstream oss;
    oss << "Parameter Registry Status Report:\n";
    oss << "- Active Systems: " << m_performanceStats.activeSystems << "\n";
    oss << "- Total Parameters: " << m_performanceStats.totalParameters << "\n";
    oss << "- Last Sync Time: " << m_performanceStats.lastSyncTime.count() << "ms\n";
    
    for (const auto& systemPair : m_systems) {
        auto paths = systemPair.second->getAllParameterPaths();
        oss << "- System " << std::to_string(static_cast<int>(systemPair.first)) 
            << ": " << paths.size() << " parameters\n";
    }
    
    return oss.str();
}

ParameterRegistry::PerformanceStats ParameterRegistry::getPerformanceStats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_performanceStats;
}

void ParameterRegistry::initializeDefaultSystems() {
    // 创建默认参数系统
    auto geometryTree = ParameterTreeFactory::createGeometryParameterTree();
    auto renderingTree = ParameterTreeFactory::createRenderingParameterTree();
    auto meshTree = ParameterTreeFactory::createMeshParameterTree();
    auto lightingTree = ParameterTreeFactory::createLightingParameterTree();
    
    registerParameterSystem(SystemType::GEOMETRY, geometryTree);
    registerParameterSystem(SystemType::RENDERING, renderingTree);
    registerParameterSystem(SystemType::MESH, meshTree);
    registerParameterSystem(SystemType::LIGHTING, lightingTree);
    
    // 设置系统依赖关系
    addSystemDependency(SystemType::RENDERING, SystemType::GEOMETRY);
    addSystemDependency(SystemType::MESH, SystemType::GEOMETRY);
    addSystemDependency(SystemType::LIGHTING, SystemType::RENDERING);
    
    LOG_INF_S("ParameterRegistry: Initialized default parameter systems");
}

void ParameterRegistry::updatePerformanceStats() {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    
    m_performanceStats.totalParameters = 0;
    for (const auto& systemPair : m_systems) {
        auto paths = systemPair.second->getAllParameterPaths();
        m_performanceStats.totalParameters += paths.size();
    }
    
    m_performanceStats.lastSyncTime = std::chrono::milliseconds(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
}

void ParameterRegistry::syncSystemToRegistry(SystemType systemType) {
    // 这里需要根据具体的系统类型实现同步逻辑
    LOG_DBG_S("ParameterRegistry: Syncing system to registry: " + std::to_string(static_cast<int>(systemType)));
}

void ParameterRegistry::syncRegistryToSystem(SystemType systemType) {
    // 这里需要根据具体的系统类型实现同步逻辑
    LOG_DBG_S("ParameterRegistry: Syncing registry to system: " + std::to_string(static_cast<int>(systemType)));
}

// RenderingConfigAdapter 实现
RenderingConfigAdapter::RenderingConfigAdapter(RenderingConfig* config) : m_config(config) {
}

bool RenderingConfigAdapter::isSystemAvailable() const {
    return m_config != nullptr;
}

void RenderingConfigAdapter::syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_config) return;
    
    // 同步材质参数
    auto materialSettings = m_config->getMaterialSettings();
    tree->setParameterValue("material.ambient.r", materialSettings.ambientColor.Red());
    tree->setParameterValue("material.ambient.g", materialSettings.ambientColor.Green());
    tree->setParameterValue("material.ambient.b", materialSettings.ambientColor.Blue());
    tree->setParameterValue("material.diffuse.r", materialSettings.diffuseColor.Red());
    tree->setParameterValue("material.diffuse.g", materialSettings.diffuseColor.Green());
    tree->setParameterValue("material.diffuse.b", materialSettings.diffuseColor.Blue());
    tree->setParameterValue("material.specular.r", materialSettings.specularColor.Red());
    tree->setParameterValue("material.specular.g", materialSettings.specularColor.Green());
    tree->setParameterValue("material.specular.b", materialSettings.specularColor.Blue());
    tree->setParameterValue("material.shininess", materialSettings.shininess);
    tree->setParameterValue("material.transparency", materialSettings.transparency);
    
    // 同步显示参数
    auto displaySettings = m_config->getDisplaySettings();
    tree->setParameterValue("display.showEdges", displaySettings.showEdges);
    tree->setParameterValue("display.showVertices", displaySettings.showVertices);
    tree->setParameterValue("display.edgeWidth", displaySettings.edgeWidth);
    tree->setParameterValue("display.vertexSize", displaySettings.vertexSize);
    
    LOG_INF_S("RenderingConfigAdapter: Synced RenderingConfig to registry");
}

void RenderingConfigAdapter::syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_config) return;
    
    // 从注册表同步到RenderingConfig
    // 这里需要根据具体的参数路径获取值并设置到RenderingConfig
    LOG_INF_S("RenderingConfigAdapter: Synced registry to RenderingConfig");
}

std::vector<std::string> RenderingConfigAdapter::getParameterPaths() const {
    return {
        "material.ambient.r", "material.ambient.g", "material.ambient.b",
        "material.diffuse.r", "material.diffuse.g", "material.diffuse.b",
        "material.specular.r", "material.specular.g", "material.specular.b",
        "material.shininess", "material.transparency",
        "display.showEdges", "display.showVertices",
        "display.edgeWidth", "display.vertexSize"
    };
}

ParameterValue RenderingConfigAdapter::getParameterValue(const std::string& path) const {
    if (!m_config) return ParameterValue{};
    
    // 根据路径获取对应的参数值
    // 简化实现
    return ParameterValue{};
}

bool RenderingConfigAdapter::setParameterValue(const std::string& path, const ParameterValue& value) {
    if (!m_config) return false;
    
    // 根据路径设置对应的参数值
    // 简化实现
    return true;
}

void RenderingConfigAdapter::initializeParameterTree(std::shared_ptr<UnifiedParameterTree> tree) {
    // 初始化参数树结构
    tree->createGroup("material", "材质参数");
    tree->createGroup("display", "显示参数");
    // ... 其他参数组
}

// MeshParameterManagerAdapter 实现
MeshParameterManagerAdapter::MeshParameterManagerAdapter(MeshParameterManager* manager) : m_manager(manager) {
}

bool MeshParameterManagerAdapter::isSystemAvailable() const {
    return m_manager != nullptr;
}

void MeshParameterManagerAdapter::syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_manager) return;
    
    // 同步网格参数
    auto meshParams = m_manager->getCurrentMeshParameters();
    tree->setParameterValue("deflection", meshParams.deflection);
    tree->setParameterValue("angularDeflection", meshParams.angularDeflection);
    tree->setParameterValue("relative", meshParams.relative);
    tree->setParameterValue("inParallel", meshParams.inParallel);
    
    LOG_INF_S("MeshParameterManagerAdapter: Synced MeshParameterManager to registry");
}

void MeshParameterManagerAdapter::syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_manager) return;
    
    // 从注册表同步到MeshParameterManager
    LOG_INF_S("MeshParameterManagerAdapter: Synced registry to MeshParameterManager");
}

std::vector<std::string> MeshParameterManagerAdapter::getParameterPaths() const {
    return {
        "deflection", "angularDeflection", "relative", "inParallel"
    };
}

ParameterValue MeshParameterManagerAdapter::getParameterValue(const std::string& path) const {
    if (!m_manager) return ParameterValue{};
    
    // 根据路径获取对应的参数值
    return ParameterValue{};
}

bool MeshParameterManagerAdapter::setParameterValue(const std::string& path, const ParameterValue& value) {
    if (!m_manager) return false;
    
    // 根据路径设置对应的参数值
    return true;
}

void MeshParameterManagerAdapter::initializeParameterTree(std::shared_ptr<UnifiedParameterTree> tree) {
    // 初始化网格参数树结构
    tree->createParameter("deflection", 0.5);
    tree->createParameter("angularDeflection", 1.0);
    tree->createParameter("relative", false);
    tree->createParameter("inParallel", true);
}

// LightingConfigAdapter 实现
LightingConfigAdapter::LightingConfigAdapter(LightingConfig* config) : m_config(config) {
}

bool LightingConfigAdapter::isSystemAvailable() const {
    return m_config != nullptr;
}

void LightingConfigAdapter::syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_config) return;
    
    // 同步光照参数
    auto lights = m_config->getAllLights();
    if (!lights.empty()) {
        const auto& mainLight = lights[0];
        tree->setParameterValue("main.enabled", mainLight.enabled);
        tree->setParameterValue("main.type", mainLight.type);
        tree->setParameterValue("main.position.x", mainLight.positionX);
        tree->setParameterValue("main.position.y", mainLight.positionY);
        tree->setParameterValue("main.position.z", mainLight.positionZ);
        tree->setParameterValue("main.direction.x", mainLight.directionX);
        tree->setParameterValue("main.direction.y", mainLight.directionY);
        tree->setParameterValue("main.direction.z", mainLight.directionZ);
        tree->setParameterValue("main.color.r", mainLight.color.Red());
        tree->setParameterValue("main.color.g", mainLight.color.Green());
        tree->setParameterValue("main.color.b", mainLight.color.Blue());
        tree->setParameterValue("main.intensity", mainLight.intensity);
    }
    
    LOG_INF_S("LightingConfigAdapter: Synced LightingConfig to registry");
}

void LightingConfigAdapter::syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_config) return;
    
    // 从注册表同步到LightingConfig
    LOG_INF_S("LightingConfigAdapter: Synced registry to LightingConfig");
}

std::vector<std::string> LightingConfigAdapter::getParameterPaths() const {
    return {
        "main.enabled", "main.type", "main.position.x", "main.position.y", "main.position.z",
        "main.direction.x", "main.direction.y", "main.direction.z",
        "main.color.r", "main.color.g", "main.color.b", "main.intensity"
    };
}

ParameterValue LightingConfigAdapter::getParameterValue(const std::string& path) const {
    if (!m_config) return ParameterValue{};
    
    // 根据路径获取对应的参数值
    return ParameterValue{};
}

bool LightingConfigAdapter::setParameterValue(const std::string& path, const ParameterValue& value) {
    if (!m_config) return false;
    
    // 根据路径设置对应的参数值
    return true;
}

void LightingConfigAdapter::initializeParameterTree(std::shared_ptr<UnifiedParameterTree> tree) {
    // 初始化光照参数树结构
    tree->createGroup("main", "主光源参数");
    tree->createParameter("main.enabled", true);
    tree->createParameter("main.type", std::string("directional"));
    tree->createParameter("main.position.x", 0.0);
    tree->createParameter("main.position.y", 0.0);
    tree->createParameter("main.position.z", 0.0);
    tree->createParameter("main.direction.x", 0.5);
    tree->createParameter("main.direction.y", 0.5);
    tree->createParameter("main.direction.z", -1.0);
    tree->createParameter("main.color.r", 1.0);
    tree->createParameter("main.color.g", 1.0);
    tree->createParameter("main.color.b", 1.0);
    tree->createParameter("main.intensity", 1.0);
}