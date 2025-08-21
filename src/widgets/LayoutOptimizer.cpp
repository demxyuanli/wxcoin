#include "widgets/LayoutOptimizer.h"
#include "widgets/ILayoutStrategy.h"
#include <algorithm>
#include <iostream>
#include <sstream>

LayoutOptimizer::LayoutOptimizer()
    : m_strategy(OptimizationStrategy::Basic)
    , m_nodeConsolidationEnabled(true)
    , m_splitterOptimizationEnabled(true)
    , m_memoryOptimizationEnabled(true)
    , m_cachingEnabled(false)
    , m_batchOperationActive(false)
    , m_memoryLimit(1024 * 1024) // 1GB default
    , m_cacheSizeLimit(100) // 100 cached layouts
{
    // Initialize default parameters
    m_parameters["node_consolidation_threshold"] = "5";
    m_parameters["splitter_ratio_tolerance"] = "0.1";
    m_parameters["memory_cleanup_threshold"] = "100";
    m_parameters["batch_size"] = "50";
    
    // Initialize metrics
    m_metrics = {};
}

void LayoutOptimizer::SetOptimizationStrategy(OptimizationStrategy strategy)
{
    m_strategy = strategy;
    
    // Apply strategy-specific parameters
    switch (strategy) {
        case OptimizationStrategy::None:
            m_nodeConsolidationEnabled = false;
            m_splitterOptimizationEnabled = false;
            m_memoryOptimizationEnabled = false;
            m_cachingEnabled = false;
            break;
            
        case OptimizationStrategy::Basic:
            m_nodeConsolidationEnabled = true;
            m_splitterOptimizationEnabled = true;
            m_memoryOptimizationEnabled = false;
            m_cachingEnabled = false;
            break;
            
        case OptimizationStrategy::Aggressive:
            m_nodeConsolidationEnabled = true;
            m_splitterOptimizationEnabled = true;
            m_memoryOptimizationEnabled = true;
            m_cachingEnabled = true;
            m_parameters["node_consolidation_threshold"] = "3";
            m_parameters["splitter_ratio_tolerance"] = "0.05";
            m_parameters["memory_cleanup_threshold"] = "50";
            break;
            
        case OptimizationStrategy::Adaptive:
            m_nodeConsolidationEnabled = true;
            m_splitterOptimizationEnabled = true;
            m_memoryOptimizationEnabled = true;
            m_cachingEnabled = true;
            break;
            
        case OptimizationStrategy::Custom:
            // Keep current settings
            break;
    }
}

void LayoutOptimizer::SetOptimizationParameters(const std::map<std::string, std::string>& params)
{
    m_parameters = params;
}

bool LayoutOptimizer::OptimizeLayout(LayoutNode* root, ILayoutStrategy* strategy)
{
    if (!root || !strategy) {
        return false;
    }
    
    try {
        UpdateProgress(0, "Starting layout optimization...");
        
        // Analyze current layout
        AnalyzeLayout(root);
        UpdateProgress(10, "Layout analysis complete");
        
        // Check if optimization is needed
        if (!ShouldOptimize(root)) {
            UpdateProgress(100, "No optimization needed");
            return true;
        }
        
        // Apply strategy-specific optimizations
        ApplyStrategySpecificOptimizations(root, strategy);
        UpdateProgress(30, "Strategy-specific optimizations applied");
        
        // Optimize node structure
        if (m_nodeConsolidationEnabled) {
            OptimizeNodeStructure(root);
            UpdateProgress(50, "Node structure optimized");
        }
        
        // Optimize splitter ratios
        if (m_splitterOptimizationEnabled) {
            OptimizeSplitterRatios(root);
            UpdateProgress(70, "Splitter ratios optimized");
        }
        
        // Memory optimization
        if (m_memoryOptimizationEnabled) {
            CleanupUnusedNodes(root);
            UpdateProgress(85, "Memory cleanup complete");
        }
        
        // Final analysis
        AnalyzeLayout(root);
        UpdateProgress(100, "Layout optimization complete");
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Layout optimization failed: " << e.what() << std::endl;
        UpdateProgress(-1, "Optimization failed");
        return false;
    }
}

LayoutMetrics LayoutOptimizer::GetMetrics() const
{
    return m_metrics;
}

void LayoutOptimizer::AnalyzeLayout(LayoutNode* root)
{
    if (!root) return;
    
    // Reset metrics
    m_metrics = {};
    
    // Measure creation time (simulated)
    m_metrics.creationTime = 0.0;
    
    // Measure calculation time (simulated)
    m_metrics.calculationTime = 0.0;
    
    // Calculate node metrics
    CalculateNodeMetrics(root);
    
    // Estimate memory usage
    m_metrics.memoryUsage = EstimateMemoryUsage(root);
    
    // Determine if layout is optimized
    m_metrics.isOptimized = (m_metrics.nodeCount < 100) && 
                           (m_metrics.memoryUsage < m_memoryLimit / 2);
}

void LayoutOptimizer::EnableNodeConsolidation(bool enable)
{
    m_nodeConsolidationEnabled = enable;
}

void LayoutOptimizer::EnableSplitterOptimization(bool enable)
{
    m_splitterOptimizationEnabled = enable;
}

void LayoutOptimizer::EnableMemoryOptimization(bool enable)
{
    m_memoryOptimizationEnabled = enable;
}

void LayoutOptimizer::EnableCaching(bool enable)
{
    m_cachingEnabled = enable;
}

void LayoutOptimizer::BeginBatchOperation()
{
    m_batchOperationActive = true;
    m_batchOperations.clear();
}

void LayoutOptimizer::EndBatchOperation()
{
    if (!m_batchOperationActive) return;
    
    // Execute all batched operations
    for (const auto& operation : m_batchOperations) {
        operation();
    }
    
    m_batchOperationActive = false;
    m_batchOperations.clear();
}

bool LayoutOptimizer::IsBatchOperationActive() const
{
    return m_batchOperationActive;
}

void LayoutOptimizer::SetMemoryLimit(size_t limitKB)
{
    m_memoryLimit = limitKB * 1024; // Convert to bytes
}

size_t LayoutOptimizer::GetMemoryLimit() const
{
    return m_memoryLimit / 1024; // Convert to KB
}

void LayoutOptimizer::CleanupUnusedNodes(LayoutNode* root)
{
    if (!root) return;
    
    // Remove empty containers
    auto& children = root->GetChildren();
    children.erase(
        std::remove_if(children.begin(), children.end(),
            [](const std::unique_ptr<LayoutNode>& child) {
                return child->GetType() == LayoutNodeType::Root && 
                       child->GetChildren().empty();
            }),
        children.end()
    );
    
    // Recursively cleanup children
    for (auto& child : children) {
        CleanupUnusedNodes(child.get());
    }
}

void LayoutOptimizer::EnableLayoutCaching(bool enable)
{
    m_cachingEnabled = enable;
}

void LayoutOptimizer::CacheLayout(LayoutNode* root, const std::string& key)
{
    if (!m_cachingEnabled || !root) return;
    
    // Serialize layout (simplified)
    std::stringstream ss;
    ss << "Cached layout for key: " << key;
    
    CachedLayout cached;
    cached.data = ss.str();
    cached.timestamp = std::chrono::steady_clock::now();
    cached.metrics = m_metrics;
    
    // Add to cache
    m_layoutCache[key] = cached;
    
    // Limit cache size
    if (m_layoutCache.size() > m_cacheSizeLimit) {
        // Remove oldest entry
        auto oldest = m_layoutCache.begin();
        for (auto it = m_layoutCache.begin(); it != m_layoutCache.end(); ++it) {
            if (it->second.timestamp < oldest->second.timestamp) {
                oldest = it;
            }
        }
        m_layoutCache.erase(oldest);
    }
}

bool LayoutOptimizer::RestoreCachedLayout(LayoutNode* root, const std::string& key)
{
    if (!m_cachingEnabled || !root) return false;
    
    auto it = m_layoutCache.find(key);
    if (it == m_layoutCache.end()) return false;
    
    // Check if cache is still valid (within 5 minutes)
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.timestamp);
    if (age.count() > 5) {
        m_layoutCache.erase(it);
        return false;
    }
    
    // Restore metrics
    m_metrics = it->second.metrics;
    
    return true;
}

void LayoutOptimizer::ClearCache()
{
    m_layoutCache.clear();
}

void LayoutOptimizer::SetProgressCallback(ProgressCallback callback)
{
    m_progressCallback = callback;
}

void LayoutOptimizer::OptimizeNodeStructure(LayoutNode* root)
{
    if (!root) return;
    
    // Consolidate similar nodes
    ConsolidateSimilarNodes(root);
    
    // Remove redundant nodes
    RemoveRedundantNodes(root);
    
    // Balance tree structure
    BalanceTreeStructure(root);
}

void LayoutOptimizer::OptimizeSplitterRatios(LayoutNode* root)
{
    if (!root) return;
    
    // Get tolerance from parameters
    double tolerance = 0.1;
    try {
        tolerance = std::stod(m_parameters["splitter_ratio_tolerance"]);
    } catch (...) {
        // Use default value
    }
    
    // Optimize splitter ratios recursively
    if (root->GetType() == LayoutNodeType::HorizontalSplitter || 
        root->GetType() == LayoutNodeType::VerticalSplitter) {
        
        double ratio = root->GetSplitterRatio();
        
        // Snap to common ratios if within tolerance
        if (std::abs(ratio - 0.5) < tolerance) {
            root->SetSplitterRatio(0.5);
        } else if (std::abs(ratio - 0.33) < tolerance) {
            root->SetSplitterRatio(0.33);
        } else if (std::abs(ratio - 0.67) < tolerance) {
            root->SetSplitterRatio(0.67);
        } else if (std::abs(ratio - 0.25) < tolerance) {
            root->SetSplitterRatio(0.25);
        } else if (std::abs(ratio - 0.75) < tolerance) {
            root->SetSplitterRatio(0.75);
        }
    }
    
    // Recursively optimize children
    for (auto& child : root->GetChildren()) {
        OptimizeSplitterRatios(child.get());
    }
}

void LayoutOptimizer::ConsolidateSimilarNodes(LayoutNode* root)
{
    if (!root) return;
    
    // Get threshold from parameters
    int threshold = 5;
    try {
        threshold = std::stoi(m_parameters["node_consolidation_threshold"]);
    } catch (...) {
        // Use default value
    }
    
    // Consolidate similar container nodes
    auto& children = root->GetChildren();
    if (children.size() > threshold) {
        // Group similar nodes
        std::vector<std::vector<LayoutNode*>> groups;
        
        for (auto& child : children) {
            bool grouped = false;
            for (auto& group : groups) {
                if (!group.empty() && group[0]->GetType() == child->GetType()) {
                    group.push_back(child.get());
                    grouped = true;
                    break;
                }
            }
            if (!grouped) {
                groups.push_back({child.get()});
            }
        }
        
        // Consolidate groups with more than threshold items
        for (auto& group : groups) {
            if (group.size() > threshold) {
                // Create a new splitter for this group
                auto splitter = std::make_unique<LayoutNode>(LayoutNodeType::HorizontalSplitter);
                for (auto* node : group) {
                    // Move children to new splitter
                    auto& nodeChildren = node->GetChildren();
                    for (auto& grandChild : nodeChildren) {
                        splitter->AddChild(std::move(grandChild));
                    }
                }
                
                // Replace group with consolidated splitter
                // This is a simplified implementation
            }
        }
    }
    
    // Recursively consolidate children
    for (auto& child : children) {
        ConsolidateSimilarNodes(child.get());
    }
}

void LayoutOptimizer::RemoveRedundantNodes(LayoutNode* root)
{
    if (!root) return;
    
    auto& children = root->GetChildren();
    
    // Remove single-child splitters
    children.erase(
        std::remove_if(children.begin(), children.end(),
            [](const std::unique_ptr<LayoutNode>& child) {
                return (child->GetType() == LayoutNodeType::HorizontalSplitter || 
                        child->GetType() == LayoutNodeType::VerticalSplitter) &&
                       child->GetChildren().size() == 1;
            }),
        children.end()
    );
    
    // Recursively remove redundant nodes from children
    for (auto& child : children) {
        RemoveRedundantNodes(child.get());
    }
}

void LayoutOptimizer::BalanceTreeStructure(LayoutNode* root)
{
    if (!root) return;
    
    // Balance tree by ensuring no branch is too deep
    int maxDepth = 10;
    int currentDepth = 0;
    
    // Simple balancing: if a branch is too deep, flatten it
    if (currentDepth > maxDepth) {
        // Flatten the tree structure
        // This is a simplified implementation
    }
    
    // Recursively balance children
    for (auto& child : root->GetChildren()) {
        BalanceTreeStructure(child.get());
    }
}

void LayoutOptimizer::CalculateNodeMetrics(LayoutNode* root)
{
    if (!root) return;
    
    // Count nodes by type
    switch (root->GetType()) {
        case LayoutNodeType::Panel:
            m_metrics.panelCount++;
            break;
        case LayoutNodeType::HorizontalSplitter:
        case LayoutNodeType::VerticalSplitter:
            m_metrics.splitterCount++;
            break;
        case LayoutNodeType::Root:
            m_metrics.containerCount++;
            break;
        default:
            break;
    }
    
    m_metrics.nodeCount++;
    
    // Recursively count children
    for (const auto& child : root->GetChildren()) {
        CalculateNodeMetrics(child.get());
    }
}

void LayoutOptimizer::MeasureOperationTime(std::function<void()> operation, double& timeMs)
{
    auto start = std::chrono::high_resolution_clock::now();
    operation();
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    timeMs = duration.count() / 1000.0;
}

double LayoutOptimizer::EstimateMemoryUsage(LayoutNode* root) const
{
    if (!root) return 0.0;
    
    // Rough estimation: each node uses about 64 bytes
    double estimatedBytes = m_metrics.nodeCount * 64.0;
    
    // Add overhead for children vectors and other data
    estimatedBytes += m_metrics.nodeCount * 32.0;
    
    return estimatedBytes / 1024.0; // Convert to KB
}

void LayoutOptimizer::UpdateProgress(int percentage, const std::string& message)
{
    if (m_progressCallback) {
        m_progressCallback(percentage, message);
    }
}

bool LayoutOptimizer::ShouldOptimize(LayoutNode* root) const
{
    if (!root) return false;
    
    // Don't optimize if already optimized
    if (m_metrics.isOptimized) return false;
    
    // Optimize if layout is large
    if (m_metrics.nodeCount > 50) return true;
    
    // Optimize if memory usage is high
    if (m_metrics.memoryUsage > m_memoryLimit / 4) return true;
    
    // Optimize if there are many splitters
    if (m_metrics.splitterCount > 20) return true;
    
    return false;
}

void LayoutOptimizer::ApplyStrategySpecificOptimizations(LayoutNode* root, ILayoutStrategy* strategy)
{
    if (!root || !strategy) return;
    
    // Get strategy name for specific optimizations
    std::string strategyName = "Unknown";
    
    // Apply strategy-specific optimizations
    if (strategyName.find("IDE") != std::string::npos) {
        // IDE strategy: optimize for fixed layouts
        // Ensure main areas are properly sized
    } else if (strategyName.find("Flexible") != std::string::npos) {
        // Flexible strategy: optimize for dynamic layouts
        // Balance tree structure
        BalanceTreeStructure(root);
    } else if (strategyName.find("Hybrid") != std::string::npos) {
        // Hybrid strategy: optimize for mixed layouts
        // Apply both IDE and Flexible optimizations
    }
}


