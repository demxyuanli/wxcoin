#pragma once

#include "ParameterRegistry.h"
#include "UpdateCoordinator.h"
#include "UnifiedParameterTree.h"
#include <memory>
#include <vector>
#include <functional>

// Forward declarations
class RenderingConfig;
class MeshParameterManager;
class LightingConfig;
class SceneManager;
class RenderingEngine;
class OCCViewer;

/**
 * @brief Unified parameter integration manager
 * Responsible for integrating existing parameter systems with the new unified parameter management system
 */
class UnifiedParameterIntegration {
public:
    // Integration status
    enum class IntegrationStatus {
        NOT_INTEGRATED,     // Not integrated
        INTEGRATING,        // Integrating
        INTEGRATED,         // Integrated
        ERROR               // Integration error
    };

    // Integration configuration
    struct IntegrationConfig {
        bool autoSyncEnabled = true;           // Auto sync
        bool bidirectionalSync = true;        // Bidirectional sync
        std::chrono::milliseconds syncInterval = std::chrono::milliseconds(100); // Sync interval
        bool enableSmartBatching = true;      // Enable smart batching
        bool enableDependencyTracking = true; // Enable dependency tracking
        bool enablePerformanceMonitoring = true; // Enable performance monitoring
    };

    static UnifiedParameterIntegration& getInstance();

    // Initialization
    bool initialize(const IntegrationConfig& config = IntegrationConfig{});
    void shutdown();

    // System integration
    bool integrateRenderingConfig(RenderingConfig* config);
    bool integrateMeshParameterManager(MeshParameterManager* manager);
    bool integrateLightingConfig(LightingConfig* config);
    bool integrateSceneManager(SceneManager* sceneManager);
    bool integrateRenderingEngine(RenderingEngine* engine);
    bool integrateOCCViewer(OCCViewer* viewer);

    // Integration status query
    IntegrationStatus getIntegrationStatus(ParameterRegistry::SystemType systemType) const;
    bool isSystemIntegrated(ParameterRegistry::SystemType systemType) const;
    std::vector<ParameterRegistry::SystemType> getIntegratedSystems() const;

    // Synchronization control
    void enableAutoSync(bool enabled);
    void setSyncInterval(std::chrono::milliseconds interval);
    void performManualSync();
    void syncFromExistingSystems();
    void syncToExistingSystems();

    // Parameter access (unified interface)
    bool setParameter(const std::string& fullPath, const ParameterValue& value);
    ParameterValue getParameter(const std::string& fullPath) const;
    bool hasParameter(const std::string& fullPath) const;

    // Batch parameter operations
    bool setParameters(const std::unordered_map<std::string, ParameterValue>& parameters);
    std::unordered_map<std::string, ParameterValue> getParameters(const std::vector<std::string>& paths) const;

    // Update coordination
    std::string scheduleParameterChange(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue);
    std::string scheduleGeometryRebuild(const std::string& geometryPath);
    std::string scheduleRenderingUpdate(const std::string& target = "");
    std::string scheduleLightingUpdate();

    // Preset management
    void saveCurrentStateAsPreset(const std::string& presetName);
    void loadPreset(const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    void deletePreset(const std::string& presetName);

    // Dependency management
    void addParameterDependency(const std::string& paramPath, const std::string& dependencyPath);
    void removeParameterDependency(const std::string& paramPath, const std::string& dependencyPath);
    std::vector<std::string> getParameterDependencies(const std::string& paramPath) const;

    // Performance monitoring
    struct PerformanceReport {
        size_t totalParameters;
        size_t activeSystems;
        size_t pendingUpdates;
        size_t executedUpdates;
        std::chrono::milliseconds averageUpdateTime;
        size_t batchGroupsCreated;
        size_t dependencyConflicts;
    };
    PerformanceReport getPerformanceReport() const;
    void resetPerformanceMetrics();

    // Validation and diagnostics
    bool validateAllParameters() const;
    std::vector<std::string> getValidationErrors() const;
    std::string getSystemDiagnostics() const;

    // Event callbacks
    using IntegrationEventCallback = std::function<void(const std::string& event, const std::string& details)>;
    int registerIntegrationEventCallback(IntegrationEventCallback callback);
    void unregisterIntegrationEventCallback(int callbackId);

    // Configuration management
    void setIntegrationConfig(const IntegrationConfig& config);
    IntegrationConfig getIntegrationConfig() const;

private:
    UnifiedParameterIntegration();
    ~UnifiedParameterIntegration();
    UnifiedParameterIntegration(const UnifiedParameterIntegration&) = delete;
    UnifiedParameterIntegration& operator=(const UnifiedParameterIntegration&) = delete;

    // Internal data structures
    IntegrationConfig m_config;
    std::unordered_map<ParameterRegistry::SystemType, IntegrationStatus> m_integrationStatus;
    std::unordered_map<int, IntegrationEventCallback> m_eventCallbacks;
    int m_nextCallbackId;

    // Synchronization thread
    std::thread m_syncThread;
    std::atomic<bool> m_syncRunning;
    std::mutex m_syncMutex;
    std::condition_variable m_syncCondition;

    // Internal methods
    void initializeDefaultIntegration();
    void syncThreadFunction();
    void performSystemSync(ParameterRegistry::SystemType systemType);
    void notifyIntegrationEvent(const std::string& event, const std::string& details);
    void updateIntegrationStatus(ParameterRegistry::SystemType systemType, IntegrationStatus status);
};

/**
 * @brief Parameter system bridge
 * Provides parameter bridging functionality for specific systems
 */
class ParameterSystemBridge {
public:
    virtual ~ParameterSystemBridge() = default;
    
    virtual ParameterRegistry::SystemType getSystemType() const = 0;
    virtual std::string getSystemName() const = 0;
    virtual bool isSystemAvailable() const = 0;
    
    virtual void syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) = 0;
    virtual void syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) = 0;
    
    virtual std::vector<std::string> getParameterPaths() const = 0;
    virtual ParameterValue getParameterValue(const std::string& path) const = 0;
    virtual bool setParameterValue(const std::string& path, const ParameterValue& value) = 0;
    
    virtual void onParameterChanged(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue) = 0;
};

/**
 * @brief Rendering system bridge
 */
class RenderingSystemBridge : public ParameterSystemBridge {
public:
    RenderingSystemBridge(RenderingConfig* config);
    ~RenderingSystemBridge() override = default;

    ParameterRegistry::SystemType getSystemType() const override { return ParameterRegistry::SystemType::RENDERING; }
    std::string getSystemName() const override { return "RenderingSystem"; }
    bool isSystemAvailable() const override;

    void syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;
    void syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;

    std::vector<std::string> getParameterPaths() const override;
    ParameterValue getParameterValue(const std::string& path) const override;
    bool setParameterValue(const std::string& path, const ParameterValue& value) override;

    void onParameterChanged(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue) override;

private:
    RenderingConfig* m_config;
    void initializeParameterMapping();
    std::unordered_map<std::string, std::function<ParameterValue()>> m_getterMap;
    std::unordered_map<std::string, std::function<void(const ParameterValue&)>> m_setterMap;
};

/**
 * @brief Mesh system bridge
 */
class MeshSystemBridge : public ParameterSystemBridge {
public:
    MeshSystemBridge(MeshParameterManager* manager);
    ~MeshSystemBridge() override = default;

    ParameterRegistry::SystemType getSystemType() const override { return ParameterRegistry::SystemType::MESH; }
    std::string getSystemName() const override { return "MeshSystem"; }
    bool isSystemAvailable() const override;

    void syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;
    void syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;

    std::vector<std::string> getParameterPaths() const override;
    ParameterValue getParameterValue(const std::string& path) const override;
    bool setParameterValue(const std::string& path, const ParameterValue& value) override;

    void onParameterChanged(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue) override;

private:
    MeshParameterManager* m_manager;
    void initializeParameterMapping();
    std::unordered_map<std::string, std::function<ParameterValue()>> m_getterMap;
    std::unordered_map<std::string, std::function<void(const ParameterValue&)>> m_setterMap;
};

/**
 * @brief 光照系统桥接器
 */
class LightingSystemBridge : public ParameterSystemBridge {
public:
    LightingSystemBridge(LightingConfig* config);
    ~LightingSystemBridge() override = default;

    ParameterRegistry::SystemType getSystemType() const override { return ParameterRegistry::SystemType::LIGHTING; }
    std::string getSystemName() const override { return "LightingSystem"; }
    bool isSystemAvailable() const override;

    void syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;
    void syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;

    std::vector<std::string> getParameterPaths() const override;
    ParameterValue getParameterValue(const std::string& path) const override;
    bool setParameterValue(const std::string& path, const ParameterValue& value) override;

    void onParameterChanged(const std::string& path, const ParameterValue& oldValue, const ParameterValue& newValue) override;

private:
    LightingConfig* m_config;
    void initializeParameterMapping();
    std::unordered_map<std::string, std::function<ParameterValue()>> m_getterMap;
    std::unordered_map<std::string, std::function<void(const ParameterValue&)>> m_setterMap;
};

// 便利宏定义
#define UNIFIED_PARAM_SET(path, value) \
    UnifiedParameterIntegration::getInstance().setParameter(path, value)

#define UNIFIED_PARAM_GET(path) \
    UnifiedParameterIntegration::getInstance().getParameter(path)

#define UNIFIED_PARAM_SCHEDULE_CHANGE(path, oldVal, newVal) \
    UnifiedParameterIntegration::getInstance().scheduleParameterChange(path, oldVal, newVal)

#define UNIFIED_PARAM_SCHEDULE_REBUILD(path) \
    UnifiedParameterIntegration::getInstance().scheduleGeometryRebuild(path)

#define UNIFIED_PARAM_SCHEDULE_RENDER(target) \
    UnifiedParameterIntegration::getInstance().scheduleRenderingUpdate(target)