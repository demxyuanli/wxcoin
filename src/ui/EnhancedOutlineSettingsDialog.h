#pragma once

#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/clrpicker.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>

#include "ui/EnhancedOutlinePreviewCanvas.h"

class wxTimer;
class wxNotebook;
class wxRadioBox;

// 增强版轮廓设置对话框
class EnhancedOutlineSettingsDialog : public wxDialog {
public:
    EnhancedOutlineSettingsDialog(wxWindow* parent, const EnhancedOutlineParams& params);
    virtual ~EnhancedOutlineSettingsDialog();
    
    // 获取编辑后的参数
    EnhancedOutlineParams getParams() const { return m_params; }
    
private:
    // UI创建方法
    void createControlPanel(wxPanel* panel);
    void createPreviewPanel(wxPanel* panel);
    void createBasicControls(wxPanel* panel);
    void createAdvancedControls(wxPanel* panel);
    void createMethodControls(wxPanel* panel);
    
    // 事件处理
    void onSliderChange(wxCommandEvent& event);
    void onColorChange(wxColourPickerEvent& event);
    void onMethodChange(wxCommandEvent& event);
    void onEnableChange(wxCommandEvent& event);
    void onCheckChange(wxCommandEvent& event);
    void onSpinChange(wxSpinDoubleEvent& event);
    void onChoiceChange(wxCommandEvent& event);
    void onResetDefaults(wxCommandEvent& event);
    void onResetView(wxCommandEvent& event);
    void onAutoRotateChange(wxCommandEvent& event);
    void onUpdateTimer(wxTimerEvent& event);
    
    // 辅助方法
    void updatePreview();
    void collectParams();
    void updateMethodDescription();
    
private:
    // 参数
    EnhancedOutlineParams m_params;
    
    // 预览画布
    EnhancedOutlinePreviewCanvas* m_previewCanvas = nullptr;
    
    // 基础控件
    wxCheckBox* m_enableCheck = nullptr;
    wxSlider* m_intensitySlider = nullptr;
    wxStaticText* m_intensityLabel = nullptr;
    wxSlider* m_thicknessSlider = nullptr;
    wxStaticText* m_thicknessLabel = nullptr;
    wxSlider* m_depthWeightSlider = nullptr;
    wxStaticText* m_depthWeightLabel = nullptr;
    wxSlider* m_normalWeightSlider = nullptr;
    wxStaticText* m_normalWeightLabel = nullptr;
    wxSlider* m_depthThresholdSlider = nullptr;
    wxStaticText* m_depthThresholdLabel = nullptr;
    wxSlider* m_normalThresholdSlider = nullptr;
    wxStaticText* m_normalThresholdLabel = nullptr;
    
    // 颜色控件
    wxColourPickerCtrl* m_outlineColorPicker = nullptr;
    wxColourPickerCtrl* m_geomColorPicker = nullptr;
    wxColourPickerCtrl* m_bgColorPicker = nullptr;
    
    // 高级控件
    wxSpinCtrlDouble* m_creaseAngleSpin = nullptr;
    wxSpinCtrlDouble* m_fadeDistanceSpin = nullptr;
    wxCheckBox* m_adaptiveThicknessCheck = nullptr;
    wxCheckBox* m_antiAliasingCheck = nullptr;
    wxChoice* m_sampleChoice = nullptr;
    wxSlider* m_innerEdgeSlider = nullptr;
    wxStaticText* m_innerEdgeLabel = nullptr;
    wxSlider* m_silhouetteBoostSlider = nullptr;
    wxStaticText* m_silhouetteBoostLabel = nullptr;
    
    // 方法选择
    wxRadioBox* m_methodRadio = nullptr;
    wxTextCtrl* m_methodDesc = nullptr;
    
    // 预览控制
    wxCheckBox* m_autoRotateCheck = nullptr;
    
    // 性能统计
    wxStaticText* m_statsLabel = nullptr;
    wxTimer* m_updateTimer = nullptr;
};