# Enhanced OutlinePass 集成示例

## 1. 基本集成示例

### 在 SceneManager 中集成

```cpp
// SceneManager.h
#include "viewer/OutlinePassManager.h"

class SceneManager {
private:
    std::unique_ptr<OutlinePassManager> m_outlineManager;
    
public:
    void initializeOutlineSystem();
    void setOutlineEnabled(bool enabled);
    void setOutlineMode(OutlinePassManager::OutlineMode mode);
    void updateOutlineParams(const EnhancedOutlineParams& params);
};
```

```cpp
// SceneManager.cpp
#include "SceneManager.h"

void SceneManager::initializeOutlineSystem() {
    if (!m_outlineManager) {
        m_outlineManager = std::make_unique<OutlinePassManager>(this, m_geometryRoot);
        
        // 设置默认参数
        EnhancedOutlineParams params;
        params.depthWeight = 1.5f;
        params.normalWeight = 1.0f;
        params.colorWeight = 0.3f;
        params.edgeIntensity = 1.0f;
        params.thickness = 1.5f;
        params.glowIntensity = 0.2f;
        
        m_outlineManager->setEnhancedParams(params);
        m_outlineManager->setEnabled(true);
        
        LOG_INF("Outline system initialized", "SceneManager");
    }
}

void SceneManager::setOutlineEnabled(bool enabled) {
    if (m_outlineManager) {
        m_outlineManager->setEnabled(enabled);
    }
}

void SceneManager::setOutlineMode(OutlinePassManager::OutlineMode mode) {
    if (m_outlineManager) {
        m_outlineManager->setOutlineMode(mode);
    }
}

void SceneManager::updateOutlineParams(const EnhancedOutlineParams& params) {
    if (m_outlineManager) {
        m_outlineManager->setEnhancedParams(params);
    }
}
```

## 2. UI 集成示例

### 轮廓设置对话框增强

```cpp
// EnhancedOutlineSettingsDialog.h
#include <wx/wx.h>
#include "viewer/EnhancedOutlinePass.h"

class EnhancedOutlineSettingsDialog : public wxDialog {
public:
    EnhancedOutlineSettingsDialog(wxWindow* parent, OutlinePassManager* outlineManager);
    
private:
    void createControls();
    void onParameterChanged(wxCommandEvent& event);
    void onModeChanged(wxCommandEvent& event);
    void onPerformanceModeChanged(wxCommandEvent& event);
    void onDebugModeChanged(wxCommandEvent& event);
    void updatePreview();
    
    OutlinePassManager* m_outlineManager;
    
    // 控件
    wxRadioBox* m_modeRadioBox;
    wxCheckBox* m_performanceModeCheck;
    wxCheckBox* m_qualityModeCheck;
    wxCheckBox* m_debugVisualizationCheck;
    
    // 参数控件
    wxSlider* m_depthWeightSlider;
    wxSlider* m_normalWeightSlider;
    wxSlider* m_colorWeightSlider;
    wxSlider* m_edgeIntensitySlider;
    wxSlider* m_thicknessSlider;
    wxSlider* m_glowIntensitySlider;
    wxSlider* m_glowRadiusSlider;
    wxSlider* m_smoothingFactorSlider;
    
    // 标签
    wxStaticText* m_depthWeightLabel;
    wxStaticText* m_normalWeightLabel;
    wxStaticText* m_colorWeightLabel;
    wxStaticText* m_edgeIntensityLabel;
    wxStaticText* m_thicknessLabel;
    wxStaticText* m_glowIntensityLabel;
    wxStaticText* m_glowRadiusLabel;
    wxStaticText* m_smoothingFactorLabel;
    
    // 预览
    OutlinePreviewCanvas* m_previewCanvas;
};
```

```cpp
// EnhancedOutlineSettingsDialog.cpp
#include "EnhancedOutlineSettingsDialog.h"

EnhancedOutlineSettingsDialog::EnhancedOutlineSettingsDialog(
    wxWindow* parent, OutlinePassManager* outlineManager)
    : wxDialog(parent, wxID_ANY, "Enhanced Outline Settings", wxDefaultPosition, wxSize(800, 600)),
      m_outlineManager(outlineManager) {
    
    createControls();
    updatePreview();
}

void EnhancedOutlineSettingsDialog::createControls() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // 左侧控制面板
    wxPanel* controlPanel = new wxPanel(this);
    wxBoxSizer* controlSizer = new wxBoxSizer(wxVERTICAL);
    
    // 模式选择
    wxString modeChoices[] = {"Legacy Mode", "Enhanced Mode"};
    m_modeRadioBox = new wxRadioBox(controlPanel, wxID_ANY, "Outline Mode", 
                                   wxDefaultPosition, wxDefaultSize, 2, modeChoices);
    controlSizer->Add(m_modeRadioBox, 0, wxALL | wxEXPAND, 5);
    
    // 性能模式
    wxStaticBoxSizer* performanceSizer = new wxStaticBoxSizer(wxVERTICAL, controlPanel, "Performance");
    m_performanceModeCheck = new wxCheckBox(controlPanel, wxID_ANY, "Performance Mode");
    m_qualityModeCheck = new wxCheckBox(controlPanel, wxID_ANY, "Quality Mode");
    m_debugVisualizationCheck = new wxCheckBox(controlPanel, wxID_ANY, "Debug Visualization");
    
    performanceSizer->Add(m_performanceModeCheck, 0, wxALL, 2);
    performanceSizer->Add(m_qualityModeCheck, 0, wxALL, 2);
    performanceSizer->Add(m_debugVisualizationCheck, 0, wxALL, 2);
    controlSizer->Add(performanceSizer, 0, wxALL | wxEXPAND, 5);
    
    // 参数控制
    wxStaticBoxSizer* paramSizer = new wxStaticBoxSizer(wxVERTICAL, controlPanel, "Parameters");
    
    // 深度权重
    wxBoxSizer* depthWeightSizer = new wxBoxSizer(wxHORIZONTAL);
    depthWeightSizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Depth Weight:"), 0, wxALIGN_CENTER_VERTICAL);
    m_depthWeightSlider = new wxSlider(controlPanel, wxID_ANY, 150, 0, 300, wxDefaultPosition, wxSize(200, -1));
    m_depthWeightLabel = new wxStaticText(controlPanel, wxID_ANY, "1.50");
    depthWeightSizer->Add(m_depthWeightSlider, 1, wxEXPAND);
    depthWeightSizer->Add(m_depthWeightLabel, 0, wxALIGN_CENTER_VERTICAL);
    paramSizer->Add(depthWeightSizer, 0, wxALL | wxEXPAND, 2);
    
    // 法线权重
    wxBoxSizer* normalWeightSizer = new wxBoxSizer(wxHORIZONTAL);
    normalWeightSizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Normal Weight:"), 0, wxALIGN_CENTER_VERTICAL);
    m_normalWeightSlider = new wxSlider(controlPanel, wxID_ANY, 100, 0, 300, wxDefaultPosition, wxSize(200, -1));
    m_normalWeightLabel = new wxStaticText(controlPanel, wxID_ANY, "1.00");
    normalWeightSizer->Add(m_normalWeightSlider, 1, wxEXPAND);
    normalWeightSizer->Add(m_normalWeightLabel, 0, wxALIGN_CENTER_VERTICAL);
    paramSizer->Add(normalWeightSizer, 0, wxALL | wxEXPAND, 2);
    
    // 颜色权重
    wxBoxSizer* colorWeightSizer = new wxBoxSizer(wxHORIZONTAL);
    colorWeightSizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Color Weight:"), 0, wxALIGN_CENTER_VERTICAL);
    m_colorWeightSlider = new wxSlider(controlPanel, wxID_ANY, 30, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_colorWeightLabel = new wxStaticText(controlPanel, wxID_ANY, "0.30");
    colorWeightSizer->Add(m_colorWeightSlider, 1, wxEXPAND);
    colorWeightSizer->Add(m_colorWeightLabel, 0, wxALIGN_CENTER_VERTICAL);
    paramSizer->Add(colorWeightSizer, 0, wxALL | wxEXPAND, 2);
    
    // 边缘强度
    wxBoxSizer* intensitySizer = new wxBoxSizer(wxHORIZONTAL);
    intensitySizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Edge Intensity:"), 0, wxALIGN_CENTER_VERTICAL);
    m_edgeIntensitySlider = new wxSlider(controlPanel, wxID_ANY, 100, 0, 200, wxDefaultPosition, wxSize(200, -1));
    m_edgeIntensityLabel = new wxStaticText(controlPanel, wxID_ANY, "1.00");
    intensitySizer->Add(m_edgeIntensitySlider, 1, wxEXPAND);
    intensitySizer->Add(m_edgeIntensityLabel, 0, wxALIGN_CENTER_VERTICAL);
    paramSizer->Add(intensitySizer, 0, wxALL | wxEXPAND, 2);
    
    // 厚度
    wxBoxSizer* thicknessSizer = new wxBoxSizer(wxHORIZONTAL);
    thicknessSizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Thickness:"), 0, wxALIGN_CENTER_VERTICAL);
    m_thicknessSlider = new wxSlider(controlPanel, wxID_ANY, 150, 10, 500, wxDefaultPosition, wxSize(200, -1));
    m_thicknessLabel = new wxStaticText(controlPanel, wxID_ANY, "1.50");
    thicknessSizer->Add(m_thicknessSlider, 1, wxEXPAND);
    thicknessSizer->Add(m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL);
    paramSizer->Add(thicknessSizer, 0, wxALL | wxEXPAND, 2);
    
    // 发光强度
    wxBoxSizer* glowIntensitySizer = new wxBoxSizer(wxHORIZONTAL);
    glowIntensitySizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Glow Intensity:"), 0, wxALIGN_CENTER_VERTICAL);
    m_glowIntensitySlider = new wxSlider(controlPanel, wxID_ANY, 20, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_glowIntensityLabel = new wxStaticText(controlPanel, wxID_ANY, "0.20");
    glowIntensitySizer->Add(m_glowIntensitySlider, 1, wxEXPAND);
    glowIntensitySizer->Add(m_glowIntensityLabel, 0, wxALIGN_CENTER_VERTICAL);
    paramSizer->Add(glowIntensitySizer, 0, wxALL | wxEXPAND, 2);
    
    // 发光半径
    wxBoxSizer* glowRadiusSizer = new wxBoxSizer(wxHORIZONTAL);
    glowRadiusSizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Glow Radius:"), 0, wxALIGN_CENTER_VERTICAL);
    m_glowRadiusSlider = new wxSlider(controlPanel, wxID_ANY, 200, 50, 1000, wxDefaultPosition, wxSize(200, -1));
    m_glowRadiusLabel = new wxStaticText(controlPanel, wxID_ANY, "2.00");
    glowRadiusSizer->Add(m_glowRadiusSlider, 1, wxEXPAND);
    glowRadiusSizer->Add(m_glowRadiusLabel, 0, wxALIGN_CENTER_VERTICAL);
    paramSizer->Add(glowRadiusSizer, 0, wxALL | wxEXPAND, 2);
    
    // 平滑因子
    wxBoxSizer* smoothingSizer = new wxBoxSizer(wxHORIZONTAL);
    smoothingSizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Smoothing:"), 0, wxALIGN_CENTER_VERTICAL);
    m_smoothingFactorSlider = new wxSlider(controlPanel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_smoothingFactorLabel = new wxStaticText(controlPanel, wxID_ANY, "0.50");
    smoothingSizer->Add(m_smoothingFactorSlider, 1, wxEXPAND);
    smoothingSizer->Add(m_smoothingFactorLabel, 0, wxALIGN_CENTER_VERTICAL);
    paramSizer->Add(smoothingSizer, 0, wxALL | wxEXPAND, 2);
    
    controlSizer->Add(paramSizer, 1, wxALL | wxEXPAND, 5);
    
    // 按钮
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(controlPanel, wxID_ANY, "Reset"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(controlPanel, wxID_OK, "OK"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(controlPanel, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
    controlSizer->Add(buttonSizer, 0, wxALIGN_CENTER);
    
    controlPanel->SetSizer(controlSizer);
    
    // 右侧预览面板
    wxPanel* previewPanel = new wxPanel(this);
    wxBoxSizer* previewSizer = new wxBoxSizer(wxVERTICAL);
    
    previewSizer->Add(new wxStaticText(previewPanel, wxID_ANY, "Preview"), 0, wxALIGN_CENTER);
    m_previewCanvas = new OutlinePreviewCanvas(previewPanel);
    previewSizer->Add(m_previewCanvas, 1, wxEXPAND);
    
    previewPanel->SetSizer(previewSizer);
    
    // 主布局
    mainSizer->Add(controlPanel, 0, wxEXPAND);
    mainSizer->Add(previewPanel, 1, wxEXPAND);
    
    SetSizer(mainSizer);
    
    // 绑定事件
    Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EnhancedOutlineSettingsDialog::onParameterChanged, this);
    Bind(wxEVT_COMMAND_RADIOBOX_SELECTED, &EnhancedOutlineSettingsDialog::onModeChanged, this);
    Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EnhancedOutlineSettingsDialog::onPerformanceModeChanged, this);
    Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EnhancedOutlineSettingsDialog::onDebugModeChanged, this);
}

void EnhancedOutlineSettingsDialog::onParameterChanged(wxCommandEvent& event) {
    // 更新标签
    m_depthWeightLabel->SetLabel(wxString::Format("%.2f", m_depthWeightSlider->GetValue() / 100.0f));
    m_normalWeightLabel->SetLabel(wxString::Format("%.2f", m_normalWeightSlider->GetValue() / 100.0f));
    m_colorWeightLabel->SetLabel(wxString::Format("%.2f", m_colorWeightSlider->GetValue() / 100.0f));
    m_edgeIntensityLabel->SetLabel(wxString::Format("%.2f", m_edgeIntensitySlider->GetValue() / 100.0f));
    m_thicknessLabel->SetLabel(wxString::Format("%.2f", m_thicknessSlider->GetValue() / 100.0f));
    m_glowIntensityLabel->SetLabel(wxString::Format("%.2f", m_glowIntensitySlider->GetValue() / 100.0f));
    m_glowRadiusLabel->SetLabel(wxString::Format("%.2f", m_glowRadiusSlider->GetValue() / 100.0f));
    m_smoothingFactorLabel->SetLabel(wxString::Format("%.2f", m_smoothingFactorSlider->GetValue() / 100.0f));
    
    // 更新参数
    EnhancedOutlineParams params;
    params.depthWeight = m_depthWeightSlider->GetValue() / 100.0f;
    params.normalWeight = m_normalWeightSlider->GetValue() / 100.0f;
    params.colorWeight = m_colorWeightSlider->GetValue() / 100.0f;
    params.edgeIntensity = m_edgeIntensitySlider->GetValue() / 100.0f;
    params.thickness = m_thicknessSlider->GetValue() / 100.0f;
    params.glowIntensity = m_glowIntensitySlider->GetValue() / 100.0f;
    params.glowRadius = m_glowRadiusSlider->GetValue() / 100.0f;
    params.smoothingFactor = m_smoothingFactorSlider->GetValue() / 100.0f;
    
    if (m_outlineManager) {
        m_outlineManager->setEnhancedParams(params);
    }
    
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onModeChanged(wxCommandEvent& event) {
    if (m_outlineManager) {
        OutlinePassManager::OutlineMode mode = 
            (m_modeRadioBox->GetSelection() == 0) ? 
            OutlinePassManager::OutlineMode::Legacy : 
            OutlinePassManager::OutlineMode::Enhanced;
        
        m_outlineManager->setOutlineMode(mode);
    }
    
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onPerformanceModeChanged(wxCommandEvent& event) {
    if (m_outlineManager) {
        m_outlineManager->setPerformanceMode(m_performanceModeCheck->GetValue());
    }
    
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onDebugModeChanged(wxCommandEvent& event) {
    if (m_outlineManager) {
        m_outlineManager->enableDebugVisualization(m_debugVisualizationCheck->GetValue());
    }
    
    updatePreview();
}

void EnhancedOutlineSettingsDialog::updatePreview() {
    if (m_previewCanvas && m_outlineManager) {
        EnhancedOutlineParams params = m_outlineManager->getEnhancedParams();
        m_previewCanvas->updateOutlineParams(params);
    }
}
```

## 3. 命令系统集成

```cpp
// ToggleEnhancedOutlineCommand.h
#include "CommandBase.h"
#include "viewer/OutlinePassManager.h"

class ToggleEnhancedOutlineCommand : public CommandBase {
public:
    ToggleEnhancedOutlineCommand(OutlinePassManager* outlineManager);
    
    void execute() override;
    void undo() override;
    bool canUndo() const override { return true; }
    
private:
    OutlinePassManager* m_outlineManager;
    bool m_previousState;
};
```

```cpp
// ToggleEnhancedOutlineCommand.cpp
#include "ToggleEnhancedOutlineCommand.h"

ToggleEnhancedOutlineCommand::ToggleEnhancedOutlineCommand(OutlinePassManager* outlineManager)
    : m_outlineManager(outlineManager), m_previousState(false) {
    
    if (m_outlineManager) {
        m_previousState = m_outlineManager->isEnabled();
    }
}

void ToggleEnhancedOutlineCommand::execute() {
    if (m_outlineManager) {
        m_outlineManager->setEnabled(!m_outlineManager->isEnabled());
    }
}

void ToggleEnhancedOutlineCommand::undo() {
    if (m_outlineManager) {
        m_outlineManager->setEnabled(m_previousState);
    }
}
```

## 4. 配置文件集成

```cpp
// OutlineConfigManager.h
#include "ConfigManager.h"
#include "viewer/EnhancedOutlinePass.h"

class OutlineConfigManager : public ConfigManager {
public:
    OutlineConfigManager();
    
    void loadOutlineSettings();
    void saveOutlineSettings();
    
    EnhancedOutlineParams getDefaultParams() const;
    SelectionOutlineConfig getDefaultSelectionConfig() const;
    
private:
    void loadLegacySettings();
    void migrateToEnhanced();
    
    EnhancedOutlineParams m_params;
    SelectionOutlineConfig m_selectionConfig;
};
```

```cpp
// OutlineConfigManager.cpp
#include "OutlineConfigManager.h"

void OutlineConfigManager::loadOutlineSettings() {
    // 加载增强参数
    m_params.depthWeight = getFloat("outline.depthWeight", 1.5f);
    m_params.normalWeight = getFloat("outline.normalWeight", 1.0f);
    m_params.colorWeight = getFloat("outline.colorWeight", 0.3f);
    m_params.edgeIntensity = getFloat("outline.edgeIntensity", 1.0f);
    m_params.thickness = getFloat("outline.thickness", 1.5f);
    m_params.glowIntensity = getFloat("outline.glowIntensity", 0.2f);
    m_params.glowRadius = getFloat("outline.glowRadius", 2.0f);
    m_params.smoothingFactor = getFloat("outline.smoothingFactor", 0.5f);
    
    // 加载选择配置
    m_selectionConfig.enableSelectionOutline = getBool("outline.selection.enabled", true);
    m_selectionConfig.enableHoverOutline = getBool("outline.hover.enabled", true);
    m_selectionConfig.selectionIntensity = getFloat("outline.selection.intensity", 1.5f);
    m_selectionConfig.hoverIntensity = getFloat("outline.hover.intensity", 1.0f);
    
    // 检查是否需要从旧版本迁移
    if (getBool("outline.legacyMode", false)) {
        migrateToEnhanced();
    }
}

void OutlineConfigManager::saveOutlineSettings() {
    // 保存增强参数
    setFloat("outline.depthWeight", m_params.depthWeight);
    setFloat("outline.normalWeight", m_params.normalWeight);
    setFloat("outline.colorWeight", m_params.colorWeight);
    setFloat("outline.edgeIntensity", m_params.edgeIntensity);
    setFloat("outline.thickness", m_params.thickness);
    setFloat("outline.glowIntensity", m_params.glowIntensity);
    setFloat("outline.glowRadius", m_params.glowRadius);
    setFloat("outline.smoothingFactor", m_params.smoothingFactor);
    
    // 保存选择配置
    setBool("outline.selection.enabled", m_selectionConfig.enableSelectionOutline);
    setBool("outline.hover.enabled", m_selectionConfig.enableHoverOutline);
    setFloat("outline.selection.intensity", m_selectionConfig.selectionIntensity);
    setFloat("outline.hover.intensity", m_selectionConfig.hoverIntensity);
    
    // 标记为增强模式
    setBool("outline.legacyMode", false);
    setBool("outline.enhancedMode", true);
}

void OutlineConfigManager::migrateToEnhanced() {
    // 从旧版本参数迁移
    float legacyDepthWeight = getFloat("outline.legacy.depthWeight", 1.5f);
    float legacyNormalWeight = getFloat("outline.legacy.normalWeight", 1.0f);
    float legacyEdgeIntensity = getFloat("outline.legacy.edgeIntensity", 1.0f);
    float legacyThickness = getFloat("outline.legacy.thickness", 1.5f);
    
    m_params.depthWeight = legacyDepthWeight;
    m_params.normalWeight = legacyNormalWeight;
    m_params.edgeIntensity = legacyEdgeIntensity;
    m_params.thickness = legacyThickness;
    
    // 设置新参数的默认值
    m_params.colorWeight = 0.3f;
    m_params.glowIntensity = 0.2f;
    m_params.glowRadius = 2.0f;
    m_params.smoothingFactor = 0.5f;
    
    LOG_INF("Migrated outline settings from legacy to enhanced", "OutlineConfigManager");
}
```

## 5. 性能监控集成

```cpp
// OutlinePerformanceMonitor.h
#include <chrono>
#include "viewer/OutlinePassManager.h"

class OutlinePerformanceMonitor {
public:
    OutlinePerformanceMonitor(OutlinePassManager* outlineManager);
    
    void startFrame();
    void endFrame();
    void logPerformanceReport();
    
    struct PerformanceMetrics {
        float averageFrameTime{ 0.0f };
        float minFrameTime{ 0.0f };
        float maxFrameTime{ 0.0f };
        int frameCount{ 0 };
        bool isOptimized{ false };
    };
    
    PerformanceMetrics getMetrics() const;
    
private:
    OutlinePassManager* m_outlineManager;
    std::chrono::high_resolution_clock::time_point m_frameStart;
    std::vector<float> m_frameTimes;
    PerformanceMetrics m_metrics;
};
```

```cpp
// OutlinePerformanceMonitor.cpp
#include "OutlinePerformanceMonitor.h"

OutlinePerformanceMonitor::OutlinePerformanceMonitor(OutlinePassManager* outlineManager)
    : m_outlineManager(outlineManager) {
    
    m_frameTimes.reserve(1000); // 预分配空间
}

void OutlinePerformanceMonitor::startFrame() {
    m_frameStart = std::chrono::high_resolution_clock::now();
}

void OutlinePerformanceMonitor::endFrame() {
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - m_frameStart);
    
    float frameTimeMs = frameDuration.count() / 1000.0f;
    m_frameTimes.push_back(frameTimeMs);
    
    // 保持最近1000帧的数据
    if (m_frameTimes.size() > 1000) {
        m_frameTimes.erase(m_frameTimes.begin());
    }
    
    // 更新统计信息
    if (m_frameTimes.size() > 0) {
        float sum = 0.0f;
        float minTime = m_frameTimes[0];
        float maxTime = m_frameTimes[0];
        
        for (float time : m_frameTimes) {
            sum += time;
            minTime = std::min(minTime, time);
            maxTime = std::max(maxTime, time);
        }
        
        m_metrics.averageFrameTime = sum / m_frameTimes.size();
        m_metrics.minFrameTime = minTime;
        m_metrics.maxFrameTime = maxTime;
        m_metrics.frameCount = m_frameTimes.size();
        
        if (m_outlineManager) {
            auto stats = m_outlineManager->getPerformanceStats();
            m_metrics.isOptimized = stats.isOptimized;
        }
    }
}

void OutlinePerformanceMonitor::logPerformanceReport() {
    LOG_INF(("Outline Performance Report:").c_str(), "OutlinePerformanceMonitor");
    LOG_INF(("  Average Frame Time: " + std::to_string(m_metrics.averageFrameTime) + " ms").c_str(), "OutlinePerformanceMonitor");
    LOG_INF(("  Min Frame Time: " + std::to_string(m_metrics.minFrameTime) + " ms").c_str(), "OutlinePerformanceMonitor");
    LOG_INF(("  Max Frame Time: " + std::to_string(m_metrics.maxFrameTime) + " ms").c_str(), "OutlinePerformanceMonitor");
    LOG_INF(("  Frame Count: " + std::to_string(m_metrics.frameCount)).c_str(), "OutlinePerformanceMonitor");
    LOG_INF(("  Optimized: " + std::string(m_metrics.isOptimized ? "Yes" : "No")).c_str(), "OutlinePerformanceMonitor");
    
    // 性能建议
    if (m_metrics.averageFrameTime > 16.67f) { // 60 FPS threshold
        LOG_WRN("Frame time exceeds 60 FPS threshold. Consider enabling performance mode.", "OutlinePerformanceMonitor");
    }
    
    if (m_metrics.maxFrameTime > 33.33f) { // 30 FPS threshold
        LOG_WRN("Maximum frame time exceeds 30 FPS threshold. Consider reducing quality settings.", "OutlinePerformanceMonitor");
    }
}
```

## 总结

这个集成示例展示了如何将 EnhancedOutlinePass 完整地集成到现有系统中：

1. **SceneManager 集成**: 在场景管理器中初始化和管理轮廓系统
2. **UI 集成**: 创建增强的设置对话框，支持所有新参数
3. **命令系统集成**: 支持撤销/重做的轮廓切换命令
4. **配置管理**: 支持参数保存和从旧版本迁移
5. **性能监控**: 实时监控轮廓渲染性能并提供优化建议

通过这些集成，你可以：
- 无缝升级现有的轮廓系统
- 保持向后兼容性
- 获得更好的性能和视觉效果
- 提供丰富的调试和监控功能