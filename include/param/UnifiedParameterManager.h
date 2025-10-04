#pragma once

#include "ParameterTree.h"
#include "ParameterUpdateManager.h"
#include "ParameterSynchronizer.h"
#include <memory>

/**
 * @brief Unified parameter manager - main interface for the parameter management system
 */
class UnifiedParameterManager {
public:
    static UnifiedParameterManager& getInstance();
    
    // System initialization
    void initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }
    
    // Parameter tree access
    ParameterTree& getParameterTree() { return ParameterTree::getInstance(); }
    const ParameterTree& getParameterTree() const { return ParameterTree::getInstance(); }
    
    // Update manager access
    ParameterUpdateManager& getUpdateManager() { return ParameterUpdateManager::getInstance(); }
    const ParameterUpdateManager& getUpdateManager() const { return ParameterUpdateManager::getInstance(); }
    
    // Synchronizer access
    ParameterSynchronizer& getSynchronizer() { return ParameterSynchronizer::getInstance(); }
    const ParameterSynchronizer& getSynchronizer() const { return ParameterSynchronizer::getInstance(); }
    
    // High-level parameter operations
    bool setParameter(const std::string& path, const ParameterValue& value);
    ParameterValue getParameter(const std::string& path) const;
    bool hasParameter(const std::string& path) const;
    
    // Batch operations
    void beginBatchOperation();
    void endBatchOperation();
    bool isInBatchOperation() const;
    
    // System integration
    void registerGeometry(std::shared_ptr<class OCCGeometry> geometry);
    void unregisterGeometry(std::shared_ptr<class OCCGeometry> geometry);
    void registerRenderingConfig(class RenderingConfig* config);
    void unregisterRenderingConfig(class RenderingConfig* config);
    
    // Performance and optimization
    void enableOptimization(bool enable);
    void setUpdateFrequencyLimit(int maxUpdatesPerSecond);
    void enableDebugMode(bool enable);
    
    // Status and monitoring
    size_t getRegisteredGeometryCount() const;
    size_t getRegisteredConfigCount() const;
    std::vector<std::string> getAllParameterPaths() const;
    std::vector<std::string> getChangedParameters() const;
    
private:
    UnifiedParameterManager();
    ~UnifiedParameterManager() = default;
    UnifiedParameterManager(const UnifiedParameterManager&) = delete;
    UnifiedParameterManager& operator=(const UnifiedParameterManager&) = delete;
    
    void setupSystemIntegration();
    void setupDefaultParameters();
    void setupUpdateCallbacks();
    
    bool m_initialized;
    std::atomic<bool> m_inBatchOperation;
    std::vector<std::string> m_batchChangedParameters;
};

/**
 * @brief Unified parameter manager initializer
 */
class UnifiedParameterManagerInitializer {
public:
    static void initialize();
    static void initializeParameterTree();
    static void initializeUpdateManager();
    static void initializeSynchronizer();
    static void setupSystemIntegration();
};