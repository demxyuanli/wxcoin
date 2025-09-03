#include "ui/EnhancedOutlineSettingsDialog.h"
#include "ui/EnhancedOutlinePreviewCanvas.h"
#include <wx/splitter.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/timer.h>
#include <iomanip>
#include <sstream>

// 定时器ID
enum {
    ID_TIMER_UPDATE = 1000
};

EnhancedOutlineSettingsDialog::EnhancedOutlineSettingsDialog(wxWindow* parent, const EnhancedOutlineParams& params)
    : wxDialog(parent, wxID_ANY, "Enhanced Outline Settings", wxDefaultPosition, wxSize(1400, 900)), 
      m_params(params) {
    
    // 创建主分割窗口
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY);
    splitter->SetMinimumPaneSize(500);
    
    // 左侧面板 - 控制面板
    wxPanel* controlPanel = new wxPanel(splitter);
    createControlPanel(controlPanel);
    
    // 右侧面板 - 预览面板
    wxPanel* previewPanel = new wxPanel(splitter);
    createPreviewPanel(previewPanel);
    
    // 设置分割窗口
    splitter->SplitVertically(controlPanel, previewPanel, 550);
    
    // 创建底部按钮
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(splitter, 1, wxEXPAND | wxALL, 5);
    
    // 按钮面板
    wxPanel* buttonPanel = new wxPanel(this);
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // 性能统计标签
    m_statsLabel = new wxStaticText(buttonPanel, wxID_ANY, "Ready");
    buttonSizer->Add(m_statsLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    
    // 按钮
    wxButton* resetBtn = new wxButton(buttonPanel, wxID_ANY, "Reset to Defaults");
    wxButton* okBtn = new wxButton(buttonPanel, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(buttonPanel, wxID_CANCEL, "Cancel");
    
    buttonSizer->Add(resetBtn, 0, wxALL, 5);
    buttonSizer->Add(okBtn, 0, wxALL, 5);
    buttonSizer->Add(cancelBtn, 0, wxALL, 5);
    
    buttonPanel->SetSizer(buttonSizer);
    mainSizer->Add(buttonPanel, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    
    // 绑定事件
    resetBtn->Bind(wxEVT_BUTTON, &EnhancedOutlineSettingsDialog::onResetDefaults, this);
    
    // 创建更新定时器
    m_updateTimer = new wxTimer(this, ID_TIMER_UPDATE);
    m_updateTimer->Start(100); // 100ms更新一次性能统计
    
    Bind(wxEVT_TIMER, &EnhancedOutlineSettingsDialog::onUpdateTimer, this, ID_TIMER_UPDATE);
    
    // 初始更新预览
    updatePreview();
}

EnhancedOutlineSettingsDialog::~EnhancedOutlineSettingsDialog() {
    if (m_updateTimer) {
        m_updateTimer->Stop();
        delete m_updateTimer;
    }
}

void EnhancedOutlineSettingsDialog::createControlPanel(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // 创建笔记本控件用于分组
    wxNotebook* notebook = new wxNotebook(panel, wxID_ANY);
    
    // 基础设置页
    wxPanel* basicPage = new wxPanel(notebook);
    createBasicControls(basicPage);
    notebook->AddPage(basicPage, "Basic Settings");
    
    // 高级设置页
    wxPanel* advancedPage = new wxPanel(notebook);
    createAdvancedControls(advancedPage);
    notebook->AddPage(advancedPage, "Advanced Settings");
    
    // 方法选择页
    wxPanel* methodPage = new wxPanel(notebook);
    createMethodControls(methodPage);
    notebook->AddPage(methodPage, "Rendering Method");
    
    sizer->Add(notebook, 1, wxEXPAND | wxALL, 10);
    panel->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createBasicControls(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // 轮廓开关
    m_enableCheck = new wxCheckBox(panel, wxID_ANY, "Enable Outline");
    m_enableCheck->SetValue(true);
    m_enableCheck->Bind(wxEVT_CHECKBOX, &EnhancedOutlineSettingsDialog::onEnableChange, this);
    sizer->Add(m_enableCheck, 0, wxALL, 10);
    
    sizer->Add(new wxStaticLine(panel), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    // 辅助函数：创建滑块
    auto makeSlider = [&](const wxString& label, int min, int max, int value, 
                         wxSlider** sliderPtr, wxStaticText** labelPtr, const wxString& suffix = "") {
        wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
        
        wxStaticText* nameLabel = new wxStaticText(panel, wxID_ANY, label);
        nameLabel->SetMinSize(wxSize(150, -1));
        box->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        
        *sliderPtr = new wxSlider(panel, wxID_ANY, value, min, max, 
                                 wxDefaultPosition, wxSize(200, -1));
        box->Add(*sliderPtr, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        
        wxString valueStr = wxString::Format("%.3f", value / 100.0) + suffix;
        *labelPtr = new wxStaticText(panel, wxID_ANY, valueStr);
        (*labelPtr)->SetMinSize(wxSize(60, -1));
        box->Add(*labelPtr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        
        sizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        
        (*sliderPtr)->Bind(wxEVT_SLIDER, &EnhancedOutlineSettingsDialog::onSliderChange, this);
    };
    
    // 基础参数滑块
    sizer->AddSpacer(10);
    makeSlider("Edge Intensity", 0, 200, int(m_params.edgeIntensity * 100), 
              &m_intensitySlider, &m_intensityLabel);
    makeSlider("Thickness", 10, 500, int(m_params.thickness * 100), 
              &m_thicknessSlider, &m_thicknessLabel, " px");
    makeSlider("Depth Weight", 0, 200, int(m_params.depthWeight * 100), 
              &m_depthWeightSlider, &m_depthWeightLabel);
    makeSlider("Normal Weight", 0, 200, int(m_params.normalWeight * 100), 
              &m_normalWeightSlider, &m_normalWeightLabel);
    makeSlider("Depth Threshold", 0, 50, int(m_params.depthThreshold * 1000), 
              &m_depthThresholdSlider, &m_depthThresholdLabel);
    makeSlider("Normal Threshold", 0, 200, int(m_params.normalThreshold * 100), 
              &m_normalThresholdSlider, &m_normalThresholdLabel);
    
    // 颜色设置
    sizer->AddSpacer(20);
    wxStaticText* colorTitle = new wxStaticText(panel, wxID_ANY, "Color Settings");
    wxFont font = colorTitle->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
    colorTitle->SetFont(font);
    sizer->Add(colorTitle, 0, wxALL, 10);
    
    // 颜色选择器
    wxFlexGridSizer* colorGrid = new wxFlexGridSizer(3, 3, 5, 10);
    colorGrid->AddGrowableCol(1, 1);
    
    // 轮廓颜色
    colorGrid->Add(new wxStaticText(panel, wxID_ANY, "Outline Color:"), 
                   0, wxALIGN_CENTER_VERTICAL);
    m_outlineColorPicker = new wxColourPickerCtrl(panel, wxID_ANY, wxColour(0, 0, 0));
    m_outlineColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &EnhancedOutlineSettingsDialog::onColorChange, this);
    colorGrid->Add(m_outlineColorPicker, 1, wxEXPAND);
    colorGrid->AddSpacer(0);
    
    // 几何体颜色
    colorGrid->Add(new wxStaticText(panel, wxID_ANY, "Geometry Color:"), 
                   0, wxALIGN_CENTER_VERTICAL);
    m_geomColorPicker = new wxColourPickerCtrl(panel, wxID_ANY, wxColour(100, 150, 200));
    m_geomColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &EnhancedOutlineSettingsDialog::onColorChange, this);
    colorGrid->Add(m_geomColorPicker, 1, wxEXPAND);
    colorGrid->AddSpacer(0);
    
    // 背景颜色
    colorGrid->Add(new wxStaticText(panel, wxID_ANY, "Background Color:"), 
                   0, wxALIGN_CENTER_VERTICAL);
    m_bgColorPicker = new wxColourPickerCtrl(panel, wxID_ANY, wxColour(240, 240, 240));
    m_bgColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &EnhancedOutlineSettingsDialog::onColorChange, this);
    colorGrid->Add(m_bgColorPicker, 1, wxEXPAND);
    colorGrid->AddSpacer(0);
    
    sizer->Add(colorGrid, 0, wxEXPAND | wxALL, 10);
    
    panel->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createAdvancedControls(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // 高级选项
    wxStaticBoxSizer* advancedBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Advanced Options");
    
    // 折痕角度
    wxBoxSizer* creaseBox = new wxBoxSizer(wxHORIZONTAL);
    creaseBox->Add(new wxStaticText(panel, wxID_ANY, "Crease Angle:"), 
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_creaseAngleSpin = new wxSpinCtrlDouble(panel, wxID_ANY);
    m_creaseAngleSpin->SetRange(0.0, 180.0);
    m_creaseAngleSpin->SetValue(m_params.creaseAngle);
    m_creaseAngleSpin->SetIncrement(5.0);
    m_creaseAngleSpin->Bind(wxEVT_SPINCTRLDOUBLE, &EnhancedOutlineSettingsDialog::onSpinChange, this);
    creaseBox->Add(m_creaseAngleSpin, 1, wxEXPAND);
    creaseBox->Add(new wxStaticText(panel, wxID_ANY, "degrees"), 
                   0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    advancedBox->Add(creaseBox, 0, wxEXPAND | wxALL, 5);
    
    // 淡出距离
    wxBoxSizer* fadeBox = new wxBoxSizer(wxHORIZONTAL);
    fadeBox->Add(new wxStaticText(panel, wxID_ANY, "Fade Distance:"), 
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_fadeDistanceSpin = new wxSpinCtrlDouble(panel, wxID_ANY);
    m_fadeDistanceSpin->SetRange(1.0, 1000.0);
    m_fadeDistanceSpin->SetValue(m_params.fadeDistance);
    m_fadeDistanceSpin->SetIncrement(10.0);
    m_fadeDistanceSpin->Bind(wxEVT_SPINCTRLDOUBLE, &EnhancedOutlineSettingsDialog::onSpinChange, this);
    fadeBox->Add(m_fadeDistanceSpin, 1, wxEXPAND);
    fadeBox->Add(new wxStaticText(panel, wxID_ANY, "units"), 
                 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    advancedBox->Add(fadeBox, 0, wxEXPAND | wxALL, 5);
    
    // 复选框选项
    m_adaptiveThicknessCheck = new wxCheckBox(panel, wxID_ANY, "Adaptive Thickness");
    m_adaptiveThicknessCheck->SetValue(m_params.adaptiveThickness);
    m_adaptiveThicknessCheck->Bind(wxEVT_CHECKBOX, &EnhancedOutlineSettingsDialog::onCheckChange, this);
    advancedBox->Add(m_adaptiveThicknessCheck, 0, wxALL, 5);
    
    m_antiAliasingCheck = new wxCheckBox(panel, wxID_ANY, "Anti-aliasing");
    m_antiAliasingCheck->SetValue(m_params.antiAliasing);
    m_antiAliasingCheck->Bind(wxEVT_CHECKBOX, &EnhancedOutlineSettingsDialog::onCheckChange, this);
    advancedBox->Add(m_antiAliasingCheck, 0, wxALL, 5);
    
    // 采样数量
    wxBoxSizer* sampleBox = new wxBoxSizer(wxHORIZONTAL);
    sampleBox->Add(new wxStaticText(panel, wxID_ANY, "Sample Count:"), 
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    wxString sampleChoices[] = { "1", "2", "4", "8", "16" };
    m_sampleChoice = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                                  5, sampleChoices);
    m_sampleChoice->SetSelection(2); // Default to 4
    m_sampleChoice->Bind(wxEVT_CHOICE, &EnhancedOutlineSettingsDialog::onChoiceChange, this);
    sampleBox->Add(m_sampleChoice, 1, wxEXPAND);
    advancedBox->Add(sampleBox, 0, wxEXPAND | wxALL, 5);
    
    sizer->Add(advancedBox, 0, wxEXPAND | wxALL, 10);
    
    // 内部边缘设置
    wxStaticBoxSizer* innerEdgeBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Inner Edge Settings");
    
    wxBoxSizer* innerIntensityBox = new wxBoxSizer(wxHORIZONTAL);
    innerIntensityBox->Add(new wxStaticText(panel, wxID_ANY, "Inner Edge Intensity:"), 
                          0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_innerEdgeSlider = new wxSlider(panel, wxID_ANY, int(m_params.innerEdgeIntensity * 100), 
                                    0, 100, wxDefaultPosition, wxSize(150, -1));
    m_innerEdgeSlider->Bind(wxEVT_SLIDER, &EnhancedOutlineSettingsDialog::onSliderChange, this);
    innerIntensityBox->Add(m_innerEdgeSlider, 1, wxEXPAND);
    m_innerEdgeLabel = new wxStaticText(panel, wxID_ANY, 
                                       wxString::Format("%.2f", m_params.innerEdgeIntensity));
    innerIntensityBox->Add(m_innerEdgeLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    innerEdgeBox->Add(innerIntensityBox, 0, wxEXPAND | wxALL, 5);
    
    wxBoxSizer* silhouetteBox = new wxBoxSizer(wxHORIZONTAL);
    silhouetteBox->Add(new wxStaticText(panel, wxID_ANY, "Silhouette Boost:"), 
                       0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_silhouetteBoostSlider = new wxSlider(panel, wxID_ANY, int(m_params.silhouetteBoost * 100), 
                                          50, 300, wxDefaultPosition, wxSize(150, -1));
    m_silhouetteBoostSlider->Bind(wxEVT_SLIDER, &EnhancedOutlineSettingsDialog::onSliderChange, this);
    silhouetteBox->Add(m_silhouetteBoostSlider, 1, wxEXPAND);
    m_silhouetteBoostLabel = new wxStaticText(panel, wxID_ANY, 
                                             wxString::Format("%.2f", m_params.silhouetteBoost));
    silhouetteBox->Add(m_silhouetteBoostLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    innerEdgeBox->Add(silhouetteBox, 0, wxEXPAND | wxALL, 5);
    
    sizer->Add(innerEdgeBox, 0, wxEXPAND | wxALL, 10);
    
    panel->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createMethodControls(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // 方法选择
    wxString methods[] = {
        "Basic (No Outline)",
        "Inverted Hull",
        "Screen Space",
        "Geometry Silhouette",
        "Stencil Buffer",
        "Multi-Pass (Best Quality)"
    };
    
    m_methodRadio = new wxRadioBox(panel, wxID_ANY, "Outline Rendering Method", 
                                   wxDefaultPosition, wxDefaultSize,
                                   6, methods, 1, wxRA_SPECIFY_COLS);
    m_methodRadio->SetSelection(1); // Default to Inverted Hull
    m_methodRadio->Bind(wxEVT_RADIOBOX, &EnhancedOutlineSettingsDialog::onMethodChange, this);
    
    sizer->Add(m_methodRadio, 0, wxEXPAND | wxALL, 10);
    
    // 方法描述
    wxStaticBoxSizer* descBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Method Description");
    m_methodDesc = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 150),
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
    descBox->Add(m_methodDesc, 1, wxEXPAND | wxALL, 5);
    
    sizer->Add(descBox, 1, wxEXPAND | wxALL, 10);
    
    // 更新描述
    updateMethodDescription();
    
    panel->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createPreviewPanel(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // 标题
    wxStaticText* title = new wxStaticText(panel, wxID_ANY, "Preview");
    wxFont font = title->GetFont();
    font.SetPointSize(font.GetPointSize() + 2);
    font.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(font);
    sizer->Add(title, 0, wxALIGN_CENTER | wxALL, 10);
    
    // 预览画布
    m_previewCanvas = new EnhancedOutlinePreviewCanvas(panel);
    m_previewCanvas->SetMinSize(wxSize(600, 600));
    sizer->Add(m_previewCanvas, 1, wxEXPAND | wxALL, 10);
    
    // 预览控制
    wxStaticBoxSizer* controlBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Preview Controls");
    
    wxButton* resetViewBtn = new wxButton(panel, wxID_ANY, "Reset View");
    resetViewBtn->Bind(wxEVT_BUTTON, &EnhancedOutlineSettingsDialog::onResetView, this);
    controlBox->Add(resetViewBtn, 0, wxALL, 5);
    
    m_autoRotateCheck = new wxCheckBox(panel, wxID_ANY, "Auto Rotate");
    m_autoRotateCheck->Bind(wxEVT_CHECKBOX, &EnhancedOutlineSettingsDialog::onAutoRotateChange, this);
    controlBox->Add(m_autoRotateCheck, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    sizer->Add(controlBox, 0, wxEXPAND | wxALL, 10);
    
    panel->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::updatePreview() {
    if (!m_previewCanvas) return;
    
    // 更新参数
    collectParams();
    
    // 应用到预览
    m_previewCanvas->updateOutlineParams(m_params);
    m_previewCanvas->setOutlineEnabled(m_enableCheck->GetValue());
    
    // 更新颜色
    m_previewCanvas->setOutlineColor(m_outlineColorPicker->GetColour());
    m_previewCanvas->setGeometryColor(m_geomColorPicker->GetColour());
    m_previewCanvas->setBackgroundColor(m_bgColorPicker->GetColour());
}

void EnhancedOutlineSettingsDialog::collectParams() {
    // 基础参数
    m_params.edgeIntensity = m_intensitySlider->GetValue() / 100.0f;
    m_params.thickness = m_thicknessSlider->GetValue() / 100.0f;
    m_params.depthWeight = m_depthWeightSlider->GetValue() / 100.0f;
    m_params.normalWeight = m_normalWeightSlider->GetValue() / 100.0f;
    m_params.depthThreshold = m_depthThresholdSlider->GetValue() / 1000.0f;
    m_params.normalThreshold = m_normalThresholdSlider->GetValue() / 100.0f;
    
    // 高级参数
    m_params.creaseAngle = m_creaseAngleSpin->GetValue();
    m_params.fadeDistance = m_fadeDistanceSpin->GetValue();
    m_params.adaptiveThickness = m_adaptiveThicknessCheck->GetValue();
    m_params.antiAliasing = m_antiAliasingCheck->GetValue();
    
    // 采样数量
    wxString sampleStr = m_sampleChoice->GetStringSelection();
    m_params.sampleCount = wxAtoi(sampleStr);
    
    // 内部边缘
    m_params.innerEdgeIntensity = m_innerEdgeSlider->GetValue() / 100.0f;
    m_params.silhouetteBoost = m_silhouetteBoostSlider->GetValue() / 100.0f;
}

void EnhancedOutlineSettingsDialog::updateMethodDescription() {
    if (!m_methodDesc) return;
    
    int sel = m_methodRadio->GetSelection();
    wxString desc;
    
    switch (sel) {
        case 0:
            desc = "Basic rendering without any outline effects. Shows the geometry as-is.";
            break;
        case 1:
            desc = "Inverted Hull Method: Renders a scaled-up version of the model with back faces "
                   "in the outline color, then renders the normal model on top. Fast and effective "
                   "for convex shapes.";
            break;
        case 2:
            desc = "Screen Space Method: Performs edge detection in screen space using depth and "
                   "normal information. Works well for any geometry but may miss some internal edges.";
            break;
        case 3:
            desc = "Geometry Silhouette: Extracts silhouette edges from the geometry based on face "
                   "normals and view direction. Provides accurate outlines but requires more computation.";
            break;
        case 4:
            desc = "Stencil Buffer Method: Uses the stencil buffer to create outlines. Renders the "
                   "object to stencil, then draws a scaled version where stencil test fails.";
            break;
        case 5:
            desc = "Multi-Pass Method: Combines multiple techniques for the best quality. Uses inverted "
                   "hull for outer silhouettes and wireframe for internal edges. Highest quality but "
                   "slower performance.";
            break;
    }
    
    m_methodDesc->SetValue(desc);
}

void EnhancedOutlineSettingsDialog::onSliderChange(wxCommandEvent& event) {
    wxSlider* slider = dynamic_cast<wxSlider*>(event.GetEventObject());
    if (!slider) return;
    
    // 更新对应的标签
    if (slider == m_intensitySlider) {
        m_intensityLabel->SetLabel(wxString::Format("%.2f", slider->GetValue() / 100.0));
    } else if (slider == m_thicknessSlider) {
        m_thicknessLabel->SetLabel(wxString::Format("%.2f px", slider->GetValue() / 100.0));
    } else if (slider == m_depthWeightSlider) {
        m_depthWeightLabel->SetLabel(wxString::Format("%.2f", slider->GetValue() / 100.0));
    } else if (slider == m_normalWeightSlider) {
        m_normalWeightLabel->SetLabel(wxString::Format("%.2f", slider->GetValue() / 100.0));
    } else if (slider == m_depthThresholdSlider) {
        m_depthThresholdLabel->SetLabel(wxString::Format("%.3f", slider->GetValue() / 1000.0));
    } else if (slider == m_normalThresholdSlider) {
        m_normalThresholdLabel->SetLabel(wxString::Format("%.2f", slider->GetValue() / 100.0));
    } else if (slider == m_innerEdgeSlider) {
        m_innerEdgeLabel->SetLabel(wxString::Format("%.2f", slider->GetValue() / 100.0));
    } else if (slider == m_silhouetteBoostSlider) {
        m_silhouetteBoostLabel->SetLabel(wxString::Format("%.2f", slider->GetValue() / 100.0));
    }
    
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onColorChange(wxColourPickerEvent& event) {
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onMethodChange(wxCommandEvent& event) {
    int sel = m_methodRadio->GetSelection();
    OutlineMethod method = static_cast<OutlineMethod>(sel);
    
    if (m_previewCanvas) {
        m_previewCanvas->setOutlineMethod(method);
    }
    
    updateMethodDescription();
}

void EnhancedOutlineSettingsDialog::onEnableChange(wxCommandEvent& event) {
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onCheckChange(wxCommandEvent& event) {
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onSpinChange(wxSpinDoubleEvent& event) {
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onChoiceChange(wxCommandEvent& event) {
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onResetDefaults(wxCommandEvent& event) {
    // 重置所有参数到默认值
    m_params = EnhancedOutlineParams();
    
    // 更新所有控件
    m_intensitySlider->SetValue(int(m_params.edgeIntensity * 100));
    m_thicknessSlider->SetValue(int(m_params.thickness * 100));
    m_depthWeightSlider->SetValue(int(m_params.depthWeight * 100));
    m_normalWeightSlider->SetValue(int(m_params.normalWeight * 100));
    m_depthThresholdSlider->SetValue(int(m_params.depthThreshold * 1000));
    m_normalThresholdSlider->SetValue(int(m_params.normalThreshold * 100));
    
    m_creaseAngleSpin->SetValue(m_params.creaseAngle);
    m_fadeDistanceSpin->SetValue(m_params.fadeDistance);
    m_adaptiveThicknessCheck->SetValue(m_params.adaptiveThickness);
    m_antiAliasingCheck->SetValue(m_params.antiAliasing);
    
    m_innerEdgeSlider->SetValue(int(m_params.innerEdgeIntensity * 100));
    m_silhouetteBoostSlider->SetValue(int(m_params.silhouetteBoost * 100));
    
    // 更新标签
    onSliderChange(wxCommandEvent());
    
    // 重置颜色
    m_outlineColorPicker->SetColour(wxColour(0, 0, 0));
    m_geomColorPicker->SetColour(wxColour(100, 150, 200));
    m_bgColorPicker->SetColour(wxColour(240, 240, 240));
    
    // 重置方法
    m_methodRadio->SetSelection(1);
    
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onResetView(wxCommandEvent& event) {
    if (m_previewCanvas) {
        m_previewCanvas->resetCamera();
    }
}

void EnhancedOutlineSettingsDialog::onAutoRotateChange(wxCommandEvent& event) {
    // TODO: 实现自动旋转功能
}

void EnhancedOutlineSettingsDialog::onUpdateTimer(wxTimerEvent& event) {
    if (!m_previewCanvas || !m_statsLabel) return;
    
    // 获取性能统计
    auto stats = m_previewCanvas->getPerformanceStats();
    
    // 更新状态标签
    wxString methodName;
    switch (stats.currentMethod) {
        case OutlineMethod::BASIC: methodName = "Basic"; break;
        case OutlineMethod::INVERTED_HULL: methodName = "Inverted Hull"; break;
        case OutlineMethod::SCREEN_SPACE: methodName = "Screen Space"; break;
        case OutlineMethod::GEOMETRY_SILHOUETTE: methodName = "Geometry"; break;
        case OutlineMethod::STENCIL_BUFFER: methodName = "Stencil"; break;
        case OutlineMethod::MULTI_PASS: methodName = "Multi-Pass"; break;
    }
    
    wxString statsText = wxString::Format("Method: %s | Render Time: %.2f ms | Draw Calls: %d | Triangles: %d",
                                         methodName, stats.renderTime, stats.drawCalls, stats.triangles);
    m_statsLabel->SetLabel(statsText);
}