#include "ImportSettingsDialog.h"
#include "logger/Logger.h"
#include <wx/statline.h>

wxBEGIN_EVENT_TABLE(ImportSettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ImportSettingsDialog::onOK)
    EVT_BUTTON(wxID_CANCEL, ImportSettingsDialog::onCancel)
wxEND_EVENT_TABLE()

ImportSettingsDialog::ImportSettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Import Settings", wxDefaultPosition, wxSize(500, 600))
    , m_deflection(1.0)
    , m_angularDeflection(1.0)
    , m_enableLOD(true)
    , m_parallelProcessing(true)
    , m_adaptiveMeshing(false)
    , m_autoOptimize(true)
    , m_importMode(0)
{
    createControls();
    layoutControls();
    bindEvents();
    
    // Apply default balanced preset
    applyPreset(1.0, 1.0, true, true);
}

ImportSettingsDialog::~ImportSettingsDialog()
{
}

void ImportSettingsDialog::createControls()
{
    // Preset buttons
    wxPanel* presetPanel = new wxPanel(this);
    wxButton* perfBtn = new wxButton(presetPanel, wxID_ANY, "🚀 Performance");
    wxButton* balBtn = new wxButton(presetPanel, wxID_ANY, "⚡ Balanced");
    wxButton* qualBtn = new wxButton(presetPanel, wxID_ANY, "💎 Quality");
    
    perfBtn->SetToolTip("Fast import with lower quality meshes");
    balBtn->SetToolTip("Balanced import settings");
    qualBtn->SetToolTip("High quality import, slower processing");
    
    perfBtn->Bind(wxEVT_BUTTON, &ImportSettingsDialog::onPresetPerformance, this);
    balBtn->Bind(wxEVT_BUTTON, &ImportSettingsDialog::onPresetBalanced, this);
    qualBtn->Bind(wxEVT_BUTTON, &ImportSettingsDialog::onPresetQuality, this);
    
    wxBoxSizer* presetSizer = new wxBoxSizer(wxHORIZONTAL);
    presetSizer->Add(perfBtn, 0, wxALL, 5);
    presetSizer->Add(balBtn, 0, wxALL, 5);
    presetSizer->Add(qualBtn, 0, wxALL, 5);
    presetPanel->SetSizer(presetSizer);
    
    // Mesh settings
    wxStaticBox* meshBox = new wxStaticBox(this, wxID_ANY, "Mesh Settings");
    
    m_deflectionCtrl = new wxSpinCtrlDouble(this, wxID_ANY, "1.0",
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
        0.01, 10.0, 1.0, 0.1);
    
    m_angularDeflectionCtrl = new wxSpinCtrlDouble(this, wxID_ANY, "1.0",
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
        0.1, 5.0, 1.0, 0.1);
    
    // Performance options
    wxStaticBox* perfBox = new wxStaticBox(this, wxID_ANY, "Performance Options");
    
    m_lodCheckBox = new wxCheckBox(this, wxID_ANY, "Enable Level of Detail (LOD)");
    m_lodCheckBox->SetValue(true);
    m_lodCheckBox->SetToolTip("Automatically adjust mesh quality during interaction");
    
    m_parallelCheckBox = new wxCheckBox(this, wxID_ANY, "Use Parallel Processing");
    m_parallelCheckBox->SetValue(true);
    m_parallelCheckBox->SetToolTip("Use multiple CPU cores for faster import");
    
    m_adaptiveCheckBox = new wxCheckBox(this, wxID_ANY, "Adaptive Meshing");
    m_adaptiveCheckBox->SetValue(false);
    m_adaptiveCheckBox->SetToolTip("Adjust mesh density based on curvature");
    
    m_autoOptimizeCheckBox = new wxCheckBox(this, wxID_ANY, "Auto-optimize for size");
    m_autoOptimizeCheckBox->SetValue(true);
    m_autoOptimizeCheckBox->SetToolTip("Automatically adjust settings based on model size");
    
    // Import mode
    wxStaticBox* modeBox = new wxStaticBox(this, wxID_ANY, "Import Mode");
    
    wxArrayString modes;
    modes.Add("Standard Import");
    modes.Add("Preview Mode (Fast)");
    modes.Add("High Quality");
    modes.Add("CAM/Analysis Mode");
    
    m_importModeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, modes);
    m_importModeChoice->SetSelection(0);
    
    // Preview text
    m_previewText = new wxStaticText(this, wxID_ANY, 
        "Current settings: Balanced mode\n"
        "Expected performance: Good\n"
        "Mesh quality: Medium");
    m_previewText->SetForegroundColour(wxColour(0, 128, 0));
}

void ImportSettingsDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, 
        "Configure import settings for optimal performance");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    
    mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
    mainSizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    // Presets
    wxStaticText* presetLabel = new wxStaticText(this, wxID_ANY, "Quick Presets:");
    mainSizer->Add(presetLabel, 0, wxALL, 10);
    mainSizer->Add(FindWindow(wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    // Mesh settings
    wxStaticBoxSizer* meshSizer = new wxStaticBoxSizer(
        static_cast<wxStaticBox*>(FindWindowById(wxID_ANY)), wxVERTICAL);
    
    wxFlexGridSizer* meshGrid = new wxFlexGridSizer(2, 2, 5, 10);
    meshGrid->AddGrowableCol(1);
    
    meshGrid->Add(new wxStaticText(this, wxID_ANY, "Mesh Deflection:"), 
        0, wxALIGN_CENTER_VERTICAL);
    meshGrid->Add(m_deflectionCtrl, 1, wxEXPAND);
    
    meshGrid->Add(new wxStaticText(this, wxID_ANY, "Angular Deflection:"), 
        0, wxALIGN_CENTER_VERTICAL);
    meshGrid->Add(m_angularDeflectionCtrl, 1, wxEXPAND);
    
    meshSizer->Add(meshGrid, 0, wxEXPAND | wxALL, 10);
    
    wxStaticText* deflectionHelp = new wxStaticText(this, wxID_ANY,
        "Lower values = higher quality, slower import\n"
        "Higher values = lower quality, faster import");
    deflectionHelp->SetForegroundColour(wxColour(128, 128, 128));
    meshSizer->Add(deflectionHelp, 0, wxALL, 10);
    
    mainSizer->Add(meshSizer, 0, wxEXPAND | wxALL, 10);
    
    // Performance options
    wxStaticBoxSizer* perfSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, "Performance Options"), wxVERTICAL);
    
    perfSizer->Add(m_lodCheckBox, 0, wxALL, 5);
    perfSizer->Add(m_parallelCheckBox, 0, wxALL, 5);
    perfSizer->Add(m_adaptiveCheckBox, 0, wxALL, 5);
    perfSizer->Add(m_autoOptimizeCheckBox, 0, wxALL, 5);
    
    mainSizer->Add(perfSizer, 0, wxEXPAND | wxALL, 10);
    
    // Import mode
    wxStaticBoxSizer* modeSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, "Import Mode"), wxVERTICAL);
    
    modeSizer->Add(m_importModeChoice, 0, wxEXPAND | wxALL, 10);
    
    mainSizer->Add(modeSizer, 0, wxEXPAND | wxALL, 10);
    
    // Preview
    wxStaticBox* previewBox = new wxStaticBox(this, wxID_ANY, "Settings Preview");
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(previewBox, wxVERTICAL);
    previewSizer->Add(m_previewText, 0, wxEXPAND | wxALL, 10);
    
    mainSizer->Add(previewSizer, 0, wxEXPAND | wxALL, 10);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(mainSizer);
    Fit();
}

void ImportSettingsDialog::bindEvents()
{
    m_deflectionCtrl->Bind(wxEVT_SPINCTRLDOUBLE, 
        &ImportSettingsDialog::onDeflectionChange, this);
    m_angularDeflectionCtrl->Bind(wxEVT_SPINCTRLDOUBLE, 
        &ImportSettingsDialog::onDeflectionChange, this);
}

void ImportSettingsDialog::onPresetPerformance(wxCommandEvent& event)
{
    LOG_INF_S("Applying Performance preset for import");
    applyPreset(2.0, 2.0, true, true);
    m_importModeChoice->SetSelection(1); // Preview mode
}

void ImportSettingsDialog::onPresetBalanced(wxCommandEvent& event)
{
    LOG_INF_S("Applying Balanced preset for import");
    applyPreset(1.0, 1.0, true, true);
    m_importModeChoice->SetSelection(0); // Standard mode
}

void ImportSettingsDialog::onPresetQuality(wxCommandEvent& event)
{
    LOG_INF_S("Applying Quality preset for import");
    applyPreset(0.2, 0.5, true, true);
    m_importModeChoice->SetSelection(2); // High quality mode
}

void ImportSettingsDialog::applyPreset(double deflection, double angular, 
                                       bool lod, bool parallel)
{
    m_deflectionCtrl->SetValue(deflection);
    m_angularDeflectionCtrl->SetValue(angular);
    m_lodCheckBox->SetValue(lod);
    m_parallelCheckBox->SetValue(parallel);
    
    // Update preview
    wxString preview;
    if (deflection >= 2.0) {
        preview = "Current settings: Performance mode\n"
                  "Expected performance: Very fast\n"
                  "Mesh quality: Low (suitable for preview)";
        m_previewText->SetForegroundColour(wxColour(255, 128, 0));
    } else if (deflection >= 1.0) {
        preview = "Current settings: Balanced mode\n"
                  "Expected performance: Good\n"
                  "Mesh quality: Medium";
        m_previewText->SetForegroundColour(wxColour(0, 128, 0));
    } else {
        preview = "Current settings: Quality mode\n"
                  "Expected performance: Slower\n"
                  "Mesh quality: High (suitable for analysis)";
        m_previewText->SetForegroundColour(wxColour(0, 0, 255));
    }
    
    m_previewText->SetLabel(preview);
}

void ImportSettingsDialog::onDeflectionChange(wxSpinDoubleEvent& event)
{
    double deflection = m_deflectionCtrl->GetValue();
    double angular = m_angularDeflectionCtrl->GetValue();
    bool lod = m_lodCheckBox->GetValue();
    bool parallel = m_parallelCheckBox->GetValue();
    
    applyPreset(deflection, angular, lod, parallel);
}

void ImportSettingsDialog::onOK(wxCommandEvent& event)
{
    // Save settings
    m_deflection = m_deflectionCtrl->GetValue();
    m_angularDeflection = m_angularDeflectionCtrl->GetValue();
    m_enableLOD = m_lodCheckBox->GetValue();
    m_parallelProcessing = m_parallelCheckBox->GetValue();
    m_adaptiveMeshing = m_adaptiveCheckBox->GetValue();
    m_autoOptimize = m_autoOptimizeCheckBox->GetValue();
    m_importMode = m_importModeChoice->GetSelection();
    
    LOG_INF_S(wxString::Format("Import settings saved: Deflection=%.2f, LOD=%s",
        m_deflection, m_enableLOD ? "On" : "Off"));
    
    EndModal(wxID_OK);
}

void ImportSettingsDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}