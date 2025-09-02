#include "docking/DockLayoutConfig.h"
#include <wx/config.h>
#include <wx/notebook.h>
#include <wx/gbsizer.h>
#include <wx/dcbuffer.h>

namespace ads {

// DockLayoutConfig implementation
void DockLayoutConfig::SaveToConfig() {
    wxConfig config("DockLayout");
    
    config.Write("TopAreaHeight", topAreaHeight);
    config.Write("BottomAreaHeight", bottomAreaHeight);
    config.Write("LeftAreaWidth", leftAreaWidth);
    config.Write("RightAreaWidth", rightAreaWidth);
    config.Write("CenterMinWidth", centerMinWidth);
    config.Write("CenterMinHeight", centerMinHeight);
    
    config.Write("UsePercentage", usePercentage);
    config.Write("TopAreaPercent", topAreaPercent);
    config.Write("BottomAreaPercent", bottomAreaPercent);
    config.Write("LeftAreaPercent", leftAreaPercent);
    config.Write("RightAreaPercent", rightAreaPercent);
    
    config.Write("MinAreaSize", minAreaSize);
    config.Write("SplitterWidth", splitterWidth);
    
    config.Write("ShowTopArea", showTopArea);
    config.Write("ShowBottomArea", showBottomArea);
    config.Write("ShowLeftArea", showLeftArea);
    config.Write("ShowRightArea", showRightArea);
    
    config.Write("EnableAnimation", enableAnimation);
    config.Write("AnimationDuration", animationDuration);
}

void DockLayoutConfig::LoadFromConfig() {
    wxConfig config("DockLayout");
    
    config.Read("TopAreaHeight", &topAreaHeight, 150);
    config.Read("BottomAreaHeight", &bottomAreaHeight, 200);
    config.Read("LeftAreaWidth", &leftAreaWidth, 250);
    config.Read("RightAreaWidth", &rightAreaWidth, 250);
    config.Read("CenterMinWidth", &centerMinWidth, 400);
    config.Read("CenterMinHeight", &centerMinHeight, 300);
    
    config.Read("UsePercentage", &usePercentage, false);
    config.Read("TopAreaPercent", &topAreaPercent, 20);
    config.Read("BottomAreaPercent", &bottomAreaPercent, 25);
    config.Read("LeftAreaPercent", &leftAreaPercent, 20);
    config.Read("RightAreaPercent", &rightAreaPercent, 20);
    
    config.Read("MinAreaSize", &minAreaSize, 100);
    config.Read("SplitterWidth", &splitterWidth, 4);
    
    config.Read("ShowTopArea", &showTopArea, true);
    config.Read("ShowBottomArea", &showBottomArea, true);
    config.Read("ShowLeftArea", &showLeftArea, true);
    config.Read("ShowRightArea", &showRightArea, true);
    
    config.Read("EnableAnimation", &enableAnimation, true);
    config.Read("AnimationDuration", &animationDuration, 200);
}

// Event table for DockLayoutConfigDialog
wxBEGIN_EVENT_TABLE(DockLayoutConfigDialog, wxDialog)
    EVT_CHECKBOX(wxID_ANY, DockLayoutConfigDialog::OnCheckChanged)
    EVT_SPINCTRL(wxID_ANY, DockLayoutConfigDialog::OnValueChanged)
    EVT_BUTTON(wxID_APPLY, DockLayoutConfigDialog::OnApply)
    EVT_BUTTON(wxID_RESET, DockLayoutConfigDialog::OnReset)
wxEND_EVENT_TABLE()

// DockLayoutConfigDialog implementation
DockLayoutConfigDialog::DockLayoutConfigDialog(wxWindow* parent, DockLayoutConfig& config, DockManager* dockManager)
    : wxDialog(parent, wxID_ANY, "Dock Layout Configuration", 
              wxDefaultPosition, wxSize(1200, 700),
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_config(config)
    , m_dockManager(dockManager)
{
    CreateControls();
    UpdateControlStates();
    UpdatePreview();
    
    // Center on parent
    CenterOnParent();
}

void DockLayoutConfigDialog::CreateControls() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create horizontal sizer for notebook and preview
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Create notebook for organized settings
    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);
    
    // Size settings page
    wxPanel* sizePage = new wxPanel(notebook);
    wxBoxSizer* sizeSizer = new wxBoxSizer(wxVERTICAL);
    CreateSizeControls(sizePage, sizeSizer);
    sizePage->SetSizer(sizeSizer);
    notebook->AddPage(sizePage, "Sizes");
    
    // Visibility settings page
    wxPanel* visibilityPage = new wxPanel(notebook);
    wxBoxSizer* visibilitySizer = new wxBoxSizer(wxVERTICAL);
    CreateVisibilityControls(visibilityPage, visibilitySizer);
    visibilityPage->SetSizer(visibilitySizer);
    notebook->AddPage(visibilityPage, "Visibility");
    
    // Options page
    wxPanel* optionsPage = new wxPanel(notebook);
    wxBoxSizer* optionsSizer = new wxBoxSizer(wxVERTICAL);
    CreateOptionControls(optionsPage, optionsSizer);
    optionsPage->SetSizer(optionsSizer);
    notebook->AddPage(optionsPage, "Options");
    
    // Add notebook to left side (50% of width)
    contentSizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    
    // Preview panel on right side (50% of width)
    CreatePreviewPanel(this, contentSizer);
    
    mainSizer->Add(contentSizer, 1, wxEXPAND);
    
    // Preset buttons
    wxStaticBoxSizer* presetBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Quick Presets");
    
    wxButton* preset2080Btn = new wxButton(this, wxID_HIGHEST + 1, "20/80 Layout");
    preset2080Btn->SetToolTip("Left: 20%, Center: 80%");
    preset2080Btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_config.usePercentage = true;
        m_config.leftAreaPercent = 20;
        m_config.rightAreaPercent = 0;
        m_config.topAreaPercent = 0;
        m_config.bottomAreaPercent = 20;
        m_config.showLeftArea = true;
        m_config.showRightArea = false;
        m_config.showTopArea = false;
        m_config.showBottomArea = true;
        UpdateControlValues();
        UpdateControlStates();
        UpdatePreview();
    });
    
    wxButton* preset3Column = new wxButton(this, wxID_HIGHEST + 2, "3-Column");
    preset3Column->SetToolTip("Left: 20%, Center: 60%, Right: 20%");
    preset3Column->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_config.usePercentage = true;
        m_config.leftAreaPercent = 20;
        m_config.rightAreaPercent = 20;
        m_config.topAreaPercent = 0;
        m_config.bottomAreaPercent = 20;
        m_config.showLeftArea = true;
        m_config.showRightArea = true;
        m_config.showTopArea = false;
        m_config.showBottomArea = true;
        UpdateControlValues();
        UpdateControlStates();
        UpdatePreview();
    });
    
    wxButton* presetIDE = new wxButton(this, wxID_HIGHEST + 3, "IDE Layout");
    presetIDE->SetToolTip("Classic IDE layout with all panels");
    presetIDE->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_config.usePercentage = true;
        m_config.leftAreaPercent = 20;
        m_config.rightAreaPercent = 25;
        m_config.topAreaPercent = 10;
        m_config.bottomAreaPercent = 25;
        m_config.showLeftArea = true;
        m_config.showRightArea = true;
        m_config.showTopArea = true;
        m_config.showBottomArea = true;
        UpdateControlValues();
        UpdateControlStates();
        UpdatePreview();
    });
    
    presetBox->Add(preset2080Btn, 0, wxALL, 5);
    presetBox->Add(preset3Column, 0, wxALL, 5);
    presetBox->Add(presetIDE, 0, wxALL, 5);
    presetBox->AddStretchSpacer();
    
    mainSizer->Add(presetBox, 0, wxEXPAND | wxALL, 5);
    
    // Button sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* resetBtn = new wxButton(this, wxID_RESET, "Reset to Defaults");
    wxButton* applyBtn = new wxButton(this, wxID_APPLY, "Apply");
    wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    
    buttonSizer->Add(resetBtn, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(applyBtn, 0, wxALL, 5);
    buttonSizer->Add(okBtn, 0, wxALL, 5);
    buttonSizer->Add(cancelBtn, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
}

void DockLayoutConfigDialog::CreateSizeControls(wxWindow* parent, wxSizer* sizer) {
    // Use percentage checkbox
    m_usePercentageCheck = new wxCheckBox(parent, wxID_ANY, "Use Percentage Values");
    m_usePercentageCheck->SetValue(m_config.usePercentage);
    sizer->Add(m_usePercentageCheck, 0, wxALL, 5);
    
    // Pixel size controls
    wxStaticBoxSizer* pixelBox = new wxStaticBoxSizer(wxVERTICAL, parent, "Size in Pixels");
    wxGridBagSizer* pixelGrid = new wxGridBagSizer(5, 5);
    
    int row = 0;
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Top Height:"), 
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_topHeightSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS, 50, 500, m_config.topAreaHeight);
    pixelGrid->Add(m_topHeightSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Bottom Height:"), 
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_bottomHeightSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                        wxSP_ARROW_KEYS, 50, 500, m_config.bottomAreaHeight);
    pixelGrid->Add(m_bottomHeightSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Left Width:"), 
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_leftWidthSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS, 50, 500, m_config.leftAreaWidth);
    pixelGrid->Add(m_leftWidthSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Right Width:"), 
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_rightWidthSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                      wxSP_ARROW_KEYS, 50, 500, m_config.rightAreaWidth);
    pixelGrid->Add(m_rightWidthSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Center Min Width:"), 
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_centerMinWidthSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                          wxSP_ARROW_KEYS, 100, 800, m_config.centerMinWidth);
    pixelGrid->Add(m_centerMinWidthSpin, wxGBPosition(row++, 1));
    
    pixelGrid->Add(new wxStaticText(parent, wxID_ANY, "Center Min Height:"), 
                   wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_centerMinHeightSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                           wxSP_ARROW_KEYS, 100, 600, m_config.centerMinHeight);
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
                                      wxSP_ARROW_KEYS, 5, 50, m_config.topAreaPercent);
    percentGrid->Add(m_topPercentSpin, wxGBPosition(row++, 1));
    
    percentGrid->Add(new wxStaticText(parent, wxID_ANY, "Bottom Height %:"), 
                     wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_bottomPercentSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                         wxSP_ARROW_KEYS, 5, 50, m_config.bottomAreaPercent);
    percentGrid->Add(m_bottomPercentSpin, wxGBPosition(row++, 1));
    
    percentGrid->Add(new wxStaticText(parent, wxID_ANY, "Left Width %:"), 
                     wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_leftPercentSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                       wxSP_ARROW_KEYS, 5, 50, m_config.leftAreaPercent);
    percentGrid->Add(m_leftPercentSpin, wxGBPosition(row++, 1));
    
    percentGrid->Add(new wxStaticText(parent, wxID_ANY, "Right Width %:"), 
                     wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_rightPercentSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                        wxSP_ARROW_KEYS, 5, 50, m_config.rightAreaPercent);
    percentGrid->Add(m_rightPercentSpin, wxGBPosition(row++, 1));
    
    percentBox->Add(percentGrid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(percentBox, 0, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigDialog::CreateVisibilityControls(wxWindow* parent, wxSizer* sizer) {
    wxStaticBoxSizer* visBox = new wxStaticBoxSizer(wxVERTICAL, parent, "Area Visibility");
    
    m_showTopCheck = new wxCheckBox(parent, wxID_ANY, "Show Top Area");
    m_showTopCheck->SetValue(m_config.showTopArea);
    visBox->Add(m_showTopCheck, 0, wxALL, 5);
    
    m_showBottomCheck = new wxCheckBox(parent, wxID_ANY, "Show Bottom Area");
    m_showBottomCheck->SetValue(m_config.showBottomArea);
    visBox->Add(m_showBottomCheck, 0, wxALL, 5);
    
    m_showLeftCheck = new wxCheckBox(parent, wxID_ANY, "Show Left Area");
    m_showLeftCheck->SetValue(m_config.showLeftArea);
    visBox->Add(m_showLeftCheck, 0, wxALL, 5);
    
    m_showRightCheck = new wxCheckBox(parent, wxID_ANY, "Show Right Area");
    m_showRightCheck->SetValue(m_config.showRightArea);
    visBox->Add(m_showRightCheck, 0, wxALL, 5);
    
    sizer->Add(visBox, 0, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigDialog::CreateOptionControls(wxWindow* parent, wxSizer* sizer) {
    wxStaticBoxSizer* optBox = new wxStaticBoxSizer(wxVERTICAL, parent, "General Options");
    wxGridBagSizer* grid = new wxGridBagSizer(5, 5);
    
    int row = 0;
    grid->Add(new wxStaticText(parent, wxID_ANY, "Minimum Area Size:"), 
              wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_minSizeSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS, 50, 300, m_config.minAreaSize);
    grid->Add(m_minSizeSpin, wxGBPosition(row++, 1));
    
    grid->Add(new wxStaticText(parent, wxID_ANY, "Splitter Width:"), 
              wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_splitterWidthSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                         wxSP_ARROW_KEYS, 1, 10, m_config.splitterWidth);
    grid->Add(m_splitterWidthSpin, wxGBPosition(row++, 1));
    
    m_enableAnimationCheck = new wxCheckBox(parent, wxID_ANY, "Enable Animation");
    m_enableAnimationCheck->SetValue(m_config.enableAnimation);
    grid->Add(m_enableAnimationCheck, wxGBPosition(row++, 0), wxGBSpan(1, 2));
    
    grid->Add(new wxStaticText(parent, wxID_ANY, "Animation Duration (ms):"), 
              wxGBPosition(row, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
    m_animationDurationSpin = new wxSpinCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                             wxSP_ARROW_KEYS, 50, 1000, m_config.animationDuration);
    grid->Add(m_animationDurationSpin, wxGBPosition(row++, 1));
    
    optBox->Add(grid, 1, wxEXPAND | wxALL, 5);
    sizer->Add(optBox, 0, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigDialog::CreatePreviewPanel(wxWindow* parent, wxSizer* sizer) {
    wxStaticBoxSizer* previewBox = new wxStaticBoxSizer(wxVERTICAL, parent, "Layout Preview");
    
    m_previewPanel = new DockLayoutPreview(parent);
    m_previewPanel->SetMinSize(wxSize(500, 400));
    m_previewPanel->SetConfig(m_config);
    
    // Add preview info text
    wxStaticText* infoText = new wxStaticText(parent, wxID_ANY, 
        "Preview shows relative sizes. Click and drag splitters in the actual application to fine-tune.");
    infoText->Wrap(480);
    
    previewBox->Add(m_previewPanel, 1, wxEXPAND | wxALL, 5);
    previewBox->Add(infoText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    sizer->Add(previewBox, 1, wxEXPAND | wxALL, 5);
}

void DockLayoutConfigDialog::OnUsePercentageChanged(wxCommandEvent& event) {
    m_config.usePercentage = m_usePercentageCheck->GetValue();
    UpdateControlStates();
    UpdatePreview();
}

void DockLayoutConfigDialog::OnValueChanged(wxSpinEvent& event) {
    // Update config from controls
    m_config.topAreaHeight = m_topHeightSpin->GetValue();
    m_config.bottomAreaHeight = m_bottomHeightSpin->GetValue();
    m_config.leftAreaWidth = m_leftWidthSpin->GetValue();
    m_config.rightAreaWidth = m_rightWidthSpin->GetValue();
    m_config.centerMinWidth = m_centerMinWidthSpin->GetValue();
    m_config.centerMinHeight = m_centerMinHeightSpin->GetValue();
    
    m_config.topAreaPercent = m_topPercentSpin->GetValue();
    m_config.bottomAreaPercent = m_bottomPercentSpin->GetValue();
    m_config.leftAreaPercent = m_leftPercentSpin->GetValue();
    m_config.rightAreaPercent = m_rightPercentSpin->GetValue();
    
    // Validate percentage values don't exceed 100% when combined
    if (m_config.usePercentage) {
        int totalHorizontal = m_config.leftAreaPercent + m_config.rightAreaPercent;
        int totalVertical = m_config.topAreaPercent + m_config.bottomAreaPercent;
        
        if (totalHorizontal >= 90) {
            wxMessageBox("Left + Right percentages should not exceed 90% to leave room for center area.", 
                        "Warning", wxOK | wxICON_WARNING);
        }
        if (totalVertical >= 90) {
            wxMessageBox("Top + Bottom percentages should not exceed 90% to leave room for center area.", 
                        "Warning", wxOK | wxICON_WARNING);
        }
    }
    
    m_config.minAreaSize = m_minSizeSpin->GetValue();
    m_config.splitterWidth = m_splitterWidthSpin->GetValue();
    m_config.animationDuration = m_animationDurationSpin->GetValue();
    
    UpdatePreview();
}

void DockLayoutConfigDialog::OnCheckChanged(wxCommandEvent& event) {
    if (event.GetEventObject() == m_usePercentageCheck) {
        OnUsePercentageChanged(event);
    } else {
        m_config.showTopArea = m_showTopCheck->GetValue();
        m_config.showBottomArea = m_showBottomCheck->GetValue();
        m_config.showLeftArea = m_showLeftCheck->GetValue();
        m_config.showRightArea = m_showRightCheck->GetValue();
        m_config.enableAnimation = m_enableAnimationCheck->GetValue();
        
        UpdatePreview();
    }
}

void DockLayoutConfigDialog::OnApply(wxCommandEvent& event) {
    ApplyToManager();
}

void DockLayoutConfigDialog::OnReset(wxCommandEvent& event) {
    m_config = DockLayoutConfig(); // Reset to defaults
    UpdateControlValues();
    UpdateControlStates();
    UpdatePreview();
}

void DockLayoutConfigDialog::UpdateControlValues() {
    // Update all controls from config
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

void DockLayoutConfigDialog::UpdatePreview() {
    if (m_previewPanel) {
        m_previewPanel->SetConfig(m_config);
        m_previewPanel->Refresh();
    }
}

void DockLayoutConfigDialog::UpdateControlStates() {
    bool usePixels = !m_config.usePercentage;
    
    // Enable/disable controls based on mode
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
    
    // Enable/disable animation duration based on animation enabled
    m_animationDurationSpin->Enable(m_config.enableAnimation);
}

void DockLayoutConfigDialog::ApplyToManager() {
    // Save configuration
    m_config.SaveToConfig();
    
    // Apply to DockManager if available
    if (m_dockManager) {
        m_dockManager->setLayoutConfig(m_config);
        
        // Apply to all containers
        auto containers = m_dockManager->dockContainers();
        for (auto* container : containers) {
            container->applyLayoutConfig();
        }
    }
}

// Event table for DockLayoutPreview
wxBEGIN_EVENT_TABLE(DockLayoutPreview, wxPanel)
    EVT_PAINT(DockLayoutPreview::OnPaint)
    EVT_SIZE(DockLayoutPreview::OnSize)
wxEND_EVENT_TABLE()

// DockLayoutPreview implementation
DockLayoutPreview::DockLayoutPreview(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);
}

void DockLayoutPreview::SetConfig(const DockLayoutConfig& config) {
    m_config = config;
    Refresh();
}

void DockLayoutPreview::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    
    DrawLayoutPreview(dc);
}

void DockLayoutPreview::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

void DockLayoutPreview::DrawLayoutPreview(wxDC& dc) {
    wxRect clientRect = GetClientRect();
    clientRect.Deflate(10); // Margin
    
    // Colors for different areas
    wxColour topColor(200, 200, 255);      // Light blue
    wxColour bottomColor(200, 255, 200);   // Light green
    wxColour leftColor(255, 200, 200);     // Light red
    wxColour rightColor(255, 255, 200);    // Light yellow
    wxColour centerColor(240, 240, 240);   // Light gray
    wxColour borderColor(100, 100, 100);   // Dark gray
    wxColour textColor(50, 50, 50);       // Dark gray for text
    
    // Calculate areas
    wxRect topRect = CalculateAreaRect(TopDockWidgetArea, clientRect);
    wxRect bottomRect = CalculateAreaRect(BottomDockWidgetArea, clientRect);
    wxRect leftRect = CalculateAreaRect(LeftDockWidgetArea, clientRect);
    wxRect rightRect = CalculateAreaRect(RightDockWidgetArea, clientRect);
    wxRect centerRect = CalculateAreaRect(CenterDockWidgetArea, clientRect);
    
    // Draw areas
    dc.SetPen(wxPen(borderColor, 1));
    
    if (m_config.showTopArea && !topRect.IsEmpty()) {
        dc.SetBrush(wxBrush(topColor));
        dc.DrawRectangle(topRect);
        
        // Draw area label and size info
        dc.SetTextForeground(textColor);
        wxString label = "Top";
        if (m_config.usePercentage) {
            label += wxString::Format(" (%d%%)", m_config.topAreaPercent);
        } else {
            label += wxString::Format(" (%dpx)", m_config.topAreaHeight);
        }
        dc.DrawText(label, topRect.x + 5, topRect.y + 5);
    }
    
    if (m_config.showBottomArea && !bottomRect.IsEmpty()) {
        dc.SetBrush(wxBrush(bottomColor));
        dc.DrawRectangle(bottomRect);
        
        // Draw area label and size info
        dc.SetTextForeground(textColor);
        wxString label = "Bottom";
        if (m_config.usePercentage) {
            label += wxString::Format(" (%d%%)", m_config.bottomAreaPercent);
        } else {
            label += wxString::Format(" (%dpx)", m_config.bottomAreaHeight);
        }
        dc.DrawText(label, bottomRect.x + 5, bottomRect.y + 5);
    }
    
    if (m_config.showLeftArea && !leftRect.IsEmpty()) {
        dc.SetBrush(wxBrush(leftColor));
        dc.DrawRectangle(leftRect);
        
        // Draw area label and size info
        dc.SetTextForeground(textColor);
        wxString label = "Left";
        if (m_config.usePercentage) {
            label += wxString::Format(" (%d%%)", m_config.leftAreaPercent);
        } else {
            label += wxString::Format(" (%dpx)", m_config.leftAreaWidth);
        }
        dc.DrawText(label, leftRect.x + 5, leftRect.y + 5);
    }
    
    if (m_config.showRightArea && !rightRect.IsEmpty()) {
        dc.SetBrush(wxBrush(rightColor));
        dc.DrawRectangle(rightRect);
        
        // Draw area label and size info
        dc.SetTextForeground(textColor);
        wxString label = "Right";
        if (m_config.usePercentage) {
            label += wxString::Format(" (%d%%)", m_config.rightAreaPercent);
        } else {
            label += wxString::Format(" (%dpx)", m_config.rightAreaWidth);
        }
        dc.DrawText(label, rightRect.x + 5, rightRect.y + 5);
    }
    
    // Always draw center
    dc.SetBrush(wxBrush(centerColor));
    dc.DrawRectangle(centerRect);
    
    // Draw center label with calculated percentage
    dc.SetTextForeground(textColor);
    wxString centerLabel = "Center";
    if (m_config.usePercentage) {
        int leftRight = (m_config.showLeftArea ? m_config.leftAreaPercent : 0) + 
                       (m_config.showRightArea ? m_config.rightAreaPercent : 0);
        int topBottom = (m_config.showTopArea ? m_config.topAreaPercent : 0) + 
                       (m_config.showBottomArea ? m_config.bottomAreaPercent : 0);
        centerLabel += wxString::Format(" (H:%d%%, V:%d%%)", 100 - leftRight, 100 - topBottom);
    }
    dc.DrawText(centerLabel, centerRect.x + 5, centerRect.y + 5);
    
    // Draw splitter lines
    dc.SetPen(wxPen(borderColor, m_config.splitterWidth));
    
    if (m_config.showTopArea) {
        dc.DrawLine(clientRect.x, topRect.GetBottom(), 
                   clientRect.GetRight(), topRect.GetBottom());
    }
    
    if (m_config.showBottomArea) {
        dc.DrawLine(clientRect.x, bottomRect.GetTop(), 
                   clientRect.GetRight(), bottomRect.GetTop());
    }
    
    if (m_config.showLeftArea) {
        dc.DrawLine(leftRect.GetRight(), leftRect.GetTop(), 
                   leftRect.GetRight(), leftRect.GetBottom());
    }
    
    if (m_config.showRightArea) {
        dc.DrawLine(rightRect.GetLeft(), rightRect.GetTop(), 
                   rightRect.GetLeft(), rightRect.GetBottom());
    }
}

wxRect DockLayoutPreview::CalculateAreaRect(DockWidgetArea area, const wxRect& totalRect) {
    int topHeight = 0, bottomHeight = 0, leftWidth = 0, rightWidth = 0;
    
    if (m_config.usePercentage) {
        // Calculate from percentages
        if (m_config.showTopArea) {
            topHeight = totalRect.height * m_config.topAreaPercent / 100;
        }
        if (m_config.showBottomArea) {
            bottomHeight = totalRect.height * m_config.bottomAreaPercent / 100;
        }
        if (m_config.showLeftArea) {
            leftWidth = totalRect.width * m_config.leftAreaPercent / 100;
        }
        if (m_config.showRightArea) {
            rightWidth = totalRect.width * m_config.rightAreaPercent / 100;
        }
    } else {
        // Scale pixel values to fit preview
        double scaleX = (double)totalRect.width / 1200.0;  // Assume 1200px reference width
        double scaleY = (double)totalRect.height / 800.0;  // Assume 800px reference height
        
        if (m_config.showTopArea) {
            topHeight = m_config.topAreaHeight * scaleY;
        }
        if (m_config.showBottomArea) {
            bottomHeight = m_config.bottomAreaHeight * scaleY;
        }
        if (m_config.showLeftArea) {
            leftWidth = m_config.leftAreaWidth * scaleX;
        }
        if (m_config.showRightArea) {
            rightWidth = m_config.rightAreaWidth * scaleX;
        }
    }
    
    // Calculate actual rectangles
    switch (area) {
    case TopDockWidgetArea:
        if (m_config.showTopArea) {
            return wxRect(totalRect.x, totalRect.y, totalRect.width, topHeight);
        }
        break;
        
    case BottomDockWidgetArea:
        if (m_config.showBottomArea) {
            return wxRect(totalRect.x, totalRect.GetBottom() - bottomHeight, 
                         totalRect.width, bottomHeight);
        }
        break;
        
    case LeftDockWidgetArea:
        if (m_config.showLeftArea) {
            int y = totalRect.y + topHeight;
            int h = totalRect.height - topHeight - bottomHeight;
            return wxRect(totalRect.x, y, leftWidth, h);
        }
        break;
        
    case RightDockWidgetArea:
        if (m_config.showRightArea) {
            int y = totalRect.y + topHeight;
            int h = totalRect.height - topHeight - bottomHeight;
            return wxRect(totalRect.GetRight() - rightWidth, y, rightWidth, h);
        }
        break;
        
    case CenterDockWidgetArea:
        {
            int x = totalRect.x + leftWidth;
            int y = totalRect.y + topHeight;
            int w = totalRect.width - leftWidth - rightWidth;
            int h = totalRect.height - topHeight - bottomHeight;
            return wxRect(x, y, w, h);
        }
        break;
    }
    
    return wxRect();
}

} // namespace ads