#include "GeometryDecompositionDialog.h"
#include "logger/Logger.h"
#include "config/ThemeManager.h"
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/font.h>
#include <filesystem>

wxBEGIN_EVENT_TABLE(GeometryDecompositionDialog, FramelessModalPopup)
    EVT_BUTTON(wxID_OK, GeometryDecompositionDialog::onOK)
    EVT_BUTTON(wxID_CANCEL, GeometryDecompositionDialog::onCancel)
    EVT_CHOICE(wxID_ANY, GeometryDecompositionDialog::onDecompositionLevelChange)
    EVT_CHOICE(wxID_ANY, GeometryDecompositionDialog::onColorSchemeChange)
wxEND_EVENT_TABLE()

GeometryDecompositionDialog::GeometryDecompositionDialog(wxWindow* parent, GeometryReader::DecompositionOptions& options, bool isLargeComplexGeometry)
    : FramelessModalPopup(parent, "Geometry Import Settings", wxSize(650, 750))
    , m_options(options)
    , m_isLargeComplexGeometry(isLargeComplexGeometry)
    , m_notebook(nullptr)
    , m_decompositionPage(nullptr)
    , m_meshQualityPage(nullptr)
    , m_enableDecompositionCheckBox(nullptr)
    , m_decompositionLevelChoice(nullptr)
    , m_colorSchemeChoice(nullptr)
    , m_consistentColoringCheckBox(nullptr)
    , m_previewText(nullptr)
    , m_previewPanel(nullptr)
    , m_colorPreviewPanel(nullptr)
    , m_fastPresetBtn(nullptr)
    , m_balancedPresetBtn(nullptr)
    , m_highQualityPresetBtn(nullptr)
    , m_ultraQualityPresetBtn(nullptr)
    , m_customPresetBtn(nullptr)
    , m_customDeflectionCtrl(nullptr)
    , m_customAngularCtrl(nullptr)
    , m_meshQualityPreviewText(nullptr)
    , m_smoothSurfacePage(nullptr)
    , m_subdivisionEnabledCheckBox(nullptr)
    , m_subdivisionLevelCtrl(nullptr)
    , m_smoothingEnabledCheckBox(nullptr)
    , m_smoothingIterationsCtrl(nullptr)
    , m_smoothingStrengthCtrl(nullptr)
    , m_smoothingCreaseAngleCtrl(nullptr)
    , m_lodEnabledCheckBox(nullptr)
    , m_lodFineDeflectionCtrl(nullptr)
    , m_lodRoughDeflectionCtrl(nullptr)
    , m_tessellationQualityCtrl(nullptr)
    , m_featurePreservationCtrl(nullptr)
    , m_enableDecomposition(options.enableDecomposition)
    , m_decompositionLevel(options.level)
    , m_colorScheme(options.colorScheme)
    , m_useConsistentColoring(options.useConsistentColoring)
    , m_meshQualityPreset(options.meshQualityPreset)
    , m_customMeshDeflection(options.customMeshDeflection)
    , m_customAngularDeflection(options.customAngularDeflection)
    , m_subdivisionEnabled(options.subdivisionEnabled)
    , m_subdivisionLevel(options.subdivisionLevel)
    , m_smoothingEnabled(options.smoothingEnabled)
    , m_smoothingIterations(options.smoothingIterations)
    , m_smoothingStrength(options.smoothingStrength)
    , m_lodEnabled(options.lodEnabled)
    , m_lodFineDeflection(options.lodFineDeflection)
    , m_lodRoughDeflection(options.lodRoughDeflection)
    , m_tessellationQuality(options.tessellationQuality)
    , m_featurePreservation(options.featurePreservation)
    , m_smoothingCreaseAngle(options.smoothingCreaseAngle)
    , m_updatingMeshQuality(false)
{
    // Set up title bar with icon
    SetTitleIcon("layers", wxSize(20, 20));
    ShowTitleIcon(true);

    createControls();
    layoutControls();
    bindEvents();

    // Apply restrictions for large complex geometries
    if (m_isLargeComplexGeometry) {
        applyLargeComplexGeometryRestrictions();
    }

    // Update UI to reflect current options
    updatePreview();
    updateMeshQualityControls();
    
    // Update smooth surface controls if restrictions were applied
    if (m_isLargeComplexGeometry) {
        // Sync UI controls with restricted values
        if (m_subdivisionLevelCtrl) {
            m_subdivisionLevelCtrl->SetValue("2");
        }
        if (m_smoothingIterationsCtrl) {
            m_smoothingIterationsCtrl->SetValue("2");
        }
        if (m_smoothingStrengthCtrl) {
            m_smoothingStrengthCtrl->SetValue("0.50");
        }
        if (m_tessellationQualityCtrl) {
            m_tessellationQualityCtrl->SetValue("2");
        }
        if (m_featurePreservationCtrl) {
            m_featurePreservationCtrl->SetValue("0.50");
        }
    }
}

GeometryDecompositionDialog::~GeometryDecompositionDialog()
{
}

void GeometryDecompositionDialog::createControls()
{
    // Create notebook for tabs
    m_notebook = new wxNotebook(m_contentPanel, wxID_ANY);

    // Create pages
    createDecompositionPage();
    createMeshQualityPage();
    createSmoothSurfacePage();

    // Add pages to notebook
    m_notebook->AddPage(m_decompositionPage, "Geometry Decomposition", true);
    m_notebook->AddPage(m_meshQualityPage, "Mesh Quality", false);
    m_notebook->AddPage(m_smoothSurfacePage, "Smooth Surface", false);
}

void GeometryDecompositionDialog::createDecompositionPage()
{
    m_decompositionPage = new wxPanel(m_notebook);

    // Enable decomposition checkbox
    m_enableDecompositionCheckBox = new wxCheckBox(m_decompositionPage, wxID_ANY, "Enable Geometry Decomposition");
    m_enableDecompositionCheckBox->SetValue(m_enableDecomposition);
    m_enableDecompositionCheckBox->SetToolTip("Enable automatic decomposition of complex geometries into separate components");

    // Decomposition level choice
    wxArrayString levelChoices;
    levelChoices.Add("No Decomposition");
    levelChoices.Add("Shape Level");
    levelChoices.Add("Solid Level (recommended)");
    levelChoices.Add("Shell Level");
    levelChoices.Add("Face Level (detailed)");

    m_decompositionLevelChoice = new wxChoice(m_decompositionPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, levelChoices);
    m_decompositionLevelChoice->SetSelection(static_cast<int>(m_decompositionLevel));
    m_decompositionLevelChoice->SetToolTip("Choose how detailed the decomposition should be");
    m_decompositionLevelChoice->Enable(m_enableDecomposition);

    // Color scheme choice
    wxArrayString colorChoices;
    colorChoices.Add("Distinct Colors (cool tones)");
    colorChoices.Add("Warm Colors");
    colorChoices.Add("Rainbow Spectrum");
    colorChoices.Add("Monochrome Blue");
    colorChoices.Add("Monochrome Green");
    colorChoices.Add("Monochrome Gray");

    m_colorSchemeChoice = new wxChoice(m_decompositionPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, colorChoices);
    m_colorSchemeChoice->SetSelection(static_cast<int>(m_colorScheme));
    m_colorSchemeChoice->SetToolTip("Choose color scheme for decomposed components");
    m_colorSchemeChoice->Enable(m_enableDecomposition);

    // Consistent coloring checkbox
    m_consistentColoringCheckBox = new wxCheckBox(m_decompositionPage, wxID_ANY, "Use Consistent Coloring");
    m_consistentColoringCheckBox->SetValue(m_useConsistentColoring);
    m_consistentColoringCheckBox->SetToolTip("Use consistent colors for similar components across imports");
    m_consistentColoringCheckBox->Enable(m_enableDecomposition);
}

void GeometryDecompositionDialog::createMeshQualityPage()
{
    m_meshQualityPage = new wxPanel(m_notebook);
    m_meshQualityPage->SetBackgroundColour(CFG_COLOUR("PrimaryBackgroundColour"));

    // Create preset buttons with enhanced styling
    wxFont buttonFont = CFG_FONT();
    buttonFont.SetPointSize(10);
    buttonFont.SetWeight(wxFONTWEIGHT_BOLD);
    wxFont buttonSubFont = CFG_FONT();
    buttonSubFont.SetPointSize(8);
    
    // Fast preset button
    m_fastPresetBtn = new wxButton(m_meshQualityPage, wxID_ANY, "Fast\nLower Quality", wxDefaultPosition, wxSize(160, 65));
    m_fastPresetBtn->SetToolTip("Fast import, lower quality mesh\nDeflection=2.0, Angular=2.0\nBest for quick previews");
    m_fastPresetBtn->SetFont(buttonFont);
    m_fastPresetBtn->SetBackgroundColour(CFG_COLOUR("ButtonbarDefaultBgColour"));
    m_fastPresetBtn->SetForegroundColour(CFG_COLOUR("ButtonbarDefaultTextColour"));

    // Balanced preset button
    m_balancedPresetBtn = new wxButton(m_meshQualityPage, wxID_ANY, "Balanced\nRecommended", wxDefaultPosition, wxSize(160, 65));
    m_balancedPresetBtn->SetToolTip("Balanced quality and performance\nDeflection=1.0, Angular=1.0\nGood for most use cases");
    m_balancedPresetBtn->SetFont(buttonFont);
    m_balancedPresetBtn->SetBackgroundColour(CFG_COLOUR("ButtonbarDefaultBgColour"));
    m_balancedPresetBtn->SetForegroundColour(CFG_COLOUR("ButtonbarDefaultTextColour"));

    // High Quality preset button (default)
    m_highQualityPresetBtn = new wxButton(m_meshQualityPage, wxID_ANY, "High Quality\nDefault", wxDefaultPosition, wxSize(160, 65));
    m_highQualityPresetBtn->SetToolTip("High quality mesh\nDeflection=0.5, Angular=0.5\nBetter visual quality");
    m_highQualityPresetBtn->SetFont(buttonFont);
    m_highQualityPresetBtn->SetBackgroundColour(CFG_COLOUR("ButtonbarDefaultBgColour"));
    m_highQualityPresetBtn->SetForegroundColour(CFG_COLOUR("ButtonbarDefaultTextColour"));

    // Ultra Quality preset button
    m_ultraQualityPresetBtn = new wxButton(m_meshQualityPage, wxID_ANY, "Ultra Quality\nSlow Import", wxDefaultPosition, wxSize(160, 65));
    m_ultraQualityPresetBtn->SetToolTip("Ultra high quality\nDeflection=0.2, Angular=0.3\nBest quality, slower import");
    m_ultraQualityPresetBtn->SetFont(buttonFont);
    m_ultraQualityPresetBtn->SetBackgroundColour(CFG_COLOUR("ButtonbarDefaultBgColour"));
    m_ultraQualityPresetBtn->SetForegroundColour(CFG_COLOUR("ButtonbarDefaultTextColour"));

    // Custom preset button
    m_customPresetBtn = new wxButton(m_meshQualityPage, wxID_ANY, "Custom\nUser Defined", wxDefaultPosition, wxSize(160, 65));
    m_customPresetBtn->SetToolTip("Custom mesh quality settings\nDefine your own deflection values");
    m_customPresetBtn->SetFont(buttonFont);
    m_customPresetBtn->SetBackgroundColour(CFG_COLOUR("ButtonbarDefaultBgColour"));
    m_customPresetBtn->SetForegroundColour(CFG_COLOUR("ButtonbarDefaultTextColour"));

    // Custom deflection controls with better styling
    m_customDeflectionCtrl = new wxTextCtrl(m_meshQualityPage, wxID_ANY, 
        wxString::Format("%.4f", m_customMeshDeflection),
        wxDefaultPosition, wxSize(120, -1), wxTE_CENTER);
    m_customDeflectionCtrl->SetToolTip("Mesh deflection value (smaller = finer mesh, 0.001-10.0)\nLower values produce finer meshes but slower import");
    m_customDeflectionCtrl->Enable(m_meshQualityPreset == GeometryReader::MeshQualityPreset::CUSTOM);
    m_customDeflectionCtrl->SetBackgroundColour(CFG_COLOUR("TextCtrlBgColour"));
    m_customDeflectionCtrl->SetForegroundColour(CFG_COLOUR("TextCtrlFgColour"));

    m_customAngularCtrl = new wxTextCtrl(m_meshQualityPage, wxID_ANY, 
        wxString::Format("%.4f", m_customAngularDeflection),
        wxDefaultPosition, wxSize(120, -1), wxTE_CENTER);
    m_customAngularCtrl->SetToolTip("Angular deflection value (smaller = smoother curves, 0.01-10.0)\nLower values produce smoother curves but more triangles");
    m_customAngularCtrl->Enable(m_meshQualityPreset == GeometryReader::MeshQualityPreset::CUSTOM);
    m_customAngularCtrl->SetBackgroundColour(CFG_COLOUR("TextCtrlBgColour"));
    m_customAngularCtrl->SetForegroundColour(CFG_COLOUR("TextCtrlFgColour"));

    // Note: m_meshQualityPreviewText will be created in layoutControls() 
    // to ensure correct parent (meshPreviewPanel)
}

void GeometryDecompositionDialog::createSmoothSurfacePage()
{
    m_smoothSurfacePage = new wxPanel(m_notebook);
    m_smoothSurfacePage->SetBackgroundColour(CFG_COLOUR("PrimaryBackgroundColour"));

    // Subdivision controls
    m_subdivisionEnabledCheckBox = new wxCheckBox(m_smoothSurfacePage, wxID_ANY, "Enable Subdivision");
    m_subdivisionEnabledCheckBox->SetValue(m_subdivisionEnabled);
    m_subdivisionEnabledCheckBox->SetToolTip("Enable subdivision surfaces for smoother meshes");

    m_subdivisionLevelCtrl = new wxTextCtrl(m_smoothSurfacePage, wxID_ANY,
        wxString::Format("%d", m_subdivisionLevel),
        wxDefaultPosition, wxSize(80, -1), wxTE_CENTER);
    m_subdivisionLevelCtrl->SetToolTip("Subdivision level (1-5, higher = smoother but slower)");
    m_subdivisionLevelCtrl->Enable(m_subdivisionEnabled);

    // Smoothing controls
    m_smoothingEnabledCheckBox = new wxCheckBox(m_smoothSurfacePage, wxID_ANY, "Enable Smoothing");
    m_smoothingEnabledCheckBox->SetValue(m_smoothingEnabled);
    m_smoothingEnabledCheckBox->SetToolTip("Enable mesh smoothing for better surface quality");

    m_smoothingIterationsCtrl = new wxTextCtrl(m_smoothSurfacePage, wxID_ANY,
        wxString::Format("%d", m_smoothingIterations),
        wxDefaultPosition, wxSize(80, -1), wxTE_CENTER);
    m_smoothingIterationsCtrl->SetToolTip("Smoothing iterations (1-10, more = smoother but slower)");
    m_smoothingIterationsCtrl->Enable(m_smoothingEnabled);

    m_smoothingStrengthCtrl = new wxTextCtrl(m_smoothSurfacePage, wxID_ANY,
        wxString::Format("%.2f", m_smoothingStrength),
        wxDefaultPosition, wxSize(80, -1), wxTE_CENTER);
    m_smoothingStrengthCtrl->SetToolTip("Smoothing strength (0.01-1.0, higher = more smoothing)");
    m_smoothingStrengthCtrl->Enable(m_smoothingEnabled);

    m_smoothingCreaseAngleCtrl = new wxTextCtrl(m_smoothSurfacePage, wxID_ANY,
        wxString::Format("%.2f", m_smoothingCreaseAngle),
        wxDefaultPosition, wxSize(80, -1), wxTE_CENTER);
    m_smoothingCreaseAngleCtrl->SetToolTip("Smoothing crease angle in degrees (0-180)");
    m_smoothingCreaseAngleCtrl->Enable(m_smoothingEnabled);

    // LOD controls
    m_lodEnabledCheckBox = new wxCheckBox(m_smoothSurfacePage, wxID_ANY, "Enable LOD");
    m_lodEnabledCheckBox->SetValue(m_lodEnabled);
    m_lodEnabledCheckBox->SetToolTip("Enable Level of Detail for performance optimization");

    m_lodFineDeflectionCtrl = new wxTextCtrl(m_smoothSurfacePage, wxID_ANY,
        wxString::Format("%.2f", m_lodFineDeflection),
        wxDefaultPosition, wxSize(80, -1), wxTE_CENTER);
    m_lodFineDeflectionCtrl->SetToolTip("LOD fine deflection (for close objects)");
    m_lodFineDeflectionCtrl->Enable(m_lodEnabled);

    m_lodRoughDeflectionCtrl = new wxTextCtrl(m_smoothSurfacePage, wxID_ANY,
        wxString::Format("%.2f", m_lodRoughDeflection),
        wxDefaultPosition, wxSize(80, -1), wxTE_CENTER);
    m_lodRoughDeflectionCtrl->SetToolTip("LOD rough deflection (for distant objects)");
    m_lodRoughDeflectionCtrl->Enable(m_lodEnabled);

    // Tessellation controls
    m_tessellationQualityCtrl = new wxTextCtrl(m_smoothSurfacePage, wxID_ANY,
        wxString::Format("%d", m_tessellationQuality),
        wxDefaultPosition, wxSize(80, -1), wxTE_CENTER);
    m_tessellationQualityCtrl->SetToolTip("Tessellation quality (1-5, higher = better quality)");

    m_featurePreservationCtrl = new wxTextCtrl(m_smoothSurfacePage, wxID_ANY,
        wxString::Format("%.2f", m_featurePreservation),
        wxDefaultPosition, wxSize(80, -1), wxTE_CENTER);
    m_featurePreservationCtrl->SetToolTip("Feature preservation (0.0-1.0, higher = preserve more features)");
}

void GeometryDecompositionDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title
    wxStaticText* title = new wxStaticText(m_contentPanel, wxID_ANY, "Configure Geometry Import");
    wxFont titleFont = CFG_FONT();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    title->SetForegroundColour(CFG_COLOUR("PrimaryTextColour"));

    mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
    mainSizer->Add(new wxStaticLine(m_contentPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 15);

    // Add notebook
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);

    // Layout decomposition page
    wxBoxSizer* decompositionSizer = new wxBoxSizer(wxVERTICAL);

    // Enable decomposition
    wxStaticBox* enableBox = new wxStaticBox(m_decompositionPage, wxID_ANY, "Decomposition Control");
    wxStaticBoxSizer* enableSizer = new wxStaticBoxSizer(enableBox, wxVERTICAL);
    enableSizer->Add(m_enableDecompositionCheckBox, 0, wxALL, 10);
    decompositionSizer->Add(enableSizer, 0, wxEXPAND | wxALL, 10);

    // Decomposition settings
    wxStaticBox* settingsBox = new wxStaticBox(m_decompositionPage, wxID_ANY, "Decomposition Settings");
    wxStaticBoxSizer* settingsSizer = new wxStaticBoxSizer(settingsBox, wxVERTICAL);

    wxFlexGridSizer* settingsGrid = new wxFlexGridSizer(3, 2, 8, 15);
    settingsGrid->AddGrowableCol(1);

    // Decomposition level
    settingsGrid->Add(new wxStaticText(m_decompositionPage, wxID_ANY, "Decomposition Level:"), 0, wxALIGN_CENTER_VERTICAL);
    settingsGrid->Add(m_decompositionLevelChoice, 1, wxEXPAND);

    // Color scheme
    settingsGrid->Add(new wxStaticText(m_decompositionPage, wxID_ANY, "Color Scheme:"), 0, wxALIGN_CENTER_VERTICAL);
    settingsGrid->Add(m_colorSchemeChoice, 1, wxEXPAND);

    // Consistent coloring
    settingsGrid->Add(new wxStaticText(m_decompositionPage, wxID_ANY, "Coloring Mode:"), 0, wxALIGN_CENTER_VERTICAL);
    settingsGrid->Add(m_consistentColoringCheckBox, 1, wxEXPAND);

    settingsSizer->Add(settingsGrid, 0, wxEXPAND | wxALL, 10);

    // Help text
    wxStaticText* helpText = new wxStaticText(m_decompositionPage, wxID_ANY,
        "* Shape Level: Decomposes assemblies into individual shapes\n"
        "* Solid Level: Further decomposes shapes into individual solid bodies\n"
        "* Shell Level: Further decomposes solids into surface shells\n"
        "* Face Level: Decomposes into individual faces (most detailed)\n"
        "* Consistent coloring ensures similar components have the same color");
    helpText->SetForegroundColour(CFG_COLOUR("PlaceholderTextColour"));
    wxFont helpFont = CFG_FONT();
    helpFont.SetPointSize(helpFont.GetPointSize() - 1);
    helpText->SetFont(helpFont);
    settingsSizer->Add(helpText, 0, wxALL, 10);

    decompositionSizer->Add(settingsSizer, 0, wxEXPAND | wxALL, 10);

    // Color scheme preview
    wxStaticBox* colorPreviewBox = new wxStaticBox(m_decompositionPage, wxID_ANY, "Color Scheme Preview");
    wxStaticBoxSizer* colorPreviewSizer = new wxStaticBoxSizer(colorPreviewBox, wxVERTICAL);
    
    m_colorPreviewPanel = new wxPanel(m_decompositionPage);
    m_colorPreviewPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
    
    wxBoxSizer* colorPreviewPanelSizer = new wxBoxSizer(wxHORIZONTAL);
    m_colorPreviewPanel->SetSizer(colorPreviewPanelSizer);
    
    colorPreviewSizer->Add(m_colorPreviewPanel, 0, wxEXPAND | wxALL, 5);
    decompositionSizer->Add(colorPreviewSizer, 0, wxEXPAND | wxALL, 10);

    // Preview section
    wxStaticBox* previewBox = new wxStaticBox(m_decompositionPage, wxID_ANY, "Settings Preview");
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(previewBox, wxVERTICAL);

    m_previewPanel = new wxPanel(m_decompositionPage);
    m_previewPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));

    m_previewText = new wxStaticText(m_previewPanel, wxID_ANY, "Preview will appear here");
    wxFont previewFont = CFG_FONT();
    previewFont.SetPointSize(previewFont.GetPointSize() + 1);
    m_previewText->SetFont(previewFont);
    m_previewText->SetForegroundColour(CFG_COLOUR("PanelTextColour"));

    wxBoxSizer* previewPanelSizer = new wxBoxSizer(wxVERTICAL);
    previewPanelSizer->Add(m_previewText, 0, wxEXPAND | wxALL, 12);
    m_previewPanel->SetSizer(previewPanelSizer);

    previewSizer->Add(m_previewPanel, 1, wxEXPAND | wxALL, 5);

    decompositionSizer->Add(previewSizer, 1, wxEXPAND | wxALL, 10);

    m_decompositionPage->SetSizer(decompositionSizer);

    // Layout mesh quality page
    wxBoxSizer* meshQualitySizer = new wxBoxSizer(wxVERTICAL);

    // Preset buttons section - use larger buttons in 2 rows with increased padding
    wxStaticBox* presetBox = new wxStaticBox(m_meshQualityPage, wxID_ANY, "Mesh Quality Presets");
    wxStaticBoxSizer* presetSizer = new wxStaticBoxSizer(presetBox, wxVERTICAL);
    
    // First row: Fast, Balanced, High Quality (3 buttons)
    wxFlexGridSizer* firstRowSizer = new wxFlexGridSizer(1, 3, 5, 5);
    firstRowSizer->AddGrowableCol(0);
    firstRowSizer->AddGrowableCol(1);
    firstRowSizer->AddGrowableCol(2);
    
    firstRowSizer->Add(m_fastPresetBtn, 1, wxEXPAND | wxALL, 5);
    firstRowSizer->Add(m_balancedPresetBtn, 1, wxEXPAND | wxALL, 5);
    firstRowSizer->Add(m_highQualityPresetBtn, 1, wxEXPAND | wxALL, 5);
    
    // Second row: Ultra Quality, Custom (2 buttons)
    wxFlexGridSizer* secondRowSizer = new wxFlexGridSizer(1, 2, 5, 5);
    secondRowSizer->AddGrowableCol(0);
    secondRowSizer->AddGrowableCol(1);
    
    secondRowSizer->Add(m_ultraQualityPresetBtn, 1, wxEXPAND | wxALL, 5);
    secondRowSizer->Add(m_customPresetBtn, 1, wxEXPAND | wxALL, 5);
    
    presetSizer->Add(firstRowSizer, 0, wxEXPAND | wxALL, 5);
    presetSizer->Add(secondRowSizer, 0, wxEXPAND | wxALL, 5);
    meshQualitySizer->Add(presetSizer, 0, wxEXPAND | wxALL, 10);

    // Custom settings section with improved layout
    wxStaticBox* customBox = new wxStaticBox(m_meshQualityPage, wxID_ANY, "Custom Quality Settings");
    wxStaticBoxSizer* customSizer = new wxStaticBoxSizer(customBox, wxVERTICAL);
    
    // Add description text
    wxStaticText* customDesc = new wxStaticText(m_meshQualityPage, wxID_ANY, 
        "Define custom mesh quality parameters (available when Custom preset is selected)");
    wxFont descFont = CFG_FONT();
    descFont.SetPointSize(descFont.GetPointSize() - 1);
    customDesc->SetFont(descFont);
    customDesc->SetForegroundColour(CFG_COLOUR("PlaceholderTextColour"));
    customSizer->Add(customDesc, 0, wxALL, 8);
    
    // Parameters in a grid layout
    wxFlexGridSizer* paramGrid = new wxFlexGridSizer(2, 2, 10, 15);
    paramGrid->AddGrowableCol(1);
    
    // Deflection parameter
    wxStaticText* deflectionLabel = new wxStaticText(m_meshQualityPage, wxID_ANY, "Mesh Deflection:");
    wxFont labelFont = CFG_FONT();
    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    deflectionLabel->SetFont(labelFont);
    deflectionLabel->SetForegroundColour(CFG_COLOUR("PanelTextColour"));
    paramGrid->Add(deflectionLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    paramGrid->Add(m_customDeflectionCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    // Angular parameter
    wxStaticText* angularLabel = new wxStaticText(m_meshQualityPage, wxID_ANY, "Angular Deflection:");
    angularLabel->SetFont(labelFont);
    angularLabel->SetForegroundColour(CFG_COLOUR("PanelTextColour"));
    paramGrid->Add(angularLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    paramGrid->Add(m_customAngularCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    customSizer->Add(paramGrid, 0, wxEXPAND | wxALL, 10);
    meshQualitySizer->Add(customSizer, 0, wxEXPAND | wxALL, 10);

    // Mesh quality preview with enhanced styling
    wxStaticBox* meshPreviewBox = new wxStaticBox(m_meshQualityPage, wxID_ANY, "Current Settings Preview");
    wxStaticBoxSizer* meshPreviewSizer = new wxStaticBoxSizer(meshPreviewBox, wxVERTICAL);
    
    wxPanel* meshPreviewPanel = new wxPanel(m_meshQualityPage);
    meshPreviewPanel->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
    meshPreviewPanel->SetMinSize(wxSize(-1, 80));
    
    // Create preview text with correct parent (meshPreviewPanel)
    if (!m_meshQualityPreviewText) {
        m_meshQualityPreviewText = new wxStaticText(meshPreviewPanel, wxID_ANY, "");
        m_meshQualityPreviewText->SetBackgroundColour(CFG_COLOUR("SecondaryBackgroundColour"));
        m_meshQualityPreviewText->SetForegroundColour(CFG_COLOUR("AccentColour"));
        wxFont previewFont = CFG_FONT();
        previewFont.SetPointSize(previewFont.GetPointSize() + 1);
        m_meshQualityPreviewText->SetFont(previewFont);
        m_meshQualityPreviewText->Wrap(400); // Enable text wrapping
    }
    
    wxBoxSizer* meshPreviewPanelSizer = new wxBoxSizer(wxVERTICAL);
    meshPreviewPanelSizer->Add(m_meshQualityPreviewText, 1, wxEXPAND | wxALL, 10);
    meshPreviewPanel->SetSizer(meshPreviewPanelSizer);
    
    meshPreviewSizer->Add(meshPreviewPanel, 1, wxEXPAND | wxALL, 5);
    meshQualitySizer->Add(meshPreviewSizer, 0, wxEXPAND | wxALL, 10);

    m_meshQualityPage->SetSizer(meshQualitySizer);

    // Layout smooth surface page
    wxBoxSizer* smoothSurfaceSizer = new wxBoxSizer(wxVERTICAL);

    // Subdivision section
    wxStaticBox* subdivisionBox = new wxStaticBox(m_smoothSurfacePage, wxID_ANY, "Subdivision Settings");
    wxStaticBoxSizer* subdivisionSizer = new wxStaticBoxSizer(subdivisionBox, wxVERTICAL);
    
    wxFlexGridSizer* subdivisionGrid = new wxFlexGridSizer(2, 2, 10, 15);
    subdivisionGrid->AddGrowableCol(1);
    
    subdivisionGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Enabled:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    subdivisionGrid->Add(m_subdivisionEnabledCheckBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    subdivisionGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Level (1-5):"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    subdivisionGrid->Add(m_subdivisionLevelCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    subdivisionSizer->Add(subdivisionGrid, 0, wxEXPAND | wxALL, 10);
    smoothSurfaceSizer->Add(subdivisionSizer, 0, wxEXPAND | wxALL, 10);

    // Smoothing section
    wxStaticBox* smoothingBox = new wxStaticBox(m_smoothSurfacePage, wxID_ANY, "Smoothing Settings");
    wxStaticBoxSizer* smoothingSizer = new wxStaticBoxSizer(smoothingBox, wxVERTICAL);
    
    wxFlexGridSizer* smoothingGrid = new wxFlexGridSizer(4, 2, 10, 15);
    smoothingGrid->AddGrowableCol(1);
    
    smoothingGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Enabled:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    smoothingGrid->Add(m_smoothingEnabledCheckBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    smoothingGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Iterations (1-10):"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    smoothingGrid->Add(m_smoothingIterationsCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    smoothingGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Strength (0.01-1.0):"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    smoothingGrid->Add(m_smoothingStrengthCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    smoothingGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Crease Angle (0-180):"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    smoothingGrid->Add(m_smoothingCreaseAngleCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    smoothingSizer->Add(smoothingGrid, 0, wxEXPAND | wxALL, 10);
    smoothSurfaceSizer->Add(smoothingSizer, 0, wxEXPAND | wxALL, 10);

    // LOD section
    wxStaticBox* lodBox = new wxStaticBox(m_smoothSurfacePage, wxID_ANY, "LOD (Level of Detail) Settings");
    wxStaticBoxSizer* lodSizer = new wxStaticBoxSizer(lodBox, wxVERTICAL);
    
    wxFlexGridSizer* lodGrid = new wxFlexGridSizer(3, 2, 10, 15);
    lodGrid->AddGrowableCol(1);
    
    lodGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Enabled:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    lodGrid->Add(m_lodEnabledCheckBox, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    lodGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Fine Deflection:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    lodGrid->Add(m_lodFineDeflectionCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    lodGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Rough Deflection:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    lodGrid->Add(m_lodRoughDeflectionCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    lodSizer->Add(lodGrid, 0, wxEXPAND | wxALL, 10);
    smoothSurfaceSizer->Add(lodSizer, 0, wxEXPAND | wxALL, 10);

    // Tessellation section
    wxStaticBox* tessellationBox = new wxStaticBox(m_smoothSurfacePage, wxID_ANY, "Tessellation Settings");
    wxStaticBoxSizer* tessellationSizer = new wxStaticBoxSizer(tessellationBox, wxVERTICAL);
    
    wxFlexGridSizer* tessellationGrid = new wxFlexGridSizer(2, 2, 10, 15);
    tessellationGrid->AddGrowableCol(1);
    
    tessellationGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Quality (1-5):"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    tessellationGrid->Add(m_tessellationQualityCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    tessellationGrid->Add(new wxStaticText(m_smoothSurfacePage, wxID_ANY, "Feature Preservation (0.0-1.0):"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    tessellationGrid->Add(m_featurePreservationCtrl, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    tessellationSizer->Add(tessellationGrid, 0, wxEXPAND | wxALL, 10);
    smoothSurfaceSizer->Add(tessellationSizer, 0, wxEXPAND | wxALL, 10);

    m_smoothSurfacePage->SetSizer(smoothSurfaceSizer);

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okBtn = new wxButton(m_contentPanel, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(m_contentPanel, wxID_CANCEL, "Cancel");

    okBtn->SetDefault();
    okBtn->SetMinSize(wxSize(80, 30));
    cancelBtn->SetMinSize(wxSize(80, 30));

    buttonSizer->Add(okBtn, 0, wxALL, 5);
    buttonSizer->Add(cancelBtn, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);

    m_contentPanel->SetSizer(mainSizer);
}

void GeometryDecompositionDialog::bindEvents()
{
    m_enableDecompositionCheckBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        bool enabled = m_enableDecompositionCheckBox->GetValue();
        m_decompositionLevelChoice->Enable(enabled);
        m_colorSchemeChoice->Enable(enabled);
        m_consistentColoringCheckBox->Enable(enabled);
        updatePreview();
    });

    m_decompositionLevelChoice->Bind(wxEVT_CHOICE, &GeometryDecompositionDialog::onDecompositionLevelChange, this);
    m_colorSchemeChoice->Bind(wxEVT_CHOICE, &GeometryDecompositionDialog::onColorSchemeChange, this);
    m_consistentColoringCheckBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        updatePreview();
    });

    // Mesh quality events - bind preset button events
    m_fastPresetBtn->Bind(wxEVT_BUTTON, &GeometryDecompositionDialog::onFastPreset, this);
    m_balancedPresetBtn->Bind(wxEVT_BUTTON, &GeometryDecompositionDialog::onBalancedPreset, this);
    m_highQualityPresetBtn->Bind(wxEVT_BUTTON, &GeometryDecompositionDialog::onHighQualityPreset, this);
    m_ultraQualityPresetBtn->Bind(wxEVT_BUTTON, &GeometryDecompositionDialog::onUltraQualityPreset, this);
    m_customPresetBtn->Bind(wxEVT_BUTTON, &GeometryDecompositionDialog::onCustomPreset, this);
    
    m_customDeflectionCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        updateMeshQualityControls();
    });
    m_customAngularCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        updateMeshQualityControls();
    });

    // Smooth surface events
    m_subdivisionEnabledCheckBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        m_subdivisionLevelCtrl->Enable(m_subdivisionEnabledCheckBox->GetValue());
    });
    
    m_smoothingEnabledCheckBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        bool enabled = m_smoothingEnabledCheckBox->GetValue();
        m_smoothingIterationsCtrl->Enable(enabled);
        m_smoothingStrengthCtrl->Enable(enabled);
        m_smoothingCreaseAngleCtrl->Enable(enabled);
    });
    
    m_lodEnabledCheckBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        bool enabled = m_lodEnabledCheckBox->GetValue();
        m_lodFineDeflectionCtrl->Enable(enabled);
        m_lodRoughDeflectionCtrl->Enable(enabled);
    });
}

void GeometryDecompositionDialog::updatePreview()
{
    wxString preview;
    wxColour previewColor = CFG_COLOUR("AccentColour"); // Default accent color

    bool enabled = m_enableDecompositionCheckBox->GetValue();

    if (!enabled) {
        preview = "Decomposition: Disabled\n"
                  "Result: Single component per file\n"
                  "Coloring: Default";
        previewColor = CFG_COLOUR("PlaceholderTextColour"); // Gray
    } else {
        // Get selected level description
        wxString levelDesc;
        int levelIndex = m_decompositionLevelChoice->GetSelection();
        switch (levelIndex) {
            case 0: levelDesc = "No Decomposition"; break;
            case 1: levelDesc = "Shape Level"; break;
            case 2: levelDesc = "Solid Level"; break;
            case 3: levelDesc = "Shell Level"; break;
            case 4: levelDesc = "Face Level"; break;
            default: levelDesc = "Unknown"; break;
        }

        // Get selected color scheme description
        wxString colorDesc;
        int colorIndex = m_colorSchemeChoice->GetSelection();
        switch (colorIndex) {
            case 0: colorDesc = "Distinct Colors"; break;
            case 1: colorDesc = "Warm Colors"; break;
            case 2: colorDesc = "Rainbow"; break;
            case 3: colorDesc = "Blue Monochrome"; break;
            case 4: colorDesc = "Green Monochrome"; break;
            case 5: colorDesc = "Gray Monochrome"; break;
            default: colorDesc = "Unknown"; break;
        }

        wxString consistency = m_consistentColoringCheckBox->GetValue() ?
                              "Consistent" : "Random";

        preview = wxString::Format("Decomposition: Enabled (%s)\n"
                                  "Color Scheme: %s\n"
                                  "Coloring: %s",
                                  levelDesc.mb_str(), colorDesc.mb_str(), consistency.mb_str());

        if (levelIndex == 3) { // Face level
            preview += "\nWarning: Face level may create many small components";
            previewColor = CFG_COLOUR("ErrorTextColour"); // Warning color
        } else {
            previewColor = CFG_COLOUR("AccentColour"); // Success color
        }
    }

    if (m_previewText) {
        m_previewText->SetLabel(preview);
        m_previewText->SetForegroundColour(previewColor);
        m_previewPanel->Refresh();
        
        // Update color preview
        updateColorPreview();
    }
}

void GeometryDecompositionDialog::updateColorPreview()
{
    if (!m_colorPreviewPanel) return;
    
    // Clear existing preview controls
    m_colorPreviewPanel->DestroyChildren();
    wxBoxSizer* sizer = dynamic_cast<wxBoxSizer*>(m_colorPreviewPanel->GetSizer());
    if (sizer) {
        sizer->Clear();
    }
    
    // Get current color scheme
    int colorIndex = m_colorSchemeChoice->GetSelection();
    GeometryReader::ColorScheme scheme = static_cast<GeometryReader::ColorScheme>(colorIndex);
    
    // Create color palette based on scheme
    std::vector<wxColour> colors;
    switch (scheme) {
        case GeometryReader::ColorScheme::DISTINCT_COLORS:
            colors = {
                wxColour(70, 130, 180), wxColour(220, 20, 60), wxColour(34, 139, 34),
                wxColour(255, 140, 0), wxColour(128, 0, 128), wxColour(255, 20, 147),
                wxColour(0, 191, 255), wxColour(255, 69, 0), wxColour(50, 205, 50),
                wxColour(138, 43, 226), wxColour(255, 105, 180), wxColour(0, 206, 209),
                wxColour(255, 215, 0), wxColour(199, 21, 133), wxColour(72, 209, 204)
            };
            break;
        case GeometryReader::ColorScheme::WARM_COLORS:
            colors = {
                wxColour(255, 140, 0), wxColour(255, 69, 0), wxColour(255, 20, 147),
                wxColour(220, 20, 60), wxColour(255, 105, 180), wxColour(255, 215, 0),
                wxColour(255, 165, 0), wxColour(255, 99, 71), wxColour(255, 160, 122),
                wxColour(255, 192, 203), wxColour(255, 228, 225), wxColour(255, 69, 0),
                wxColour(255, 140, 0), wxColour(255, 20, 147), wxColour(255, 215, 0)
            };
            break;
        case GeometryReader::ColorScheme::RAINBOW:
            colors = {
                wxColour(255, 0, 0), wxColour(255, 127, 0), wxColour(255, 255, 0),
                wxColour(127, 255, 0), wxColour(0, 255, 0), wxColour(0, 255, 127),
                wxColour(0, 255, 255), wxColour(0, 127, 255), wxColour(0, 0, 255),
                wxColour(127, 0, 255), wxColour(255, 0, 255), wxColour(255, 0, 127),
                wxColour(255, 64, 0), wxColour(255, 191, 0), wxColour(191, 255, 0)
            };
            break;
        case GeometryReader::ColorScheme::MONOCHROME_BLUE:
            colors = {
                wxColour(25, 25, 112), wxColour(47, 79, 79), wxColour(70, 130, 180),
                wxColour(100, 149, 237), wxColour(135, 206, 235), wxColour(173, 216, 230),
                wxColour(176, 224, 230), wxColour(175, 238, 238), wxColour(95, 158, 160),
                wxColour(72, 209, 204), wxColour(64, 224, 208), wxColour(0, 206, 209),
                wxColour(0, 191, 255), wxColour(30, 144, 255), wxColour(0, 0, 255)
            };
            break;
        case GeometryReader::ColorScheme::MONOCHROME_GREEN:
            colors = {
                wxColour(0, 100, 0), wxColour(34, 139, 34), wxColour(50, 205, 50),
                wxColour(124, 252, 0), wxColour(127, 255, 0), wxColour(173, 255, 47),
                wxColour(154, 205, 50), wxColour(107, 142, 35), wxColour(85, 107, 47),
                wxColour(107, 142, 35), wxColour(154, 205, 50), wxColour(173, 255, 47),
                wxColour(127, 255, 0), wxColour(124, 252, 0), wxColour(50, 205, 50)
            };
            break;
        case GeometryReader::ColorScheme::MONOCHROME_GRAY:
            colors = {
                wxColour(64, 64, 64), wxColour(96, 96, 96), wxColour(128, 128, 128),
                wxColour(160, 160, 160), wxColour(192, 192, 192), wxColour(211, 211, 211),
                wxColour(220, 220, 220), wxColour(230, 230, 230), wxColour(105, 105, 105),
                wxColour(169, 169, 169), wxColour(200, 200, 200), wxColour(210, 210, 210),
                wxColour(220, 220, 220), wxColour(230, 230, 230), wxColour(240, 240, 240)
            };
            break;
        default:
            colors = { wxColour(128, 128, 128) };
            break;
    }
    
    // Create color swatches
    const int swatchSize = 20;
    const int swatchSpacing = 4;
    
    for (size_t i = 0; i < colors.size() && i < 12; ++i) { // Show first 12 colors
        wxPanel* swatch = new wxPanel(m_colorPreviewPanel);
        swatch->SetMinSize(wxSize(swatchSize, swatchSize));
        swatch->SetMaxSize(wxSize(swatchSize, swatchSize));
        swatch->SetBackgroundColour(colors[i]);
        
        // Add border
        swatch->SetWindowStyleFlag(wxBORDER_SIMPLE);
        
        sizer->Add(swatch, 0, wxALL, swatchSpacing);
    }
    
    // Add text label
    wxString schemeName;
    switch (scheme) {
        case GeometryReader::ColorScheme::DISTINCT_COLORS: schemeName = "Distinct Colors"; break;
        case GeometryReader::ColorScheme::WARM_COLORS: schemeName = "Warm Colors"; break;
        case GeometryReader::ColorScheme::RAINBOW: schemeName = "Rainbow"; break;
        case GeometryReader::ColorScheme::MONOCHROME_BLUE: schemeName = "Blue Monochrome"; break;
        case GeometryReader::ColorScheme::MONOCHROME_GREEN: schemeName = "Green Monochrome"; break;
        case GeometryReader::ColorScheme::MONOCHROME_GRAY: schemeName = "Gray Monochrome"; break;
        default: schemeName = "Unknown"; break;
    }
    
    wxStaticText* label = new wxStaticText(m_colorPreviewPanel, wxID_ANY, 
        wxString::Format("Sample: %s (%d colors)", schemeName, (int)colors.size()));
    wxFont labelFont = CFG_FONT();
    labelFont.SetPointSize(labelFont.GetPointSize() - 1);
    label->SetFont(labelFont);
    label->SetForegroundColour(CFG_COLOUR("PlaceholderTextColour"));
    
    sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    
    m_colorPreviewPanel->Layout();
}

GeometryReader::DecompositionOptions GeometryDecompositionDialog::getDecompositionOptions() const
{
    GeometryReader::DecompositionOptions options;
    options.enableDecomposition = m_enableDecompositionCheckBox->GetValue();
    options.level = static_cast<GeometryReader::DecompositionLevel>(m_decompositionLevelChoice->GetSelection());
    options.colorScheme = static_cast<GeometryReader::ColorScheme>(m_colorSchemeChoice->GetSelection());
    options.useConsistentColoring = m_consistentColoringCheckBox->GetValue();
    
    // Mesh quality settings - use current preset
    options.meshQualityPreset = m_meshQualityPreset;
    
    // Parse custom values
    double deflection = 1.0, angular = 1.0;
    if (m_customDeflectionCtrl->GetValue().ToDouble(&deflection)) {
        options.customMeshDeflection = deflection;
    }
    if (m_customAngularCtrl->GetValue().ToDouble(&angular)) {
        options.customAngularDeflection = angular;
    }
    
    // Smooth surface settings
    options.subdivisionEnabled = m_subdivisionEnabledCheckBox->GetValue();
    long subdivisionLevel = 2;
    if (m_subdivisionLevelCtrl->GetValue().ToLong(&subdivisionLevel)) {
        // Limit subdivision level for large complex geometries
        if (m_isLargeComplexGeometry && subdivisionLevel > 2) {
            subdivisionLevel = 2;
            LOG_INF_S("Subdivision level limited to 2 for large/complex geometry");
        }
        options.subdivisionLevel = static_cast<int>(subdivisionLevel);
    }
    
    options.smoothingEnabled = m_smoothingEnabledCheckBox->GetValue();
    long smoothingIterations = 3;
    if (m_smoothingIterationsCtrl->GetValue().ToLong(&smoothingIterations)) {
        // Limit smoothing iterations for large complex geometries
        if (m_isLargeComplexGeometry && smoothingIterations > 2) {
            smoothingIterations = 2;
            LOG_INF_S("Smoothing iterations limited to 2 for large/complex geometry");
        }
        options.smoothingIterations = static_cast<int>(smoothingIterations);
    }
    
    double smoothingStrength = 0.7;
    if (m_smoothingStrengthCtrl->GetValue().ToDouble(&smoothingStrength)) {
        // Limit smoothing strength for large complex geometries
        if (m_isLargeComplexGeometry && smoothingStrength > 0.5) {
            smoothingStrength = 0.5;
            LOG_INF_S("Smoothing strength limited to 0.5 for large/complex geometry");
        }
        options.smoothingStrength = smoothingStrength;
    }
    
    double smoothingCreaseAngle = 0.6;
    if (m_smoothingCreaseAngleCtrl->GetValue().ToDouble(&smoothingCreaseAngle)) {
        options.smoothingCreaseAngle = smoothingCreaseAngle;
    }
    
    options.lodEnabled = m_lodEnabledCheckBox->GetValue();
    double lodFine = 0.15;
    if (m_lodFineDeflectionCtrl->GetValue().ToDouble(&lodFine)) {
        options.lodFineDeflection = lodFine;
    }
    
    double lodRough = 0.3;
    if (m_lodRoughDeflectionCtrl->GetValue().ToDouble(&lodRough)) {
        options.lodRoughDeflection = lodRough;
    }
    
    long tessellationQuality = 3;
    if (m_tessellationQualityCtrl->GetValue().ToLong(&tessellationQuality)) {
        // Limit tessellation quality for large complex geometries
        if (m_isLargeComplexGeometry && tessellationQuality > 2) {
            tessellationQuality = 2;
            LOG_INF_S("Tessellation quality limited to 2 for large/complex geometry");
        }
        options.tessellationQuality = static_cast<int>(tessellationQuality);
    }
    
    double featurePreservation = 0.8;
    if (m_featurePreservationCtrl->GetValue().ToDouble(&featurePreservation)) {
        // Limit feature preservation for large complex geometries
        if (m_isLargeComplexGeometry && featurePreservation > 0.5) {
            featurePreservation = 0.5;
            LOG_INF_S("Feature preservation limited to 0.5 for large/complex geometry");
        }
        options.featurePreservation = featurePreservation;
    }
    
    // Force balanced mesh quality preset for large complex geometries
    if (m_isLargeComplexGeometry) {
        options.meshQualityPreset = GeometryReader::MeshQualityPreset::BALANCED;
        LOG_INF_S("Mesh quality preset forced to BALANCED for large/complex geometry");
    }
    
    return options;
}

void GeometryDecompositionDialog::updateMeshQualityControls()
{
    // Prevent recursive calls
    if (m_updatingMeshQuality) {
        return;
    }
    
    m_updatingMeshQuality = true;
    
    GeometryReader::MeshQualityPreset preset = m_meshQualityPreset;
    
    // Enable/disable custom controls based on preset
    bool isCustom = (preset == GeometryReader::MeshQualityPreset::CUSTOM);
    m_customDeflectionCtrl->Enable(isCustom);
    m_customAngularCtrl->Enable(isCustom);
    
    // Update preview text
    wxString previewText;
    double deflection = 1.0, angular = 1.0;
    
    switch (preset) {
        case GeometryReader::MeshQualityPreset::FAST:
            deflection = 2.0; angular = 2.0;
            previewText = wxString::Format("Preset: Fast Quality\n"
                "Deflection: %.2f  |  Angular: %.2f\n"
                "Fast import speed, coarser mesh quality\n"
                "Best for: Quick previews and large assemblies", deflection, angular);
            break;
        case GeometryReader::MeshQualityPreset::BALANCED:
            deflection = 1.0; angular = 1.0;
            if (m_isLargeComplexGeometry) {
                previewText = wxString::Format("Preset: Balanced Quality (Required for Large/Complex Geometry)\n"
                    "Deflection: %.2f  |  Angular: %.2f\n"
                    "High-quality options are disabled for performance\n"
                    "Using balanced settings for large/complex geometries", deflection, angular);
            } else {
                previewText = wxString::Format("Preset: Balanced Quality\n"
                    "Deflection: %.2f  |  Angular: %.2f\n"
                    "Good balance of speed and visual quality\n"
                    "Best for: General use and interactive work", deflection, angular);
            }
            break;
        case GeometryReader::MeshQualityPreset::HIGH_QUALITY:
            // Match MeshQualityDialog "quality" preset parameters
            deflection = 0.5; angular = 0.5;
            previewText = wxString::Format("Preset: High Quality (Default)\n"
                "Deflection: %.2f  |  Angular: %.2f\n"
                "High quality mesh with better visual detail\n"
                "Best for: Production work and presentations", deflection, angular);
            break;
        case GeometryReader::MeshQualityPreset::ULTRA_QUALITY:
            // Match MeshQualityDialog "ultra" preset parameters
            deflection = 0.2; angular = 0.3;
            previewText = wxString::Format("Preset: Ultra Quality\n"
                "Deflection: %.2f  |  Angular: %.2f\n"
                "Ultra high quality with maximum smoothness\n"
                "Best for: Final rendering and critical surfaces", deflection, angular);
            break;
        case GeometryReader::MeshQualityPreset::CUSTOM:
            m_customDeflectionCtrl->GetValue().ToDouble(&deflection);
            m_customAngularCtrl->GetValue().ToDouble(&angular);
            previewText = wxString::Format("Preset: Custom Settings\n"
                "Deflection: %.4f  |  Angular: %.4f\n"
                "User-defined mesh quality parameters\n"
                "Fine-tune mesh quality for specific needs", deflection, angular);
            break;
        default:
            previewText = "Unknown preset";
            break;
    }
    
    // Update deflection controls with preset values (only if not custom)
    // This will trigger wxEVT_TEXT, but the flag prevents recursion
    if (!isCustom) {
        wxString deflectionStr = wxString::Format("%.4f", deflection);
        wxString angularStr = wxString::Format("%.4f", angular);
        
        // Only update if value actually changed to avoid unnecessary events
        if (m_customDeflectionCtrl->GetValue() != deflectionStr) {
            m_customDeflectionCtrl->SetValue(deflectionStr);
        }
        if (m_customAngularCtrl->GetValue() != angularStr) {
            m_customAngularCtrl->SetValue(angularStr);
        }
    }
    
    // Update preview text if it exists
    if (m_meshQualityPreviewText) {
        m_meshQualityPreviewText->SetLabel(previewText);
        m_meshQualityPreviewText->SetForegroundColour(CFG_COLOUR("AccentColour"));
        m_meshQualityPreviewText->Wrap(400); // Enable text wrapping for better display
    }
    
    // Update button colors to show selected preset
    updatePresetButtonColors();
    
    m_updatingMeshQuality = false;
}

void GeometryDecompositionDialog::onFastPreset(wxCommandEvent& event)
{
    m_meshQualityPreset = GeometryReader::MeshQualityPreset::FAST;
    updateMeshQualityControls();
}

void GeometryDecompositionDialog::onBalancedPreset(wxCommandEvent& event)
{
    m_meshQualityPreset = GeometryReader::MeshQualityPreset::BALANCED;
    updateMeshQualityControls();
}

void GeometryDecompositionDialog::onHighQualityPreset(wxCommandEvent& event)
{
    m_meshQualityPreset = GeometryReader::MeshQualityPreset::HIGH_QUALITY;
    updateMeshQualityControls();
}

void GeometryDecompositionDialog::onUltraQualityPreset(wxCommandEvent& event)
{
    m_meshQualityPreset = GeometryReader::MeshQualityPreset::ULTRA_QUALITY;
    updateMeshQualityControls();
}

void GeometryDecompositionDialog::onCustomPreset(wxCommandEvent& event)
{
    m_meshQualityPreset = GeometryReader::MeshQualityPreset::CUSTOM;
    updateMeshQualityControls();
}

void GeometryDecompositionDialog::updatePresetButtonColors()
{
    // Default button colors (unselected) - use theme colors
    wxColour defaultBg = CFG_COLOUR("ButtonbarDefaultBgColour");
    wxColour defaultFg = CFG_COLOUR("ButtonbarDefaultTextColour");
    
    // Selected button colors - use theme accent colors with variations
    wxColour fastSelectedBg = CFG_COLOUR("ButtonbarDefaultHoverBgColour");
    wxColour fastSelectedFg = CFG_COLOUR("AccentColour");
    
    wxColour balancedSelectedBg = CFG_COLOUR("ButtonbarDefaultHoverBgColour");
    wxColour balancedSelectedFg = CFG_COLOUR("AccentColour");
    
    wxColour highQualitySelectedBg = CFG_COLOUR("AccentColour"); // Use accent color for default
    wxColour highQualitySelectedFg = CFG_COLOUR("DropdownSelectionTextColour");
    
    wxColour ultraSelectedBg = CFG_COLOUR("HighlightColour");
    wxColour ultraSelectedFg = CFG_COLOUR("DropdownSelectionTextColour");
    
    wxColour customSelectedBg = CFG_COLOUR("ButtonbarDefaultPressedBgColour");
    wxColour customSelectedFg = CFG_COLOUR("ButtonbarDefaultTextColour");
    
    // Reset all buttons to default colors
    if (m_fastPresetBtn) {
        m_fastPresetBtn->SetBackgroundColour(defaultBg);
        m_fastPresetBtn->SetForegroundColour(defaultFg);
    }
    if (m_balancedPresetBtn) {
        m_balancedPresetBtn->SetBackgroundColour(defaultBg);
        m_balancedPresetBtn->SetForegroundColour(defaultFg);
    }
    if (m_highQualityPresetBtn) {
        m_highQualityPresetBtn->SetBackgroundColour(defaultBg);
        m_highQualityPresetBtn->SetForegroundColour(defaultFg);
    }
    if (m_ultraQualityPresetBtn) {
        m_ultraQualityPresetBtn->SetBackgroundColour(defaultBg);
        m_ultraQualityPresetBtn->SetForegroundColour(defaultFg);
    }
    if (m_customPresetBtn) {
        m_customPresetBtn->SetBackgroundColour(defaultBg);
        m_customPresetBtn->SetForegroundColour(defaultFg);
    }
    
    // Highlight selected preset with appropriate color
    switch (m_meshQualityPreset) {
        case GeometryReader::MeshQualityPreset::FAST:
            if (m_fastPresetBtn) {
                m_fastPresetBtn->SetBackgroundColour(fastSelectedBg);
                m_fastPresetBtn->SetForegroundColour(fastSelectedFg);
            }
            break;
        case GeometryReader::MeshQualityPreset::BALANCED:
            if (m_balancedPresetBtn) {
                m_balancedPresetBtn->SetBackgroundColour(balancedSelectedBg);
                m_balancedPresetBtn->SetForegroundColour(balancedSelectedFg);
            }
            break;
        case GeometryReader::MeshQualityPreset::HIGH_QUALITY:
            if (m_highQualityPresetBtn) {
                m_highQualityPresetBtn->SetBackgroundColour(highQualitySelectedBg);
                m_highQualityPresetBtn->SetForegroundColour(highQualitySelectedFg);
            }
            break;
        case GeometryReader::MeshQualityPreset::ULTRA_QUALITY:
            if (m_ultraQualityPresetBtn) {
                m_ultraQualityPresetBtn->SetBackgroundColour(ultraSelectedBg);
                m_ultraQualityPresetBtn->SetForegroundColour(ultraSelectedFg);
            }
            break;
        case GeometryReader::MeshQualityPreset::CUSTOM:
            if (m_customPresetBtn) {
                m_customPresetBtn->SetBackgroundColour(customSelectedBg);
                m_customPresetBtn->SetForegroundColour(customSelectedFg);
            }
            break;
        default:
            break;
    }
    
    // Refresh buttons to show color changes
    if (m_fastPresetBtn) m_fastPresetBtn->Refresh();
    if (m_balancedPresetBtn) m_balancedPresetBtn->Refresh();
    if (m_highQualityPresetBtn) m_highQualityPresetBtn->Refresh();
    if (m_ultraQualityPresetBtn) m_ultraQualityPresetBtn->Refresh();
    if (m_customPresetBtn) m_customPresetBtn->Refresh();
}

void GeometryDecompositionDialog::onOK(wxCommandEvent& event)
{
    // Save settings to the referenced options
    m_options = getDecompositionOptions();

    LOG_INF_S(wxString::Format("Geometry decomposition settings saved: Enabled=%s, Level=%d, Scheme=%d",
        m_options.enableDecomposition ? "Yes" : "No",
        static_cast<int>(m_options.level),
        static_cast<int>(m_options.colorScheme)));

    EndModal(wxID_OK);
}

void GeometryDecompositionDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void GeometryDecompositionDialog::onDecompositionLevelChange(wxCommandEvent& event)
{
    updatePreview();
}

void GeometryDecompositionDialog::onColorSchemeChange(wxCommandEvent& event)
{
    updatePreview();
}

bool GeometryDecompositionDialog::isLargeComplexGeometry(const std::vector<std::string>& filePaths)
{
    const size_t LARGE_FILE_THRESHOLD = 30 * 1024 * 1024; // 30MB
    const size_t TOTAL_SIZE_THRESHOLD = 100 * 1024 * 1024; // 100MB total
    
    size_t totalSize = 0;
    size_t largeFileCount = 0;
    
    for (const auto& filePath : filePaths) {
        try {
            if (std::filesystem::exists(filePath)) {
                size_t fileSize = std::filesystem::file_size(filePath);
                totalSize += fileSize;
                
                if (fileSize >= LARGE_FILE_THRESHOLD) {
                    largeFileCount++;
                }
            }
        } catch (const std::exception& e) {
            LOG_WRN_S("Failed to check file size for " + filePath + ": " + std::string(e.what()));
        }
    }
    
    // Consider large/complex if:
    // 1. Any single file is >= 30MB, OR
    // 2. Total size of all files >= 100MB, OR
    // 3. Multiple large files (>= 2 files >= 30MB)
    bool isLarge = (largeFileCount > 0) || (totalSize >= TOTAL_SIZE_THRESHOLD) || (largeFileCount >= 2);
    
    if (isLarge) {
        LOG_INF_S("Large/complex geometry detected: " + std::to_string(largeFileCount) + 
                 " large files, total size: " + std::to_string(totalSize / (1024*1024)) + " MB");
    }
    
    return isLarge;
}

bool GeometryDecompositionDialog::isComplexGeometryByCounts(int faceCount, int assemblyCount)
{
    const int FACE_COUNT_THRESHOLD = 2000;      // 2000 faces
    const int ASSEMBLY_COUNT_THRESHOLD = 200;   // 200 assembly components
    
    bool isComplex = (faceCount > FACE_COUNT_THRESHOLD) || (assemblyCount > ASSEMBLY_COUNT_THRESHOLD);
    
    if (isComplex) {
        LOG_INF_S("Complex geometry detected by counts: faces=" + std::to_string(faceCount) + 
                 ", assemblies=" + std::to_string(assemblyCount));
    }
    
    return isComplex;
}

void GeometryDecompositionDialog::applyLargeComplexGeometryRestrictions()
{
    LOG_INF_S("Applying restrictions for large/complex geometry - using balanced settings");
    
    // Force balanced mesh quality preset
    m_meshQualityPreset = GeometryReader::MeshQualityPreset::BALANCED;
    m_customMeshDeflection = 1.0;
    m_customAngularDeflection = 1.0;
    
    // Use basic smooth parameters (not high quality)
    m_subdivisionEnabled = true;
    m_subdivisionLevel = 2;  // Basic level, not high
    m_smoothingEnabled = true;
    m_smoothingIterations = 2;  // Basic iterations, not high (3+)
    m_smoothingStrength = 0.5;  // Basic strength, not high (0.7+)
    m_smoothingCreaseAngle = 30.0;  // Standard crease angle
    
    // Enable LOD for performance
    m_lodEnabled = true;
    m_lodFineDeflection = 0.2;  // Basic fine deflection
    m_lodRoughDeflection = 0.5;  // Basic rough deflection
    
    // Use balanced tessellation quality (not high)
    m_tessellationQuality = 2;  // Balanced quality, not high (3+)
    m_featurePreservation = 0.5;  // Balanced preservation, not high (0.8+)
    
    // Disable high-quality preset buttons
    if (m_highQualityPresetBtn) {
        m_highQualityPresetBtn->Enable(false);
        m_highQualityPresetBtn->SetToolTip("High Quality preset disabled for large/complex geometries");
    }
    if (m_ultraQualityPresetBtn) {
        m_ultraQualityPresetBtn->Enable(false);
        m_ultraQualityPresetBtn->SetToolTip("Ultra Quality preset disabled for large/complex geometries");
    }
    
    // Limit subdivision level to max 2
    if (m_subdivisionLevelCtrl) {
        m_subdivisionLevelCtrl->SetValue("2");
        // Note: We can't easily limit the range of wxTextCtrl, but we'll validate in getDecompositionOptions
    }
    
    // Limit smoothing iterations to max 2
    if (m_smoothingIterationsCtrl) {
        m_smoothingIterationsCtrl->SetValue("2");
    }
    
    // Limit smoothing strength to max 0.5
    if (m_smoothingStrengthCtrl) {
        m_smoothingStrengthCtrl->SetValue("0.50");
    }
    
    // Limit tessellation quality to max 2
    if (m_tessellationQualityCtrl) {
        m_tessellationQualityCtrl->SetValue("2");
    }
    
    // Limit feature preservation to max 0.5
    if (m_featurePreservationCtrl) {
        m_featurePreservationCtrl->SetValue("0.50");
    }
    
    // Add warning message
    if (m_meshQualityPreviewText) {
        wxString warning = "Large/Complex Geometry Detected:\n";
        warning += "High-quality options are disabled for performance.\n";
        warning += "Using balanced mesh quality settings.";
        m_meshQualityPreviewText->SetLabel(warning);
        m_meshQualityPreviewText->SetForegroundColour(CFG_COLOUR("ErrorTextColour"));
    }
}