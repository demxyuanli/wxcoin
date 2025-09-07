// DockLayoutConfigDialog.cpp - Implementation of DockLayoutConfigDialog class

#include "docking/DockLayoutConfig.h"
#include "docking/DockContainerWidget.h"
#include "docking/DockManager.h"
#include <wx/config.h>
#include <wx/notebook.h>
#include <wx/gbsizer.h>
#include <wx/dcbuffer.h>

namespace ads {

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

        // Apply to the main container
        if (wxWindow* containerWidget = m_dockManager->containerWidget()) {
            if (DockContainerWidget* container = dynamic_cast<DockContainerWidget*>(containerWidget)) {
                container->applyLayoutConfig();
            }
        }
    }
}

} // namespace ads
