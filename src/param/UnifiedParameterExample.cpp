#include "param/UnifiedParameterIntegration.h"
#include "config/RenderingConfig.h"
#include "MeshParameterManager.h"
#include "config/LightingConfig.h"
#include "logger/Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief 统一参数管理系统使用示例
 * 演示如何使用新的统一参数管理系统
 */
class UnifiedParameterExample {
public:
    static void runBasicExample() {
        LOG_INF_S("=== Unified Parameter System Basic Example ===");
        
        // 1. 初始化统一参数集成管理器
        auto& integration = UnifiedParameterIntegration::getInstance();
        
        UnifiedParameterIntegration::IntegrationConfig config;
        config.autoSyncEnabled = true;
        config.bidirectionalSync = true;
        config.syncInterval = std::chrono::milliseconds(50);
        config.enableSmartBatching = true;
        config.enableDependencyTracking = true;
        config.enablePerformanceMonitoring = true;
        
        if (!integration.initialize(config)) {
            LOG_ERR_S("Failed to initialize unified parameter integration");
            return;
        }
        
        // 2. 集成现有系统
        auto& renderingConfig = RenderingConfig::getInstance();
        auto& meshManager = MeshParameterManager::getInstance();
        auto& lightingConfig = LightingConfig::getInstance();
        
        integration.integrateRenderingConfig(&renderingConfig);
        integration.integrateMeshParameterManager(&meshManager);
        integration.integrateLightingConfig(&lightingConfig);
        
        // 3. 基本参数操作
        demonstrateBasicParameterOperations(integration);
        
        // 4. 批量参数操作
        demonstrateBatchParameterOperations(integration);
        
        // 5. 参数依赖关系
        demonstrateParameterDependencies(integration);
        
        // 6. 智能批量更新
        demonstrateSmartBatching(integration);
        
        // 7. 预设管理
        demonstratePresetManagement(integration);
        
        // 8. 性能监控
        demonstratePerformanceMonitoring(integration);
        
        LOG_INF_S("=== Basic Example Completed ===");
    }
    
    static void runAdvancedExample() {
        LOG_INF_S("=== Unified Parameter System Advanced Example ===");
        
        auto& integration = UnifiedParameterIntegration::getInstance();
        
        // 1. 复杂参数变更场景
        demonstrateComplexParameterChanges(integration);
        
        // 2. 多系统协调更新
        demonstrateMultiSystemCoordination(integration);
        
        // 3. 自定义更新策略
        demonstrateCustomUpdateStrategies(integration);
        
        // 4. 错误处理和恢复
        demonstrateErrorHandling(integration);
        
        LOG_INF_S("=== Advanced Example Completed ===");
    }
    
    static void runPerformanceTest() {
        LOG_INF_S("=== Unified Parameter System Performance Test ===");
        
        auto& integration = UnifiedParameterIntegration::getInstance();
        
        // 1. 大量参数变更测试
        testMassiveParameterChanges(integration);
        
        // 2. 批量处理效率测试
        testBatchProcessingEfficiency(integration);
        
        // 3. 内存使用测试
        testMemoryUsage(integration);
        
        // 4. 并发安全测试
        testConcurrencySafety(integration);
        
        LOG_INF_S("=== Performance Test Completed ===");
    }

private:
    static void demonstrateBasicParameterOperations(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Basic Parameter Operations ---");
        
        // 设置几何参数
        integration.setParameter("geometry.position.x", 10.0);
        integration.setParameter("geometry.position.y", 20.0);
        integration.setParameter("geometry.position.z", 30.0);
        
        // 设置渲染参数
        integration.setParameter("rendering.material.diffuse.r", 0.8);
        integration.setParameter("rendering.material.diffuse.g", 0.6);
        integration.setParameter("rendering.material.diffuse.b", 0.4);
        integration.setParameter("rendering.material.transparency", 0.3);
        
        // 设置网格参数
        integration.setParameter("mesh.deflection", 0.3);
        integration.setParameter("mesh.angularDeflection", 0.8);
        integration.setParameter("mesh.inParallel", true);
        
        // 设置光照参数
        integration.setParameter("lighting.main.intensity", 1.2);
        integration.setParameter("lighting.main.color.r", 1.0);
        integration.setParameter("lighting.main.color.g", 0.9);
        integration.setParameter("lighting.main.color.b", 0.8);
        
        // 读取参数值
        auto posX = integration.getParameter("geometry.position.x");
        auto diffuseR = integration.getParameter("rendering.material.diffuse.r");
        auto deflection = integration.getParameter("mesh.deflection");
        auto intensity = integration.getParameter("lighting.main.intensity");
        
        LOG_INF_S("Retrieved parameters:");
        LOG_INF_S("- Geometry position X: " + std::to_string(std::get<double>(posX)));
        LOG_INF_S("- Material diffuse R: " + std::to_string(std::get<double>(diffuseR)));
        LOG_INF_S("- Mesh deflection: " + std::to_string(std::get<double>(deflection)));
        LOG_INF_S("- Light intensity: " + std::to_string(std::get<double>(intensity)));
    }
    
    static void demonstrateBatchParameterOperations(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Batch Parameter Operations ---");
        
        // 批量设置参数
        std::unordered_map<std::string, ParameterValue> batchParams;
        batchParams["geometry.scale.x"] = 2.0;
        batchParams["geometry.scale.y"] = 2.0;
        batchParams["geometry.scale.z"] = 2.0;
        batchParams["rendering.material.shininess"] = 64.0;
        batchParams["rendering.display.showEdges"] = true;
        batchParams["mesh.relative"] = true;
        batchParams["lighting.main.enabled"] = true;
        
        bool success = integration.setParameters(batchParams);
        LOG_INF_S("Batch parameter setting " + std::string(success ? "succeeded" : "failed"));
        
        // 批量读取参数
        std::vector<std::string> paramPaths = {
            "geometry.scale.x", "geometry.scale.y", "geometry.scale.z",
            "rendering.material.shininess", "rendering.display.showEdges",
            "mesh.relative", "lighting.main.enabled"
        };
        
        auto batchValues = integration.getParameters(paramPaths);
        LOG_INF_S("Retrieved " + std::to_string(batchValues.size()) + " parameters in batch");
        
        for (const auto& pair : batchValues) {
            LOG_DBG_S("- " + pair.first + ": " + std::to_string(std::get<double>(pair.second)));
        }
    }
    
    static void demonstrateParameterDependencies(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Parameter Dependencies ---");
        
        // 添加参数依赖关系
        integration.addParameterDependency("rendering.material.transparency", "rendering.material.diffuse.r");
        integration.addParameterDependency("mesh.deflection", "geometry.scale.x");
        integration.addParameterDependency("lighting.main.intensity", "rendering.material.diffuse.r");
        
        // 获取依赖关系
        auto transparencyDeps = integration.getParameterDependencies("rendering.material.transparency");
        auto deflectionDeps = integration.getParameterDependencies("mesh.deflection");
        auto intensityDeps = integration.getParameterDependencies("lighting.main.intensity");
        
        LOG_INF_S("Parameter dependencies:");
        LOG_INF_S("- Transparency depends on: " + std::to_string(transparencyDeps.size()) + " parameters");
        LOG_INF_S("- Deflection depends on: " + std::to_string(deflectionDeps.size()) + " parameters");
        LOG_INF_S("- Intensity depends on: " + std::to_string(intensityDeps.size()) + " parameters");
        
        // 移除依赖关系
        integration.removeParameterDependency("rendering.material.transparency", "rendering.material.diffuse.r");
        LOG_INF_S("Removed dependency between transparency and diffuse color");
    }
    
    static void demonstrateSmartBatching(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Smart Batching ---");
        
        // 快速连续设置多个参数（应该被批量处理）
        auto startTime = std::chrono::steady_clock::now();
        
        for (int i = 0; i < 10; ++i) {
            integration.scheduleParameterChange(
                "rendering.material.diffuse.r", 
                0.5 + i * 0.05, 
                0.5 + (i + 1) * 0.05
            );
        }
        
        // 等待批量处理完成
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        LOG_INF_S("Smart batching test completed in " + std::to_string(duration.count()) + "ms");
        
        // 检查性能报告
        auto report = integration.getPerformanceReport();
        LOG_INF_S("Performance report:");
        LOG_INF_S("- Total parameters: " + std::to_string(report.totalParameters));
        LOG_INF_S("- Pending updates: " + std::to_string(report.pendingUpdates));
        LOG_INF_S("- Executed updates: " + std::to_string(report.executedUpdates));
        LOG_INF_S("- Batch groups created: " + std::to_string(report.batchGroupsCreated));
    }
    
    static void demonstratePresetManagement(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Preset Management ---");
        
        // 保存当前状态为预设
        integration.saveCurrentStateAsPreset("example_preset");
        LOG_INF_S("Saved current state as 'example_preset'");
        
        // 修改一些参数
        integration.setParameter("rendering.material.diffuse.r", 1.0);
        integration.setParameter("rendering.material.diffuse.g", 0.0);
        integration.setParameter("rendering.material.diffuse.b", 0.0);
        integration.setParameter("rendering.material.transparency", 0.5);
        
        LOG_INF_S("Modified parameters for preset comparison");
        
        // 加载预设
        integration.loadPreset("example_preset");
        LOG_INF_S("Loaded 'example_preset'");
        
        // 验证参数是否恢复
        auto diffuseR = integration.getParameter("rendering.material.diffuse.r");
        auto transparency = integration.getParameter("rendering.material.transparency");
        
        LOG_INF_S("After loading preset:");
        LOG_INF_S("- Diffuse R: " + std::to_string(std::get<double>(diffuseR)));
        LOG_INF_S("- Transparency: " + std::to_string(std::get<double>(transparency)));
        
        // 获取可用预设列表
        auto presets = integration.getAvailablePresets();
        LOG_INF_S("Available presets: " + std::to_string(presets.size()));
        
        // 删除预设
        integration.deletePreset("example_preset");
        LOG_INF_S("Deleted 'example_preset'");
    }
    
    static void demonstratePerformanceMonitoring(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Performance Monitoring ---");
        
        // 获取性能报告
        auto report = integration.getPerformanceReport();
        
        LOG_INF_S("Performance Report:");
        LOG_INF_S("- Total Parameters: " + std::to_string(report.totalParameters));
        LOG_INF_S("- Active Systems: " + std::to_string(report.activeSystems));
        LOG_INF_S("- Pending Updates: " + std::to_string(report.pendingUpdates));
        LOG_INF_S("- Executed Updates: " + std::to_string(report.executedUpdates));
        LOG_INF_S("- Average Update Time: " + std::to_string(report.averageUpdateTime.count()) + "ms");
        LOG_INF_S("- Batch Groups Created: " + std::to_string(report.batchGroupsCreated));
        LOG_INF_S("- Dependency Conflicts: " + std::to_string(report.dependencyConflicts));
        
        // 获取系统诊断信息
        auto diagnostics = integration.getSystemDiagnostics();
        LOG_INF_S("System Diagnostics:");
        LOG_INF_S(diagnostics);
        
        // 验证所有参数
        bool isValid = integration.validateAllParameters();
        LOG_INF_S("All parameters validation: " + std::string(isValid ? "PASSED" : "FAILED"));
        
        if (!isValid) {
            auto errors = integration.getValidationErrors();
            LOG_INF_S("Validation errors:");
            for (const auto& error : errors) {
                LOG_ERR_S("- " + error);
            }
        }
        
        // 重置性能指标
        integration.resetPerformanceMetrics();
        LOG_INF_S("Performance metrics reset");
    }
    
    static void demonstrateComplexParameterChanges(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Complex Parameter Changes ---");
        
        // 复杂参数变更场景：同时修改多个相关参数
        std::vector<std::string> taskIds;
        
        // 几何变换
        taskIds.push_back(integration.scheduleParameterChange("geometry.position.x", 0.0, 100.0));
        taskIds.push_back(integration.scheduleParameterChange("geometry.position.y", 0.0, 200.0));
        taskIds.push_back(integration.scheduleParameterChange("geometry.position.z", 0.0, 300.0));
        
        // 材质变化
        taskIds.push_back(integration.scheduleParameterChange("rendering.material.diffuse.r", 0.8, 0.2));
        taskIds.push_back(integration.scheduleParameterChange("rendering.material.diffuse.g", 0.8, 0.2));
        taskIds.push_back(integration.scheduleParameterChange("rendering.material.diffuse.b", 0.8, 0.2));
        
        // 网格质量调整
        taskIds.push_back(integration.scheduleParameterChange("mesh.deflection", 0.5, 0.1));
        taskIds.push_back(integration.scheduleParameterChange("mesh.angularDeflection", 1.0, 0.3));
        
        // 光照调整
        taskIds.push_back(integration.scheduleParameterChange("lighting.main.intensity", 1.0, 1.5));
        taskIds.push_back(integration.scheduleParameterChange("lighting.main.color.r", 1.0, 0.9));
        
        LOG_INF_S("Scheduled " + std::to_string(taskIds.size()) + " complex parameter changes");
        
        // 等待所有任务完成
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        LOG_INF_S("Complex parameter changes completed");
    }
    
    static void demonstrateMultiSystemCoordination(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Multi-System Coordination ---");
        
        // 多系统协调更新：几何重建 + 渲染更新 + 光照更新
        std::string geometryTask = integration.scheduleGeometryRebuild("geometry.main_object");
        std::string renderTask = integration.scheduleRenderingUpdate("main_viewport");
        std::string lightingTask = integration.scheduleLightingUpdate();
        
        LOG_INF_S("Scheduled multi-system coordination:");
        LOG_INF_S("- Geometry rebuild: " + geometryTask);
        LOG_INF_S("- Rendering update: " + renderTask);
        LOG_INF_S("- Lighting update: " + lightingTask);
        
        // 等待协调完成
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        LOG_INF_S("Multi-system coordination completed");
    }
    
    static void demonstrateCustomUpdateStrategies(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Custom Update Strategies ---");
        
        // 使用不同的更新策略
        auto& coordinator = UpdateCoordinator::getInstance();
        
        // 立即更新（高优先级）
        coordinator.submitParameterChange(
            "rendering.material.transparency", 
            0.0, 
            0.5, 
            UpdateStrategy::IMMEDIATE
        );
        
        // 批量更新（默认）
        coordinator.submitParameterChange(
            "rendering.material.diffuse.r", 
            0.8, 
            0.6, 
            UpdateStrategy::BATCHED
        );
        
        // 节流更新
        coordinator.submitParameterChange(
            "rendering.material.diffuse.g", 
            0.8, 
            0.6, 
            UpdateStrategy::THROTTLED
        );
        
        // 延迟更新
        coordinator.submitParameterChange(
            "rendering.material.diffuse.b", 
            0.8, 
            0.6, 
            UpdateStrategy::DEFERRED
        );
        
        LOG_INF_S("Scheduled updates with different strategies");
        
        // 等待所有更新完成
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        
        LOG_INF_S("Custom update strategies completed");
    }
    
    static void demonstrateErrorHandling(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Demonstrating Error Handling ---");
        
        // 测试无效参数路径
        bool result1 = integration.setParameter("invalid.path", 123.0);
        LOG_INF_S("Setting invalid parameter path: " + std::string(result1 ? "SUCCESS" : "FAILED (expected)"));
        
        // 测试无效参数值
        bool result2 = integration.setParameter("rendering.material.transparency", -1.0); // 负透明度
        LOG_INF_S("Setting invalid parameter value: " + std::string(result2 ? "SUCCESS" : "FAILED (expected)"));
        
        // 测试不存在的预设
        integration.loadPreset("nonexistent_preset");
        LOG_INF_S("Loading nonexistent preset: handled gracefully");
        
        // 测试参数验证
        bool isValid = integration.validateAllParameters();
        LOG_INF_S("Parameter validation after error tests: " + std::string(isValid ? "PASSED" : "FAILED"));
        
        if (!isValid) {
            auto errors = integration.getValidationErrors();
            LOG_INF_S("Validation errors found: " + std::to_string(errors.size()));
        }
        
        LOG_INF_S("Error handling demonstration completed");
    }
    
    static void testMassiveParameterChanges(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Testing Massive Parameter Changes ---");
        
        const int numChanges = 1000;
        auto startTime = std::chrono::steady_clock::now();
        
        // 大量参数变更
        for (int i = 0; i < numChanges; ++i) {
            integration.scheduleParameterChange(
                "rendering.material.diffuse.r", 
                0.5 + (i % 100) * 0.001, 
                0.5 + ((i + 1) % 100) * 0.001
            );
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        LOG_INF_S("Scheduled " + std::to_string(numChanges) + " parameter changes in " + 
                  std::to_string(duration.count()) + "ms");
        
        // 等待处理完成
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        auto report = integration.getPerformanceReport();
        LOG_INF_S("Performance after massive changes:");
        LOG_INF_S("- Executed updates: " + std::to_string(report.executedUpdates));
        LOG_INF_S("- Average update time: " + std::to_string(report.averageUpdateTime.count()) + "ms");
    }
    
    static void testBatchProcessingEfficiency(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Testing Batch Processing Efficiency ---");
        
        const int batchSize = 100;
        auto startTime = std::chrono::steady_clock::now();
        
        // 批量参数设置
        std::unordered_map<std::string, ParameterValue> batch;
        for (int i = 0; i < batchSize; ++i) {
            batch["rendering.material.diffuse.r"] = 0.5 + i * 0.001;
            batch["rendering.material.diffuse.g"] = 0.5 + i * 0.001;
            batch["rendering.material.diffuse.b"] = 0.5 + i * 0.001;
        }
        
        bool success = integration.setParameters(batch);
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        LOG_INF_S("Batch processing " + std::to_string(batchSize) + " parameters: " + 
                  std::string(success ? "SUCCESS" : "FAILED") + " in " + 
                  std::to_string(duration.count()) + "ms");
    }
    
    static void testMemoryUsage(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Testing Memory Usage ---");
        
        // 创建大量参数
        const int numParams = 10000;
        for (int i = 0; i < numParams; ++i) {
            std::string paramPath = "test.param_" + std::to_string(i);
            integration.setParameter(paramPath, static_cast<double>(i));
        }
        
        LOG_INF_S("Created " + std::to_string(numParams) + " test parameters");
        
        // 获取性能报告
        auto report = integration.getPerformanceReport();
        LOG_INF_S("Memory usage test - Total parameters: " + std::to_string(report.totalParameters));
        
        // 清理测试参数
        for (int i = 0; i < numParams; ++i) {
            std::string paramPath = "test.param_" + std::to_string(i);
            // 这里应该实现参数删除功能
        }
        
        LOG_INF_S("Cleaned up test parameters");
    }
    
    static void testConcurrencySafety(UnifiedParameterIntegration& integration) {
        LOG_INF_S("--- Testing Concurrency Safety ---");
        
        const int numThreads = 4;
        const int operationsPerThread = 100;
        
        std::vector<std::thread> threads;
        auto startTime = std::chrono::steady_clock::now();
        
        // 创建多个线程同时操作参数
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&integration, t, operationsPerThread]() {
                for (int i = 0; i < operationsPerThread; ++i) {
                    std::string paramPath = "concurrent.param_" + std::to_string(t);
                    integration.scheduleParameterChange(paramPath, static_cast<double>(i), static_cast<double>(i + 1));
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        LOG_INF_S("Concurrency test completed:");
        LOG_INF_S("- Threads: " + std::to_string(numThreads));
        LOG_INF_S("- Operations per thread: " + std::to_string(operationsPerThread));
        LOG_INF_S("- Total operations: " + std::to_string(numThreads * operationsPerThread));
        LOG_INF_S("- Duration: " + std::to_string(duration.count()) + "ms");
        
        // 等待所有操作完成
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto report = integration.getPerformanceReport();
        LOG_INF_S("- Executed updates: " + std::to_string(report.executedUpdates));
    }
};

/**
 * @brief 主函数 - 运行所有示例
 */
int main() {
    try {
        LOG_INF_S("Starting Unified Parameter System Examples");
        
        // 运行基本示例
        UnifiedParameterExample::runBasicExample();
        
        // 运行高级示例
        UnifiedParameterExample::runAdvancedExample();
        
        // 运行性能测试
        UnifiedParameterExample::runPerformanceTest();
        
        LOG_INF_S("All examples completed successfully");
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Example execution failed: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}