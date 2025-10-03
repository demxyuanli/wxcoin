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
 * @brief 参数同步器 - 负责在参数树和现有系统之间同步参数
 */
class ParameterSynchronizer {
public:
    static ParameterSynchronizer& getInstance();
    
    // 几何对象同步
    void synchronizeGeometry(std::shared_ptr<OCCGeometry> geometry);
    void unsynchronizeGeometry(std::shared_ptr<OCCGeometry> geometry);
    
    // 渲染配置同步
    void synchronizeRenderingConfig(RenderingConfig* config);
    void unsynchronizeRenderingConfig(RenderingConfig* config);
    
    // 参数同步控制
    void enableSynchronization(bool enable) { m_synchronizationEnabled = enable; }
    bool isSynchronizationEnabled() const { return m_synchronizationEnabled; }
    
    // 同步方向控制
    void setSyncDirection(const std::string& parameterPath, bool treeToSystem, bool systemToTree);
    void setDefaultSyncDirection(bool treeToSystem, bool systemToTree);
    
    // 批量同步
    void beginBatchSync();
    void endBatchSync();
    bool isInBatchSync() const { return m_inBatchSync; }
    
    // 同步状态查询
    bool isParameterSynchronized(const std::string& parameterPath) const;
    std::vector<std::string> getSynchronizedParameters() const;
    
private:
    ParameterSynchronizer();
    ~ParameterSynchronizer() = default;
    ParameterSynchronizer(const ParameterSynchronizer&) = delete;
    ParameterSynchronizer& operator=(const ParameterSynchronizer&) = delete;
    
    // 内部方法
    void setupGeometrySynchronization(std::shared_ptr<OCCGeometry> geometry);
    void setupRenderingConfigSynchronization(RenderingConfig* config);
    void onParameterTreeChanged(const std::string& path, const ParameterValue& value);
    void onSystemParameterChanged(const std::string& path, const ParameterValue& value);
    
    // 参数映射
    std::map<std::string, std::string> m_parameterToGeometryProperty;
    std::map<std::string, std::string> m_parameterToConfigProperty;
    
    // 同步状态
    std::atomic<bool> m_synchronizationEnabled;
    std::atomic<bool> m_inBatchSync;
    bool m_defaultTreeToSystem;
    bool m_defaultSystemToTree;
    
    // 同步对象
    std::map<std::shared_ptr<OCCGeometry>, std::vector<std::string>> m_synchronizedGeometries;
    std::map<RenderingConfig*, std::vector<std::string>> m_synchronizedConfigs;
    
    // 回调管理
    std::vector<ParameterChangedCallback> m_treeCallbacks;
    std::vector<ParameterChangedCallback> m_systemCallbacks;
    
    mutable std::mutex m_syncMutex;
};

/**
 * @brief 几何参数同步器
 */
class GeometryParameterSynchronizer {
public:
    explicit GeometryParameterSynchronizer(std::shared_ptr<OCCGeometry> geometry);
    ~GeometryParameterSynchronizer();
    
    // 参数同步
    void syncFromTree();
    void syncToTree();
    void setupParameterMappings();
    
    // 参数更新回调
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
 * @brief 渲染配置参数同步器
 */
class RenderingConfigParameterSynchronizer {
public:
    explicit RenderingConfigParameterSynchronizer(RenderingConfig* config);
    ~RenderingConfigParameterSynchronizer();
    
    // 参数同步
    void syncFromTree();
    void syncToTree();
    void setupParameterMappings();
    
    // 参数更新回调
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
 * @brief 参数同步器初始化器
 */
class ParameterSynchronizerInitializer {
public:
    static void initialize();
    static void initializeParameterMappings();
    static void initializeDefaultSynchronizations();
};