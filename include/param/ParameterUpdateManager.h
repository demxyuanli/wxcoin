#pragma once

#include "ParameterTree.h"
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>

// 前向声明
class OCCGeometry;
class RenderingConfig;

/**
 * @brief 更新类型枚举
 */
enum class UpdateType {
    Geometry,           // 几何更新
    Rendering,          // 渲染更新
    Display,            // 显示更新
    Lighting,           // 光照更新
    Material,           // 材质更新
    Texture,            // 纹理更新
    Shadow,             // 阴影更新
    Quality,            // 质量更新
    Transform,          // 变换更新
    Color,              // 颜色更新
    FullRefresh         // 完全刷新
};

/**
 * @brief 更新优先级枚举
 */
enum class UpdatePriority {
    Low = 0,            // 低优先级
    Normal = 1,         // 普通优先级
    High = 2,           // 高优先级
    Critical = 3        // 关键优先级
};

/**
 * @brief 更新任务结构
 */
struct UpdateTask {
    UpdateType type;
    UpdatePriority priority;
    std::string parameterPath;
    ParameterValue value;
    std::chrono::steady_clock::time_point timestamp;
    std::function<void()> updateFunction;
    
    UpdateTask(UpdateType t, UpdatePriority p, const std::string& path, 
               const ParameterValue& val, std::function<void()> func)
        : type(t), priority(p), parameterPath(path), value(val), 
          timestamp(std::chrono::steady_clock::now()), updateFunction(func) {}
};

/**
 * @brief 参数到更新类型的映射
 */
class ParameterUpdateMapping {
public:
    static void initializeMappings();
    static UpdateType getUpdateType(const std::string& parameterPath);
    static UpdatePriority getUpdatePriority(const std::string& parameterPath);
    static std::vector<UpdateType> getAffectedUpdateTypes(const std::string& parameterPath);
    
private:
    static std::map<std::string, UpdateType> s_parameterToUpdateType;
    static std::map<std::string, UpdatePriority> s_parameterToPriority;
    static std::map<std::string, std::vector<UpdateType>> s_parameterToAffectedTypes;
};

/**
 * @brief 更新接口基类
 */
class IUpdateInterface {
public:
    virtual ~IUpdateInterface() = default;
    virtual void updateGeometry() = 0;
    virtual void updateRendering() = 0;
    virtual void updateDisplay() = 0;
    virtual void updateLighting() = 0;
    virtual void updateMaterial() = 0;
    virtual void updateTexture() = 0;
    virtual void updateShadow() = 0;
    virtual void updateQuality() = 0;
    virtual void updateTransform() = 0;
    virtual void updateColor() = 0;
    virtual void fullRefresh() = 0;
};

/**
 * @brief 参数更新管理器
 */
class ParameterUpdateManager {
public:
    static ParameterUpdateManager& getInstance();
    
    // 更新接口注册
    void registerUpdateInterface(std::shared_ptr<IUpdateInterface> interface);
    void unregisterUpdateInterface(std::shared_ptr<IUpdateInterface> interface);
    
    // 参数变更处理
    void onParameterChanged(const std::string& path, const ParameterValue& value);
    void onBatchUpdate(const std::vector<std::string>& changedPaths);
    
    // 更新任务管理
    void addUpdateTask(const UpdateTask& task);
    void processUpdateTasks();
    void clearUpdateTasks();
    
    // 批量更新控制
    void beginBatchUpdate();
    void endBatchUpdate();
    bool isInBatchUpdate() const { return m_inBatchUpdate; }
    
    // 更新策略配置
    void setUpdateStrategy(UpdateType type, std::function<void()> strategy);
    void setBatchUpdateThreshold(size_t threshold) { m_batchUpdateThreshold = threshold; }
    void setUpdateDelay(std::chrono::milliseconds delay) { m_updateDelay = delay; }
    
    // 性能优化
    void enableUpdateOptimization(bool enable) { m_optimizationEnabled = enable; }
    void setUpdateFrequencyLimit(int maxUpdatesPerSecond);
    
    // 调试和监控
    size_t getPendingTaskCount() const;
    std::vector<std::string> getPendingParameterPaths() const;
    void enableDebugMode(bool enable) { m_debugMode = enable; }
    
private:
    ParameterUpdateManager();
    ~ParameterUpdateManager() = default;
    ParameterUpdateManager(const ParameterUpdateManager&) = delete;
    ParameterUpdateManager& operator=(const ParameterUpdateManager&) = delete;
    
    // 内部方法
    void scheduleUpdate(const std::string& parameterPath, const ParameterValue& value);
    void executeUpdate(UpdateType type);
    void optimizeUpdateTasks();
    void mergeUpdateTasks();
    bool shouldSkipUpdate(const std::string& parameterPath) const;
    
    // 更新接口管理
    std::vector<std::shared_ptr<IUpdateInterface>> m_updateInterfaces;
    mutable std::mutex m_interfacesMutex;
    
    // 更新任务管理
    std::vector<UpdateTask> m_updateTasks;
    mutable std::mutex m_tasksMutex;
    
    // 批量更新状态
    std::atomic<bool> m_inBatchUpdate;
    std::vector<std::string> m_batchChangedPaths;
    mutable std::mutex m_batchMutex;
    
    // 更新策略
    std::map<UpdateType, std::function<void()>> m_updateStrategies;
    size_t m_batchUpdateThreshold;
    std::chrono::milliseconds m_updateDelay;
    
    // 性能优化
    std::atomic<bool> m_optimizationEnabled;
    int m_maxUpdatesPerSecond;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    std::set<std::string> m_recentlyUpdatedPaths;
    
    // 调试
    std::atomic<bool> m_debugMode;
};

/**
 * @brief 几何对象更新接口实现
 */
class GeometryUpdateInterface : public IUpdateInterface {
public:
    explicit GeometryUpdateInterface(std::shared_ptr<OCCGeometry> geometry);
    
    void updateGeometry() override;
    void updateRendering() override;
    void updateDisplay() override;
    void updateLighting() override;
    void updateMaterial() override;
    void updateTexture() override;
    void updateShadow() override;
    void updateQuality() override;
    void updateTransform() override;
    void updateColor() override;
    void fullRefresh() override;
    
private:
    std::shared_ptr<OCCGeometry> m_geometry;
};

/**
 * @brief 渲染配置更新接口实现
 */
class RenderingConfigUpdateInterface : public IUpdateInterface {
public:
    explicit RenderingConfigUpdateInterface(RenderingConfig* config);
    
    void updateGeometry() override;
    void updateRendering() override;
    void updateDisplay() override;
    void updateLighting() override;
    void updateMaterial() override;
    void updateTexture() override;
    void updateShadow() override;
    void updateQuality() override;
    void updateTransform() override;
    void updateColor() override;
    void fullRefresh() override;
    
private:
    RenderingConfig* m_config;
};

/**
 * @brief 参数更新管理器初始化器
 */
class ParameterUpdateManagerInitializer {
public:
    static void initialize();
    static void initializeParameterMappings();
    static void initializeUpdateStrategies();
    static void initializeDefaultInterfaces();
};