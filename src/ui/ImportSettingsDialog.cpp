#include "ImportSettingsDialog.h"
#include "logger/Logger.h"
#include <wx/statline.h>

wxBEGIN_EVENT_TABLE(ImportSettingsDialog, FramelessModalPopup)
    EVT_BUTTON(wxID_OK, ImportSettingsDialog::onOK)
    EVT_BUTTON(wxID_CANCEL, ImportSettingsDialog::onCancel)
wxEND_EVENT_TABLE()

ImportSettingsDialog::ImportSettingsDialog(wxWindow* parent)
    : FramelessModalPopup(parent, "Import Settings", wxSize(800, 800))
    , m_presetPanel(nullptr)
    , m_deflectionCtrl(nullptr)
    , m_angularDeflectionCtrl(nullptr)
    , m_lodCheckBox(nullptr)
    , m_parallelCheckBox(nullptr)
    , m_adaptiveCheckBox(nullptr)
    , m_autoOptimizeCheckBox(nullptr)
    , m_normalProcessingCheckBox(nullptr)
    , m_importModeChoice(nullptr)
    , m_previewText(nullptr)
    , m_fineTessellationCheckBox(nullptr)
    , m_tessellationDeflectionCtrl(nullptr)
    , m_tessellationAngleCtrl(nullptr)
    , m_tessellationMinPointsCtrl(nullptr)
    , m_tessellationMaxPointsCtrl(nullptr)
    , m_adaptiveTessellationCheckBox(nullptr)
    , m_deflection(1.0)
    , m_angularDeflection(1.0)
    , m_enableLOD(true)
    , m_parallelProcessing(true)
    , m_adaptiveMeshing(false)
    , m_autoOptimize(true)
    , m_normalProcessing(false)
    , m_importMode(0)
    , m_enableFineTessellation(true)
    , m_tessellationDeflection(0.01)
    , m_tessellationAngle(0.1)
    , m_tessellationMinPoints(3)
    , m_tessellationMaxPoints(100)
    , m_enableAdaptiveTessellation(true)
{
    // Set up title bar with icon
    SetTitleIcon("cog", wxSize(20, 20));
    ShowTitleIcon(true);

    createControls();
    layoutControls();
    bindEvents();

    // Apply default balanced preset
    applyPreset(1.0, 1.0, true, true, false);  // Changed: Default to disabled
}

ImportSettingsDialog::~ImportSettingsDialog()
{
}

void ImportSettingsDialog::createControls()
{
    // Preset buttons - more compact
    m_presetPanel = new wxPanel(this);
    wxButton* perfBtn = new wxButton(m_presetPanel, wxID_ANY, "Performance");
    wxButton* balBtn = new wxButton(m_presetPanel, wxID_ANY, "Balanced");
    wxButton* qualBtn = new wxButton(m_presetPanel, wxID_ANY, "Quality");
    
    perfBtn->SetToolTip("Fast import with lower quality meshes");
    balBtn->SetToolTip("Balanced import settings");
    qualBtn->SetToolTip("High quality import, slower processing");
    
    // Style buttons
    perfBtn->SetMinSize(wxSize(80, 28));
    balBtn->SetMinSize(wxSize(80, 28));
    qualBtn->SetMinSize(wxSize(80, 28));
    
    perfBtn->Bind(wxEVT_BUTTON, &ImportSettingsDialog::onPresetPerformance, this);
    balBtn->Bind(wxEVT_BUTTON, &ImportSettingsDialog::onPresetBalanced, this);
    qualBtn->Bind(wxEVT_BUTTON, &ImportSettingsDialog::onPresetQuality, this);
    
    wxBoxSizer* presetSizer = new wxBoxSizer(wxHORIZONTAL);
    presetSizer->Add(perfBtn, 0, wxALL, 3);
    presetSizer->Add(balBtn, 0, wxALL, 3);
    presetSizer->Add(qualBtn, 0, wxALL, 3);
    m_presetPanel->SetSizer(presetSizer);
    
    // Mesh settings
    wxStaticBox* meshBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Mesh Settings");

    m_deflectionCtrl = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, "1.0",
        wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
        0.01, 10.0, 1.0, 0.1);

    m_angularDeflectionCtrl = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, "1.0",
        wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
        0.1, 5.0, 1.0, 0.1);

    // Performance options
    wxStaticBox* perfBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Performance Options");

    m_lodCheckBox = new wxCheckBox(m_contentPanel, wxID_ANY, "Enable LOD");
    m_lodCheckBox->SetValue(true);
    m_lodCheckBox->SetToolTip("Automatically adjust mesh quality during interaction");

    m_parallelCheckBox = new wxCheckBox(m_contentPanel, wxID_ANY, "Parallel Processing");
    m_parallelCheckBox->SetValue(true);
    m_parallelCheckBox->SetToolTip("Use multiple CPU cores for faster import");

    m_adaptiveCheckBox = new wxCheckBox(m_contentPanel, wxID_ANY, "Adaptive Meshing");
    m_adaptiveCheckBox->SetValue(false);
    m_adaptiveCheckBox->SetToolTip("Adjust mesh density based on curvature");

    m_autoOptimizeCheckBox = new wxCheckBox(m_contentPanel, wxID_ANY, "Auto-optimize");
    m_autoOptimizeCheckBox->SetValue(true);
    m_autoOptimizeCheckBox->SetToolTip("Automatically adjust settings based on model size");

    m_normalProcessingCheckBox = new wxCheckBox(m_contentPanel, wxID_ANY, "Normal Processing");
    m_normalProcessingCheckBox->SetValue(false);
    m_normalProcessingCheckBox->SetToolTip("Fix face normal directions for consistent rendering");
    
    // New tessellation controls
    m_fineTessellationCheckBox = new wxCheckBox(m_contentPanel, wxID_ANY, "Fine Tessellation");
    m_fineTessellationCheckBox->SetValue(true);
    m_fineTessellationCheckBox->SetToolTip("Enable fine tessellation for smooth surfaces");

    m_tessellationDeflectionCtrl = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, "0.01",
        wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
        0.001, 1.0, 0.01, 0.001);
    m_tessellationDeflectionCtrl->SetToolTip("Surface deflection - smaller = smoother");

    m_tessellationAngleCtrl = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, "0.1",
        wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS,
        0.01, 1.0, 0.1, 0.01);
    m_tessellationAngleCtrl->SetToolTip("Angular deflection - smaller = more triangles");

    m_tessellationMinPointsCtrl = new wxSpinCtrl(m_contentPanel, wxID_ANY, "3",
        wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS,
        2, 20, 3);
    m_tessellationMinPointsCtrl->SetToolTip("Minimum points per edge");

    m_tessellationMaxPointsCtrl = new wxSpinCtrl(m_contentPanel, wxID_ANY, "100",
        wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS,
        10, 500, 100);
    m_tessellationMaxPointsCtrl->SetToolTip("Maximum points per edge");

    m_adaptiveTessellationCheckBox = new wxCheckBox(m_contentPanel, wxID_ANY, "Adaptive Tessellation");
    m_adaptiveTessellationCheckBox->SetValue(true);
    m_adaptiveTessellationCheckBox->SetToolTip("Adjust tessellation based on surface curvature");
    
    // Import mode
    wxStaticBox* modeBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Import Mode");

    wxArrayString modes;
    modes.Add("Standard Import");
    modes.Add("Preview Mode (Fast)");
    modes.Add("High Quality");
    modes.Add("CAM/Analysis Mode");

    m_importModeChoice = new wxChoice(m_contentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, modes);
    m_importModeChoice->SetSelection(0);
    
    // Preview text will be created in layoutControls() to avoid parent window issues
}

void ImportSettingsDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title
    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY,
        "Configure Import Settings");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 3);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);

    mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 8);
    mainSizer->Add(new wxStaticLine(m_contentPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 15);
    
    // Create a horizontal layout for better space utilization
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left column
    wxBoxSizer* leftColumn = new wxBoxSizer(wxVERTICAL);
    
    // Presets
    wxStaticText* presetLabel = new wxStaticText(m_contentPanel, wxID_ANY, "Quick Presets:");
    wxFont labelFont = presetLabel->GetFont();
    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    presetLabel->SetFont(labelFont);
    leftColumn->Add(presetLabel, 0, wxALL, 5);
    leftColumn->Add(m_presetPanel, 0, wxEXPAND | wxALL, 5);

    // Mesh settings - more compact
    wxStaticBox* meshBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Mesh Quality");
    wxStaticBoxSizer* meshSizer = new wxStaticBoxSizer(meshBox, wxVERTICAL);

    wxFlexGridSizer* meshGrid = new wxFlexGridSizer(2, 2, 3, 8);
    meshGrid->AddGrowableCol(1);

    meshGrid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Deflection:"),
        0, wxALIGN_CENTER_VERTICAL);
    meshGrid->Add(m_deflectionCtrl, 1, wxEXPAND);

    meshGrid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Angular:"),
        0, wxALIGN_CENTER_VERTICAL);
    meshGrid->Add(m_angularDeflectionCtrl, 1, wxEXPAND);

    meshSizer->Add(meshGrid, 0, wxEXPAND | wxALL, 8);

    wxStaticText* deflectionHelp = new wxStaticText(m_contentPanel, wxID_ANY,
        "Lower = higher quality, slower\nHigher = lower quality, faster");
    deflectionHelp->SetForegroundColour(wxColour(100, 100, 100));
    wxFont helpFont = deflectionHelp->GetFont();
    helpFont.SetPointSize(helpFont.GetPointSize() - 1);
    deflectionHelp->SetFont(helpFont);
    meshSizer->Add(deflectionHelp, 0, wxALL, 5);

    leftColumn->Add(meshSizer, 0, wxEXPAND | wxALL, 5);

    // Performance options - more compact
    wxStaticBox* perfBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Performance");
    wxStaticBoxSizer* perfSizer = new wxStaticBoxSizer(perfBox, wxVERTICAL);

    perfSizer->Add(m_lodCheckBox, 0, wxALL, 3);
    perfSizer->Add(m_parallelCheckBox, 0, wxALL, 3);
    perfSizer->Add(m_adaptiveCheckBox, 0, wxALL, 3);
    perfSizer->Add(m_autoOptimizeCheckBox, 0, wxALL, 3);
    perfSizer->Add(m_normalProcessingCheckBox, 0, wxALL, 3);
    
    leftColumn->Add(perfSizer, 0, wxEXPAND | wxALL, 5);
    
    // Tessellation settings - new section
    wxStaticBox* tessellationBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Surface Tessellation");
    wxStaticBoxSizer* tessellationSizer = new wxStaticBoxSizer(tessellationBox, wxVERTICAL);

    tessellationSizer->Add(m_fineTessellationCheckBox, 0, wxALL, 3);
    tessellationSizer->Add(m_adaptiveTessellationCheckBox, 0, wxALL, 3);

    // Tessellation parameters grid
    wxFlexGridSizer* tessellationGrid = new wxFlexGridSizer(4, 2, 3, 8);
    tessellationGrid->AddGrowableCol(1);

    tessellationGrid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Deflection:"),
        0, wxALIGN_CENTER_VERTICAL);
    tessellationGrid->Add(m_tessellationDeflectionCtrl, 1, wxEXPAND);

    tessellationGrid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Angle:"),
        0, wxALIGN_CENTER_VERTICAL);
    tessellationGrid->Add(m_tessellationAngleCtrl, 1, wxEXPAND);

    tessellationGrid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Min Points:"),
        0, wxALIGN_CENTER_VERTICAL);
    tessellationGrid->Add(m_tessellationMinPointsCtrl, 1, wxEXPAND);

    tessellationGrid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Max Points:"),
        0, wxALIGN_CENTER_VERTICAL);
    tessellationGrid->Add(m_tessellationMaxPointsCtrl, 1, wxEXPAND);

    tessellationSizer->Add(tessellationGrid, 0, wxEXPAND | wxALL, 8);

    wxStaticText* tessellationHelp = new wxStaticText(m_contentPanel, wxID_ANY,
        "Fine tessellation creates smoother surfaces\nSmaller values = better quality, slower");
    tessellationHelp->SetForegroundColour(wxColour(100, 100, 100));
    wxFont tessellationHelpFont = tessellationHelp->GetFont();
    tessellationHelpFont.SetPointSize(tessellationHelpFont.GetPointSize() - 1);
    tessellationHelp->SetFont(tessellationHelpFont);
    tessellationSizer->Add(tessellationHelp, 0, wxALL, 5);

    leftColumn->Add(tessellationSizer, 1, wxEXPAND | wxALL, 5);

    // Right column
    wxBoxSizer* rightColumn = new wxBoxSizer(wxVERTICAL);

    // Import mode
    wxStaticBox* modeBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Import Mode");
    wxStaticBoxSizer* modeSizer = new wxStaticBoxSizer(modeBox, wxVERTICAL);

    modeSizer->Add(m_importModeChoice, 0, wxEXPAND | wxALL, 8);
    
    rightColumn->Add(modeSizer, 0, wxEXPAND | wxALL, 5);
    
    // Preview - enhanced
    wxStaticBox* previewBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Settings Preview");
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(previewBox, wxVERTICAL);

    // Add a background panel for better visual separation
    wxPanel* previewPanel = new wxPanel(m_contentPanel);
    previewPanel->SetBackgroundColour(wxColour(248, 248, 248));

    // Create preview text with correct parent window
    m_previewText = new wxStaticText(previewPanel, wxID_ANY,
        "Current settings: Balanced mode\n"
        "Expected performance: Good\n"
        "Mesh quality: Medium");
    m_previewText->SetForegroundColour(wxColour(0, 150, 0));

    // Set font for preview text
    wxFont previewFont = m_previewText->GetFont();
    previewFont.SetPointSize(previewFont.GetPointSize() + 1);
    m_previewText->SetFont(previewFont);

    // Add a border to the preview panel
    wxBoxSizer* previewPanelSizer = new wxBoxSizer(wxVERTICAL);
    previewPanelSizer->Add(m_previewText, 0, wxEXPAND | wxALL, 12);
    previewPanel->SetSizer(previewPanelSizer);

    previewSizer->Add(previewPanel, 1, wxEXPAND | wxALL, 5);
    
    rightColumn->Add(previewSizer, 1, wxEXPAND | wxALL, 5);
    
    // Add columns to content sizer - give more space to left column
    contentSizer->Add(leftColumn, 2, wxEXPAND | wxALL, 8);
    contentSizer->Add(rightColumn, 1, wxEXPAND | wxALL, 8);
    
    mainSizer->Add(contentSizer, 1, wxEXPAND | wxALL, 5);
    
    // Buttons - centered at bottom
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    
    // Style buttons
    okBtn->SetDefault();
    okBtn->SetMinSize(wxSize(80, 30));
    cancelBtn->SetMinSize(wxSize(80, 30));
    
    buttonSizer->Add(okBtn, 0, wxALL, 5);
    buttonSizer->Add(cancelBtn, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    
    m_contentPanel->SetSizer(mainSizer);
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
    applyPreset(2.0, 2.0, true, true, false);  // Performance: disable normal processing for speed
    m_importModeChoice->SetSelection(1); // Preview mode
}

void ImportSettingsDialog::onPresetBalanced(wxCommandEvent& event)
{
    LOG_INF_S("Applying Balanced preset for import");
    applyPreset(1.0, 1.0, true, true, false);  // Balanced: disable normal processing by default
    m_importModeChoice->SetSelection(0); // Standard mode
}

void ImportSettingsDialog::onPresetQuality(wxCommandEvent& event)
{
    LOG_INF_S("Applying Quality preset for import");
    applyPreset(0.2, 0.5, true, true, true);  // Quality: enable normal processing for best quality
    m_importModeChoice->SetSelection(2); // High quality mode
}

void ImportSettingsDialog::applyPreset(double deflection, double angular, 
                                       bool lod, bool parallel, bool normalProcessing)
{
    m_deflectionCtrl->SetValue(deflection);
    m_angularDeflectionCtrl->SetValue(angular);
    m_lodCheckBox->SetValue(lod);
    m_parallelCheckBox->SetValue(parallel);
    m_normalProcessingCheckBox->SetValue(normalProcessing);
    
    // Apply tessellation presets based on deflection
    if (deflection >= 2.0) {
        // Performance mode - basic tessellation
        m_fineTessellationCheckBox->SetValue(false);
        m_tessellationDeflectionCtrl->SetValue(0.1);
        m_tessellationAngleCtrl->SetValue(0.5);
        m_tessellationMinPointsCtrl->SetValue(3);
        m_tessellationMaxPointsCtrl->SetValue(20);
        m_adaptiveTessellationCheckBox->SetValue(false);
    } else if (deflection >= 1.0) {
        // Balanced mode - good tessellation
        m_fineTessellationCheckBox->SetValue(true);
        m_tessellationDeflectionCtrl->SetValue(0.01);
        m_tessellationAngleCtrl->SetValue(0.1);
        m_tessellationMinPointsCtrl->SetValue(3);
        m_tessellationMaxPointsCtrl->SetValue(100);
        m_adaptiveTessellationCheckBox->SetValue(true);
    } else {
        // Quality mode - fine tessellation
        m_fineTessellationCheckBox->SetValue(true);
        m_tessellationDeflectionCtrl->SetValue(0.005);
        m_tessellationAngleCtrl->SetValue(0.05);
        m_tessellationMinPointsCtrl->SetValue(5);
        m_tessellationMaxPointsCtrl->SetValue(200);
        m_adaptiveTessellationCheckBox->SetValue(true);
    }
    
    // Update preview with enhanced formatting
    wxString preview;
    wxColour previewColor;
    
    if (deflection >= 2.0) {
        preview = "Current settings: Performance mode\n"
                  "Expected performance: Very fast\n"
                  "Mesh quality: Low (suitable for preview)";
        previewColor = wxColour(255, 140, 0); // Orange
    } else if (deflection >= 1.0) {
        preview = "Current settings: Balanced mode\n"
                  "Expected performance: Good\n"
                  "Mesh quality: Medium";
        previewColor = wxColour(0, 150, 0); // Green
    } else {
        preview = "Current settings: Quality mode\n"
                  "Expected performance: Slower\n"
                  "Mesh quality: High (suitable for analysis)";
        previewColor = wxColour(0, 100, 200); // Blue
    }
    
    if (m_previewText) {
        m_previewText->SetLabel(preview);
        m_previewText->SetForegroundColour(previewColor);
    }
}

void ImportSettingsDialog::onDeflectionChange(wxSpinDoubleEvent& event)
{
    double deflection = m_deflectionCtrl->GetValue();
    double angular = m_angularDeflectionCtrl->GetValue();
    bool lod = m_lodCheckBox->GetValue();
    bool parallel = m_parallelCheckBox->GetValue();
    bool normalProcessing = m_normalProcessingCheckBox->GetValue();
    
    applyPreset(deflection, angular, lod, parallel, normalProcessing);
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
    m_normalProcessing = m_normalProcessingCheckBox->GetValue();
    m_importMode = m_importModeChoice->GetSelection();
    
    // Save new tessellation settings
    m_enableFineTessellation = m_fineTessellationCheckBox->GetValue();
    m_tessellationDeflection = m_tessellationDeflectionCtrl->GetValue();
    m_tessellationAngle = m_tessellationAngleCtrl->GetValue();
    m_tessellationMinPoints = m_tessellationMinPointsCtrl->GetValue();
    m_tessellationMaxPoints = m_tessellationMaxPointsCtrl->GetValue();
    m_enableAdaptiveTessellation = m_adaptiveTessellationCheckBox->GetValue();
    
    LOG_INF_S(wxString::Format("Import settings saved: Deflection=%.2f, LOD=%s, FineTessellation=%s",
        m_deflection, m_enableLOD ? "On" : "Off", m_enableFineTessellation ? "On" : "Off"));
    
    EndModal(wxID_OK);
}

void ImportSettingsDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}