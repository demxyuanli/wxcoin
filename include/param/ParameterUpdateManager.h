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

// Forward declarations
class OCCGeometry;
class RenderingConfig;

/**
 * @brief Update type enumeration
 */
enum class UpdateType {
    Geometry,           // Geometry update
    Rendering,          // Rendering update
    Display,            // Display update
    Lighting,           // Lighting update
    Material,           // Material update
    Texture,            // Texture update
    Shadow,             // Shadow update
    Quality,            // Quality update
    Transform,          // Transform update
    Color,              // Color update
    FullRefresh         // Full refresh
};

/**
 * @brief Update priority enumeration
 */
enum class UpdatePriority {
    Low = 0,            // Low priority
    Normal = 1,         // Normal priority
    High = 2,           // High priority
    Critical = 3        // Critical priority
};

/**
 * @brief Update task structure
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
 * @brief Parameter to update type mapping
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
 * @brief Base class for update interfaces
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
 * @brief Parameter update manager
 */
class ParameterUpdateManager {
public:
    static ParameterUpdateManager& getInstance();
    
    // Update interface registration
    void registerUpdateInterface(std::shared_ptr<IUpdateInterface> interface);
    void unregisterUpdateInterface(std::shared_ptr<IUpdateInterface> interface);
    
    // Parameter change handling
    void onParameterChanged(const std::string& path, const ParameterValue& value);
    void onBatchUpdate(const std::vector<std::string>& changedPaths);
    
    // Update task management
    void addUpdateTask(const UpdateTask& task);
    void processUpdateTasks();
    void clearUpdateTasks();
    
    // Batch update control
    void beginBatchUpdate();
    void endBatchUpdate();
    bool isInBatchUpdate() const { return m_inBatchUpdate; }
    
    // Update strategy configuration
    void setUpdateStrategy(UpdateType type, std::function<void()> strategy);
    void setBatchUpdateThreshold(size_t threshold) { m_batchUpdateThreshold = threshold; }
    void setUpdateDelay(std::chrono::milliseconds delay) { m_updateDelay = delay; }
    
    // Performance optimization
    void enableUpdateOptimization(bool enable) { m_optimizationEnabled = enable; }
    void setUpdateFrequencyLimit(int maxUpdatesPerSecond);
    
    // Debugging and monitoring
    size_t getPendingTaskCount() const;
    std::vector<std::string> getPendingParameterPaths() const;
    void enableDebugMode(bool enable) { m_debugMode = enable; }
    
private:
    ParameterUpdateManager();
    ~ParameterUpdateManager() = default;
    ParameterUpdateManager(const ParameterUpdateManager&) = delete;
    ParameterUpdateManager& operator=(const ParameterUpdateManager&) = delete;
    
    // Internal methods
    void scheduleUpdate(const std::string& parameterPath, const ParameterValue& value);
    void executeUpdate(UpdateType type);
    void optimizeUpdateTasks();
    void mergeUpdateTasks();
    bool shouldSkipUpdate(const std::string& parameterPath) const;
    
    // Update interface management
    std::vector<std::shared_ptr<IUpdateInterface>> m_updateInterfaces;
    mutable std::mutex m_interfacesMutex;
    
    // Update task management
    std::vector<UpdateTask> m_updateTasks;
    mutable std::mutex m_tasksMutex;
    
    // Batch update state
    std::atomic<bool> m_inBatchUpdate;
    std::vector<std::string> m_batchChangedPaths;
    mutable std::mutex m_batchMutex;
    
    // Update strategies
    std::map<UpdateType, std::function<void()>> m_updateStrategies;
    size_t m_batchUpdateThreshold;
    std::chrono::milliseconds m_updateDelay;
    
    // Performance optimization
    std::atomic<bool> m_optimizationEnabled;
    int m_maxUpdatesPerSecond;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    std::set<std::string> m_recentlyUpdatedPaths;
    
    // Debugging
    std::atomic<bool> m_debugMode;
};

/**
 * @brief Geometry object update interface implementation
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
 * @brief Rendering configuration update interface implementation
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
 * @brief Parameter update manager initializer
 */
class ParameterUpdateManagerInitializer {
public:
    static void initialize();
    static void initializeParameterMappings();
    static void initializeUpdateStrategies();
    static void initializeDefaultInterfaces();
};