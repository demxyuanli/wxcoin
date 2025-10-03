#pragma once

#include "ParameterTree.h"
#include "ParameterUpdateManager.h"
#include <memory>
#include <map>
#include <functional>
#include <mutex>

// 前向声明
class OCCGeometry;
class RenderingConfig;

/**
 * @brief Parameter synchronizer - responsible for synchronizing parameters between parameter tree and existing systems
 */
class ParameterSynchronizer {
public:
    static ParameterSynchronizer& getInstance();
    
    // Geometry object synchronization
    void synchronizeGeometry(std::shared_ptr<OCCGeometry> geometry);
    void unsynchronizeGeometry(std::shared_ptr<OCCGeometry> geometry);
    
    // Rendering configuration synchronization
    void synchronizeRenderingConfig(RenderingConfig* config);
    void unsynchronizeRenderingConfig(RenderingConfig* config);
    
    // Parameter synchronization control
    void enableSynchronization(bool enable) { m_synchronizationEnabled = enable; }
    bool isSynchronizationEnabled() const { return m_synchronizationEnabled; }
    
    // Synchronization direction control
    void setSyncDirection(const std::string& parameterPath, bool treeToSystem, bool systemToTree);
    void setDefaultSyncDirection(bool treeToSystem, bool systemToTree);
    
    // Batch synchronization
    void beginBatchSync();
    void endBatchSync();
    bool isInBatchSync() const { return m_inBatchSync; }
    
    // Synchronization status query
    bool isParameterSynchronized(const std::string& parameterPath) const;
    std::vector<std::string> getSynchronizedParameters() const;
    
private:
    ParameterSynchronizer();
    ~ParameterSynchronizer() = default;
    ParameterSynchronizer(const ParameterSynchronizer&) = delete;
    ParameterSynchronizer& operator=(const ParameterSynchronizer&) = delete;
    
    // Internal methods
    void setupGeometrySynchronization(std::shared_ptr<OCCGeometry> geometry);
    void setupRenderingConfigSynchronization(RenderingConfig* config);
    void onParameterTreeChanged(const std::string& path, const ParameterValue& value);
    void onSystemParameterChanged(const std::string& path, const ParameterValue& value);
    
    // Parameter mapping
    std::map<std::string, std::string> m_parameterToGeometryProperty;
    std::map<std::string, std::string> m_parameterToConfigProperty;
    
    // Synchronization state
    std::atomic<bool> m_synchronizationEnabled;
    std::atomic<bool> m_inBatchSync;
    bool m_defaultTreeToSystem;
    bool m_defaultSystemToTree;
    
    // Synchronized objects
    std::map<std::shared_ptr<OCCGeometry>, std::vector<std::string>> m_synchronizedGeometries;
    std::map<RenderingConfig*, std::vector<std::string>> m_synchronizedConfigs;
    
    // Callback management
    std::vector<ParameterChangedCallback> m_treeCallbacks;
    std::vector<ParameterChangedCallback> m_systemCallbacks;
    
    mutable std::mutex m_syncMutex;
};

/**
 * @brief Geometry parameter synchronizer
 */
class GeometryParameterSynchronizer {
public:
    explicit GeometryParameterSynchronizer(std::shared_ptr<OCCGeometry> geometry);
    ~GeometryParameterSynchronizer();
    
    // Parameter synchronization
    void syncFromTree();
    void syncToTree();
    void setupParameterMappings();
    
    // Parameter update callbacks
    void onTreeParameterChanged(const std::string& path, const ParameterValue& value);
    void onGeometryPropertyChanged(const std::string& property, const ParameterValue& value);
    
private:
    std::shared_ptr<OCCGeometry> m_geometry;
    std::map<std::string, std::function<void(const ParameterValue&)>> m_propertySetters;
    std::map<std::string, std::function<ParameterValue()>> m_propertyGetters;
    
    void initializePropertyMappings();
    void setupTreeCallbacks();
    void setupGeometryCallbacks();
};

/**
 * @brief Rendering configuration parameter synchronizer
 */
class RenderingConfigParameterSynchronizer {
public:
    explicit RenderingConfigParameterSynchronizer(RenderingConfig* config);
    ~RenderingConfigParameterSynchronizer();
    
    // Parameter synchronization
    void syncFromTree();
    void syncToTree();
    void setupParameterMappings();
    
    // Parameter update callbacks
    void onTreeParameterChanged(const std::string& path, const ParameterValue& value);
    void onConfigPropertyChanged(const std::string& property, const ParameterValue& value);
    
private:
    RenderingConfig* m_config;
    std::map<std::string, std::function<void(const ParameterValue&)>> m_propertySetters;
    std::map<std::string, std::function<ParameterValue()>> m_propertyGetters;
    
    void initializePropertyMappings();
    void setupTreeCallbacks();
    void setupConfigCallbacks();
};

/**
 * @brief Parameter synchronizer initializer
 */
class ParameterSynchronizerInitializer {
public:
    static void initialize();
    static void initializeParameterMappings();
    static void initializeDefaultSynchronizations();
};