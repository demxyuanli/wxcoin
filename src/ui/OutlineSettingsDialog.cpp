#include "ui/OutlineSettingsDialog.h"
#include "OCCViewer.h"
#include "logger/Logger.h"

#include <wx/button.h>
#include <wx/statbox.h>

enum {
    ID_ENABLE_OUTLINE = 1000,
    ID_ENABLE_HOVER,
    ID_INTENSITY_SLIDER,
    ID_THICKNESS_SLIDER,
    ID_DEPTH_WEIGHT_SLIDER,
    ID_NORMAL_WEIGHT_SLIDER,
    ID_DEPTH_THRESHOLD_SLIDER,
    ID_NORMAL_THRESHOLD_SLIDER,
    ID_RESET_DEFAULTS
};

wxBEGIN_EVENT_TABLE(OutlineSettingsDialog, wxDialog)
    EVT_CHECKBOX(ID_ENABLE_OUTLINE, OutlineSettingsDialog::onCheckboxChange)
    EVT_CHECKBOX(ID_ENABLE_HOVER, OutlineSettingsDialog::onCheckboxChange)
    EVT_SLIDER(ID_INTENSITY_SLIDER, OutlineSettingsDialog::onSliderChange)
    EVT_SLIDER(ID_THICKNESS_SLIDER, OutlineSettingsDialog::onSliderChange)
    EVT_SLIDER(ID_DEPTH_WEIGHT_SLIDER, OutlineSettingsDialog::onSliderChange)
    EVT_SLIDER(ID_NORMAL_WEIGHT_SLIDER, OutlineSettingsDialog::onSliderChange)
    EVT_SLIDER(ID_DEPTH_THRESHOLD_SLIDER, OutlineSettingsDialog::onSliderChange)
    EVT_SLIDER(ID_NORMAL_THRESHOLD_SLIDER, OutlineSettingsDialog::onSliderChange)
    EVT_BUTTON(ID_RESET_DEFAULTS, OutlineSettingsDialog::onResetDefaults)
    EVT_CLOSE(OutlineSettingsDialog::onClose)
wxEND_EVENT_TABLE()

OutlineSettingsDialog::OutlineSettingsDialog(wxWindow* parent, OCCViewer* occViewer)
    : wxDialog(parent, wxID_ANY, "Outline Settings", wxDefaultPosition, wxSize(400, 500),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_occViewer(occViewer)
{
    if (m_occViewer) {
        m_params = m_occViewer->getOutlineParams();
    }
    
    createControls();
    updateControls();
    
    LOG_INF_S("OutlineSettingsDialog created");
}

OutlineSettingsDialog::~OutlineSettingsDialog() {
    LOG_INF_S("OutlineSettingsDialog destroyed");
}

void OutlineSettingsDialog::createControls() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Enable/disable controls
    auto* enableBox = new wxStaticBoxSizer(wxVERTICAL, this, "Enable Features");
    
    m_enableOutline = new wxCheckBox(this, ID_ENABLE_OUTLINE, "Enable Outline Rendering");
    m_enableHover = new wxCheckBox(this, ID_ENABLE_HOVER, "Enable Hover Highlighting");
    
    enableBox->Add(m_enableOutline, 0, wxALL, 5);
    enableBox->Add(m_enableHover, 0, wxALL, 5);
    mainSizer->Add(enableBox, 0, wxEXPAND | wxALL, 10);
    
    // Parameter controls
    auto* paramBox = new wxStaticBoxSizer(wxVERTICAL, this, "Outline Parameters");
    
    // Intensity (0.0 - 1.0)
    auto* intensitySizer = new wxBoxSizer(wxHORIZONTAL);
    intensitySizer->Add(new wxStaticText(this, wxID_ANY, "Intensity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_intensitySlider = new wxSlider(this, ID_INTENSITY_SLIDER, 100, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_intensityLabel = new wxStaticText(this, wxID_ANY, "1.00");
    intensitySizer->Add(m_intensitySlider, 1, wxEXPAND);
    intensitySizer->Add(m_intensityLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    paramBox->Add(intensitySizer, 0, wxEXPAND | wxALL, 5);
    
    // Thickness (0.5 - 3.0)
    auto* thicknessSizer = new wxBoxSizer(wxHORIZONTAL);
    thicknessSizer->Add(new wxStaticText(this, wxID_ANY, "Thickness:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_thicknessSlider = new wxSlider(this, ID_THICKNESS_SLIDER, 50, 25, 150, wxDefaultPosition, wxSize(200, -1));
    m_thicknessLabel = new wxStaticText(this, wxID_ANY, "1.00");
    thicknessSizer->Add(m_thicknessSlider, 1, wxEXPAND);
    thicknessSizer->Add(m_thicknessLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    paramBox->Add(thicknessSizer, 0, wxEXPAND | wxALL, 5);
    
    // Depth Weight (0.0 - 2.0)
    auto* depthWeightSizer = new wxBoxSizer(wxHORIZONTAL);
    depthWeightSizer->Add(new wxStaticText(this, wxID_ANY, "Depth Weight:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_depthWeightSlider = new wxSlider(this, ID_DEPTH_WEIGHT_SLIDER, 100, 0, 200, wxDefaultPosition, wxSize(200, -1));
    m_depthWeightLabel = new wxStaticText(this, wxID_ANY, "2.00");
    depthWeightSizer->Add(m_depthWeightSlider, 1, wxEXPAND);
    depthWeightSizer->Add(m_depthWeightLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    paramBox->Add(depthWeightSizer, 0, wxEXPAND | wxALL, 5);
    
    // Normal Weight (0.0 - 2.0)
    auto* normalWeightSizer = new wxBoxSizer(wxHORIZONTAL);
    normalWeightSizer->Add(new wxStaticText(this, wxID_ANY, "Normal Weight:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_normalWeightSlider = new wxSlider(this, ID_NORMAL_WEIGHT_SLIDER, 50, 0, 200, wxDefaultPosition, wxSize(200, -1));
    m_normalWeightLabel = new wxStaticText(this, wxID_ANY, "1.00");
    normalWeightSizer->Add(m_normalWeightSlider, 1, wxEXPAND);
    normalWeightSizer->Add(m_normalWeightLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    paramBox->Add(normalWeightSizer, 0, wxEXPAND | wxALL, 5);
    
    // Depth Threshold (0.0001 - 0.01)
    auto* depthThresholdSizer = new wxBoxSizer(wxHORIZONTAL);
    depthThresholdSizer->Add(new wxStaticText(this, wxID_ANY, "Depth Threshold:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_depthThresholdSlider = new wxSlider(this, ID_DEPTH_THRESHOLD_SLIDER, 5, 1, 100, wxDefaultPosition, wxSize(200, -1));
    m_depthThresholdLabel = new wxStaticText(this, wxID_ANY, "0.0005");
    depthThresholdSizer->Add(m_depthThresholdSlider, 1, wxEXPAND);
    depthThresholdSizer->Add(m_depthThresholdLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    paramBox->Add(depthThresholdSizer, 0, wxEXPAND | wxALL, 5);
    
    // Normal Threshold (0.01 - 1.0)
    auto* normalThresholdSizer = new wxBoxSizer(wxHORIZONTAL);
    normalThresholdSizer->Add(new wxStaticText(this, wxID_ANY, "Normal Threshold:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_normalThresholdSlider = new wxSlider(this, ID_NORMAL_THRESHOLD_SLIDER, 10, 1, 100, wxDefaultPosition, wxSize(200, -1));
    m_normalThresholdLabel = new wxStaticText(this, wxID_ANY, "0.10");
    normalThresholdSizer->Add(m_normalThresholdSlider, 1, wxEXPAND);
    normalThresholdSizer->Add(m_normalThresholdLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    paramBox->Add(normalThresholdSizer, 0, wxEXPAND | wxALL, 5);
    
    mainSizer->Add(paramBox, 1, wxEXPAND | wxALL, 10);
    
    // Buttons
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, ID_RESET_DEFAULTS, "Reset Defaults"), 0, wxRIGHT, 10);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(mainSizer);
}

void OutlineSettingsDialog::updateControls() {
    if (!m_occViewer) return;
    
    // Update checkboxes
    m_enableOutline->SetValue(m_occViewer->isOutlineEnabled());
    m_enableHover->SetValue(m_occViewer->isHoverHighlightEnabled());
    
    // Update sliders and labels
    m_intensitySlider->SetValue(static_cast<int>(m_params.edgeIntensity * 100));
    m_intensityLabel->SetLabel(wxString::Format("%.2f", m_params.edgeIntensity));
    
    m_thicknessSlider->SetValue(static_cast<int>(m_params.thickness * 50));
    m_thicknessLabel->SetLabel(wxString::Format("%.2f", m_params.thickness));
    
    m_depthWeightSlider->SetValue(static_cast<int>(m_params.depthWeight * 50));
    m_depthWeightLabel->SetLabel(wxString::Format("%.2f", m_params.depthWeight));
    
    m_normalWeightSlider->SetValue(static_cast<int>(m_params.normalWeight * 100));
    m_normalWeightLabel->SetLabel(wxString::Format("%.2f", m_params.normalWeight));
    
    m_depthThresholdSlider->SetValue(static_cast<int>(m_params.depthThreshold * 10000));
    m_depthThresholdLabel->SetLabel(wxString::Format("%.4f", m_params.depthThreshold));
    
    m_normalThresholdSlider->SetValue(static_cast<int>(m_params.normalThreshold * 100));
    m_normalThresholdLabel->SetLabel(wxString::Format("%.2f", m_params.normalThreshold));
}

void OutlineSettingsDialog::onSliderChange(wxCommandEvent& event) {
    if (!m_occViewer) return;
    
    // Update parameter values from sliders
    m_params.edgeIntensity = m_intensitySlider->GetValue() / 100.0f;
    m_params.thickness = m_thicknessSlider->GetValue() / 50.0f;
    m_params.depthWeight = m_depthWeightSlider->GetValue() / 50.0f;
    m_params.normalWeight = m_normalWeightSlider->GetValue() / 100.0f;
    m_params.depthThreshold = m_depthThresholdSlider->GetValue() / 10000.0f;
    m_params.normalThreshold = m_normalThresholdSlider->GetValue() / 100.0f;
    
    // Update labels
    m_intensityLabel->SetLabel(wxString::Format("%.2f", m_params.edgeIntensity));
    m_thicknessLabel->SetLabel(wxString::Format("%.2f", m_params.thickness));
    m_depthWeightLabel->SetLabel(wxString::Format("%.2f", m_params.depthWeight));
    m_normalWeightLabel->SetLabel(wxString::Format("%.2f", m_params.normalWeight));
    m_depthThresholdLabel->SetLabel(wxString::Format("%.4f", m_params.depthThreshold));
    m_normalThresholdLabel->SetLabel(wxString::Format("%.2f", m_params.normalThreshold));
    
    // Apply changes immediately for real-time preview
    applyParams();
}

void OutlineSettingsDialog::onCheckboxChange(wxCommandEvent& event) {
    if (!m_occViewer) return;
    
    switch (event.GetId()) {
        case ID_ENABLE_OUTLINE:
            m_occViewer->setOutlineEnabled(m_enableOutline->GetValue());
            LOG_INF_S(std::string("Outline ") + (m_enableOutline->GetValue() ? "enabled" : "disabled") + " via UI");
            break;
            
        case ID_ENABLE_HOVER:
            m_occViewer->setHoverHighlightEnabled(m_enableHover->GetValue());
            LOG_INF_S(std::string("Hover highlighting ") + (m_enableHover->GetValue() ? "enabled" : "disabled") + " via UI");
            break;
    }
}

void OutlineSettingsDialog::onResetDefaults(wxCommandEvent& event) {
    // Reset to default parameters
    m_params.edgeIntensity = 1.0f;
    m_params.depthWeight = 2.0f;
    m_params.normalWeight = 1.0f;
    m_params.depthThreshold = 0.0005f;
    m_params.normalThreshold = 0.1f;
    m_params.thickness = 1.0f;
    
    updateControls();
    applyParams();
    
    LOG_INF_S("Outline parameters reset to defaults");
}

void OutlineSettingsDialog::onClose(wxCloseEvent& event) {
    if (event.CanVeto() && IsModal()) {
        EndModal(wxID_CANCEL);
    } else {
        Destroy();
    }
}

void OutlineSettingsDialog::applyParams() {
    if (m_occViewer) {
        m_occViewer->setOutlineParams(m_params);
    }
}