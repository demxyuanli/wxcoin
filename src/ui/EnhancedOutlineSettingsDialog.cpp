#include "ui/EnhancedOutlineSettingsDialog.h"
#include "ui/OutlinePreviewCanvas.h"
#include "ui/EnhancedOutlinePreviewCanvas.h"
#include <wx/splitter.h>
#include <wx/statline.h>
#include <wx/notebook.h>
#include <iomanip>
#include <sstream>

EnhancedOutlineSettingsDialog::EnhancedOutlineSettingsDialog(wxWindow* parent, 
                                                           const ImageOutlineParams& legacyParams,
                                                           const EnhancedOutlineParams& enhancedParams)
    : wxDialog(parent, wxID_ANY, "Enhanced Outline Settings", wxDefaultPosition, wxSize(1400, 900)),
      m_legacyParams(legacyParams), m_enhancedParams(enhancedParams) {
    
    // Initialize dialog parameters from enhanced params
    m_dialogParams.depthWeight = enhancedParams.depthWeight;
    m_dialogParams.normalWeight = enhancedParams.normalWeight;
    m_dialogParams.colorWeight = enhancedParams.colorWeight;
    m_dialogParams.depthThreshold = enhancedParams.depthThreshold;
    m_dialogParams.normalThreshold = enhancedParams.normalThreshold;
    m_dialogParams.colorThreshold = enhancedParams.colorThreshold;
    m_dialogParams.edgeIntensity = enhancedParams.edgeIntensity;
    m_dialogParams.thickness = enhancedParams.thickness;
    m_dialogParams.glowIntensity = enhancedParams.glowIntensity;
    m_dialogParams.glowRadius = enhancedParams.glowRadius;
    m_dialogParams.adaptiveThreshold = enhancedParams.adaptiveThreshold;
    m_dialogParams.smoothingFactor = enhancedParams.smoothingFactor;
    m_dialogParams.backgroundFade = enhancedParams.backgroundFade;
    
    // Initialize colors
    m_dialogParams.outlineColor[0] = enhancedParams.outlineColor[0];
    m_dialogParams.outlineColor[1] = enhancedParams.outlineColor[1];
    m_dialogParams.outlineColor[2] = enhancedParams.outlineColor[2];
    m_dialogParams.glowColor[0] = enhancedParams.glowColor[0];
    m_dialogParams.glowColor[1] = enhancedParams.glowColor[1];
    m_dialogParams.glowColor[2] = enhancedParams.glowColor[2];
    m_dialogParams.backgroundColor[0] = enhancedParams.backgroundColor[0];
    m_dialogParams.backgroundColor[1] = enhancedParams.backgroundColor[1];
    m_dialogParams.backgroundColor[2] = enhancedParams.backgroundColor[2];
    
    createControls();
    updateLabels();
    updatePreview();
    
    CenterOnParent();
}

void EnhancedOutlineSettingsDialog::createControls() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Create main splitter
    wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY);
    splitter->SetMinimumPaneSize(500);
    
    // Left panel - controls
    wxPanel* controlPanel = new wxPanel(splitter);
    wxBoxSizer* controlSizer = new wxBoxSizer(wxVERTICAL);
    
    // Mode selection
    wxStaticBoxSizer* modeSizer = new wxStaticBoxSizer(wxVERTICAL, controlPanel, "Outline Mode");
    wxString modeChoices[] = {"Legacy Mode", "Enhanced Mode"};
    m_modeRadioBox = new wxRadioBox(controlPanel, wxID_ANY, "Select Mode", 
                                   wxDefaultPosition, wxDefaultSize, 2, modeChoices);
    m_modeRadioBox->SetSelection(1); // Default to Enhanced
    modeSizer->Add(m_modeRadioBox, 0, wxALL | wxEXPAND, 5);
    
    // Performance mode
    wxArrayString perfChoices;
    perfChoices.Add("Balanced");
    perfChoices.Add("Performance");
    perfChoices.Add("Quality");
    m_performanceModeChoice = new wxChoice(controlPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, perfChoices);
    m_performanceModeChoice->SetSelection(0);
    modeSizer->Add(new wxStaticText(controlPanel, wxID_ANY, "Performance Mode:"), 0, wxALL, 2);
    modeSizer->Add(m_performanceModeChoice, 0, wxALL | wxEXPAND, 2);
    
    // Debug visualization
    m_debugVisualizationCheck = new wxCheckBox(controlPanel, wxID_ANY, "Debug Visualization");
    modeSizer->Add(m_debugVisualizationCheck, 0, wxALL, 2);
    
    controlSizer->Add(modeSizer, 0, wxALL | wxEXPAND, 5);
    
    // Create notebook for different parameter categories
    wxNotebook* notebook = new wxNotebook(controlPanel, wxID_ANY);
    
    // Basic parameters tab
    wxPanel* basicPanel = new wxPanel(notebook);
    createBasicParametersPanel(basicPanel);
    notebook->AddPage(basicPanel, "Basic Parameters");
    
    // Advanced parameters tab
    wxPanel* advancedPanel = new wxPanel(notebook);
    createAdvancedParametersPanel(advancedPanel);
    notebook->AddPage(advancedPanel, "Advanced Parameters");
    
    // Performance tab
    wxPanel* performancePanel = new wxPanel(notebook);
    createPerformancePanel(performancePanel);
    notebook->AddPage(performancePanel, "Performance");
    
    // Selection tab
    wxPanel* selectionPanel = new wxPanel(notebook);
    createSelectionPanel(selectionPanel);
    notebook->AddPage(selectionPanel, "Selection");
    
    // Color tab
    wxPanel* colorPanel = new wxPanel(notebook);
    createColorPanel(colorPanel);
    notebook->AddPage(colorPanel, "Colors");
    
    controlSizer->Add(notebook, 1, wxALL | wxEXPAND, 5);
    
    // Performance info
    wxStaticBoxSizer* perfInfoSizer = new wxStaticBoxSizer(wxVERTICAL, controlPanel, "Performance Info");
    m_performanceInfoLabel = new wxStaticText(controlPanel, wxID_ANY, "Frame Time: 0.0ms\nMode: Balanced");
    perfInfoSizer->Add(m_performanceInfoLabel, 0, wxALL, 5);
    
    m_performanceReportButton = new wxButton(controlPanel, wxID_ANY, "Performance Report");
    perfInfoSizer->Add(m_performanceReportButton, 0, wxALL, 5);
    
    controlSizer->Add(perfInfoSizer, 0, wxALL | wxEXPAND, 5);
    
    // Buttons
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* resetBtn = new wxButton(controlPanel, wxID_ANY, "Reset");
    wxButton* okBtn = new wxButton(controlPanel, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(controlPanel, wxID_CANCEL, "Cancel");
    
    btnSizer->Add(resetBtn, 0, wxALL, 5);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(okBtn, 0, wxALL, 5);
    btnSizer->Add(cancelBtn, 0, wxALL, 5);
    controlSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);
    
    controlPanel->SetSizer(controlSizer);
    
    // Right panel - preview
    wxPanel* previewPanel = new wxPanel(splitter);
    createPreviewPanel(previewPanel);
    
    // Split the window
    splitter->SplitVertically(controlPanel, previewPanel, 600);
    
    mainSizer->Add(splitter, 1, wxEXPAND);
    SetSizerAndFit(mainSizer);
    
    // Bind events
    Bind(wxEVT_COMMAND_RADIOBOX_SELECTED, &EnhancedOutlineSettingsDialog::onModeChanged, this);
    Bind(wxEVT_COMMAND_CHOICE_SELECTED, &EnhancedOutlineSettingsDialog::onPerformanceModeChanged, this);
    Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EnhancedOutlineSettingsDialog::onDebugVisualizationChanged, this);
    Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EnhancedOutlineSettingsDialog::onSliderChanged, this);
    Bind(wxEVT_COMMAND_COLOURPICKER_CHANGED, &EnhancedOutlineSettingsDialog::onColorChanged, this);
    Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EnhancedOutlineSettingsDialog::onCheckBoxChanged, this);
    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EnhancedOutlineSettingsDialog::onResetClicked, this);
    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EnhancedOutlineSettingsDialog::onPerformanceReportClicked, this);
    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EnhancedOutlineSettingsDialog::onOk, this);
}

void EnhancedOutlineSettingsDialog::createBasicParametersPanel(wxWindow* parent) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Core edge detection parameters
    sizer->Add(createSliderWithLabel(parent, "Depth Weight", 0, 300, int(m_dialogParams.depthWeight * 100),
                                   &m_depthWeightSlider, &m_depthWeightLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Normal Weight", 0, 300, int(m_dialogParams.normalWeight * 100),
                                   &m_normalWeightSlider, &m_normalWeightLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Color Weight", 0, 100, int(m_dialogParams.colorWeight * 100),
                                   &m_colorWeightSlider, &m_colorWeightLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Depth Threshold", 0, 50, int(m_dialogParams.depthThreshold * 1000),
                                   &m_depthThresholdSlider, &m_depthThresholdLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Normal Threshold", 0, 200, int(m_dialogParams.normalThreshold * 100),
                                   &m_normalThresholdSlider, &m_normalThresholdLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Color Threshold", 0, 100, int(m_dialogParams.colorThreshold * 100),
                                   &m_colorThresholdSlider, &m_colorThresholdLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Edge Intensity", 0, 200, int(m_dialogParams.edgeIntensity * 100),
                                   &m_edgeIntensitySlider, &m_edgeIntensityLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Thickness", 10, 500, int(m_dialogParams.thickness * 100),
                                   &m_thicknessSlider, &m_thicknessLabel), 0, wxALL | wxEXPAND, 2);
    
    parent->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createAdvancedParametersPanel(wxWindow* parent) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(createSliderWithLabel(parent, "Glow Intensity", 0, 100, int(m_dialogParams.glowIntensity * 100),
                                   &m_glowIntensitySlider, &m_glowIntensityLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Glow Radius", 50, 1000, int(m_dialogParams.glowRadius * 100),
                                   &m_glowRadiusSlider, &m_glowRadiusLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Adaptive Threshold", 0, 100, int(m_dialogParams.adaptiveThreshold * 100),
                                   &m_adaptiveThresholdSlider, &m_adaptiveThresholdLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Smoothing Factor", 0, 100, int(m_dialogParams.smoothingFactor * 100),
                                   &m_smoothingFactorSlider, &m_smoothingFactorLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Background Fade", 0, 100, int(m_dialogParams.backgroundFade * 100),
                                   &m_backgroundFadeSlider, &m_backgroundFadeLabel), 0, wxALL | wxEXPAND, 2);
    
    parent->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createPerformancePanel(wxWindow* parent) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    wxArrayString downsampleChoices;
    downsampleChoices.Add("1x (Full Quality)");
    downsampleChoices.Add("2x (Half Resolution)");
    downsampleChoices.Add("4x (Quarter Resolution)");
    
    sizer->Add(createChoiceWithLabel(parent, "Downsample Factor", downsampleChoices, 
                                   m_dialogParams.downsampleFactor - 1, &m_downsampleChoice), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createCheckBoxWithLabel(parent, "Early Culling", m_dialogParams.enableEarlyCulling, &m_earlyCullingCheck), 0, wxALL, 2);
    sizer->Add(createCheckBoxWithLabel(parent, "Multi-Sample Anti-Aliasing", m_dialogParams.enableMultiSample, &m_multiSampleCheck), 0, wxALL, 2);
    
    parent->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createSelectionPanel(wxWindow* parent) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(createCheckBoxWithLabel(parent, "Selection Outline", m_dialogParams.enableSelectionOutline, &m_selectionOutlineCheck), 0, wxALL, 2);
    sizer->Add(createCheckBoxWithLabel(parent, "Hover Outline", m_dialogParams.enableHoverOutline, &m_hoverOutlineCheck), 0, wxALL, 2);
    sizer->Add(createCheckBoxWithLabel(parent, "All Objects Outline", m_dialogParams.enableAllObjectsOutline, &m_allObjectsOutlineCheck), 0, wxALL, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Selection Intensity", 0, 300, int(m_dialogParams.selectionIntensity * 100),
                                   &m_selectionIntensitySlider, &m_selectionIntensityLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Hover Intensity", 0, 300, int(m_dialogParams.hoverIntensity * 100),
                                   &m_hoverIntensitySlider, &m_hoverIntensityLabel), 0, wxALL | wxEXPAND, 2);
    
    sizer->Add(createSliderWithLabel(parent, "Default Intensity", 0, 300, int(m_dialogParams.defaultIntensity * 100),
                                   &m_defaultIntensitySlider, &m_defaultIntensityLabel), 0, wxALL | wxEXPAND, 2);
    
    parent->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createColorPanel(wxWindow* parent) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    wxColour bgColor(m_dialogParams.backgroundColor[0] * 255, m_dialogParams.backgroundColor[1] * 255, m_dialogParams.backgroundColor[2] * 255);
    wxColour outlineColor(m_dialogParams.outlineColor[0] * 255, m_dialogParams.outlineColor[1] * 255, m_dialogParams.outlineColor[2] * 255);
    wxColour glowColor(m_dialogParams.glowColor[0] * 255, m_dialogParams.glowColor[1] * 255, m_dialogParams.glowColor[2] * 255);
    
    sizer->Add(createColorPickerWithLabel(parent, "Background Color", bgColor, &m_backgroundColorPicker), 0, wxALL | wxEXPAND, 2);
    sizer->Add(createColorPickerWithLabel(parent, "Outline Color", outlineColor, &m_outlineColorPicker), 0, wxALL | wxEXPAND, 2);
    sizer->Add(createColorPickerWithLabel(parent, "Glow Color", glowColor, &m_glowColorPicker), 0, wxALL | wxEXPAND, 2);
    sizer->Add(createColorPickerWithLabel(parent, "Geometry Color", m_dialogParams.geometryColor, &m_geometryColorPicker), 0, wxALL | wxEXPAND, 2);
    sizer->Add(createColorPickerWithLabel(parent, "Hover Color", m_dialogParams.hoverColor, &m_hoverColorPicker), 0, wxALL | wxEXPAND, 2);
    sizer->Add(createColorPickerWithLabel(parent, "Selection Color", m_dialogParams.selectionColor, &m_selectionColorPicker), 0, wxALL | wxEXPAND, 2);
    
    parent->SetSizer(sizer);
}

void EnhancedOutlineSettingsDialog::createPreviewPanel(wxWindow* parent) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Preview title
    wxStaticText* title = new wxStaticText(parent, wxID_ANY, "Preview");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    sizer->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
    sizer->Add(new wxStaticLine(parent), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    // Create preview notebook
    m_previewNotebook = new wxNotebook(parent, wxID_ANY);
    
    // Legacy preview tab
    wxPanel* legacyPreviewPanel = new wxPanel(m_previewNotebook);
    wxBoxSizer* legacySizer = new wxBoxSizer(wxVERTICAL);
    m_legacyPreviewCanvas = new OutlinePreviewCanvas(legacyPreviewPanel, wxID_ANY, 
                                                    wxDefaultPosition, wxSize(600, 600));
    legacySizer->Add(m_legacyPreviewCanvas, 1, wxEXPAND | wxALL, 10);
    legacyPreviewPanel->SetSizer(legacySizer);
    m_previewNotebook->AddPage(legacyPreviewPanel, "Legacy Preview");
    
    // Enhanced preview tab
    wxPanel* enhancedPreviewPanel = new wxPanel(m_previewNotebook);
    wxBoxSizer* enhancedSizer = new wxBoxSizer(wxVERTICAL);
    m_enhancedPreviewCanvas = new EnhancedOutlinePreviewCanvas(enhancedPreviewPanel, wxID_ANY, 
                                                             wxDefaultPosition, wxSize(600, 600));
    enhancedSizer->Add(m_enhancedPreviewCanvas, 1, wxEXPAND | wxALL, 10);
    enhancedPreviewPanel->SetSizer(enhancedSizer);
    m_previewNotebook->AddPage(enhancedPreviewPanel, "Enhanced Preview");
    
    sizer->Add(m_previewNotebook, 1, wxEXPAND | wxALL, 10);
    
    // Preview instructions
    wxStaticText* instructions = new wxStaticText(parent, wxID_ANY, 
        "Left click and drag to rotate\nDouble click to select objects");
    sizer->Add(instructions, 0, wxALL | wxALIGN_CENTER, 5);
    
    parent->SetSizer(sizer);
}

// Helper methods implementation
wxBoxSizer* EnhancedOutlineSettingsDialog::createSliderWithLabel(wxWindow* parent, const wxString& label, 
                                                                int min, int max, int value,
                                                                wxSlider** sliderPtr, wxStaticText** labelPtr) {
    wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
    
    wxStaticText* nameLabel = new wxStaticText(parent, wxID_ANY, label);
    nameLabel->SetMinSize(wxSize(120, -1));
    box->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    *sliderPtr = new wxSlider(parent, wxID_ANY, value, min, max, 
                             wxDefaultPosition, wxSize(200, -1));
    box->Add(*sliderPtr, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    *labelPtr = new wxStaticText(parent, wxID_ANY, wxString::Format("%.3f", value / 100.0));
    (*labelPtr)->SetMinSize(wxSize(50, -1));
    box->Add(*labelPtr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    return box;
}

wxBoxSizer* EnhancedOutlineSettingsDialog::createColorPickerWithLabel(wxWindow* parent, const wxString& label,
                                                                     const wxColour& color, wxColourPickerCtrl** pickerPtr) {
    wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
    
    wxStaticText* nameLabel = new wxStaticText(parent, wxID_ANY, label);
    nameLabel->SetMinSize(wxSize(120, -1));
    box->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    *pickerPtr = new wxColourPickerCtrl(parent, wxID_ANY, color, 
                                       wxDefaultPosition, wxSize(100, -1));
    box->Add(*pickerPtr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    return box;
}

wxBoxSizer* EnhancedOutlineSettingsDialog::createCheckBoxWithLabel(wxWindow* parent, const wxString& label,
                                                                   bool checked, wxCheckBox** checkBoxPtr) {
    wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
    
    *checkBoxPtr = new wxCheckBox(parent, wxID_ANY, label);
    (*checkBoxPtr)->SetValue(checked);
    box->Add(*checkBoxPtr, 0, wxALL, 5);
    
    return box;
}

wxBoxSizer* EnhancedOutlineSettingsDialog::createChoiceWithLabel(wxWindow* parent, const wxString& label,
                                                                const wxArrayString& choices, int selection,
                                                                wxChoice** choicePtr) {
    wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
    
    wxStaticText* nameLabel = new wxStaticText(parent, wxID_ANY, label);
    nameLabel->SetMinSize(wxSize(120, -1));
    box->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    *choicePtr = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
    (*choicePtr)->SetSelection(selection);
    box->Add(*choicePtr, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    return box;
}

// Event handlers
void EnhancedOutlineSettingsDialog::onModeChanged(wxCommandEvent& event) {
    m_currentMode = (m_modeRadioBox->GetSelection() == 0) ? 
                   OutlinePassManager::OutlineMode::Legacy : 
                   OutlinePassManager::OutlineMode::Enhanced;
    
    updateParameterVisibility();
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onPerformanceModeChanged(wxCommandEvent& event) {
    int selection = m_performanceModeChoice->GetSelection();
    
    if (m_enhancedPreviewCanvas) {
        switch (selection) {
            case 0: m_enhancedPreviewCanvas->setBalancedMode(); break;
            case 1: m_enhancedPreviewCanvas->setPerformanceMode(true); break;
            case 2: m_enhancedPreviewCanvas->setQualityMode(true); break;
        }
    }
    
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onDebugVisualizationChanged(wxCommandEvent& event) {
    if (m_enhancedPreviewCanvas) {
        m_enhancedPreviewCanvas->setDebugMode(m_debugVisualizationCheck->GetValue() ? 
                                            OutlineDebugMode::ShowEdgeMask : 
                                            OutlineDebugMode::Final);
    }
    
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onSliderChanged(wxCommandEvent& event) {
    updateLabels();
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onColorChanged(wxColourPickerEvent& event) {
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onCheckBoxChanged(wxCommandEvent& event) {
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onResetClicked(wxCommandEvent& event) {
    // Reset to default values
    EnhancedOutlineParams defaults;
    
    m_depthWeightSlider->SetValue(int(defaults.depthWeight * 100));
    m_normalWeightSlider->SetValue(int(defaults.normalWeight * 100));
    m_colorWeightSlider->SetValue(int(defaults.colorWeight * 100));
    m_depthThresholdSlider->SetValue(int(defaults.depthThreshold * 1000));
    m_normalThresholdSlider->SetValue(int(defaults.normalThreshold * 100));
    m_colorThresholdSlider->SetValue(int(defaults.colorThreshold * 100));
    m_edgeIntensitySlider->SetValue(int(defaults.edgeIntensity * 100));
    m_thicknessSlider->SetValue(int(defaults.thickness * 100));
    m_glowIntensitySlider->SetValue(int(defaults.glowIntensity * 100));
    m_glowRadiusSlider->SetValue(int(defaults.glowRadius * 100));
    m_adaptiveThresholdSlider->SetValue(int(defaults.adaptiveThreshold * 100));
    m_smoothingFactorSlider->SetValue(int(defaults.smoothingFactor * 100));
    m_backgroundFadeSlider->SetValue(int(defaults.backgroundFade * 100));
    
    updateLabels();
    updatePreview();
}

void EnhancedOutlineSettingsDialog::onPerformanceReportClicked(wxCommandEvent& event) {
    if (m_enhancedPreviewCanvas) {
        auto perfInfo = m_enhancedPreviewCanvas->getPerformanceInfo();
        
        wxString report = wxString::Format(
            "Performance Report:\n"
            "Frame Time: %.2f ms\n"
            "Average Frame Time: %.2f ms\n"
            "Frame Count: %d\n"
            "Mode: %s\n"
            "Optimized: %s",
            perfInfo.frameTime,
            perfInfo.averageFrameTime,
            perfInfo.frameCount,
            perfInfo.performanceMode,
            perfInfo.isOptimized ? "Yes" : "No"
        );
        
        wxMessageBox(report, "Performance Report", wxOK | wxICON_INFORMATION);
    }
}

void EnhancedOutlineSettingsDialog::onOk(wxCommandEvent& event) {
    EndModal(wxID_OK);
}

// Update methods
void EnhancedOutlineSettingsDialog::updateLabels() {
    auto formatValue = [](double value, int precision = 3) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        return wxString(ss.str());
    };
    
    m_depthWeightLabel->SetLabel(formatValue(m_depthWeightSlider->GetValue() / 100.0, 2));
    m_normalWeightLabel->SetLabel(formatValue(m_normalWeightSlider->GetValue() / 100.0, 2));
    m_colorWeightLabel->SetLabel(formatValue(m_colorWeightSlider->GetValue() / 100.0, 2));
    m_depthThresholdLabel->SetLabel(formatValue(m_depthThresholdSlider->GetValue() / 1000.0, 3));
    m_normalThresholdLabel->SetLabel(formatValue(m_normalThresholdSlider->GetValue() / 100.0, 2));
    m_colorThresholdLabel->SetLabel(formatValue(m_colorThresholdSlider->GetValue() / 100.0, 2));
    m_edgeIntensityLabel->SetLabel(formatValue(m_edgeIntensitySlider->GetValue() / 100.0, 2));
    m_thicknessLabel->SetLabel(formatValue(m_thicknessSlider->GetValue() / 100.0, 2));
    m_glowIntensityLabel->SetLabel(formatValue(m_glowIntensitySlider->GetValue() / 100.0, 2));
    m_glowRadiusLabel->SetLabel(formatValue(m_glowRadiusSlider->GetValue() / 100.0, 2));
    m_adaptiveThresholdLabel->SetLabel(formatValue(m_adaptiveThresholdSlider->GetValue() / 100.0, 2));
    m_smoothingFactorLabel->SetLabel(formatValue(m_smoothingFactorSlider->GetValue() / 100.0, 2));
    m_backgroundFadeLabel->SetLabel(formatValue(m_backgroundFadeSlider->GetValue() / 100.0, 2));
    m_selectionIntensityLabel->SetLabel(formatValue(m_selectionIntensitySlider->GetValue() / 100.0, 2));
    m_hoverIntensityLabel->SetLabel(formatValue(m_hoverIntensitySlider->GetValue() / 100.0, 2));
    m_defaultIntensityLabel->SetLabel(formatValue(m_defaultIntensitySlider->GetValue() / 100.0, 2));
}

void EnhancedOutlineSettingsDialog::updatePreview() {
    // Update dialog parameters from controls
    m_dialogParams.depthWeight = m_depthWeightSlider->GetValue() / 100.0f;
    m_dialogParams.normalWeight = m_normalWeightSlider->GetValue() / 100.0f;
    m_dialogParams.colorWeight = m_colorWeightSlider->GetValue() / 100.0f;
    m_dialogParams.depthThreshold = m_depthThresholdSlider->GetValue() / 1000.0f;
    m_dialogParams.normalThreshold = m_normalThresholdSlider->GetValue() / 100.0f;
    m_dialogParams.colorThreshold = m_colorThresholdSlider->GetValue() / 100.0f;
    m_dialogParams.edgeIntensity = m_edgeIntensitySlider->GetValue() / 100.0f;
    m_dialogParams.thickness = m_thicknessSlider->GetValue() / 100.0f;
    m_dialogParams.glowIntensity = m_glowIntensitySlider->GetValue() / 100.0f;
    m_dialogParams.glowRadius = m_glowRadiusSlider->GetValue() / 100.0f;
    m_dialogParams.adaptiveThreshold = m_adaptiveThresholdSlider->GetValue() / 100.0f;
    m_dialogParams.smoothingFactor = m_smoothingFactorSlider->GetValue() / 100.0f;
    m_dialogParams.backgroundFade = m_backgroundFadeSlider->GetValue() / 100.0f;
    
    // Update colors
    wxColour bgColor = m_backgroundColorPicker->GetColour();
    m_dialogParams.backgroundColor[0] = bgColor.Red() / 255.0f;
    m_dialogParams.backgroundColor[1] = bgColor.Green() / 255.0f;
    m_dialogParams.backgroundColor[2] = bgColor.Blue() / 255.0f;
    
    wxColour outlineColor = m_outlineColorPicker->GetColour();
    m_dialogParams.outlineColor[0] = outlineColor.Red() / 255.0f;
    m_dialogParams.outlineColor[1] = outlineColor.Green() / 255.0f;
    m_dialogParams.outlineColor[2] = outlineColor.Blue() / 255.0f;
    
    wxColour glowColor = m_glowColorPicker->GetColour();
    m_dialogParams.glowColor[0] = glowColor.Red() / 255.0f;
    m_dialogParams.glowColor[1] = glowColor.Green() / 255.0f;
    m_dialogParams.glowColor[2] = glowColor.Blue() / 255.0f;
    
    // Update enhanced parameters
    m_enhancedParams = EnhancedOutlineParams();
    m_enhancedParams.depthWeight = m_dialogParams.depthWeight;
    m_enhancedParams.normalWeight = m_dialogParams.normalWeight;
    m_enhancedParams.colorWeight = m_dialogParams.colorWeight;
    m_enhancedParams.depthThreshold = m_dialogParams.depthThreshold;
    m_enhancedParams.normalThreshold = m_dialogParams.normalThreshold;
    m_enhancedParams.colorThreshold = m_dialogParams.colorThreshold;
    m_enhancedParams.edgeIntensity = m_dialogParams.edgeIntensity;
    m_enhancedParams.thickness = m_dialogParams.thickness;
    m_enhancedParams.glowIntensity = m_dialogParams.glowIntensity;
    m_enhancedParams.glowRadius = m_dialogParams.glowRadius;
    m_enhancedParams.adaptiveThreshold = m_dialogParams.adaptiveThreshold;
    m_enhancedParams.smoothingFactor = m_dialogParams.smoothingFactor;
    m_enhancedParams.backgroundFade = m_dialogParams.backgroundFade;
    
    // Update color arrays
    for (int i = 0; i < 3; i++) {
        m_enhancedParams.outlineColor[i] = m_dialogParams.outlineColor[i];
        m_enhancedParams.glowColor[i] = m_dialogParams.glowColor[i];
        m_enhancedParams.backgroundColor[i] = m_dialogParams.backgroundColor[i];
    }
    
    // Update legacy parameters
    m_legacyParams.depthWeight = m_dialogParams.depthWeight;
    m_legacyParams.normalWeight = m_dialogParams.normalWeight;
    m_legacyParams.depthThreshold = m_dialogParams.depthThreshold;
    m_legacyParams.normalThreshold = m_dialogParams.normalThreshold;
    m_legacyParams.edgeIntensity = m_dialogParams.edgeIntensity;
    m_legacyParams.thickness = m_dialogParams.thickness;
    
    // Update preview canvases
    if (m_legacyPreviewCanvas) {
        m_legacyPreviewCanvas->updateOutlineParams(m_legacyParams);
        m_legacyPreviewCanvas->setBackgroundColor(bgColor);
        m_legacyPreviewCanvas->setOutlineColor(outlineColor);
        m_legacyPreviewCanvas->setGeometryColor(m_geometryColorPicker->GetColour());
    }
    
    if (m_enhancedPreviewCanvas) {
        m_enhancedPreviewCanvas->updateOutlineParams(m_enhancedParams);
        m_enhancedPreviewCanvas->setBackgroundColor(bgColor);
        m_enhancedPreviewCanvas->setOutlineColor(outlineColor);
        m_enhancedPreviewCanvas->setGlowColor(glowColor);
        m_enhancedPreviewCanvas->setGeometryColor(m_geometryColorPicker->GetColour());
        m_enhancedPreviewCanvas->setHoverColor(m_hoverColorPicker->GetColour());
        m_enhancedPreviewCanvas->setSelectionColor(m_selectionColorPicker->GetColour());
    }
    
    // Update performance info
    if (m_enhancedPreviewCanvas) {
        auto perfInfo = m_enhancedPreviewCanvas->getPerformanceInfo();
        wxString perfText = wxString::Format("Frame Time: %.2fms\nMode: %s", 
                                            perfInfo.frameTime, perfInfo.performanceMode);
        m_performanceInfoLabel->SetLabel(perfText);
    }
}

void EnhancedOutlineSettingsDialog::updateParameterVisibility() {
    // Show/hide controls based on selected mode
    // This could be implemented to show only relevant controls for each mode
}

void EnhancedOutlineSettingsDialog::migrateParameters() {
    // Migrate parameters between legacy and enhanced modes
    if (m_currentMode == OutlinePassManager::OutlineMode::Enhanced) {
        // Migrate from legacy to enhanced
        m_enhancedParams.depthWeight = m_legacyParams.depthWeight;
        m_enhancedParams.normalWeight = m_legacyParams.normalWeight;
        m_enhancedParams.depthThreshold = m_legacyParams.depthThreshold;
        m_enhancedParams.normalThreshold = m_legacyParams.normalThreshold;
        m_enhancedParams.edgeIntensity = m_legacyParams.edgeIntensity;
        m_enhancedParams.thickness = m_legacyParams.thickness;
    } else {
        // Migrate from enhanced to legacy
        m_legacyParams.depthWeight = m_enhancedParams.depthWeight;
        m_legacyParams.normalWeight = m_enhancedParams.normalWeight;
        m_legacyParams.depthThreshold = m_enhancedParams.depthThreshold;
        m_legacyParams.normalThreshold = m_enhancedParams.normalThreshold;
        m_legacyParams.edgeIntensity = m_enhancedParams.edgeIntensity;
        m_legacyParams.thickness = m_enhancedParams.thickness;
    }
}

// Getter methods
ImageOutlineParams EnhancedOutlineSettingsDialog::getLegacyParams() const {
    return m_legacyParams;
}

EnhancedOutlineParams EnhancedOutlineSettingsDialog::getEnhancedParams() const {
    return m_enhancedParams;
}

EnhancedOutlineDialogParams EnhancedOutlineSettingsDialog::getDialogParams() const {
    return m_dialogParams;
}

OutlinePassManager::OutlineMode EnhancedOutlineSettingsDialog::getSelectedMode() const {
    return m_currentMode;
}

// Setter methods
void EnhancedOutlineSettingsDialog::setLegacyParams(const ImageOutlineParams& params) {
    m_legacyParams = params;
    updatePreview();
}

void EnhancedOutlineSettingsDialog::setEnhancedParams(const EnhancedOutlineParams& params) {
    m_enhancedParams = params;
    updatePreview();
}

void EnhancedOutlineSettingsDialog::setDialogParams(const EnhancedOutlineDialogParams& params) {
    m_dialogParams = params;
    updatePreview();
}