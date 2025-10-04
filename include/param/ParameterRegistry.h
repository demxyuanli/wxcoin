#pragma once

#include "UnifiedParameterTree.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

// Forward declarations
class UpdateCoordinator;
class RenderingConfig;
class MeshParameterManager;
class LightingConfig;

/**
 * @brief 参数注册表
 * 统一管理所有参数系统，提供参数注册、查找和协调功能
 */
class ParameterRegistry {
public:
    // 参数系统类型
    enum class SystemType {
        GEOMETRY,       // 几何表示参数
        RENDERING,      // 渲染控制参数
        MESH,          // 网格参数
        LIGHTING,      // 光照参数
        NAVIGATION,    // 导航参数
        DISPLAY,       // 显示参数
        PERFORMANCE    // 性能参数
    };

    // 参数变更通知
    struct ParameterSystemChange {
        SystemType systemType;
        std::string parameterPath;
        ParameterValue oldValue;
        ParameterValue newValue;
        std::chrono::steady_clock::time_point timestamp;
    };

    using SystemChangeCallback = std::function<void(const ParameterSystemChange&)>;

    static ParameterRegistry& getInstance();

    // 系统注册
    void registerParameterSystem(SystemType type, std::shared_ptr<UnifiedParameterTree> tree);
    void unregisterParameterSystem(SystemType type);
    std::shared_ptr<UnifiedParameterTree> getParameterSystem(SystemType type) const;

    // 参数访问
    bool setParameter(SystemType systemType, const std::string& path, const ParameterValue& value);
    ParameterValue getParameter(SystemType systemType, const std::string& path) const;
    bool hasParameter(SystemType systemType, const std::string& path) const;

    // 跨系统参数操作
    bool setParameterByFullPath(const std::string& fullPath, const ParameterValue& value);
    ParameterValue getParameterByFullPath(const std::string& fullPath) const;
    bool hasParameterByFullPath(const std::string& fullPath) const;

    // 批量操作
    bool setParametersBySystem(SystemType systemType, const std::unordered_map<std::string, ParameterValue>& values);
    std::unordered_map<std::string, ParameterValue> getAllParametersBySystem(SystemType systemType) const;

    // 参数路径解析
    std::pair<SystemType, std::string> parseFullPath(const std::string& fullPath) const;
    std::string buildFullPath(SystemType systemType, const std::string& path) const;

    // 系统间依赖管理
    void addSystemDependency(SystemType dependentSystem, SystemType dependencySystem);
    void removeSystemDependency(SystemType dependentSystem, SystemType dependencySystem);
    std::vector<SystemType> getDependentSystems(SystemType systemType) const;

    // 变更通知
    int registerSystemChangeCallback(SystemChangeCallback callback);
    void unregisterSystemChangeCallback(int callbackId);
    void notifySystemChange(const ParameterSystemChange& change);

    // 与现有系统的集成
    void integrateRenderingConfig(RenderingConfig* config);
    void integrateMeshParameterManager(MeshParameterManager* manager);
    void integrateLightingConfig(LightingConfig* config);

    // 同步操作
    void syncFromExistingSystems();
    void syncToExistingSystems();

    // 预设管理
    void savePreset(const std::string& presetName);
    void loadPreset(const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    void deletePreset(const std::string& presetName);

    // 验证和诊断
    bool validateAllSystems() const;
    std::vector<std::string> getValidationReport() const;
    std::string getSystemStatusReport() const;

    // 性能监控
    struct PerformanceStats {
        size_t totalParameters;
        size_t activeSystems;
        std::chrono::milliseconds lastSyncTime;
        size_t changeNotificationsSent;
        size_t batchUpdatesPerformed;
    };
    PerformanceStats getPerformanceStats() const;

private:
    ParameterRegistry();
    ~ParameterRegistry();
    ParameterRegistry(const ParameterRegistry&) = delete;
    ParameterRegistry& operator=(const ParameterRegistry&) = delete;

    // 内部数据结构
    std::unordered_map<SystemType, std::shared_ptr<UnifiedParameterTree>> m_systems;
    std::unordered_map<SystemType, std::vector<SystemType>> m_systemDependencies;
    std::unordered_map<int, SystemChangeCallback> m_systemCallbacks;
    int m_nextCallbackId;

    // 现有系统集成
    RenderingConfig* m_renderingConfig;
    MeshParameterManager* m_meshParameterManager;
    LightingConfig* m_lightingConfig;

    // 性能统计
    mutable std::mutex m_statsMutex;
    PerformanceStats m_performanceStats;

    // 内部辅助方法
    void initializeDefaultSystems();
    void updatePerformanceStats();
    void syncSystemToRegistry(SystemType systemType);
    void syncRegistryToSystem(SystemType systemType);
};

/**
 * @brief 参数系统适配器基类
 * 为现有参数系统提供统一的适配接口
 */
class ParameterSystemAdapter {
public:
    virtual ~ParameterSystemAdapter() = default;
    
    virtual SystemType getSystemType() const = 0;
    virtual std::string getSystemName() const = 0;
    virtual bool isSystemAvailable() const = 0;
    
    virtual void syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) = 0;
    virtual void syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) = 0;
    
    virtual std::vector<std::string> getParameterPaths() const = 0;
    virtual ParameterValue getParameterValue(const std::string& path) const = 0;
    virtual bool setParameterValue(const std::string& path, const ParameterValue& value) = 0;
};

/**
 * @brief 渲染配置适配器
 */
class RenderingConfigAdapter : public ParameterSystemAdapter {
public:
    RenderingConfigAdapter(RenderingConfig* config);
    ~RenderingConfigAdapter() override = default;

    SystemType getSystemType() const override { return SystemType::RENDERING; }
    std::string getSystemName() const override { return "RenderingConfig"; }
    bool isSystemAvailable() const override;

    void syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;
    void syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;

    std::vector<std::string> getParameterPaths() const override;
    ParameterValue getParameterValue(const std::string& path) const override;
    bool setParameterValue(const std::string& path, const ParameterValue& value) override;

private:
    RenderingConfig* m_config;
    void initializeParameterTree(std::shared_ptr<UnifiedParameterTree> tree);
};

/**
 * @brief 网格参数管理器适配器
 */
class MeshParameterManagerAdapter : public ParameterSystemAdapter {
public:
    MeshParameterManagerAdapter(MeshParameterManager* manager);
    ~MeshParameterManagerAdapter() override = default;

    SystemType getSystemType() const override { return SystemType::MESH; }
    std::string getSystemName() const override { return "MeshParameterManager"; }
    bool isSystemAvailable() const override;

    void syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;
    void syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;

    std::vector<std::string> getParameterPaths() const override;
    ParameterValue getParameterValue(const std::string& path) const override;
    bool setParameterValue(const std::string& path, const ParameterValue& value) override;

private:
    MeshParameterManager* m_manager;
    void initializeParameterTree(std::shared_ptr<UnifiedParameterTree> tree);
};

/**
 * @brief 光照配置适配器
 */
class LightingConfigAdapter : public ParameterSystemAdapter {
public:
    LightingConfigAdapter(LightingConfig* config);
    ~LightingConfigAdapter() override = default;

    SystemType getSystemType() const override { return SystemType::LIGHTING; }
    std::string getSystemName() const override { return "LightingConfig"; }
    bool isSystemAvailable() const override;

    void syncToRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;
    void syncFromRegistry(std::shared_ptr<UnifiedParameterTree> tree) override;

    std::vector<std::string> getParameterPaths() const override;
    ParameterValue getParameterValue(const std::string& path) const override;
    bool setParameterValue(const std::string& path, const ParameterValue& value) override;

private:
    LightingConfig* m_config;
    void initializeParameterTree(std::shared_ptr<UnifiedParameterTree> tree);
};

// 便利宏定义
#define REGISTER_PARAM(system, path, value) \
    ParameterRegistry::getInstance().setParameter(ParameterRegistry::SystemType::system, path, value)

#define GET_PARAM(system, path) \
    ParameterRegistry::getInstance().getParameter(ParameterRegistry::SystemType::system, path)

#define SET_PARAM_FULL(fullPath, value) \
    ParameterRegistry::getInstance().setParameterByFullPath(fullPath, value)

#define GET_PARAM_FULL(fullPath) \
    ParameterRegistry::getInstance().getParameterByFullPath(fullPath)