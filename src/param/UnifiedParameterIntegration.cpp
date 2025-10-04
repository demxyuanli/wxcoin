#include "param/UnifiedParameterIntegration.h"
#include "config/RenderingConfig.h"
#include "MeshParameterManager.h"
#include "config/LightingConfig.h"
#include "SceneManager.h"
#include "RenderingEngine.h"
#include "OCCViewer.h"
#include "logger/Logger.h"
#include <algorithm>
#include <chrono>

// UnifiedParameterIntegration 实现
UnifiedParameterIntegration& UnifiedParameterIntegration::getInstance() {
    static UnifiedParameterIntegration instance;
    return instance;
}

UnifiedParameterIntegration::UnifiedParameterIntegration()
    : m_nextCallbackId(1)
    , m_syncRunning(false) {
    
    LOG_INF_S("UnifiedParameterIntegration: Created integration manager");
}

UnifiedParameterIntegration::~UnifiedParameterIntegration() {
    shutdown();
}

bool UnifiedParameterIntegration::initialize(const IntegrationConfig& config) {
    m_config = config;
    
    // 初始化参数注册表和更新协调器
    auto& registry = ParameterRegistry::getInstance();
    auto& coordinator = UpdateCoordinator::getInstance();
    
    if (!coordinator.initialize()) {
        LOG_ERR_S("UnifiedParameterIntegration: Failed to initialize update coordinator");
        return false;
    }
    
    // 初始化默认集成
    initializeDefaultIntegration();
    
    // 启动同步线程
    if (m_config.autoSyncEnabled) {
        m_syncRunning = true;
        m_syncThread = std::thread(&UnifiedParameterIntegration::syncThreadFunction, this);
    }
    
    LOG_INF_S("UnifiedParameterIntegration: Initialized successfully");
    return true;
}

void UnifiedParameterIntegration::shutdown() {
    if (m_syncRunning) {
        m_syncRunning = false;
        m_syncCondition.notify_all();
        
        if (m_syncThread.joinable()) {
            m_syncThread.join();
        }
    }
    
    auto& coordinator = UpdateCoordinator::getInstance();
    coordinator.shutdown();
    
    LOG_INF_S("UnifiedParameterIntegration: Shutdown completed");
}

bool UnifiedParameterIntegration::integrateRenderingConfig(RenderingConfig* config) {
    if (!config) {
        LOG_ERR_S("UnifiedParameterIntegration: RenderingConfig is null");
        return false;
    }
    
    auto& registry = ParameterRegistry::getInstance();
    registry.integrateRenderingConfig(config);
    
    // 创建桥接器
    auto bridge = std::make_unique<RenderingSystemBridge>(config);
    
    // 同步参数
    if (m_config.bidirectionalSync) {
        auto tree = registry.getParameterSystem(ParameterRegistry::SystemType::RENDERING);
        if (tree) {
            bridge->syncToRegistry(tree);
        }
    }
    
    updateIntegrationStatus(ParameterRegistry::SystemType::RENDERING, IntegrationStatus::INTEGRATED);
    notifyIntegrationEvent("system_integrated", "RenderingConfig");
    
    LOG_INF_S("UnifiedParameterIntegration: Integrated RenderingConfig");
    return true;
}

bool UnifiedParameterIntegration::integrateMeshParameterManager(MeshParameterManager* manager) {
    if (!manager) {
        LOG_ERR_S("UnifiedParameterIntegration: MeshParameterManager is null");
        return false;
    }
    
    auto& registry = ParameterRegistry::getInstance();
    registry.integrateMeshParameterManager(manager);
    
    // 创建桥接器
    auto bridge = std::make_unique<MeshSystemBridge>(manager);
    
    // 同步参数
    if (m_config.bidirectionalSync) {
        auto tree = registry.getParameterSystem(ParameterRegistry::SystemType::MESH);
        if (tree) {
            bridge->syncToRegistry(tree);
        }
    }
    
    updateIntegrationStatus(ParameterRegistry::SystemType::MESH, IntegrationStatus::INTEGRATED);
    notifyIntegrationEvent("system_integrated", "MeshParameterManager");
    
    LOG_INF_S("UnifiedParameterIntegration: Integrated MeshParameterManager");
    return true;
}

bool UnifiedParameterIntegration::integrateLightingConfig(LightingConfig* config) {
    if (!config) {
        LOG_ERR_S("UnifiedParameterIntegration: LightingConfig is null");
        return false;
    }
    
    auto& registry = ParameterRegistry::getInstance();
    registry.integrateLightingConfig(config);
    
    // 创建桥接器
    auto bridge = std::make_unique<LightingSystemBridge>(config);
    
    // 同步参数
    if (m_config.bidirectionalSync) {
        auto tree = registry.getParameterSystem(ParameterRegistry::SystemType::LIGHTING);
        if (tree) {
            bridge->syncToRegistry(tree);
        }
    }
    
    updateIntegrationStatus(ParameterRegistry::SystemType::LIGHTING, IntegrationStatus::INTEGRATED);
    notifyIntegrationEvent("system_integrated", "LightingConfig");
    
    LOG_INF_S("UnifiedParameterIntegration: Integrated LightingConfig");
    return true;
}

bool UnifiedParameterIntegration::integrateSceneManager(SceneManager* sceneManager) {
    if (!sceneManager) {
        LOG_ERR_S("UnifiedParameterIntegration: SceneManager is null");
        return false;
    }
    
    auto& coordinator = UpdateCoordinator::getInstance();
    coordinator.setSceneManager(sceneManager);
    
    LOG_INF_S("UnifiedParameterIntegration: Integrated SceneManager");
    return true;
}

bool UnifiedParameterIntegration::integrateRenderingEngine(RenderingEngine* engine) {
    if (!engine) {
        LOG_ERR_S("UnifiedParameterIntegration: RenderingEngine is null");
        return false;
    }
    
    auto& coordinator = UpdateCoordinator::getInstance();
    coordinator.setRenderingEngine(engine);
    
    LOG_INF_S("UnifiedParameterIntegration: Integrated RenderingEngine");
    return true;
}

bool UnifiedParameterIntegration::integrateOCCViewer(OCCViewer* viewer) {
    if (!viewer) {
        LOG_ERR_S("UnifiedParameterIntegration: OCCViewer is null");
        return false;
    }
    
    auto& coordinator = UpdateCoordinator::getInstance();
    coordinator.setOCCViewer(viewer);
    
    LOG_INF_S("UnifiedParameterIntegration: Integrated OCCViewer");
    return true;
}

UnifiedParameterIntegration::IntegrationStatus UnifiedParameterIntegration::getIntegrationStatus(ParameterRegistry::SystemType systemType) const {
    auto it = m_integrationStatus.find(systemType);
    return (it != m_integrationStatus.end()) ? it->second : IntegrationStatus::NOT_INTEGRATED;
}

bool UnifiedParameterIntegration::isSystemIntegrated(ParameterRegistry::SystemType systemType) const {
    return getIntegrationStatus(systemType) == IntegrationStatus::INTEGRATED;
}

std::vector<ParameterRegistry::SystemType> UnifiedParameterIntegration::getIntegratedSystems() const {
    std::vector<ParameterRegistry::SystemType> integrated;
    
    for (const auto& pair : m_integrationStatus) {
        if (pair.second == IntegrationStatus::INTEGRATED) {
            integrated.push_back(pair.first);
        }
    }
    
    return integrated;
}

void UnifiedParameterIntegration::enableAutoSync(bool enabled) {
    m_config.autoSyncEnabled = enabled;
    
    if (enabled && !m_syncRunning) {
        m_syncRunning = true;
        m_syncThread = std::thread(&UnifiedParameterIntegration::syncThreadFunction, this);
    } else if (!enabled && m_syncRunning) {
        m_syncRunning = false;
        m_syncCondition.notify_all();
        if (m_syncThread.joinable()) {
            m_syncThread.join();
        }
    }
    
    LOG_INF_S("UnifiedParameterIntegration: Auto sync " + std::string(enabled ? "enabled" : "disabled"));
}

void UnifiedParameterIntegration::setSyncInterval(std::chrono::milliseconds interval) {
    m_config.syncInterval = interval;
    LOG_DBG_S("UnifiedParameterIntegration: Sync interval set to " + std::to_string(interval.count()) + "ms");
}

void UnifiedParameterIntegration::performManualSync() {
    LOG_INF_S("UnifiedParameterIntegration: Performing manual sync");
    
    auto integratedSystems = getIntegratedSystems();
    for (auto systemType : integratedSystems) {
        performSystemSync(systemType);
    }
}

void UnifiedParameterIntegration::syncFromExistingSystems() {
    LOG_INF_S("UnifiedParameterIntegration: Syncing from existing systems");
    
    auto& registry = ParameterRegistry::getInstance();
    registry.syncFromExistingSystems();
}

void UnifiedParameterIntegration::syncToExistingSystems() {
    LOG_INF_S("UnifiedParameterIntegration: Syncing to existing systems");
    
    auto& registry = ParameterRegistry::getInstance();
    registry.syncToExistingSystems();
}

bool UnifiedParameterIntegration::setParameter(const std::string& fullPath, const ParameterValue& value) {
    auto& registry = ParameterRegistry::getInstance();
    return registry.setParameterByFullPath(fullPath, value);
}

ParameterValue UnifiedParameterIntegration::getParameter(const std::string& fullPath) const {
    auto& registry = ParameterRegistry::getInstance();
    return registry.getParameterByFullPath(fullPath);
}

bool UnifiedParameterIntegration::hasParameter(const std::string& fullPath) const {
    auto& registry = ParameterRegistry::getInstance();
    return registry.hasParameterByFullPath(fullPath);
}

bool UnifiedParameterIntegration::setParameters(const std::unordered_map<std::string, ParameterValue>& parameters) {
    auto& registry = ParameterRegistry::getInstance();
    
    bool allSuccess = true;
    for (const auto& pair : parameters) {
        if (!registry.setParameterByFullPath(pair.first, pair.second)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

std::unordered_map<std::string, ParameterValue> UnifiedParameterIntegration::getParameters(const std::vector<std::string>& paths) const {
    std::unordered_map<std::string, ParameterValue> result;
    
    for (const std::string& path : paths) {
        result[path] = getParameter(path);
    }
    
    return result;
}

std::string UnifiedParameterIntegration::scheduleParameterChange(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue) {
    auto& coordinator = UpdateCoordinator::getInstance();
    return coordinator.submitParameterChange(path, oldValue, newValue);
}

std::string UnifiedParameterIntegration::scheduleGeometryRebuild(const std::string& geometryPath) {
    auto& coordinator = UpdateCoordinator::getInstance();
    coordinator.scheduleGeometryRebuild(geometryPath);
    return "geometry_rebuild_" + geometryPath;
}

std::string UnifiedParameterIntegration::scheduleRenderingUpdate(const std::string& target) {
    auto& coordinator = UpdateCoordinator::getInstance();
    coordinator.scheduleRenderingUpdate(target);
    return "rendering_update_" + target;
}

std::string UnifiedParameterIntegration::scheduleLightingUpdate() {
    auto& coordinator = UpdateCoordinator::getInstance();
    coordinator.scheduleLightingUpdate();
    return "lighting_update";
}

void UnifiedParameterIntegration::saveCurrentStateAsPreset(const std::string& presetName) {
    auto& registry = ParameterRegistry::getInstance();
    registry.savePreset(presetName);
    LOG_INF_S("UnifiedParameterIntegration: Saved current state as preset: " + presetName);
}

void UnifiedParameterIntegration::loadPreset(const std::string& presetName) {
    auto& registry = ParameterRegistry::getInstance();
    registry.loadPreset(presetName);
    LOG_INF_S("UnifiedParameterIntegration: Loaded preset: " + presetName);
}

std::vector<std::string> UnifiedParameterIntegration::getAvailablePresets() const {
    auto& registry = ParameterRegistry::getInstance();
    return registry.getAvailablePresets();
}

void UnifiedParameterIntegration::deletePreset(const std::string& presetName) {
    auto& registry = ParameterRegistry::getInstance();
    registry.deletePreset(presetName);
    LOG_INF_S("UnifiedParameterIntegration: Deleted preset: " + presetName);
}

void UnifiedParameterIntegration::addParameterDependency(const std::string& paramPath, const std::string& dependencyPath) {
    auto& registry = ParameterRegistry::getInstance();
    auto [systemType, path] = registry.parseFullPath(paramPath);
    auto system = registry.getParameterSystem(systemType);
    if (system) {
        system->addDependency(path, dependencyPath);
    }
}

void UnifiedParameterIntegration::removeParameterDependency(const std::string& paramPath, const std::string& dependencyPath) {
    auto& registry = ParameterRegistry::getInstance();
    auto [systemType, path] = registry.parseFullPath(paramPath);
    auto system = registry.getParameterSystem(systemType);
    if (system) {
        system->removeDependency(path, dependencyPath);
    }
}

std::vector<std::string> UnifiedParameterIntegration::getParameterDependencies(const std::string& paramPath) const {
    auto& registry = ParameterRegistry::getInstance();
    auto [systemType, path] = registry.parseFullPath(paramPath);
    auto system = registry.getParameterSystem(systemType);
    if (system) {
        return system->getDependentParameters(path);
    }
    return {};
}

UnifiedParameterIntegration::PerformanceReport UnifiedParameterIntegration::getPerformanceReport() const {
    auto& registry = ParameterRegistry::getInstance();
    auto& coordinator = UpdateCoordinator::getInstance();
    
    auto registryStats = registry.getPerformanceStats();
    auto coordinatorMetrics = coordinator.getPerformanceMetrics();
    
    PerformanceReport report;
    report.totalParameters = registryStats.totalParameters;
    report.activeSystems = registryStats.activeSystems;
    report.pendingUpdates = coordinatorMetrics.totalTasksSubmitted - coordinatorMetrics.totalTasksExecuted;
    report.executedUpdates = coordinatorMetrics.totalTasksExecuted;
    report.averageUpdateTime = coordinatorMetrics.averageExecutionTime;
    report.batchGroupsCreated = coordinatorMetrics.totalBatchGroups;
    report.dependencyConflicts = coordinatorMetrics.dependencyConflicts;
    
    return report;
}

void UnifiedParameterIntegration::resetPerformanceMetrics() {
    auto& coordinator = UpdateCoordinator::getInstance();
    coordinator.resetPerformanceMetrics();
    LOG_INF_S("UnifiedParameterIntegration: Performance metrics reset");
}

bool UnifiedParameterIntegration::validateAllParameters() const {
    auto& registry = ParameterRegistry::getInstance();
    return registry.validateAllSystems();
}

std::vector<std::string> UnifiedParameterIntegration::getValidationErrors() const {
    auto& registry = ParameterRegistry::getInstance();
    return registry.getValidationReport();
}

std::string UnifiedParameterIntegration::getSystemDiagnostics() const {
    auto& registry = ParameterRegistry::getInstance();
    auto report = getPerformanceReport();
    
    std::ostringstream oss;
    oss << "Unified Parameter Integration Diagnostics:\n";
    oss << "- Total Parameters: " << report.totalParameters << "\n";
    oss << "- Active Systems: " << report.activeSystems << "\n";
    oss << "- Pending Updates: " << report.pendingUpdates << "\n";
    oss << "- Executed Updates: " << report.executedUpdates << "\n";
    oss << "- Average Update Time: " << report.averageUpdateTime.count() << "ms\n";
    oss << "- Batch Groups Created: " << report.batchGroupsCreated << "\n";
    oss << "- Dependency Conflicts: " << report.dependencyConflicts << "\n";
    
    auto integratedSystems = getIntegratedSystems();
    oss << "- Integrated Systems: ";
    for (size_t i = 0; i < integratedSystems.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << std::to_string(static_cast<int>(integratedSystems[i]));
    }
    oss << "\n";
    
    return oss.str();
}

int UnifiedParameterIntegration::registerIntegrationEventCallback(IntegrationEventCallback callback) {
    int id = m_nextCallbackId++;
    m_eventCallbacks[id] = callback;
    LOG_DBG_S("UnifiedParameterIntegration: Registered integration event callback with ID " + std::to_string(id));
    return id;
}

void UnifiedParameterIntegration::unregisterIntegrationEventCallback(int callbackId) {
    auto it = m_eventCallbacks.find(callbackId);
    if (it != m_eventCallbacks.end()) {
        m_eventCallbacks.erase(it);
        LOG_DBG_S("UnifiedParameterIntegration: Unregistered integration event callback with ID " + std::to_string(callbackId));
    }
}

void UnifiedParameterIntegration::setIntegrationConfig(const IntegrationConfig& config) {
    m_config = config;
    LOG_INF_S("UnifiedParameterIntegration: Integration config updated");
}

UnifiedParameterIntegration::IntegrationConfig UnifiedParameterIntegration::getIntegrationConfig() const {
    return m_config;
}

// 私有方法实现
void UnifiedParameterIntegration::initializeDefaultIntegration() {
    // 初始化默认集成状态
    m_integrationStatus[ParameterRegistry::SystemType::GEOMETRY] = IntegrationStatus::INTEGRATED;
    m_integrationStatus[ParameterRegistry::SystemType::RENDERING] = IntegrationStatus::NOT_INTEGRATED;
    m_integrationStatus[ParameterRegistry::SystemType::MESH] = IntegrationStatus::NOT_INTEGRATED;
    m_integrationStatus[ParameterRegistry::SystemType::LIGHTING] = IntegrationStatus::NOT_INTEGRATED;
    m_integrationStatus[ParameterRegistry::SystemType::NAVIGATION] = IntegrationStatus::NOT_INTEGRATED;
    m_integrationStatus[ParameterRegistry::SystemType::DISPLAY] = IntegrationStatus::NOT_INTEGRATED;
    m_integrationStatus[ParameterRegistry::SystemType::PERFORMANCE] = IntegrationStatus::NOT_INTEGRATED;
    
    LOG_INF_S("UnifiedParameterIntegration: Initialized default integration status");
}

void UnifiedParameterIntegration::syncThreadFunction() {
    LOG_INF_S("UnifiedParameterIntegration: Sync thread started");
    
    while (m_syncRunning) {
        std::unique_lock<std::mutex> lock(m_syncMutex);
        
        if (m_syncCondition.wait_for(lock, m_config.syncInterval, [this]() { return !m_syncRunning; })) {
            break;
        }
        
        if (!m_syncRunning) {
            break;
        }
        
        lock.unlock();
        
        // 执行同步
        performManualSync();
    }
    
    LOG_INF_S("UnifiedParameterIntegration: Sync thread stopped");
}

void UnifiedParameterIntegration::performSystemSync(ParameterRegistry::SystemType systemType) {
    auto& registry = ParameterRegistry::getInstance();
    auto system = registry.getParameterSystem(systemType);
    
    if (!system) {
        LOG_WRN_S("UnifiedParameterIntegration: System not found for sync: " + std::to_string(static_cast<int>(systemType)));
        return;
    }
    
    // 这里可以实现具体的同步逻辑
    LOG_DBG_S("UnifiedParameterIntegration: Syncing system " + std::to_string(static_cast<int>(systemType)));
}

void UnifiedParameterIntegration::notifyIntegrationEvent(const std::string& event, const std::string& details) {
    for (const auto& pair : m_eventCallbacks) {
        try {
            pair.second(event, details);
        } catch (const std::exception& e) {
            LOG_ERR_S("UnifiedParameterIntegration: Integration event callback failed: " + std::string(e.what()));
        }
    }
}

void UnifiedParameterIntegration::updateIntegrationStatus(ParameterRegistry::SystemType systemType, IntegrationStatus status) {
    m_integrationStatus[systemType] = status;
    LOG_DBG_S("UnifiedParameterIntegration: Updated integration status for system " + 
              std::to_string(static_cast<int>(systemType)) + " to " + 
              std::to_string(static_cast<int>(status)));
}

// RenderingSystemBridge 实现
RenderingSystemBridge::RenderingSystemBridge(RenderingConfig* config) : m_config(config) {
    initializeParameterMapping();
}

bool RenderingSystemBridge::isSystemAvailable() const {
    return m_config != nullptr;
}

void RenderingSystemBridge::syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
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
    
    LOG_INF_S("RenderingSystemBridge: Synced RenderingConfig to registry");
}

void RenderingSystemBridge::syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_config) return;
    
    // 从注册表同步到RenderingConfig
    // 这里需要根据具体的参数路径获取值并设置到RenderingConfig
    LOG_INF_S("RenderingSystemBridge: Synced registry to RenderingConfig");
}

std::vector<std::string> RenderingSystemBridge::getParameterPaths() const {
    return {
        "material.ambient.r", "material.ambient.g", "material.ambient.b",
        "material.diffuse.r", "material.diffuse.g", "material.diffuse.b",
        "material.specular.r", "material.specular.g", "material.specular.b",
        "material.shininess", "material.transparency"
    };
}

ParameterValue RenderingSystemBridge::getParameterValue(const std::string& path) const {
    if (!m_config) return ParameterValue{};
    
    auto it = m_getterMap.find(path);
    if (it != m_getterMap.end()) {
        return it->second();
    }
    
    return ParameterValue{};
}

bool RenderingSystemBridge::setParameterValue(const std::string& path, const ParameterValue& value) {
    if (!m_config) return false;
    
    auto it = m_setterMap.find(path);
    if (it != m_setterMap.end()) {
        try {
            it->second(value);
            return true;
        } catch (const std::exception& e) {
            LOG_ERR_S("RenderingSystemBridge: Failed to set parameter " + path + ": " + e.what());
            return false;
        }
    }
    
    return false;
}

void RenderingSystemBridge::onParameterChanged(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue) {
    LOG_DBG_S("RenderingSystemBridge: Parameter changed: " + path);
    // 这里可以触发渲染更新
}

void RenderingSystemBridge::initializeParameterMapping() {
    if (!m_config) return;
    
    // 初始化getter映射
    m_getterMap["material.ambient.r"] = [this]() { return m_config->getMaterialSettings().ambientColor.Red(); };
    m_getterMap["material.ambient.g"] = [this]() { return m_config->getMaterialSettings().ambientColor.Green(); };
    m_getterMap["material.ambient.b"] = [this]() { return m_config->getMaterialSettings().ambientColor.Blue(); };
    m_getterMap["material.diffuse.r"] = [this]() { return m_config->getMaterialSettings().diffuseColor.Red(); };
    m_getterMap["material.diffuse.g"] = [this]() { return m_config->getMaterialSettings().diffuseColor.Green(); };
    m_getterMap["material.diffuse.b"] = [this]() { return m_config->getMaterialSettings().diffuseColor.Blue(); };
    m_getterMap["material.specular.r"] = [this]() { return m_config->getMaterialSettings().specularColor.Red(); };
    m_getterMap["material.specular.g"] = [this]() { return m_config->getMaterialSettings().specularColor.Green(); };
    m_getterMap["material.specular.b"] = [this]() { return m_config->getMaterialSettings().specularColor.Blue(); };
    m_getterMap["material.shininess"] = [this]() { return m_config->getMaterialSettings().shininess; };
    m_getterMap["material.transparency"] = [this]() { return m_config->getMaterialSettings().transparency; };
    
    // 初始化setter映射
    m_setterMap["material.ambient.r"] = [this](const ParameterValue& value) {
        if (std::holds_alternative<double>(value)) {
            auto settings = m_config->getMaterialSettings();
            settings.ambientColor.SetRed(std::get<double>(value));
            m_config->setMaterialSettings(settings);
        }
    };
    // ... 其他setter映射
}

// MeshSystemBridge 实现
MeshSystemBridge::MeshSystemBridge(MeshParameterManager* manager) : m_manager(manager) {
    initializeParameterMapping();
}

bool MeshSystemBridge::isSystemAvailable() const {
    return m_manager != nullptr;
}

void MeshSystemBridge::syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_manager) return;
    
    auto meshParams = m_manager->getCurrentMeshParameters();
    tree->setParameterValue("deflection", meshParams.deflection);
    tree->setParameterValue("angularDeflection", meshParams.angularDeflection);
    tree->setParameterValue("relative", meshParams.relative);
    tree->setParameterValue("inParallel", meshParams.inParallel);
    
    LOG_INF_S("MeshSystemBridge: Synced MeshParameterManager to registry");
}

void MeshSystemBridge::syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_manager) return;
    
    // 从注册表同步到MeshParameterManager
    LOG_INF_S("MeshSystemBridge: Synced registry to MeshParameterManager");
}

std::vector<std::string> MeshSystemBridge::getParameterPaths() const {
    return {
        "deflection", "angularDeflection", "relative", "inParallel"
    };
}

ParameterValue MeshSystemBridge::getParameterValue(const std::string& path) const {
    if (!m_manager) return ParameterValue{};
    
    auto it = m_getterMap.find(path);
    if (it != m_getterMap.end()) {
        return it->second();
    }
    
    return ParameterValue{};
}

bool MeshSystemBridge::setParameterValue(const std::string& path, const ParameterValue& value) {
    if (!m_manager) return false;
    
    auto it = m_setterMap.find(path);
    if (it != m_setterMap.end()) {
        try {
            it->second(value);
            return true;
        } catch (const std::exception& e) {
            LOG_ERR_S("MeshSystemBridge: Failed to set parameter " + path + ": " + e.what());
            return false;
        }
    }
    
    return false;
}

void MeshSystemBridge::onParameterChanged(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue) {
    LOG_DBG_S("MeshSystemBridge: Parameter changed: " + path);
    // 这里可以触发网格重建
}

void MeshSystemBridge::initializeParameterMapping() {
    if (!m_manager) return;
    
    // 初始化getter映射
    m_getterMap["deflection"] = [this]() { return m_manager->getCurrentMeshParameters().deflection; };
    m_getterMap["angularDeflection"] = [this]() { return m_manager->getCurrentMeshParameters().angularDeflection; };
    m_getterMap["relative"] = [this]() { return m_manager->getCurrentMeshParameters().relative; };
    m_getterMap["inParallel"] = [this]() { return m_manager->getCurrentMeshParameters().inParallel; };
    
    // 初始化setter映射
    m_setterMap["deflection"] = [this](const ParameterValue& value) {
        if (std::holds_alternative<double>(value)) {
            m_manager->setParameter(MeshParameterManager::Category::BASIC_MESH, "deflection", std::get<double>(value));
        }
    };
    // ... 其他setter映射
}

// LightingSystemBridge 实现
LightingSystemBridge::LightingSystemBridge(LightingConfig* config) : m_config(config) {
    initializeParameterMapping();
}

bool LightingSystemBridge::isSystemAvailable() const {
    return m_config != nullptr;
}

void LightingSystemBridge::syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_config) return;
    
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
    
    LOG_INF_S("LightingSystemBridge: Synced LightingConfig to registry");
}

void LightingSystemBridge::syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) {
    if (!m_config) return;
    
    // 从注册表同步到LightingConfig
    LOG_INF_S("LightingSystemBridge: Synced registry to LightingConfig");
}

std::vector<std::string> LightingSystemBridge::getParameterPaths() const {
    return {
        "main.enabled", "main.type", "main.position.x", "main.position.y", "main.position.z",
        "main.direction.x", "main.direction.y", "main.direction.z",
        "main.color.r", "main.color.g", "main.color.b", "main.intensity"
    };
}

ParameterValue LightingSystemBridge::getParameterValue(const std::string& path) const {
    if (!m_config) return ParameterValue{};
    
    auto it = m_getterMap.find(path);
    if (it != m_getterMap.end()) {
        return it->second();
    }
    
    return ParameterValue{};
}

bool LightingSystemBridge::setParameterValue(const std::string& path, const ParameterValue& value) {
    if (!m_config) return false;
    
    auto it = m_setterMap.find(path);
    if (it != m_setterMap.end()) {
        try {
            it->second(value);
            return true;
        } catch (const std::exception& e) {
            LOG_ERR_S("LightingSystemBridge: Failed to set parameter " + path + ": " + e.what());
            return false;
        }
    }
    
    return false;
}

void LightingSystemBridge::onParameterChanged(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue) {
    LOG_DBG_S("LightingSystemBridge: Parameter changed: " + path);
    // 这里可以触发光照更新
}

void LightingSystemBridge::initializeParameterMapping() {
    if (!m_config) return;
    
    // 初始化getter映射
    auto lights = m_config->getAllLights();
    if (!lights.empty()) {
        const auto& mainLight = lights[0];
        m_getterMap["main.enabled"] = [mainLight]() { return mainLight.enabled; };
        m_getterMap["main.type"] = [mainLight]() { return std::string(mainLight.type); };
        m_getterMap["main.position.x"] = [mainLight]() { return mainLight.positionX; };
        m_getterMap["main.position.y"] = [mainLight]() { return mainLight.positionY; };
        m_getterMap["main.position.z"] = [mainLight]() { return mainLight.positionZ; };
        m_getterMap["main.direction.x"] = [mainLight]() { return mainLight.directionX; };
        m_getterMap["main.direction.y"] = [mainLight]() { return mainLight.directionY; };
        m_getterMap["main.direction.z"] = [mainLight]() { return mainLight.directionZ; };
        m_getterMap["main.color.r"] = [mainLight]() { return mainLight.color.Red(); };
        m_getterMap["main.color.g"] = [mainLight]() { return mainLight.color.Green(); };
        m_getterMap["main.color.b"] = [mainLight]() { return mainLight.color.Blue(); };
        m_getterMap["main.intensity"] = [mainLight]() { return mainLight.intensity; };
    }
    
    // 初始化setter映射
    // ... setter映射实现
}