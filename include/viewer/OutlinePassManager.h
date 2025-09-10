#pragma once

#include "viewer/EnhancedOutlinePass.h"
#include "viewer/ImageOutlinePass.h"
#include <memory>
#include <vector>

class SceneManager;
class SoSeparator;
class SoSelection;

class OutlinePassManager {
public:
    enum class OutlineMode {
        Disabled,   // 禁用轮廓渲染
        Legacy,     // 使用原始的ImageOutlinePass
        Enhanced    // 使用增强的EnhancedOutlinePass
    };

    OutlinePassManager(SceneManager* sceneManager, SoSeparator* captureRoot);
    ~OutlinePassManager();

    // 核心功能
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // 模式切换
    void setOutlineMode(OutlineMode mode);
    OutlineMode getOutlineMode() const { return m_currentMode; }

    // 参数管理 (兼容两种模式)
    void setLegacyParams(const ImageOutlineParams& params);
    void setEnhancedParams(const EnhancedOutlineParams& params);
    
    ImageOutlineParams getLegacyParams() const;
    EnhancedOutlineParams getEnhancedParams() const;

    // 选择管理
    void setSelectionRoot(SoSelection* selectionRoot);
    void setSelectedObjects(const std::vector<int>& objectIds);
    void setHoveredObject(int objectId);
    void clearSelection();
    void clearHover();

    // 性能优化
    void setPerformanceMode(bool enabled);
    void setQualityMode(bool enabled);
    void setBalancedMode();

    // 调试功能
    void setDebugMode(OutlineDebugMode mode);
    void enableDebugVisualization(bool enabled);

    // 刷新和更新
    void refresh();
    void forceUpdate();

    // 统计信息
    struct PerformanceStats {
        float renderTime{ 0.0f };
        int textureMemoryUsage{ 0 };
        int shaderCompileTime{ 0 };
        bool isOptimized{ false };
    };
    
    PerformanceStats getPerformanceStats() const;

private:
    void initializePasses();
    void migrateLegacyToEnhanced();
    void migrateEnhancedToLegacy();
    void updatePerformanceSettings();
    void logPerformanceInfo();

    SceneManager* m_sceneManager;
    SoSeparator* m_captureRoot;
    SoSelection* m_selectionRoot;

    OutlineMode m_currentMode{ OutlineMode::Disabled };
    bool m_enabled{ false };
    bool m_performanceMode{ false };
    bool m_qualityMode{ false };
    bool m_debugVisualization{ false };

    // 两种轮廓渲染器
    std::unique_ptr<ImageOutlinePass> m_legacyPass;
    std::unique_ptr<EnhancedOutlinePass> m_enhancedPass;

    // 参数缓存
    ImageOutlineParams m_legacyParams;
    EnhancedOutlineParams m_enhancedParams;

    // 性能统计
    PerformanceStats m_performanceStats;
};