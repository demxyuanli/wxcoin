#include "config/editor/DockLayoutConfigEditor.h"
#include "config/UnifiedConfigManager.h"
#include "docking/DockLayoutConfig.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/gbsizer.h>
#include <wx/notebook.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(DockLayoutConfigEditor, ConfigCategoryEditor)
    EVT_CHECKBOX(wxID_ANY, DockLayoutConfigEditor::onCheckChanged)
    EVT_SPINCTRL(wxID_ANY, DockLayoutConfigEditor::onValueChanged)
    EVT_BUTTON(wxID_HIGHEST + 1, DockLayoutConfigEditor::onPresetButton)
    EVT_BUTTON(wxID_HIGHEST + 2, DockLayoutConfigEditor::onPresetButton)
    EVT_BUTTON(wxID_HIGHEST + 3, DockLayoutConfigEditor::onPresetButton)
wxEND_EVENT_TABLE()

DockLayoutConfigEditor::DockLayoutConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId)
    : ConfigCategoryEditor(parent, configManager, categoryId)
    , m_usePercentageCheck(nullptr)
    , m_topHeightSpin(nullptr)
    , m_bottomHeightSpin(nullptr)
    , m_leftWidthSpin(nullptr)
    , m_rightWidthSpin(nullptr)
    , m_centerMinWidthSpin(nullptr)
    , m_centerMinHeightSpin(nullptr)
    , m_topPercentSpin(nullptr)
    , m_bottomPercentSpin(nullptr)
    , m_leftPercentSpin(nullptr)
    , m_rightPercentSpin(nullptr)
    , m_showTopCheck(nullptr)
    , m_showBottomCheck(nullptr)
    , m_showLeftCheck(nullptr)
    , m_showRightCheck(nullptr)
    , m_minSizeSpin(nullptr)
    , m_splitterWidthSpin(nullptr)
    , m_enableAnimationCheck(nullptr)
    , m_animationDurationSpin(nullptr)
    , m_previewPanel(nullptr)
    , m_notebook(nullptr)
{
    createUI();
}

DockLayoutConfigEditor::~DockLayoutConfigEditor() {
}

void DockLayoutConfigEditor::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);
    
    // Create horizontal sizer for notebook and preview
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Create notebook for organized settings
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    // Size settings page
    wxPanel* sizePage = new wxPanel(m_notebook);
    wxBoxSizer* sizeSizer = new wxBoxSizer(wxVERTICAL);
    createSizeControls(sizePage, sizeSizer);
    sizePage->SetSizer(sizeSizer);
    m_notebook->AddPage(sizePage, "Sizes");
    
    // Visibility settings page
    wxPanel* visibilityPage = new wxPanel(m_notebook);
    wxBoxSizer* visibilitySizer = new wxBoxSizer(wxVERTICAL);
    createVisibilityControls(visibilityPage, visibilitySizer);
    visibilityPage->SetSizer(visibilitySizer);
    m_notebook->AddPage(visibilityPage, "Visibility");
    
    // Options page
    wxPanel* optionsPage = new wxPanel(m_notebook);
    wxBoxSizer* optionsSizer = new wxBoxSizer(wxVERTICAL);
    createOptionControls(optionsPage, optionsSizer);
    optionsPage->SetSizer(optionsSizer);
    m_notebook->AddPage(optionsPage, "Options");
    
    // Add notebook to left side
    contentSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
    
    // Preview panel on right side
    createPreviewPanel(contentSizer);
    
    mainSizer->Add(contentSizer, 1, wxEXPAND);
    
    // Preset buttons
    createPresetButtons(mainSizer);
    
    FitInside();
}

void DockLayoutConfigEditor::createSizeControls(wxWindow* parent, wxSizer* sizer) {
    // Use percentage checkbox
    m_usePercentageCheck = new wxCheckBox(parent, wxID_ANY, "Use Percentage Values");
    sizer->Add(m_usePercentageCheck, 0, wxALL, 5);
    m_usePercentageCheck->Bind(wxEVT_CHECKBOX, &DockLayoutConfigEditor::onUsePercentageChanged, this);
    
    // Pixel size controls
    wxStaticBoxSizer* pixelBox = new wxStaticBoxSizer(wxVERTICAL, parent, "Size in Pixels");
    wxGridBagSizer* pixelGrid = new wxGridBagSizer(5, 5);
    
    int row = 0;
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Top Height:"),
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_topHeightSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS, 50, 500, 150);
    pixelGrid->Add(m_topHeightSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Bottom Height:"),
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_bottomHeightSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                        wxSP_ARROW_KEYS, 50, 500, 200);
    pixelGrid->Add(m_bottomHeightSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Left Width:"),
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_leftWidthSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS, 50, 500, 240);
    pixelGrid->Add(m_leftWidthSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Right Width:"),
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_rightWidthSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                      wxSP_ARROW_KEYS, 50, 500, 250);
    pixelGrid->Add(m_rightWidthSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Center Min Width:"),
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_centerMinWidthSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                          wxSP_ARROW_KEYS, 100, 800, 400);
    pixelGrid->Add(m_centerMinWidthSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Center Min Height:"),
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_centerMinHeightSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                           wxSP_ARROW_KEYS, 100, 600, 300);
    pixelGrid->Add(m_centerMinHeightSpin, wxGBPosition(row++, 1));
    
    pixelBox->Add(pixelGrid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(pixelBox, 0, wxEXPAND | wxALL, 5);
    
    // Percentage size controls
    wxStaticBoxSizer* percentBox = new wxStaticBoxSizer(wxVERTICAL, parent, "Size in Percentage");
    wxGridBagSizer* percentGrid = new wxGridBagSizer(5, 5);
    
    row = 0;
    percentGrid->Add(new wxStaticText(parent, wxID_ANY, "Top Height %:"),
                     wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_topPercentSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                      wxSP_ARROW_KEYS, 5, 50, 0);
    percentGrid->Add(m_topPercentSpin, wxGBPosition(row++, 1));
    
    percentGrid->Add(new wxStaticText(parent, wxID_ANY, "Bottom Height %:"),
                     wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_bottomPercentSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                         wxSP_ARROW_KEYS, 5, 50, 20);
    percentGrid->Add(m_bottomPercentSpin, wxGBPosition(row++, 1));
    
    percentGrid->Add(new wxStaticText(parent, wxID_ANY, "Left Width %:"),
                     wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_leftPercentSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                       wxSP_ARROW_KEYS, 5, 50, 15);
    percentGrid->Add(m_leftPercentSpin, wxGBPosition(row++, 1));
    
    percentGrid->Add(new wxStaticText(parent, wxID_ANY, "Right Width %:"),
                     wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_rightPercentSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                        wxSP_ARROW_KEYS, 5, 50, 0);
    percentGrid->Add(m_rightPercentSpin, wxGBPosition(row++, 1));
    
    percentBox->Add(percentGrid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(percentBox, 0, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigEditor::createVisibilityControls(wxWindow* parent, wxSizer* sizer) {
    wxStaticBoxSizer* visBox = new wxStaticBoxSizer(wxVERTICAL, parent, "Area Visibility");
    
    m_showTopCheck = new wxCheckBox(parent, wxID_ANY, "Show Top Area");
    visBox->Add(m_showTopCheck, 0, wxALL, 5);
    
    m_showBottomCheck = new wxCheckBox(parent, wxID_ANY, "Show Bottom Area");
    visBox->Add(m_showBottomCheck, 0, wxALL, 5);
    
    m_showLeftCheck = new wxCheckBox(parent, wxID_ANY, "Show Left Area");
    visBox->Add(m_showLeftCheck, 0, wxALL, 5);
    
    m_showRightCheck = new wxCheckBox(parent, wxID_ANY, "Show Right Area");
    visBox->Add(m_showRightCheck, 0, wxALL, 5);
    
    sizer->Add(visBox, 0, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigEditor::createOptionControls(wxWindow* parent, wxSizer* sizer) {
    wxStaticBoxSizer* optBox = new wxStaticBoxSizer(wxVERTICAL, parent, "General Options");
    wxGridBagSizer* grid = new wxGridBagSizer(5, 5);
    
    int row = 0;
    grid->Add(new wxStaticText(parent, wxID_ANY, "Minimum Area Size:"),
              wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_minSizeSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS, 50, 300, 100);
    grid->Add(m_minSizeSpin, wxGBPosition(row++, 1));
    
    grid->Add(new wxStaticText(parent, wxID_ANY, "Splitter Width:"),
              wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_splitterWidthSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                         wxSP_ARROW_KEYS, 1, 10, 4);
    grid->Add(m_splitterWidthSpin, wxGBPosition(row++, 1));
    
    m_enableAnimationCheck = new wxCheckBox(parent, wxID_ANY, "Enable Animation");
    grid->Add(m_enableAnimationCheck, wxGBPosition(row++, 0), wxGBSpan(1, 2));
    
    grid->Add(new wxStaticText(parent, wxID_ANY, "Animation Duration (ms):"),
              wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_animationDurationSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                             wxSP_ARROW_KEYS, 50, 1000, 200);
    grid->Add(m_animationDurationSpin, wxGBPosition(row++, 1));
    
    optBox->Add(grid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(optBox, 0, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigEditor::createPreviewPanel(wxSizer* sizer) {
    wxStaticBoxSizer* previewBox = new wxStaticBoxSizer(wxVERTICAL, this, "Layout Preview");
    
    m_previewPanel = new ads::DockLayoutPreview(this);
    m_previewPanel->SetMinSize(wxSize(400, 300));
    
    // Add preview info text
    wxStaticText* infoText = new wxStaticText(this, wxID_ANY,
        "Preview shows relative sizes. Click and drag splitters in the actual application to fine-tune.");
    infoText->Wrap(380);
    
    previewBox->Add(m_previewPanel, 1, wxEXPAND | wxALL, 5);
    previewBox->Add(infoText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    sizer->Add(previewBox, 1, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigEditor::createPresetButtons(wxSizer* sizer) {
    wxStaticBoxSizer* presetBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Quick Presets");
    
    wxButton* preset2080Btn = new wxButton(this, wxID_HIGHEST + 1, "20/80 Layout");
    preset2080Btn->SetToolTip("Left: 20%, Center: 80%");
    
    wxButton* preset3Column = new wxButton(this, wxID_HIGHEST + 2, "3-Column");
    preset3Column->SetToolTip("Left: 20%, Center: 60%, Right: 20%");
    
    wxButton* presetIDE = new wxButton(this, wxID_HIGHEST + 3, "IDE Layout");
    presetIDE->SetToolTip("Classic IDE layout with all panels");
    
    presetBox->Add(preset2080Btn, 0, wxALL, 5);
    presetBox->Add(preset3Column, 0, wxALL, 5);
    presetBox->Add(presetIDE, 0, wxALL, 5);
    presetBox->AddStretchSpacer();
    
    sizer->Add(presetBox, 0, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigEditor::loadConfig() {
    // Load configuration from UnifiedConfigManager
    ConfigManager& configManager = ConfigManager::getInstance();
    const std::string section = "DockLayout";
    
    m_config.topAreaHeight = configManager.getInt(section, "TopAreaHeight", 150);
    m_config.bottomAreaHeight = configManager.getInt(section, "BottomAreaHeight", 200);
    m_config.leftAreaWidth = configManager.getInt(section, "LeftAreaWidth", 240);
    m_config.rightAreaWidth = configManager.getInt(section, "RightAreaWidth", 250);
    m_config.centerMinWidth = configManager.getInt(section, "CenterMinWidth", 400);
    m_config.centerMinHeight = configManager.getInt(section, "CenterMinHeight", 300);
    
    m_config.usePercentage = configManager.getBool(section, "UsePercentage", true);
    m_config.topAreaPercent = configManager.getInt(section, "TopAreaPercent", 0);
    m_config.bottomAreaPercent = configManager.getInt(section, "BottomAreaPercent", 20);
    m_config.leftAreaPercent = configManager.getInt(section, "LeftAreaPercent", 15);
    m_config.rightAreaPercent = configManager.getInt(section, "RightAreaPercent", 0);
    
    m_config.minAreaSize = configManager.getInt(section, "MinAreaSize", 100);
    m_config.splitterWidth = configManager.getInt(section, "SplitterWidth", 4);
    
    m_config.showTopArea = configManager.getBool(section, "ShowTopArea", false);
    m_config.showBottomArea = configManager.getBool(section, "ShowBottomArea", true);
    m_config.showLeftArea = configManager.getBool(section, "ShowLeftArea", true);
    m_config.showRightArea = configManager.getBool(section, "ShowRightArea", false);
    
    m_config.enableAnimation = configManager.getBool(section, "EnableAnimation", true);
    m_config.animationDuration = configManager.getInt(section, "AnimationDuration", 200);
    
    // Store original config for change detection
    m_originalConfig = m_config;
    
    // Update controls and preview
    updateControlsFromConfig();
    updateControlStates();
    updatePreview();
}

void DockLayoutConfigEditor::saveConfig() {
    // Update config from controls
    updateConfigFromControls();
    
    // Save to ConfigManager
    ConfigManager& configManager = ConfigManager::getInstance();
    const std::string section = "DockLayout";
    
    configManager.setInt(section, "TopAreaHeight", m_config.topAreaHeight);
    configManager.setInt(section, "BottomAreaHeight", m_config.bottomAreaHeight);
    configManager.setInt(section, "LeftAreaWidth", m_config.leftAreaWidth);
    configManager.setInt(section, "RightAreaWidth", m_config.rightAreaWidth);
    configManager.setInt(section, "CenterMinWidth", m_config.centerMinWidth);
    configManager.setInt(section, "CenterMinHeight", m_config.centerMinHeight);
    
    configManager.setBool(section, "UsePercentage", m_config.usePercentage);
    configManager.setInt(section, "TopAreaPercent", m_config.topAreaPercent);
    configManager.setInt(section, "BottomAreaPercent", m_config.bottomAreaPercent);
    configManager.setInt(section, "LeftAreaPercent", m_config.leftAreaPercent);
    configManager.setInt(section, "RightAreaPercent", m_config.rightAreaPercent);
    
    configManager.setInt(section, "MinAreaSize", m_config.minAreaSize);
    configManager.setInt(section, "SplitterWidth", m_config.splitterWidth);
    
    configManager.setBool(section, "ShowTopArea", m_config.showTopArea);
    configManager.setBool(section, "ShowBottomArea", m_config.showBottomArea);
    configManager.setBool(section, "ShowLeftArea", m_config.showLeftArea);
    configManager.setBool(section, "ShowRightArea", m_config.showRightArea);
    
    configManager.setBool(section, "EnableAnimation", m_config.enableAnimation);
    configManager.setInt(section, "AnimationDuration", m_config.animationDuration);
    
    configManager.save();
    
    // Update original config
    m_originalConfig = m_config;
}

void DockLayoutConfigEditor::resetConfig() {
    // Reset to default values
    m_config = ads::DockLayoutConfig();
    
    updateControlsFromConfig();
    updateControlStates();
    updatePreview();
}

bool DockLayoutConfigEditor::hasChanges() const {
    if (!m_usePercentageCheck) return false;
    
    // Get current values from controls (need to update config first)
    // Since this is const, we'll compare with what we have in m_config
    // But we need to update m_config from controls - use mutable or const_cast
    const_cast<DockLayoutConfigEditor*>(this)->updateConfigFromControls();
    
    // Compare with original config
    return !(m_config == m_originalConfig);
}

void DockLayoutConfigEditor::updateConfigFromControls() {
    if (!m_usePercentageCheck || !m_topHeightSpin) return;
    
    m_config.topAreaHeight = m_topHeightSpin->GetValue();
    m_config.bottomAreaHeight = m_bottomHeightSpin->GetValue();
    m_config.leftAreaWidth = m_leftWidthSpin->GetValue();
    m_config.rightAreaWidth = m_rightWidthSpin->GetValue();
    m_config.centerMinWidth = m_centerMinWidthSpin->GetValue();
    m_config.centerMinHeight = m_centerMinHeightSpin->GetValue();
    
    m_config.usePercentage = m_usePercentageCheck->GetValue();
    m_config.topAreaPercent = m_topPercentSpin->GetValue();
    m_config.bottomAreaPercent = m_bottomPercentSpin->GetValue();
    m_config.leftAreaPercent = m_leftPercentSpin->GetValue();
    m_config.rightAreaPercent = m_rightPercentSpin->GetValue();
    
    m_config.minAreaSize = m_minSizeSpin->GetValue();
    m_config.splitterWidth = m_splitterWidthSpin->GetValue();
    
    m_config.showTopArea = m_showTopCheck->GetValue();
    m_config.showBottomArea = m_showBottomCheck->GetValue();
    m_config.showLeftArea = m_showLeftCheck->GetValue();
    m_config.showRightArea = m_showRightCheck->GetValue();
    
    m_config.enableAnimation = m_enableAnimationCheck->GetValue();
    m_config.animationDuration = m_animationDurationSpin->GetValue();
}

void DockLayoutConfigEditor::updateControlsFromConfig() {
    if (!m_usePercentageCheck) return;
    
    m_usePercentageCheck->SetValue(m_config.usePercentage);
    m_topHeightSpin->SetValue(m_config.topAreaHeight);
    m_bottomHeightSpin->SetValue(m_config.bottomAreaHeight);
    m_leftWidthSpin->SetValue(m_config.leftAreaWidth);
    m_rightWidthSpin->SetValue(m_config.rightAreaWidth);
    m_centerMinWidthSpin->SetValue(m_config.centerMinWidth);
    m_centerMinHeightSpin->SetValue(m_config.centerMinHeight);
    
    m_topPercentSpin->SetValue(m_config.topAreaPercent);
    m_bottomPercentSpin->SetValue(m_config.bottomAreaPercent);
    m_leftPercentSpin->SetValue(m_config.leftAreaPercent);
    m_rightPercentSpin->SetValue(m_config.rightAreaPercent);
    
    m_showTopCheck->SetValue(m_config.showTopArea);
    m_showBottomCheck->SetValue(m_config.showBottomArea);
    m_showLeftCheck->SetValue(m_config.showLeftArea);
    m_showRightCheck->SetValue(m_config.showRightArea);
    
    m_minSizeSpin->SetValue(m_config.minAreaSize);
    m_splitterWidthSpin->SetValue(m_config.splitterWidth);
    m_enableAnimationCheck->SetValue(m_config.enableAnimation);
    m_animationDurationSpin->SetValue(m_config.animationDuration);
}

void DockLayoutConfigEditor::updateControlStates() {
    if (!m_usePercentageCheck) return;
    
    bool usePixels = !m_config.usePercentage;
    
    m_topHeightSpin->Enable(usePixels);
    m_bottomHeightSpin->Enable(usePixels);
    m_leftWidthSpin->Enable(usePixels);
    m_rightWidthSpin->Enable(usePixels);
    m_centerMinWidthSpin->Enable(usePixels);
    m_centerMinHeightSpin->Enable(usePixels);
    
    m_topPercentSpin->Enable(!usePixels);
    m_bottomPercentSpin->Enable(!usePixels);
    m_leftPercentSpin->Enable(!usePixels);
    m_rightPercentSpin->Enable(!usePixels);
    
    m_animationDurationSpin->Enable(m_config.enableAnimation);
}

void DockLayoutConfigEditor::updatePreview() {
    if (m_previewPanel) {
        updateConfigFromControls();
        m_previewPanel->SetConfig(m_config);
        m_previewPanel->Refresh();
    }
}

void DockLayoutConfigEditor::onUsePercentageChanged(wxCommandEvent& event) {
    updateConfigFromControls();
    updateControlStates();
    updatePreview();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void DockLayoutConfigEditor::onValueChanged(wxSpinEvent& event) {
    updateConfigFromControls();
    
    // Validate percentage values
    if (m_config.usePercentage) {
        int totalHorizontal = m_config.leftAreaPercent + m_config.rightAreaPercent;
        int totalVertical = m_config.topAreaPercent + m_config.bottomAreaPercent;
        
        if (totalHorizontal >= 90 || totalVertical >= 90) {
            // Warning is handled by preview display
        }
    }
    
    updatePreview();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void DockLayoutConfigEditor::onCheckChanged(wxCommandEvent& event) {
    updateConfigFromControls();
    updateControlStates();
    updatePreview();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void DockLayoutConfigEditor::onPresetButton(wxCommandEvent& event) {
    int id = event.GetId();
    
    if (id == wxID_HIGHEST + 1) {
        // 20/80 Layout
        m_config.usePercentage = true;
        m_config.leftAreaPercent = 20;
        m_config.rightAreaPercent = 0;
        m_config.topAreaPercent = 0;
        m_config.bottomAreaPercent = 20;
        m_config.showLeftArea = true;
        m_config.showRightArea = false;
        m_config.showTopArea = false;
        m_config.showBottomArea = true;
    } else if (id == wxID_HIGHEST + 2) {
        // 3-Column
        m_config.usePercentage = true;
        m_config.leftAreaPercent = 20;
        m_config.rightAreaPercent = 20;
        m_config.topAreaPercent = 0;
        m_config.bottomAreaPercent = 20;
        m_config.showLeftArea = true;
        m_config.showRightArea = true;
        m_config.showTopArea = false;
        m_config.showBottomArea = true;
    } else if (id == wxID_HIGHEST + 3) {
        // IDE Layout
        m_config.usePercentage = true;
        m_config.leftAreaPercent = 20;
        m_config.rightAreaPercent = 25;
        m_config.topAreaPercent = 10;
        m_config.bottomAreaPercent = 25;
        m_config.showLeftArea = true;
        m_config.showRightArea = true;
        m_config.showTopArea = true;
        m_config.showBottomArea = true;
    }
    
    updateControlsFromConfig();
    updateControlStates();
    updatePreview();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

