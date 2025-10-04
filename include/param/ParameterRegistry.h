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
 * @brief Parameter registry
 * Unified management of all parameter systems, provides parameter registration, lookup and coordination functionality
 */
class ParameterRegistry {
public:
    // Parameter system types
    enum class SystemType {
        GEOMETRY,       // Geometry representation parameters
        RENDERING,      // Rendering control parameters
        MESH,          // Mesh parameters
        LIGHTING,      // Lighting parameters
        NAVIGATION,    // Navigation parameters
        DISPLAY,       // Display parameters
        PERFORMANCE    // Performance parameters
    };

    // Parameter change notification
    struct ParameterSystemChange {
        SystemType systemType;
        std::string parameterPath;
        ParameterValue oldValue;
        ParameterValue newValue;
        std::chrono::steady_clock::time_point timestamp;
    };

    using SystemChangeCallback = std::function<void(const ParameterSystemChange&)>;

    static ParameterRegistry& getInstance();

    // System registration
    void registerParameterSystem(SystemType type, std::shared_ptr<UnifiedParameterTree> tree);
    void unregisterParameterSystem(SystemType type);
    std::shared_ptr<UnifiedParameterTree> getParameterSystem(SystemType type) const;

    // Parameter access
    bool setParameter(SystemType systemType, const std::string& path, const ParameterValue& value);
    ParameterValue getParameter(SystemType systemType, const std::string& path) const;
    bool hasParameter(SystemType systemType, const std::string& path) const;

    // Cross-system parameter operations
    bool setParameterByFullPath(const std::string& fullPath, const ParameterValue& value);
    ParameterValue getParameterByFullPath(const std::string& fullPath) const;
    bool hasParameterByFullPath(const std::string& fullPath) const;

    // Batch operations
    bool setParametersBySystem(SystemType systemType, const std::unordered_map<std::string, ParameterValue>& values);
    std::unordered_map<std::string, ParameterValue> getAllParametersBySystem(SystemType systemType) const;

    // Parameter path parsing
    std::pair<SystemType, std::string> parseFullPath(const std::string& fullPath) const;
    std::string buildFullPath(SystemType systemType, const std::string& path) const;

    // Inter-system dependency management
    void addSystemDependency(SystemType dependentSystem, SystemType dependencySystem);
    void removeSystemDependency(SystemType dependentSystem, SystemType dependencySystem);
    std::vector<SystemType> getDependentSystems(SystemType systemType) const;

    // Change notification
    int registerSystemChangeCallback(SystemChangeCallback callback);
    void unregisterSystemChangeCallback(int callbackId);
    void notifySystemChange(const ParameterSystemChange& change);

    // Integration with existing systems
    void integrateRenderingConfig(RenderingConfig* config);
    void integrateMeshParameterManager(MeshParameterManager* manager);
    void integrateLightingConfig(LightingConfig* config);

    // Synchronization operations
    void syncFromExistingSystems();
    void syncToExistingSystems();

    // Preset management
    void savePreset(const std::string& presetName);
    void loadPreset(const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    void deletePreset(const std::string& presetName);

    // Validation and diagnostics
    bool validateAllSystems() const;
    std::vector<std::string> getValidationReport() const;
    std::string getSystemStatusReport() const;

    // Performance monitoring
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

    // Internal data structures
    std::unordered_map<SystemType, std::shared_ptr<UnifiedParameterTree>> m_systems;
    std::unordered_map<SystemType, std::vector<SystemType>> m_systemDependencies;
    std::unordered_map<int, SystemChangeCallback> m_systemCallbacks;
    int m_nextCallbackId;

    // Existing system integration
    RenderingConfig* m_renderingConfig;
    MeshParameterManager* m_meshParameterManager;
    LightingConfig* m_lightingConfig;

    // Performance statistics
    mutable std::mutex m_statsMutex;
    PerformanceStats m_performanceStats;

    // Internal helper methods
    void initializeDefaultSystems();
    void updatePerformanceStats();
    void syncSystemToRegistry(SystemType systemType);
    void syncRegistryToSystem(SystemType systemType);
};

/**
 * @brief Parameter system adapter base class
 * Provides unified adapter interface for existing parameter systems
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
 * @brief Rendering configuration adapter
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
 * @brief Mesh parameter manager adapter
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
 * @brief Lighting configuration adapter
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

// Convenience macro definitions
#define REGISTER_PARAM(system, path, value) \
    ParameterRegistry::getInstance().setParameter(ParameterRegistry::SystemType::system, path, value)

#define GET_PARAM(system, path) \
    ParameterRegistry::getInstance().getParameter(ParameterRegistry::SystemType::system, path)

#define SET_PARAM_FULL(fullPath, value) \
    ParameterRegistry::getInstance().setParameterByFullPath(fullPath, value)

#define GET_PARAM_FULL(fullPath) \
    ParameterRegistry::getInstance().getParameterByFullPath(fullPath)