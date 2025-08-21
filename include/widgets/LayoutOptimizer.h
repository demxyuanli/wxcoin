#pragma once

#include "ILayoutStrategy.h"
#include "LayoutEngine.h"
#include <memory>
#include <vector>
#include <chrono>
#include <functional>
#include <map>
#include <string>

// Performance metrics for layout optimization
struct LayoutMetrics {
    double creationTime;      // Time to create layout (ms)
    double calculationTime;   // Time to calculate layout (ms)
    double optimizationTime;  // Time to optimize layout (ms)
    int nodeCount;           // Number of nodes in layout tree
    int panelCount;          // Number of panels
    int splitterCount;       // Number of splitters
    int containerCount;      // Number of containers
    double memoryUsage;      // Estimated memory usage (KB)
    bool isOptimized;        // Whether layout is optimized
};

// Layout optimization strategies
enum class OptimizationStrategy {
    None,                   // No optimization
    Basic,                  // Basic cleanup and validation
    Aggressive,            // Aggressive optimization for large layouts
    Adaptive,              // Adaptive optimization based on layout size
    Custom                 // Custom optimization with parameters
};

// Layout optimizer for performance tuning
class LayoutOptimizer {
public:
    LayoutOptimizer();
    ~LayoutOptimizer() = default;

    // Set optimization strategy
    void SetOptimizationStrategy(OptimizationStrategy strategy);
    
    // Set custom optimization parameters
    void SetOptimizationParameters(const std::map<std::string, std::string>& params);
    
    // Optimize layout for performance
    bool OptimizeLayout(LayoutNode* root, ILayoutStrategy* strategy);
    
    // Get performance metrics
    LayoutMetrics GetMetrics() const;
    
    // Analyze layout performance
    void AnalyzeLayout(LayoutNode* root);
    
    // Enable/disable specific optimizations
    void EnableNodeConsolidation(bool enable);
    void EnableSplitterOptimization(bool enable);
    void EnableMemoryOptimization(bool enable);
    void EnableCaching(bool enable);
    
    // Batch operations for large layouts
    void BeginBatchOperation();
    void EndBatchOperation();
    bool IsBatchOperationActive() const;
    
    // Memory management
    void SetMemoryLimit(size_t limitKB);
    size_t GetMemoryLimit() const;
    void CleanupUnusedNodes(LayoutNode* root);
    
    // Caching
    void EnableLayoutCaching(bool enable);
    void CacheLayout(LayoutNode* root, const std::string& key);
    bool RestoreCachedLayout(LayoutNode* root, const std::string& key);
    void ClearCache();
    
    // Progress reporting
    using ProgressCallback = std::function<void(int percentage, const std::string& message)>;
    void SetProgressCallback(ProgressCallback callback);

private:
    // Optimization methods
    void OptimizeNodeStructure(LayoutNode* root);
    void OptimizeSplitterRatios(LayoutNode* root);
    void ConsolidateSimilarNodes(LayoutNode* root);
    void RemoveRedundantNodes(LayoutNode* root);
    void BalanceTreeStructure(LayoutNode* root);
    
    // Performance analysis
    void CalculateNodeMetrics(LayoutNode* root);
    void MeasureOperationTime(std::function<void()> operation, double& timeMs);
    double EstimateMemoryUsage(LayoutNode* root) const;
    
    // Caching implementation
    struct CachedLayout {
        std::string data;
        std::chrono::steady_clock::time_point timestamp;
        LayoutMetrics metrics;
    };
    
    // Internal state
    OptimizationStrategy m_strategy;
    std::map<std::string, std::string> m_parameters;
    LayoutMetrics m_metrics;
    
    // Optimization flags
    bool m_nodeConsolidationEnabled;
    bool m_splitterOptimizationEnabled;
    bool m_memoryOptimizationEnabled;
    bool m_cachingEnabled;
    
    // Batch operation state
    bool m_batchOperationActive;
    std::vector<std::function<void()>> m_batchOperations;
    
    // Memory management
    size_t m_memoryLimit;
    
    // Caching
    std::map<std::string, CachedLayout> m_layoutCache;
    size_t m_cacheSizeLimit;
    
    // Progress reporting
    ProgressCallback m_progressCallback;
    
    // Helper methods
    void UpdateProgress(int percentage, const std::string& message);
    bool ShouldOptimize(LayoutNode* root) const;
    void ApplyStrategySpecificOptimizations(LayoutNode* root, ILayoutStrategy* strategy);
};


