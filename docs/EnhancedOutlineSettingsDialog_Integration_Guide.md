# EnhancedOutlineSettingsDialog 与预览视口集成指南

## 概述

EnhancedOutlineSettingsDialog 是一个完全集成的轮廓设置对话框，提供了对 EnhancedOutlinePass 的完整预览和控制功能。它包含两个预览视口：Legacy 和 Enhanced，支持实时参数调整和性能监控。

## 主要特性

### 1. 双预览系统
- **Legacy Preview**: 使用原始的 OutlinePreviewCanvas
- **Enhanced Preview**: 使用新的 EnhancedOutlinePreviewCanvas
- **实时切换**: 可以在两种模式间无缝切换

### 2. 完整的参数控制
- **基础参数**: 深度、法线、颜色权重和阈值
- **高级参数**: 发光效果、平滑因子、自适应阈值
- **性能参数**: 降采样、早期剔除、多采样抗锯齿
- **选择参数**: 选中、悬停、普通对象的轮廓设置
- **颜色参数**: 所有轮廓颜色的实时调整

### 3. 性能监控
- **实时帧率**: 显示当前帧时间和平均帧时间
- **性能模式**: 平衡、性能、质量三种模式
- **性能报告**: 详细的性能统计信息

### 4. 交互式预览
- **鼠标控制**: 左键拖拽旋转，双击选择对象
- **自动旋转**: 可选的自动旋转动画
- **多模型**: 支持多种预览模型（立方体、球体、圆柱体等）

## 使用方法

### 基本集成

```cpp
// 在现有的轮廓设置命令中集成
#include "ui/EnhancedOutlineSettingsDialog.h"

void ToggleOutlineSettingsCommand::execute() {
    // 获取当前参数
    ImageOutlineParams legacyParams = m_outlineManager->getLegacyParams();
    EnhancedOutlineParams enhancedParams = m_outlineManager->getEnhancedParams();
    
    // 创建增强对话框
    EnhancedOutlineSettingsDialog dialog(GetParent(), legacyParams, enhancedParams);
    
    if (dialog.ShowModal() == wxID_OK) {
        // 获取用户设置的参数
        OutlinePassManager::OutlineMode selectedMode = dialog.getSelectedMode();
        ImageOutlineParams newLegacyParams = dialog.getLegacyParams();
        EnhancedOutlineParams newEnhancedParams = dialog.getEnhancedParams();
        
        // 应用设置
        m_outlineManager->setOutlineMode(selectedMode);
        m_outlineManager->setLegacyParams(newLegacyParams);
        m_outlineManager->setEnhancedParams(newEnhancedParams);
        
        // 刷新场景
        m_outlineManager->refresh();
    }
}
```

### 与现有系统集成

```cpp
// 在 SceneManager 中集成
class SceneManager {
private:
    std::unique_ptr<OutlinePassManager> m_outlineManager;
    
public:
    void showOutlineSettings() {
        if (!m_outlineManager) {
            initializeOutlineSystem();
        }
        
        ImageOutlineParams legacyParams = m_outlineManager->getLegacyParams();
        EnhancedOutlineParams enhancedParams = m_outlineManager->getEnhancedParams();
        
        EnhancedOutlineSettingsDialog dialog(GetParent(), legacyParams, enhancedParams);
        
        if (dialog.ShowModal() == wxID_OK) {
            // 应用设置
            OutlinePassManager::OutlineMode mode = dialog.getSelectedMode();
            m_outlineManager->setOutlineMode(mode);
            
            if (mode == OutlinePassManager::OutlineMode::Legacy) {
                m_outlineManager->setLegacyParams(dialog.getLegacyParams());
            } else {
                m_outlineManager->setEnhancedParams(dialog.getEnhancedParams());
            }
            
            // 保存设置到配置文件
            saveOutlineSettings();
        }
    }
    
private:
    void initializeOutlineSystem() {
        m_outlineManager = std::make_unique<OutlinePassManager>(this, m_geometryRoot);
        
        // 加载保存的设置
        loadOutlineSettings();
        
        // 启用轮廓渲染
        m_outlineManager->setEnabled(true);
    }
    
    void loadOutlineSettings() {
        // 从配置文件加载设置
        EnhancedOutlineParams params;
        params.depthWeight = getConfigFloat("outline.depthWeight", 1.5f);
        params.normalWeight = getConfigFloat("outline.normalWeight", 1.0f);
        params.colorWeight = getConfigFloat("outline.colorWeight", 0.3f);
        params.edgeIntensity = getConfigFloat("outline.edgeIntensity", 1.0f);
        params.thickness = getConfigFloat("outline.thickness", 1.5f);
        params.glowIntensity = getConfigFloat("outline.glowIntensity", 0.2f);
        
        m_outlineManager->setEnhancedParams(params);
    }
    
    void saveOutlineSettings() {
        if (m_outlineManager) {
            EnhancedOutlineParams params = m_outlineManager->getEnhancedParams();
            
            setConfigFloat("outline.depthWeight", params.depthWeight);
            setConfigFloat("outline.normalWeight", params.normalWeight);
            setConfigFloat("outline.colorWeight", params.colorWeight);
            setConfigFloat("outline.edgeIntensity", params.edgeIntensity);
            setConfigFloat("outline.thickness", params.thickness);
            setConfigFloat("outline.glowIntensity", params.glowIntensity);
        }
    }
};
```

### 菜单集成

```cpp
// 在菜单系统中添加轮廓设置
void MainFrame::createMenuBar() {
    wxMenuBar* menuBar = new wxMenuBar();
    
    // 视图菜单
    wxMenu* viewMenu = new wxMenu();
    viewMenu->Append(ID_OUTLINE_SETTINGS, "Outline Settings...", "Configure outline rendering parameters");
    
    menuBar->Append(viewMenu, "&View");
    SetMenuBar(menuBar);
    
    // 绑定事件
    Bind(wxEVT_MENU, &MainFrame::onOutlineSettings, this, ID_OUTLINE_SETTINGS);
}

void MainFrame::onOutlineSettings(wxCommandEvent& event) {
    if (m_sceneManager) {
        m_sceneManager->showOutlineSettings();
    }
}
```

### 工具栏集成

```cpp
// 在工具栏中添加轮廓设置按钮
void MainFrame::createToolBar() {
    wxToolBar* toolBar = CreateToolBar();
    
    // 轮廓设置按钮
    wxBitmap outlineIcon = wxArtProvider::GetBitmap(wxART_SETTINGS, wxART_TOOLBAR);
    toolBar->AddTool(ID_OUTLINE_SETTINGS, "Outline Settings", outlineIcon, 
                    "Configure outline rendering parameters");
    
    toolBar->Realize();
    
    // 绑定事件
    Bind(wxEVT_TOOL, &MainFrame::onOutlineSettings, this, ID_OUTLINE_SETTINGS);
}
```

## 高级功能

### 1. 自定义预览模型

```cpp
// 添加自定义预览模型
void EnhancedOutlineSettingsDialog::addCustomPreviewModel() {
    if (m_enhancedPreviewCanvas) {
        // 创建自定义模型
        SoSeparator* customModel = new SoSeparator;
        customModel->ref();
        
        // 添加几何体
        SoCube* cube = new SoCube;
        cube->width = 1.0f;
        cube->height = 1.0f;
        cube->depth = 1.0f;
        customModel->addChild(cube);
        
        // 添加到预览画布
        m_enhancedPreviewCanvas->addPreviewModel("Custom Model", customModel);
    }
}
```

### 2. 性能优化建议

```cpp
// 根据性能自动调整设置
void EnhancedOutlineSettingsDialog::autoOptimizePerformance() {
    if (m_enhancedPreviewCanvas) {
        auto perfInfo = m_enhancedPreviewCanvas->getPerformanceInfo();
        
        if (perfInfo.averageFrameTime > 16.67f) { // 低于60FPS
            // 自动启用性能模式
            m_performanceModeChoice->SetSelection(1); // Performance mode
            onPerformanceModeChanged(wxCommandEvent());
            
            // 显示建议
            wxMessageBox("Performance mode enabled due to low frame rate.", 
                        "Auto Optimization", wxOK | wxICON_INFORMATION);
        }
    }
}
```

### 3. 预设管理

```cpp
// 预设管理功能
class OutlinePresetManager {
public:
    struct Preset {
        wxString name;
        EnhancedOutlineParams params;
        wxString description;
    };
    
    void savePreset(const wxString& name, const EnhancedOutlineParams& params) {
        Preset preset;
        preset.name = name;
        preset.params = params;
        preset.description = "Custom preset";
        
        m_presets[name] = preset;
        saveToFile();
    }
    
    void loadPreset(const wxString& name, EnhancedOutlineParams& params) {
        auto it = m_presets.find(name);
        if (it != m_presets.end()) {
            params = it->second.params;
        }
    }
    
    wxArrayString getPresetNames() const {
        wxArrayString names;
        for (const auto& pair : m_presets) {
            names.Add(pair.first);
        }
        return names;
    }
    
private:
    std::map<wxString, Preset> m_presets;
    
    void saveToFile() {
        // 保存预设到文件
    }
    
    void loadFromFile() {
        // 从文件加载预设
    }
};
```

### 4. 实时协作

```cpp
// 支持实时参数同步
class OutlineSettingsSync {
public:
    void broadcastSettings(const EnhancedOutlineParams& params) {
        // 广播设置到其他客户端
        wxString json = paramsToJson(params);
        m_networkManager->broadcast("outline_settings", json);
    }
    
    void receiveSettings(const wxString& json) {
        // 接收其他客户端的设置
        EnhancedOutlineParams params = jsonToParams(json);
        
        // 更新本地设置
        if (m_dialog) {
            m_dialog->setEnhancedParams(params);
        }
    }
    
private:
    wxString paramsToJson(const EnhancedOutlineParams& params) {
        // 将参数转换为JSON
        wxJSONValue json;
        json["depthWeight"] = params.depthWeight;
        json["normalWeight"] = params.normalWeight;
        json["colorWeight"] = params.colorWeight;
        json["edgeIntensity"] = params.edgeIntensity;
        json["thickness"] = params.thickness;
        json["glowIntensity"] = params.glowIntensity;
        
        wxJSONWriter writer;
        wxString result;
        writer.Write(json, result);
        return result;
    }
    
    EnhancedOutlineParams jsonToParams(const wxString& json) {
        // 将JSON转换为参数
        EnhancedOutlineParams params;
        
        wxJSONValue root;
        wxJSONReader reader;
        if (reader.Parse(json, &root) == 0) {
            params.depthWeight = root["depthWeight"].AsDouble();
            params.normalWeight = root["normalWeight"].AsDouble();
            params.colorWeight = root["colorWeight"].AsDouble();
            params.edgeIntensity = root["edgeIntensity"].AsDouble();
            params.thickness = root["thickness"].AsDouble();
            params.glowIntensity = root["glowIntensity"].AsDouble();
        }
        
        return params;
    }
};
```

## 调试和故障排除

### 常见问题

#### 1. 预览不显示
- 检查 OpenGL 上下文是否正确初始化
- 确认 EnhancedOutlinePass 是否正确创建
- 验证场景图结构是否正确

#### 2. 性能问题
- 启用性能模式：`setPerformanceMode(true)`
- 增加降采样因子：`setDownsampleFactor(2)`
- 启用早期剔除：`setEarlyCullingEnabled(true)`

#### 3. 参数不生效
- 确认 `updatePreview()` 被正确调用
- 检查参数范围是否在有效范围内
- 验证着色器参数是否正确传递

### 调试工具

```cpp
// 调试信息输出
void EnhancedOutlineSettingsDialog::enableDebugMode() {
    if (m_enhancedPreviewCanvas) {
        m_enhancedPreviewCanvas->setDebugMode(OutlineDebugMode::ShowEdgeMask);
    }
    
    // 输出调试信息
    LOG_INF("Debug mode enabled", "EnhancedOutlineSettingsDialog");
    
    // 显示调试信息窗口
    wxDialog* debugDialog = new wxDialog(this, wxID_ANY, "Debug Information");
    wxTextCtrl* debugText = new wxTextCtrl(debugDialog, wxID_ANY, "", 
                                          wxDefaultPosition, wxSize(400, 300), 
                                          wxTE_MULTILINE | wxTE_READONLY);
    
    wxString debugInfo = wxString::Format(
        "Current Parameters:\n"
        "Depth Weight: %.2f\n"
        "Normal Weight: %.2f\n"
        "Color Weight: %.2f\n"
        "Edge Intensity: %.2f\n"
        "Thickness: %.2f\n"
        "Glow Intensity: %.2f\n",
        m_dialogParams.depthWeight,
        m_dialogParams.normalWeight,
        m_dialogParams.colorWeight,
        m_dialogParams.edgeIntensity,
        m_dialogParams.thickness,
        m_dialogParams.glowIntensity
    );
    
    debugText->SetValue(debugInfo);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(debugText, 1, wxEXPAND | wxALL, 10);
    debugDialog->SetSizer(sizer);
    debugDialog->ShowModal();
}
```

## 总结

EnhancedOutlineSettingsDialog 提供了：

1. **完整的预览系统**: 支持 Legacy 和 Enhanced 两种模式
2. **实时参数调整**: 所有参数都可以实时预览效果
3. **性能监控**: 实时显示性能信息和优化建议
4. **交互式预览**: 支持鼠标控制和对象选择
5. **完整的集成**: 与现有系统无缝集成

这个集成方案为你的 3D 应用提供了专业级的轮廓设置界面，用户可以直观地调整所有参数并实时看到效果，大大提升了用户体验。