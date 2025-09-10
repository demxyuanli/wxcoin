#pragma once

#include <wx/wx.h>
#include <wx/clrpicker.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include "viewer/ImageOutlinePass.h"
#include "viewer/EnhancedOutlinePass.h"
#include "viewer/OutlinePassManager.h"

class OutlinePreviewCanvas;
class EnhancedOutlinePreviewCanvas;

// Enhanced outline parameters including all new features
struct EnhancedOutlineDialogParams {
    // Core edge detection parameters
    float depthWeight = 1.5f;
    float normalWeight = 1.0f;
    float colorWeight = 0.3f;
    
    // Threshold parameters
    float depthThreshold = 0.001f;
    float normalThreshold = 0.4f;
    float colorThreshold = 0.1f;
    
    // Visual parameters
    float edgeIntensity = 1.0f;
    float thickness = 1.5f;
    float glowIntensity = 0.2f;
    float glowRadius = 2.0f;
    
    // Advanced parameters
    float adaptiveThreshold = 1.0f;
    float smoothingFactor = 0.5f;
    float backgroundFade = 0.8f;
    
    // Color parameters
    wxColour backgroundColor{ 51, 51, 51 };     // Dark gray
    wxColour outlineColor{ 0, 0, 0 };          // Black
    wxColour glowColor{ 255, 255, 0 };         // Yellow
    wxColour geometryColor{ 200, 200, 200 };   // Light gray
    wxColour hoverColor{ 255, 128, 0 };         // Orange
    wxColour selectionColor{ 255, 0, 0 };      // Red
    
    // Performance parameters
    int downsampleFactor = 1;
    bool enableEarlyCulling = true;
    bool enableMultiSample = false;
    
    // Selection parameters
    bool enableSelectionOutline = true;
    bool enableHoverOutline = true;
    bool enableAllObjectsOutline = false;
    float selectionIntensity = 1.5f;
    float hoverIntensity = 1.0f;
    float defaultIntensity = 0.8f;
};

class EnhancedOutlineSettingsDialog : public wxDialog {
public:
    EnhancedOutlineSettingsDialog(wxWindow* parent, 
                                 const ImageOutlineParams& legacyParams,
                                 const EnhancedOutlineParams& enhancedParams);
    
    // Get parameters
    ImageOutlineParams getLegacyParams() const;
    EnhancedOutlineParams getEnhancedParams() const;
    EnhancedOutlineDialogParams getDialogParams() const;
    OutlinePassManager::OutlineMode getSelectedMode() const;
    
    // Set parameters
    void setLegacyParams(const ImageOutlineParams& params);
    void setEnhancedParams(const EnhancedOutlineParams& params);
    void setDialogParams(const EnhancedOutlineDialogParams& params);

private:
    // Parameters
    ImageOutlineParams m_legacyParams;
    EnhancedOutlineParams m_enhancedParams;
    EnhancedOutlineDialogParams m_dialogParams;
    OutlinePassManager::OutlineMode m_currentMode{ OutlinePassManager::OutlineMode::Enhanced };
    
    // UI Controls - Mode Selection
    wxRadioBox* m_modeRadioBox{ nullptr };
    wxChoice* m_performanceModeChoice{ nullptr };
    wxCheckBox* m_debugVisualizationCheck{ nullptr };
    
    // UI Controls - Basic Parameters
    wxSlider* m_depthWeightSlider{ nullptr };
    wxSlider* m_normalWeightSlider{ nullptr };
    wxSlider* m_colorWeightSlider{ nullptr };
    wxSlider* m_depthThresholdSlider{ nullptr };
    wxSlider* m_normalThresholdSlider{ nullptr };
    wxSlider* m_colorThresholdSlider{ nullptr };
    wxSlider* m_edgeIntensitySlider{ nullptr };
    wxSlider* m_thicknessSlider{ nullptr };
    
    // UI Controls - Advanced Parameters
    wxSlider* m_glowIntensitySlider{ nullptr };
    wxSlider* m_glowRadiusSlider{ nullptr };
    wxSlider* m_adaptiveThresholdSlider{ nullptr };
    wxSlider* m_smoothingFactorSlider{ nullptr };
    wxSlider* m_backgroundFadeSlider{ nullptr };
    
    // UI Controls - Performance Parameters
    wxChoice* m_downsampleChoice{ nullptr };
    wxCheckBox* m_earlyCullingCheck{ nullptr };
    wxCheckBox* m_multiSampleCheck{ nullptr };
    
    // UI Controls - Selection Parameters
    wxCheckBox* m_selectionOutlineCheck{ nullptr };
    wxCheckBox* m_hoverOutlineCheck{ nullptr };
    wxCheckBox* m_allObjectsOutlineCheck{ nullptr };
    wxSlider* m_selectionIntensitySlider{ nullptr };
    wxSlider* m_hoverIntensitySlider{ nullptr };
    wxSlider* m_defaultIntensitySlider{ nullptr };
    
    // UI Controls - Color Pickers
    wxColourPickerCtrl* m_backgroundColorPicker{ nullptr };
    wxColourPickerCtrl* m_outlineColorPicker{ nullptr };
    wxColourPickerCtrl* m_glowColorPicker{ nullptr };
    wxColourPickerCtrl* m_geometryColorPicker{ nullptr };
    wxColourPickerCtrl* m_hoverColorPicker{ nullptr };
    wxColourPickerCtrl* m_selectionColorPicker{ nullptr };
    
    // Value Labels
    wxStaticText* m_depthWeightLabel{ nullptr };
    wxStaticText* m_normalWeightLabel{ nullptr };
    wxStaticText* m_colorWeightLabel{ nullptr };
    wxStaticText* m_depthThresholdLabel{ nullptr };
    wxStaticText* m_normalThresholdLabel{ nullptr };
    wxStaticText* m_colorThresholdLabel{ nullptr };
    wxStaticText* m_edgeIntensityLabel{ nullptr };
    wxStaticText* m_thicknessLabel{ nullptr };
    wxStaticText* m_glowIntensityLabel{ nullptr };
    wxStaticText* m_glowRadiusLabel{ nullptr };
    wxStaticText* m_adaptiveThresholdLabel{ nullptr };
    wxStaticText* m_smoothingFactorLabel{ nullptr };
    wxStaticText* m_backgroundFadeLabel{ nullptr };
    wxStaticText* m_selectionIntensityLabel{ nullptr };
    wxStaticText* m_hoverIntensityLabel{ nullptr };
    wxStaticText* m_defaultIntensityLabel{ nullptr };
    
    // Preview
    OutlinePreviewCanvas* m_legacyPreviewCanvas{ nullptr };
    EnhancedOutlinePreviewCanvas* m_enhancedPreviewCanvas{ nullptr };
    wxNotebook* m_previewNotebook{ nullptr };
    
    // Performance monitoring
    wxStaticText* m_performanceInfoLabel{ nullptr };
    wxButton* m_performanceReportButton{ nullptr };
    
    // Event handlers
    void onModeChanged(wxCommandEvent& event);
    void onPerformanceModeChanged(wxCommandEvent& event);
    void onDebugVisualizationChanged(wxCommandEvent& event);
    void onSliderChanged(wxCommandEvent& event);
    void onColorChanged(wxColourPickerEvent& event);
    void onCheckBoxChanged(wxCommandEvent& event);
    void onChoiceChanged(wxCommandEvent& event);
    void onResetClicked(wxCommandEvent& event);
    void onPerformanceReportClicked(wxCommandEvent& event);
    void onOk(wxCommandEvent& event);
    
    // Helper methods
    void createControls();
    void createBasicParametersPanel(wxWindow* parent);
    void createAdvancedParametersPanel(wxWindow* parent);
    void createPerformancePanel(wxWindow* parent);
    void createSelectionPanel(wxWindow* parent);
    void createColorPanel(wxWindow* parent);
    void createPreviewPanel(wxWindow* parent);
    
    void updateLabels();
    void updatePreview();
    void updateParameterVisibility();
    void migrateParameters();
    
    // Helper lambda for creating sliders
    wxBoxSizer* createSliderWithLabel(wxWindow* parent, const wxString& label, 
                                     int min, int max, int value,
                                     wxSlider** sliderPtr, wxStaticText** labelPtr);
    
    // Helper lambda for creating color pickers
    wxBoxSizer* createColorPickerWithLabel(wxWindow* parent, const wxString& label,
                                          const wxColour& color, wxColourPickerCtrl** pickerPtr);
    
    // Helper lambda for creating checkboxes
    wxBoxSizer* createCheckBoxWithLabel(wxWindow* parent, const wxString& label,
                                       bool checked, wxCheckBox** checkBoxPtr);
    
    // Helper lambda for creating choices
    wxBoxSizer* createChoiceWithLabel(wxWindow* parent, const wxString& label,
                                     const wxArrayString& choices, int selection,
                                     wxChoice** choicePtr);
};