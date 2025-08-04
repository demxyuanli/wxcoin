#include "renderpreview/GlobalSettingsPanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/PreviewCanvas.h"
#include "renderpreview/AntiAliasingManager.h"
#include "renderpreview/RenderingManager.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/colordlg.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

BEGIN_EVENT_TABLE(GlobalSettingsPanel, wxPanel)
    EVT_LISTBOX(wxID_ANY, GlobalSettingsPanel::onLightSelected)
    EVT_BUTTON(wxID_ANY, GlobalSettingsPanel::onAddLight)
    EVT_BUTTON(wxID_ANY, GlobalSettingsPanel::onRemoveLight)
    EVT_TEXT(wxID_ANY, GlobalSettingsPanel::onLightPropertyChanged)
    EVT_CHOICE(wxID_ANY, GlobalSettingsPanel::onLightPropertyChanged)
    EVT_CHECKBOX(wxID_ANY, GlobalSettingsPanel::onLightPropertyChanged)
    EVT_CHOICE(wxID_ANY, GlobalSettingsPanel::onAntiAliasingChanged)
    EVT_SLIDER(wxID_ANY, GlobalSettingsPanel::onAntiAliasingChanged)
    EVT_CHECKBOX(wxID_ANY, GlobalSettingsPanel::onAntiAliasingChanged)
    EVT_CHOICE(wxID_ANY, GlobalSettingsPanel::onRenderingModeChanged)
END_EVENT_TABLE()

GlobalSettingsPanel::GlobalSettingsPanel(wxWindow* parent, RenderPreviewDialog* dialog, wxWindowID id)
    : wxPanel(parent, id)
    , m_parentDialog(dialog)
    , m_currentLightIndex(-1)
    , m_autoApply(false)
    , m_antiAliasingManager(nullptr)
    , m_renderingManager(nullptr)
{
    LOG_INF_S("GlobalSettingsPanel::GlobalSettingsPanel: Initializing");
    createUI();
    bindEvents();
    loadSettings();
    
    // Initialize control states after loading settings
    updateControlStates();
    
    LOG_INF_S("GlobalSettingsPanel::GlobalSettingsPanel: Initialized successfully");
}

GlobalSettingsPanel::~GlobalSettingsPanel()
{
    LOG_INF_S("GlobalSettingsPanel::~GlobalSettingsPanel: Destroying");
}

void GlobalSettingsPanel::createUI()
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for tabs
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    // Create tab panels
    auto* lightingPanel = new wxPanel(m_notebook, wxID_ANY);
    auto* lightPresetsPanel = new wxPanel(m_notebook, wxID_ANY);
    auto* antiAliasingPanel = new wxPanel(m_notebook, wxID_ANY);
    auto* renderingModePanel = new wxPanel(m_notebook, wxID_ANY);
    
    // Set up tab panels with their content
    lightingPanel->SetSizer(createLightingTab(lightingPanel));
    lightPresetsPanel->SetSizer(createLightPresetsTab(lightPresetsPanel));
    antiAliasingPanel->SetSizer(createAntiAliasingTab(antiAliasingPanel));
    renderingModePanel->SetSizer(createRenderingModeTab(renderingModePanel));
    
    // Add tabs to notebook
    m_notebook->AddPage(lightingPanel, "Lighting");
    m_notebook->AddPage(lightPresetsPanel, "Light Presets");
    m_notebook->AddPage(antiAliasingPanel, "Anti-aliasing");
    m_notebook->AddPage(renderingModePanel, "Rendering Mode");
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 2);
    
    // Add Global Settings buttons at the bottom
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Auto-apply checkbox
    m_globalAutoApplyCheckBox = new wxCheckBox(this, wxID_ANY, wxT("Auto"));
    m_globalAutoApplyCheckBox->Bind(wxEVT_CHECKBOX, &GlobalSettingsPanel::OnGlobalAutoApply, this);
    buttonSizer->Add(m_globalAutoApplyCheckBox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    
    // Apply button
    m_globalApplyButton = new wxButton(this, wxID_APPLY, wxT("Apply"));
    m_globalApplyButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnGlobalApply, this);
    buttonSizer->Add(m_globalApplyButton, 0, wxALL, 2);
    
    // Save button
    m_globalSaveButton = new wxButton(this, wxID_SAVE, wxT("Save"));
    m_globalSaveButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnGlobalSave, this);
    buttonSizer->Add(m_globalSaveButton, 0, wxALL, 2);
    
    // Reset button
    m_globalResetButton = new wxButton(this, wxID_RESET, wxT("Reset"));
    m_globalResetButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnGlobalReset, this);
    buttonSizer->Add(m_globalResetButton, 0, wxALL, 2);
    
    // Undo button
    m_globalUndoButton = new wxButton(this, wxID_UNDO, wxT("Undo"));
    m_globalUndoButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnGlobalUndo, this);
    buttonSizer->Add(m_globalUndoButton, 0, wxALL, 2);
    
    // Redo button
    m_globalRedoButton = new wxButton(this, wxID_REDO, wxT("Redo"));
    m_globalRedoButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnGlobalRedo, this);
    buttonSizer->Add(m_globalRedoButton, 0, wxALL, 2);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
}

wxSizer* GlobalSettingsPanel::createLightingTab(wxWindow* parent)
{
    auto* lightingSizer = new wxBoxSizer(wxVERTICAL);
    
    // Light list section with better styling
    auto* listBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Light List");
    
    // Create a custom list with color preview buttons
    auto* lightListSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create a scrollable panel for the light list
    auto* lightListPanel = new wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
    lightListPanel->SetScrollRate(10, 10);
    m_lightListSizer = new wxBoxSizer(wxVERTICAL);
    lightListPanel->SetSizer(m_lightListSizer);
    
    lightListSizer->Add(lightListPanel, 1, wxEXPAND | wxALL, 8);
    listBoxSizer->Add(lightListSizer, 1, wxEXPAND | wxALL, 8);
    
    // Light list buttons with better spacing
    auto* listButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_addLightButton = new wxButton(parent, wxID_ANY, "Add Light");
    m_removeLightButton = new wxButton(parent, wxID_ANY, "Remove Light");
    listButtonSizer->Add(m_addLightButton, 1, wxEXPAND | wxRIGHT, 5);
    listButtonSizer->Add(m_removeLightButton, 1, wxEXPAND | wxLEFT, 5);
    listBoxSizer->Add(listButtonSizer, 0, wxEXPAND | wxALL, 8);
    
    lightingSizer->Add(listBoxSizer, 0, wxEXPAND | wxALL, 10);
    
    // Light properties section with better organization
    auto* propertiesBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Light Properties");
    
    // Basic properties in a grid layout
    auto* basicGridSizer = new wxFlexGridSizer(2, 2, 10, 15);
    basicGridSizer->AddGrowableCol(1, 1);
    
    // Light name
    basicGridSizer->Add(new wxStaticText(parent, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_lightNameText = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, -1));
    basicGridSizer->Add(m_lightNameText, 1, wxEXPAND);
    
    // Light type
    basicGridSizer->Add(new wxStaticText(parent, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL);
    m_lightTypeChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, -1));
    m_lightTypeChoice->Append("directional");
    m_lightTypeChoice->Append("point");
    m_lightTypeChoice->Append("spot");
    basicGridSizer->Add(m_lightTypeChoice, 1, wxEXPAND);
    
    propertiesBoxSizer->Add(basicGridSizer, 0, wxEXPAND | wxALL, 10);
    
    // Position section
    auto* positionSizer = new wxBoxSizer(wxHORIZONTAL);
    positionSizer->Add(new wxStaticText(parent, wxID_ANY, "Position:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    auto* posControlsSizer = new wxBoxSizer(wxHORIZONTAL);
    posControlsSizer->Add(new wxStaticText(parent, wxID_ANY, "X:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_positionXSpin = new wxSpinCtrlDouble(parent, wxID_ANY, "0.0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, -100.0, 100.0, 0.0, 0.1);
    posControlsSizer->Add(m_positionXSpin, 0, wxRIGHT, 8);
    
    posControlsSizer->Add(new wxStaticText(parent, wxID_ANY, "Y:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_positionYSpin = new wxSpinCtrlDouble(parent, wxID_ANY, "0.0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, -100.0, 100.0, 0.0, 0.1);
    posControlsSizer->Add(m_positionYSpin, 0, wxRIGHT, 8);
    
    posControlsSizer->Add(new wxStaticText(parent, wxID_ANY, "Z:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_positionZSpin = new wxSpinCtrlDouble(parent, wxID_ANY, "10.0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, -100.0, 100.0, 10.0, 0.1);
    posControlsSizer->Add(m_positionZSpin, 0);
    
    positionSizer->Add(posControlsSizer, 1, wxEXPAND);
    propertiesBoxSizer->Add(positionSizer, 0, wxEXPAND | wxALL, 10);
    
    // Direction section
    auto* directionSizer = new wxBoxSizer(wxHORIZONTAL);
    directionSizer->Add(new wxStaticText(parent, wxID_ANY, "Direction:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    auto* dirControlsSizer = new wxBoxSizer(wxHORIZONTAL);
    dirControlsSizer->Add(new wxStaticText(parent, wxID_ANY, "X:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_directionXSpin = new wxSpinCtrlDouble(parent, wxID_ANY, "0.0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, -1.0, 1.0, 0.0, 0.1);
    dirControlsSizer->Add(m_directionXSpin, 0, wxRIGHT, 8);
    
    dirControlsSizer->Add(new wxStaticText(parent, wxID_ANY, "Y:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_directionYSpin = new wxSpinCtrlDouble(parent, wxID_ANY, "0.0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, -1.0, 1.0, 0.0, 0.1);
    dirControlsSizer->Add(m_directionYSpin, 0, wxRIGHT, 8);
    
    dirControlsSizer->Add(new wxStaticText(parent, wxID_ANY, "Z:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_directionZSpin = new wxSpinCtrlDouble(parent, wxID_ANY, "-1.0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, -1.0, 1.0, -1.0, 0.1);
    dirControlsSizer->Add(m_directionZSpin, 0);
    
    directionSizer->Add(dirControlsSizer, 1, wxEXPAND);
    propertiesBoxSizer->Add(directionSizer, 0, wxEXPAND | wxALL, 10);
    
    // Intensity and color in a horizontal layout
    auto* intensityColorSizer = new wxBoxSizer(wxHORIZONTAL);
    
    auto* intensitySizer = new wxBoxSizer(wxHORIZONTAL);
    intensitySizer->Add(new wxStaticText(parent, wxID_ANY, "Intensity:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_intensitySpin = new wxSpinCtrlDouble(parent, wxID_ANY, "1.0", wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS, 0.0, 10.0, 1.0, 0.1);
    intensitySizer->Add(m_intensitySpin, 0);
    intensityColorSizer->Add(intensitySizer, 0, wxRIGHT, 20);
    
    auto* colorSizer = new wxBoxSizer(wxHORIZONTAL);
    colorSizer->Add(new wxStaticText(parent, wxID_ANY, "Color:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_lightColorButton = new wxButton(parent, wxID_ANY, "White", wxDefaultPosition, wxSize(100, -1));
    m_lightColorButton->SetBackgroundColour(wxColour(255, 255, 255));
    colorSizer->Add(m_lightColorButton, 0);
    intensityColorSizer->Add(colorSizer, 0);
    
    propertiesBoxSizer->Add(intensityColorSizer, 0, wxALL, 10);
    
    // Enabled checkbox
    m_lightEnabledCheckBox = new wxCheckBox(parent, wxID_ANY, "Enabled");
    m_lightEnabledCheckBox->SetValue(true);
    propertiesBoxSizer->Add(m_lightEnabledCheckBox, 0, wxALL, 10);
    
    lightingSizer->Add(propertiesBoxSizer, 1, wxEXPAND | wxALL, 10);
    
    return lightingSizer;
}

wxSizer* GlobalSettingsPanel::createAntiAliasingTab(wxWindow* parent)
{
    auto* antiAliasingSizer = new wxBoxSizer(wxVERTICAL);
    
    // Anti-aliasing Presets Section
    auto* presetsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Anti-aliasing Presets");
    
    auto* presetsLabel = new wxStaticText(parent, wxID_ANY, "Choose an anti-aliasing preset:");
    presetsBoxSizer->Add(presetsLabel, 0, wxALL, 8);
    
    m_antiAliasingChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
    m_antiAliasingChoice->Append("None");
    m_antiAliasingChoice->Append("MSAA 2x");
    m_antiAliasingChoice->Append("MSAA 4x");
    m_antiAliasingChoice->Append("MSAA 8x");
    m_antiAliasingChoice->Append("FXAA Low");
    m_antiAliasingChoice->Append("FXAA Medium");
    m_antiAliasingChoice->Append("FXAA High");
    m_antiAliasingChoice->SetSelection(2); // Default to MSAA 4x
    presetsBoxSizer->Add(m_antiAliasingChoice, 0, wxEXPAND | wxALL, 8);
    
    antiAliasingSizer->Add(presetsBoxSizer, 0, wxEXPAND | wxALL, 8);
    
    // Performance Impact Section
    auto* performanceBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Performance Impact");
    
    auto* impactLabel = new wxStaticText(parent, wxID_ANY, "Performance Impact: Low");
    impactLabel->SetForegroundColour(wxColour(0, 128, 0));
    performanceBoxSizer->Add(impactLabel, 0, wxALL, 8);
    
    auto* qualityLabel = new wxStaticText(parent, wxID_ANY, "Quality: MSAA 4x");
    qualityLabel->SetForegroundColour(wxColour(0, 0, 128));
    performanceBoxSizer->Add(qualityLabel, 0, wxALL, 8);
    
    antiAliasingSizer->Add(performanceBoxSizer, 0, wxEXPAND | wxALL, 8);
    
    // Legacy controls (kept for backward compatibility)
    auto* legacyBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Advanced Settings");
    
    auto* msaaLabel = new wxStaticText(parent, wxID_ANY, "MSAA Samples:");
    legacyBoxSizer->Add(msaaLabel, 0, wxALL, 8);
    
    m_msaaSamplesSlider = new wxSlider(parent, wxID_ANY, 4, 2, 16, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    legacyBoxSizer->Add(m_msaaSamplesSlider, 0, wxEXPAND | wxALL, 8);
    
    m_fxaaCheckBox = new wxCheckBox(parent, wxID_ANY, "Enable FXAA");
    legacyBoxSizer->Add(m_fxaaCheckBox, 0, wxALL, 8);
    
    antiAliasingSizer->Add(legacyBoxSizer, 0, wxEXPAND | wxALL, 8);
    
    return antiAliasingSizer;
}

wxSizer* GlobalSettingsPanel::createRenderingModeTab(wxWindow* parent)
{
    auto* renderingSizer = new wxBoxSizer(wxVERTICAL);
    
    // Rendering Presets Section
    auto* presetsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Rendering Presets");
    
    auto* presetsLabel = new wxStaticText(parent, wxID_ANY, "Choose a rendering preset:");
    presetsBoxSizer->Add(presetsLabel, 0, wxALL, 8);
    
    m_renderingModeChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
    m_renderingModeChoice->Append("Performance");
    m_renderingModeChoice->Append("Balanced");
    m_renderingModeChoice->Append("Quality");
    m_renderingModeChoice->Append("Ultra");
    m_renderingModeChoice->Append("Wireframe");
    m_renderingModeChoice->SetSelection(1); // Default to Balanced
    presetsBoxSizer->Add(m_renderingModeChoice, 0, wxEXPAND | wxALL, 8);
    
    renderingSizer->Add(presetsBoxSizer, 0, wxEXPAND | wxALL, 8);
    
    // Performance Impact Section
    auto* performanceBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Performance Impact");
    
    auto* impactLabel = new wxStaticText(parent, wxID_ANY, "Performance Impact: Medium");
    impactLabel->SetForegroundColour(wxColour(255, 165, 0));
    performanceBoxSizer->Add(impactLabel, 0, wxALL, 8);
    
    auto* qualityLabel = new wxStaticText(parent, wxID_ANY, "Quality: Balanced");
    qualityLabel->SetForegroundColour(wxColour(0, 0, 128));
    performanceBoxSizer->Add(qualityLabel, 0, wxALL, 8);
    
    auto* fpsLabel = new wxStaticText(parent, wxID_ANY, "Estimated FPS: 60");
    fpsLabel->SetForegroundColour(wxColour(0, 128, 0));
    performanceBoxSizer->Add(fpsLabel, 0, wxALL, 8);
    
    renderingSizer->Add(performanceBoxSizer, 0, wxEXPAND | wxALL, 8);
    
    // Legacy controls (kept for backward compatibility)
    auto* legacyBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Legacy Mode Settings");
    
    auto* modeLabel = new wxStaticText(parent, wxID_ANY, "Legacy Rendering Mode:");
    legacyBoxSizer->Add(modeLabel, 0, wxALL, 8);
    
    auto* legacyChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
    legacyChoice->Append("Solid");
    legacyChoice->Append("Wireframe");
    legacyChoice->Append("Points");
    legacyChoice->Append("Hidden Line");
    legacyChoice->Append("Shaded");
    legacyChoice->SetSelection(4);
    legacyBoxSizer->Add(legacyChoice, 0, wxEXPAND | wxALL, 8);
    
    renderingSizer->Add(legacyBoxSizer, 0, wxEXPAND | wxALL, 8);
    
    return renderingSizer;
}

wxSizer* GlobalSettingsPanel::createLightPresetsTab(wxWindow* parent)
{
    auto* presetsSizer = new wxBoxSizer(wxVERTICAL);
    
    // Header section
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* headerText = new wxStaticText(parent, wxID_ANY, "Choose a lighting preset to apply to your scene");
    headerText->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    headerSizer->Add(headerText, 1, wxALIGN_CENTER_VERTICAL);
    presetsSizer->Add(headerSizer, 0, wxALL, 15);
    
    // Main presets grid with better organization
    auto* presetsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Lighting Presets");
    
    // Create preset buttons in a 4x2 grid for better visual balance
    auto* gridSizer = new wxGridSizer(4, 2, 10, 10);
    
    // Studio Lighting
    m_studioButton = new wxButton(parent, wxID_ANY, "Studio\nLighting", wxDefaultPosition, wxSize(120, 60));
    m_studioButton->SetBackgroundColour(wxColour(240, 248, 255));
    m_studioButton->SetToolTip("Professional studio lighting with soft shadows");
    gridSizer->Add(m_studioButton, 0, wxEXPAND);
    
    // Outdoor Lighting
    m_outdoorButton = new wxButton(parent, wxID_ANY, "Outdoor\nNatural", wxDefaultPosition, wxSize(120, 60));
    m_outdoorButton->SetBackgroundColour(wxColour(255, 255, 224));
    m_outdoorButton->SetToolTip("Natural outdoor lighting with sunlight");
    gridSizer->Add(m_outdoorButton, 0, wxEXPAND);
    
    // Dramatic Lighting
    m_dramaticButton = new wxButton(parent, wxID_ANY, "Dramatic\nShadows", wxDefaultPosition, wxSize(120, 60));
    m_dramaticButton->SetBackgroundColour(wxColour(255, 228, 225));
    m_dramaticButton->SetToolTip("Dramatic lighting with strong contrasts");
    gridSizer->Add(m_dramaticButton, 0, wxEXPAND);
    
    // Warm Lighting
    m_warmButton = new wxButton(parent, wxID_ANY, "Warm\nTones", wxDefaultPosition, wxSize(120, 60));
    m_warmButton->SetBackgroundColour(wxColour(255, 240, 245));
    m_warmButton->SetToolTip("Warm, cozy lighting atmosphere");
    gridSizer->Add(m_warmButton, 0, wxEXPAND);
    
    // Cool Lighting
    m_coolButton = new wxButton(parent, wxID_ANY, "Cool\nBlues", wxDefaultPosition, wxSize(120, 60));
    m_coolButton->SetBackgroundColour(wxColour(240, 255, 255));
    m_coolButton->SetToolTip("Cool, blue-tinted lighting");
    gridSizer->Add(m_coolButton, 0, wxEXPAND);
    
    // Minimal Lighting
    m_minimalButton = new wxButton(parent, wxID_ANY, "Minimal\nClean", wxDefaultPosition, wxSize(120, 60));
    m_minimalButton->SetBackgroundColour(wxColour(245, 245, 245));
    m_minimalButton->SetToolTip("Minimal, clean lighting setup");
    gridSizer->Add(m_minimalButton, 0, wxEXPAND);
    
    // FreeCAD Lighting
    m_freeCADButton = new wxButton(parent, wxID_ANY, "FreeCAD\nClassic", wxDefaultPosition, wxSize(120, 60));
    m_freeCADButton->SetBackgroundColour(wxColour(255, 248, 220));
    m_freeCADButton->SetToolTip("Classic FreeCAD lighting style");
    gridSizer->Add(m_freeCADButton, 0, wxEXPAND);
    
    // Navcube Lighting
    m_navcubeButton = new wxButton(parent, wxID_ANY, "Navcube\nStyle", wxDefaultPosition, wxSize(120, 60));
    m_navcubeButton->SetBackgroundColour(wxColour(240, 248, 255));
    m_navcubeButton->SetToolTip("NavigationCube-style lighting");
    gridSizer->Add(m_navcubeButton, 0, wxEXPAND);
    
    presetsBoxSizer->Add(gridSizer, 0, wxALL, 15);
    presetsSizer->Add(presetsBoxSizer, 1, wxEXPAND | wxALL, 10);
    
    // Current preset status with better styling
    auto* statusBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Current Status");
    
    auto* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    m_currentPresetLabel = new wxStaticText(parent, wxID_ANY, "No preset applied");
    m_currentPresetLabel->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    statusSizer->Add(m_currentPresetLabel, 1, wxALIGN_CENTER_VERTICAL);
    
    statusBoxSizer->Add(statusSizer, 0, wxALL, 10);
    presetsSizer->Add(statusBoxSizer, 0, wxEXPAND | wxALL, 10);
    
    return presetsSizer;
}

void GlobalSettingsPanel::updateLightList()
{
    // Clear existing light list items
    m_lightListSizer->Clear(true);
    
    // Create list items for each light
    for (size_t i = 0; i < m_lights.size(); ++i) {
        auto* lightItemSizer = new wxBoxSizer(wxHORIZONTAL);
        
        // Light name button (for selection)
        auto* nameButton = new wxButton(m_lightListSizer->GetContainingWindow(), wxID_ANY, m_lights[i].name, 
            wxDefaultPosition, wxSize(-1, 25));
        nameButton->SetToolTip("Click to select this light");
        
        // Color preview button
        auto* colorButton = new wxButton(m_lightListSizer->GetContainingWindow(), wxID_ANY, "", 
            wxDefaultPosition, wxSize(30, 25));
        colorButton->SetBackgroundColour(m_lights[i].color);
        colorButton->SetToolTip("Click to change light color");
        
        // Bind events
        nameButton->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent& event) {
            // Select this light
            m_currentLightIndex = static_cast<int>(i);
            onLightSelected(event);
        });
        
        colorButton->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent& event) {
            // Open color dialog for this light
            wxColourDialog dialog(this);
            if (dialog.ShowModal() == wxID_OK) {
                wxColour color = dialog.GetColourData().GetColour();
                m_lights[i].color = color;
                
                // Update the color button
                wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
                if (button) {
                    button->SetBackgroundColour(color);
                }
                
                // Update the main color button if this light is selected
                if (m_currentLightIndex == static_cast<int>(i)) {
                    m_lightColorButton->SetBackgroundColour(color);
                    m_lightColorButton->SetLabel(wxString::Format("RGB(%d,%d,%d)", color.Red(), color.Green(), color.Blue()));
                }
                
                onLightingChanged(event);
            }
        });
        
        lightItemSizer->Add(nameButton, 1, wxEXPAND | wxRIGHT, 5);
        lightItemSizer->Add(colorButton, 0, wxEXPAND);
        
        m_lightListSizer->Add(lightItemSizer, 0, wxEXPAND | wxALL, 2);
    }
    
    // Layout the sizer
    m_lightListSizer->Layout();
    m_lightListSizer->GetContainingWindow()->Layout();
}

void GlobalSettingsPanel::bindEvents()
{
    // Lighting events (note: light selection is now handled in updateLightList)
    m_addLightButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onAddLight, this);
    m_removeLightButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onRemoveLight, this);
    m_lightNameText->Bind(wxEVT_TEXT, &GlobalSettingsPanel::onLightPropertyChanged, this);
    m_lightTypeChoice->Bind(wxEVT_CHOICE, &GlobalSettingsPanel::onLightPropertyChanged, this);
    m_positionXSpin->Bind(wxEVT_SPINCTRLDOUBLE, &GlobalSettingsPanel::onLightPropertyChangedSpin, this);
    m_positionYSpin->Bind(wxEVT_SPINCTRLDOUBLE, &GlobalSettingsPanel::onLightPropertyChangedSpin, this);
    m_positionZSpin->Bind(wxEVT_SPINCTRLDOUBLE, &GlobalSettingsPanel::onLightPropertyChangedSpin, this);
    m_directionXSpin->Bind(wxEVT_SPINCTRLDOUBLE, &GlobalSettingsPanel::onLightPropertyChangedSpin, this);
    m_directionYSpin->Bind(wxEVT_SPINCTRLDOUBLE, &GlobalSettingsPanel::onLightPropertyChangedSpin, this);
    m_directionZSpin->Bind(wxEVT_SPINCTRLDOUBLE, &GlobalSettingsPanel::onLightPropertyChangedSpin, this);
    m_intensitySpin->Bind(wxEVT_SPINCTRLDOUBLE, &GlobalSettingsPanel::onLightPropertyChangedSpin, this);
    m_lightColorButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        wxColourDialog dialog(this);
        if (dialog.ShowModal() == wxID_OK) {
            wxColour color = dialog.GetColourData().GetColour();
            m_lightColorButton->SetBackgroundColour(color);
            m_lightColorButton->SetLabel(wxString::Format("RGB(%d,%d,%d)", color.Red(), color.Green(), color.Blue()));
            onLightPropertyChanged(event);
        }
    });
    m_lightEnabledCheckBox->Bind(wxEVT_CHECKBOX, &GlobalSettingsPanel::onLightPropertyChanged, this);
    
    // Anti-aliasing events
    m_antiAliasingChoice->Bind(wxEVT_CHOICE, &GlobalSettingsPanel::onAntiAliasingChanged, this);
    m_msaaSamplesSlider->Bind(wxEVT_SLIDER, &GlobalSettingsPanel::onAntiAliasingChanged, this);
    m_fxaaCheckBox->Bind(wxEVT_CHECKBOX, &GlobalSettingsPanel::onAntiAliasingChanged, this);
    
    // Rendering mode events
    m_renderingModeChoice->Bind(wxEVT_CHOICE, &GlobalSettingsPanel::onRenderingModeChanged, this);
    
    // Light preset events
    m_studioButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onStudioPreset, this);
    m_outdoorButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onOutdoorPreset, this);
    m_dramaticButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onDramaticPreset, this);
    m_warmButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onWarmPreset, this);
    m_coolButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onCoolPreset, this);
    m_minimalButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onMinimalPreset, this);
    m_freeCADButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onFreeCADPreset, this);
    m_navcubeButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::onNavcubePreset, this);
}

// Lighting methods
std::vector<RenderLightSettings> GlobalSettingsPanel::getLights() const
{
    return m_lights;
}

void GlobalSettingsPanel::setLights(const std::vector<RenderLightSettings>& lights)
{
    m_lights = lights;
    updateLightList();
}

void GlobalSettingsPanel::addLight(const RenderLightSettings& light)
{
    m_lights.push_back(light);
    updateLightList();
    m_currentLightIndex = static_cast<int>(m_lights.size()) - 1;
    onLightSelected(wxCommandEvent());
}

void GlobalSettingsPanel::removeLight(int index)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights.erase(m_lights.begin() + index);
        updateLightList();
        if (m_lights.empty()) {
            m_currentLightIndex = -1;
        } else {
            m_currentLightIndex = std::min(m_currentLightIndex, static_cast<int>(m_lights.size()) - 1);
            onLightSelected(wxCommandEvent());
        }
    }
}

void GlobalSettingsPanel::updateLight(int index, const RenderLightSettings& light)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index] = light;
        updateLightList();
    }
}

void GlobalSettingsPanel::onLightSelected(wxCommandEvent& event)
{
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        const auto& light = m_lights[m_currentLightIndex];
        
        m_lightNameText->SetValue(light.name);
        m_lightTypeChoice->SetSelection(m_lightTypeChoice->FindString(light.type));
        m_positionXSpin->SetValue(light.positionX);
        m_positionYSpin->SetValue(light.positionY);
        m_positionZSpin->SetValue(light.positionZ);
        m_directionXSpin->SetValue(light.directionX);
        m_directionYSpin->SetValue(light.directionY);
        m_directionZSpin->SetValue(light.directionZ);
        m_intensitySpin->SetValue(light.intensity);
        m_lightColorButton->SetBackgroundColour(light.color);
        m_lightColorButton->SetLabel(wxString::Format("RGB(%d,%d,%d)", light.color.Red(), light.color.Green(), light.color.Blue()));
        m_lightEnabledCheckBox->SetValue(light.enabled);
        
        // Update control states based on current light name
        updateControlStates();
    }
}

void GlobalSettingsPanel::onAddLight(wxCommandEvent& event)
{
    RenderLightSettings newLight;
    newLight.name = "New Light " + std::to_string(m_lights.size() + 1);
    newLight.type = "directional";
    newLight.positionX = 0.0;
    newLight.positionY = 0.0;
    newLight.positionZ = 10.0;
    newLight.directionX = 0.0;
    newLight.directionY = 0.0;
    newLight.directionZ = -1.0;
    newLight.color = wxColour(255, 255, 255);
    newLight.intensity = 1.0;
    newLight.enabled = true;
    
    m_lights.push_back(newLight);
    m_currentLightIndex = static_cast<int>(m_lights.size()) - 1;
    updateLightList();
    onLightSelected(wxCommandEvent());
}

void GlobalSettingsPanel::onRemoveLight(wxCommandEvent& event)
{
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        m_lights.erase(m_lights.begin() + m_currentLightIndex);
        
        if (m_lights.empty()) {
            m_currentLightIndex = -1;
        } else {
            m_currentLightIndex = std::min(m_currentLightIndex, static_cast<int>(m_lights.size()) - 1);
        }
        
        updateLightList();
        
        if (m_currentLightIndex >= 0) {
            onLightSelected(wxCommandEvent());
        }
    }
}

void GlobalSettingsPanel::onLightPropertyChanged(wxCommandEvent& event)
{
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        auto& light = m_lights[m_currentLightIndex];
        light.name = m_lightNameText->GetValue().ToStdString();
        
        // Check if selection is valid before getting string
        int selection = m_lightTypeChoice->GetSelection();
        if (selection != wxNOT_FOUND && selection >= 0 && selection < static_cast<int>(m_lightTypeChoice->GetCount())) {
            light.type = m_lightTypeChoice->GetString(selection).ToStdString();
        }
        
        light.enabled = m_lightEnabledCheckBox->GetValue();
        
        // Update the light list to reflect name changes
        updateLightList();
        
        // Update control states based on light name
        updateControlStates();
        
        // Always apply lighting changes immediately for better user experience
        if (m_parentDialog) {
            m_parentDialog->applyGlobalSettingsToCanvas();
        }
        
        onLightingChanged(event);
    }
}

void GlobalSettingsPanel::onLightPropertyChangedSpin(wxSpinDoubleEvent& event)
{
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        auto& light = m_lights[m_currentLightIndex];
        
        // Get the control that triggered the event
        wxSpinCtrlDouble* sourceControl = dynamic_cast<wxSpinCtrlDouble*>(event.GetEventObject());
        bool shouldApplyChanges = true;
        
        // Update position values
        light.positionX = m_positionXSpin->GetValue();
        light.positionY = m_positionYSpin->GetValue();
        light.positionZ = m_positionZSpin->GetValue();
        
        // Update direction values
        light.directionX = m_directionXSpin->GetValue();
        light.directionY = m_directionYSpin->GetValue();
        light.directionZ = m_directionZSpin->GetValue();
        
        // Update intensity value
        light.intensity = m_intensitySpin->GetValue();
        
        // Check if position changes should affect rendering
        if (sourceControl == m_positionXSpin || sourceControl == m_positionYSpin || sourceControl == m_positionZSpin) {
            if (light.type == "directional") {
                // For directional lights, position changes don't affect rendering
                shouldApplyChanges = false;
                LOG_INF_S("GlobalSettingsPanel::onLightPropertyChangedSpin: Position change ignored for directional light");
            }
        }
        
        // Update control states to ensure UI reflects current state
        updateControlStates();
        
        // Apply lighting changes only if they should affect rendering
        if (shouldApplyChanges && m_parentDialog) {
            m_parentDialog->applyGlobalSettingsToCanvas();
            LOG_INF_S("GlobalSettingsPanel::onLightPropertyChangedSpin: Applied changes to canvas");
        }
        
        onLightingChanged(wxCommandEvent());
    }
}

void GlobalSettingsPanel::onLightingChanged(wxCommandEvent& event)
{
    // Auto-apply if enabled
    if (m_autoApply && m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

// Anti-aliasing methods
int GlobalSettingsPanel::getAntiAliasingMethod() const
{
    return m_antiAliasingChoice ? m_antiAliasingChoice->GetSelection() : 1;
}

int GlobalSettingsPanel::getMSAASamples() const
{
    return m_msaaSamplesSlider ? m_msaaSamplesSlider->GetValue() : 4;
}

bool GlobalSettingsPanel::isFXAAEnabled() const
{
    return m_fxaaCheckBox ? m_fxaaCheckBox->GetValue() : false;
}

// Rendering mode methods
int GlobalSettingsPanel::getRenderingMode() const
{
    return m_renderingModeChoice ? m_renderingModeChoice->GetSelection() : 4;
}

void GlobalSettingsPanel::onAntiAliasingChanged(wxCommandEvent& event)
{
    if (m_antiAliasingManager && m_antiAliasingChoice) {
        int selection = m_antiAliasingChoice->GetSelection();
        wxString presetName = m_antiAliasingChoice->GetString(selection);
        
        // Apply the selected preset
        m_antiAliasingManager->applyPreset(presetName.ToStdString());
        
        LOG_INF_S("GlobalSettingsPanel::onAntiAliasingChanged: Applied preset '" + presetName.ToStdString() + "'");
    }
    
    // Auto-apply if enabled
    if (m_autoApply && m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void GlobalSettingsPanel::onRenderingModeChanged(wxCommandEvent& event)
{
    if (m_renderingManager && m_renderingModeChoice) {
        int selection = m_renderingModeChoice->GetSelection();
        wxString presetName = m_renderingModeChoice->GetString(selection);
        
        // Apply the selected preset
        m_renderingManager->applyPreset(presetName.ToStdString());
        
        LOG_INF_S("GlobalSettingsPanel::onRenderingModeChanged: Applied preset '" + presetName.ToStdString() + "'");
    }
    
    // Auto-apply if enabled
    if (m_autoApply && m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

// Setter methods for anti-aliasing
void GlobalSettingsPanel::setAntiAliasingMethod(int method)
{
    if (m_antiAliasingChoice && method >= 0 && method < m_antiAliasingChoice->GetCount()) {
        m_antiAliasingChoice->SetSelection(method);
    }
}

void GlobalSettingsPanel::setMSAASamples(int samples)
{
    if (m_msaaSamplesSlider) {
        m_msaaSamplesSlider->SetValue(samples);
    }
}

void GlobalSettingsPanel::setFXAAEnabled(bool enabled)
{
    if (m_fxaaCheckBox) {
        m_fxaaCheckBox->SetValue(enabled);
    }
}

// Setter method for rendering mode
void GlobalSettingsPanel::setRenderingMode(int mode)
{
    if (m_renderingModeChoice && mode >= 0 && mode < m_renderingModeChoice->GetCount()) {
        m_renderingModeChoice->SetSelection(mode);
    }
}

void GlobalSettingsPanel::setAutoApply(bool enabled)
{
    m_autoApply = enabled;
}

void GlobalSettingsPanel::setAntiAliasingManager(AntiAliasingManager* manager)
{
    m_antiAliasingManager = manager;
}

void GlobalSettingsPanel::setRenderingManager(RenderingManager* manager)
{
    m_renderingManager = manager;
}

void GlobalSettingsPanel::loadSettings()
{
    try {
        // Load from render_settings.ini file
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString exeDir = exeFile.GetPath();
        wxString renderConfigPath = exeDir + wxFileName::GetPathSeparator() + "render_settings.ini";
        
        // Create wxFileConfig for render settings
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, renderConfigPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
        
        // Load anti-aliasing settings
        if (m_antiAliasingChoice) {
            int method;
            if (renderConfig.Read("Global.AntiAliasing.Method", &method, 2)) { // Default to MSAA 4x (index 2)
                // Validate method value against current choice count
                if (method >= 0 && method < m_antiAliasingChoice->GetCount()) {
                    m_antiAliasingChoice->SetSelection(method);
                } else {
                    // If invalid, default to MSAA 4x
                    m_antiAliasingChoice->SetSelection(2);
                    LOG_WRN_S("GlobalSettingsPanel::loadSettings: Invalid anti-aliasing method " + std::to_string(method) + ", defaulting to MSAA 4x");
                }
            }
        }
        if (m_msaaSamplesSlider) {
            int samples;
            if (renderConfig.Read("Global.AntiAliasing.MSAASamples", &samples, 4)) {
                m_msaaSamplesSlider->SetValue(samples);
            }
        }
        if (m_fxaaCheckBox) {
            bool enabled;
            if (renderConfig.Read("Global.AntiAliasing.FXAAEnabled", &enabled, false)) {
                m_fxaaCheckBox->SetValue(enabled);
            }
        }
        if (m_renderingModeChoice) {
            int mode;
            if (renderConfig.Read("Global.RenderingMode", &mode, 1)) { // Default to Balanced (index 1)
                // Validate mode value against current choice count
                if (mode >= 0 && mode < m_renderingModeChoice->GetCount()) {
                    m_renderingModeChoice->SetSelection(mode);
                } else {
                    // If invalid, default to Balanced
                    m_renderingModeChoice->SetSelection(1);
                    LOG_WRN_S("GlobalSettingsPanel::loadSettings: Invalid rendering mode " + std::to_string(mode) + ", defaulting to Balanced");
                }
            }
        }
        
        // Load lighting settings
        m_lights.clear();
        int lightCount = 0;
        if (renderConfig.Read("Global.Lighting.Count", &lightCount, 0) && lightCount > 0) {
            for (int i = 0; i < lightCount; ++i) {
                RenderLightSettings light;
                std::string lightPrefix = "Global.Lighting.Light" + std::to_string(i);
                
                renderConfig.Read(lightPrefix + ".Enabled", &light.enabled, true);
                wxString lightName;
                renderConfig.Read(lightPrefix + ".Name", &lightName, "Light " + std::to_string(i + 1));
                light.name = lightName.ToStdString();
                
                wxString lightType;
                renderConfig.Read(lightPrefix + ".Type", &lightType, "directional");
                light.type = lightType.ToStdString();
                renderConfig.Read(lightPrefix + ".PositionX", &light.positionX, 0.0);
                renderConfig.Read(lightPrefix + ".PositionY", &light.positionY, 0.0);
                renderConfig.Read(lightPrefix + ".PositionZ", &light.positionZ, 10.0);
                renderConfig.Read(lightPrefix + ".DirectionX", &light.directionX, 0.0);
                renderConfig.Read(lightPrefix + ".DirectionY", &light.directionY, 0.0);
                renderConfig.Read(lightPrefix + ".DirectionZ", &light.directionZ, -1.0);
                
                int colorR, colorG, colorB;
                renderConfig.Read(lightPrefix + ".ColorR", &colorR, 255);
                renderConfig.Read(lightPrefix + ".ColorG", &colorG, 255);
                renderConfig.Read(lightPrefix + ".ColorB", &colorB, 255);
                light.color = wxColour(colorR, colorG, colorB);
                
                renderConfig.Read(lightPrefix + ".Intensity", &light.intensity, 1.0);
                renderConfig.Read(lightPrefix + ".SpotAngle", &light.spotAngle, 30.0);
                renderConfig.Read(lightPrefix + ".SpotExponent", &light.spotExponent, 1.0);
                
                m_lights.push_back(light);
            }
        } else {
            // Initialize default light if no lights are saved
            RenderLightSettings defaultLight;
            defaultLight.name = "Default Light";
            defaultLight.type = "directional";
            defaultLight.positionX = 5.0;
            defaultLight.positionY = 5.0;
            defaultLight.positionZ = 10.0;
            defaultLight.directionX = -0.5;
            defaultLight.directionY = -0.5;
            defaultLight.directionZ = -1.0;
            defaultLight.color = wxColour(255, 255, 255);
            defaultLight.intensity = 1.0;
            defaultLight.enabled = true;
            m_lights.push_back(defaultLight);
        }
        
        updateLightList();
        
        // Update control states after loading settings
        updateControlStates();
        
        LOG_INF_S("GlobalSettingsPanel::loadSettings: Settings loaded from render_settings.ini successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("GlobalSettingsPanel::loadSettings: Failed to load settings: " + std::string(e.what()));
    }
}

void GlobalSettingsPanel::saveSettings()
{
    try {
        // Create a separate config file for render settings
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString exeDir = exeFile.GetPath();
        wxString renderConfigPath = exeDir + wxFileName::GetPathSeparator() + "render_settings.ini";
        
        // Create wxFileConfig for render settings
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, renderConfigPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
        
        // Save anti-aliasing settings
        if (m_antiAliasingChoice) {
            renderConfig.Write("Global.AntiAliasing.Method", m_antiAliasingChoice->GetSelection());
        }
        if (m_msaaSamplesSlider) {
            renderConfig.Write("Global.AntiAliasing.MSAASamples", m_msaaSamplesSlider->GetValue());
        }
        if (m_fxaaCheckBox) {
            renderConfig.Write("Global.AntiAliasing.FXAAEnabled", m_fxaaCheckBox->GetValue());
        }
        if (m_renderingModeChoice) {
            renderConfig.Write("Global.RenderingMode", m_renderingModeChoice->GetSelection());
        }
        
        // Save lighting settings
        renderConfig.Write("Global.Lighting.Count", static_cast<int>(m_lights.size()));
        
        for (size_t i = 0; i < m_lights.size(); ++i) {
            const auto& light = m_lights[i];
            std::string lightPrefix = "Global.Lighting.Light" + std::to_string(i);
            
            renderConfig.Write(lightPrefix + ".Enabled", light.enabled);
            renderConfig.Write(lightPrefix + ".Name", wxString(light.name));
            renderConfig.Write(lightPrefix + ".Type", wxString(light.type));
            renderConfig.Write(lightPrefix + ".PositionX", light.positionX);
            renderConfig.Write(lightPrefix + ".PositionY", light.positionY);
            renderConfig.Write(lightPrefix + ".PositionZ", light.positionZ);
            renderConfig.Write(lightPrefix + ".DirectionX", light.directionX);
            renderConfig.Write(lightPrefix + ".DirectionY", light.directionY);
            renderConfig.Write(lightPrefix + ".DirectionZ", light.directionZ);
            renderConfig.Write(lightPrefix + ".ColorR", light.color.Red());
            renderConfig.Write(lightPrefix + ".ColorG", light.color.Green());
            renderConfig.Write(lightPrefix + ".ColorB", light.color.Blue());
            renderConfig.Write(lightPrefix + ".Intensity", light.intensity);
            renderConfig.Write(lightPrefix + ".SpotAngle", light.spotAngle);
            renderConfig.Write(lightPrefix + ".SpotExponent", light.spotExponent);
        }
        
        renderConfig.Flush();
        LOG_INF_S("GlobalSettingsPanel::saveSettings: Settings saved to render_settings.ini successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("GlobalSettingsPanel::saveSettings: Failed to save settings: " + std::string(e.what()));
    }
}

void GlobalSettingsPanel::resetToDefaults()
{
    // Reset anti-aliasing settings
    if (m_antiAliasingChoice) {
        m_antiAliasingChoice->SetSelection(1);
    }
    if (m_msaaSamplesSlider) {
        m_msaaSamplesSlider->SetValue(4);
    }
    if (m_fxaaCheckBox) {
        m_fxaaCheckBox->SetValue(false);
    }
    if (m_renderingModeChoice) {
        m_renderingModeChoice->SetSelection(4);
    }
    
    // Reset lighting to default
    m_lights.clear();
    RenderLightSettings defaultLight;
    defaultLight.name = "Default Light";
    defaultLight.type = "directional";
    defaultLight.positionX = 5.0;
    defaultLight.positionY = 5.0;
    defaultLight.positionZ = 10.0;
    defaultLight.directionX = -0.5;
    defaultLight.directionY = -0.5;
    defaultLight.directionZ = -1.0;
    defaultLight.color = wxColour(255, 255, 255);
    defaultLight.intensity = 1.0;
    defaultLight.enabled = true;
    m_lights.push_back(defaultLight);
    updateLightList();
    
    // Update control states after reset
    updateControlStates();
    
    LOG_INF_S("GlobalSettingsPanel::resetToDefaults: Settings reset to defaults");
}

// Light preset event handlers
void GlobalSettingsPanel::onStudioPreset(wxCommandEvent& event)
{
    // Clear existing lights
    m_lights.clear();
    
    // Add studio lighting setup
    RenderLightSettings keyLight;
    keyLight.name = "Key Light";
    keyLight.type = "directional";
    keyLight.positionX = 5.0;
    keyLight.positionY = 5.0;
    keyLight.positionZ = 10.0;
    keyLight.directionX = -0.5;
    keyLight.directionY = -0.5;
    keyLight.directionZ = -1.0;
    keyLight.color = wxColour(255, 255, 255);
    keyLight.intensity = 1.0;
    keyLight.enabled = true;
    m_lights.push_back(keyLight);
    
    RenderLightSettings fillLight;
    fillLight.name = "Fill Light";
    fillLight.type = "directional";
    fillLight.positionX = -3.0;
    fillLight.positionY = 2.0;
    fillLight.positionZ = 8.0;
    fillLight.directionX = 0.3;
    fillLight.directionY = -0.2;
    fillLight.directionZ = -1.0;
    fillLight.color = wxColour(240, 240, 255);
    fillLight.intensity = 0.4;
    fillLight.enabled = true;
    m_lights.push_back(fillLight);
    
    RenderLightSettings rimLight;
    rimLight.name = "Rim Light";
    rimLight.type = "directional";
    rimLight.positionX = 0.0;
    rimLight.positionY = -4.0;
    rimLight.positionZ = 6.0;
    rimLight.directionX = 0.0;
    rimLight.directionY = 0.4;
    rimLight.directionZ = -1.0;
    rimLight.color = wxColour(255, 255, 240);
    rimLight.intensity = 0.6;
    rimLight.enabled = true;
    m_lights.push_back(rimLight);
    
    updateLightList();
    m_currentPresetLabel->SetLabel("Studio Lighting");
    onLightingChanged(event);
}

void GlobalSettingsPanel::onOutdoorPreset(wxCommandEvent& event)
{
    // Clear existing lights
    m_lights.clear();
    
    // Add outdoor lighting setup
    RenderLightSettings sunLight;
    sunLight.name = "Sun Light";
    sunLight.type = "directional";
    sunLight.positionX = 10.0;
    sunLight.positionY = 10.0;
    sunLight.positionZ = 15.0;
    sunLight.directionX = -0.7;
    sunLight.directionY = -0.7;
    sunLight.directionZ = -1.0;
    sunLight.color = wxColour(255, 255, 240);
    sunLight.intensity = 1.2;
    sunLight.enabled = true;
    m_lights.push_back(sunLight);
    
    RenderLightSettings skyLight;
    skyLight.name = "Sky Light";
    skyLight.type = "directional";
    skyLight.positionX = 0.0;
    skyLight.positionY = 0.0;
    skyLight.positionZ = 20.0;
    skyLight.directionX = 0.0;
    skyLight.directionY = 0.0;
    skyLight.directionZ = -1.0;
    skyLight.color = wxColour(200, 220, 255);
    skyLight.intensity = 0.8;
    skyLight.enabled = true;
    m_lights.push_back(skyLight);
    
    updateLightList();
    m_currentPresetLabel->SetLabel("Outdoor Lighting");
    onLightingChanged(event);
}

void GlobalSettingsPanel::onDramaticPreset(wxCommandEvent& event)
{
    // Clear existing lights
    m_lights.clear();
    
    // Add dramatic lighting setup
    RenderLightSettings mainLight;
    mainLight.name = "Main Light";
    mainLight.type = "directional";
    mainLight.positionX = 8.0;
    mainLight.positionY = 8.0;
    mainLight.positionZ = 12.0;
    mainLight.directionX = -0.8;
    mainLight.directionY = -0.8;
    mainLight.directionZ = -1.0;
    mainLight.color = wxColour(255, 255, 255);
    mainLight.intensity = 1.5;
    mainLight.enabled = true;
    m_lights.push_back(mainLight);
    
    RenderLightSettings accentLight;
    accentLight.name = "Accent Light";
    accentLight.type = "directional";
    accentLight.positionX = -2.0;
    accentLight.positionY = 1.0;
    accentLight.positionZ = 5.0;
    accentLight.directionX = 0.3;
    accentLight.directionY = -0.1;
    accentLight.directionZ = -1.0;
    accentLight.color = wxColour(255, 200, 200);
    accentLight.intensity = 0.3;
    accentLight.enabled = true;
    m_lights.push_back(accentLight);
    
    updateLightList();
    m_currentPresetLabel->SetLabel("Dramatic Lighting");
    onLightingChanged(event);
}

void GlobalSettingsPanel::onWarmPreset(wxCommandEvent& event)
{
    // Clear existing lights
    m_lights.clear();
    
    // Add warm lighting setup
    RenderLightSettings warmMain;
    warmMain.name = "Warm Main";
    warmMain.type = "directional";
    warmMain.positionX = 6.0;
    warmMain.positionY = 6.0;
    warmMain.positionZ = 10.0;
    warmMain.directionX = -0.6;
    warmMain.directionY = -0.6;
    warmMain.directionZ = -1.0;
    warmMain.color = wxColour(255, 240, 220);
    warmMain.intensity = 1.0;
    warmMain.enabled = true;
    m_lights.push_back(warmMain);
    
    RenderLightSettings warmFill;
    warmFill.name = "Warm Fill";
    warmFill.type = "directional";
    warmFill.positionX = -2.0;
    warmFill.positionY = 3.0;
    warmFill.positionZ = 8.0;
    warmFill.directionX = 0.2;
    warmFill.directionY = -0.3;
    warmFill.directionZ = -1.0;
    warmFill.color = wxColour(255, 220, 200);
    warmFill.intensity = 0.5;
    warmFill.enabled = true;
    m_lights.push_back(warmFill);
    
    updateLightList();
    m_currentPresetLabel->SetLabel("Warm Lighting");
    onLightingChanged(event);
}

void GlobalSettingsPanel::onCoolPreset(wxCommandEvent& event)
{
    // Clear existing lights
    m_lights.clear();
    
    // Add cool lighting setup
    RenderLightSettings coolMain;
    coolMain.name = "Cool Main";
    coolMain.type = "directional";
    coolMain.positionX = 5.0;
    coolMain.positionY = 5.0;
    coolMain.positionZ = 10.0;
    coolMain.directionX = -0.5;
    coolMain.directionY = -0.5;
    coolMain.directionZ = -1.0;
    coolMain.color = wxColour(220, 240, 255);
    coolMain.intensity = 1.0;
    coolMain.enabled = true;
    m_lights.push_back(coolMain);
    
    RenderLightSettings coolFill;
    coolFill.name = "Cool Fill";
    coolFill.type = "directional";
    coolFill.positionX = -3.0;
    coolFill.positionY = 2.0;
    coolFill.positionZ = 8.0;
    coolFill.directionX = 0.3;
    coolFill.directionY = -0.2;
    coolFill.directionZ = -1.0;
    coolFill.color = wxColour(200, 220, 255);
    coolFill.intensity = 0.4;
    coolFill.enabled = true;
    m_lights.push_back(coolFill);
    
    updateLightList();
    m_currentPresetLabel->SetLabel("Cool Lighting");
    onLightingChanged(event);
}

void GlobalSettingsPanel::onMinimalPreset(wxCommandEvent& event)
{
    // Clear existing lights
    m_lights.clear();
    
    // Add minimal lighting setup
    RenderLightSettings simpleLight;
    simpleLight.name = "Simple Light";
    simpleLight.type = "directional";
    simpleLight.positionX = 3.0;
    simpleLight.positionY = 3.0;
    simpleLight.positionZ = 8.0;
    simpleLight.directionX = -0.3;
    simpleLight.directionY = -0.3;
    simpleLight.directionZ = -1.0;
    simpleLight.color = wxColour(255, 255, 255);
    simpleLight.intensity = 0.8;
    simpleLight.enabled = true;
    m_lights.push_back(simpleLight);
    
    updateLightList();
    m_currentPresetLabel->SetLabel("Minimal Lighting");
    onLightingChanged(event);
}

void GlobalSettingsPanel::onFreeCADPreset(wxCommandEvent& event)
{
    // Clear existing lights
    m_lights.clear();
    
    // Add FreeCAD three-light model
    RenderLightSettings mainLight;
    mainLight.name = "Main Light";
    mainLight.type = "directional";
    mainLight.positionX = 5.0;
    mainLight.positionY = 5.0;
    mainLight.positionZ = 10.0;
    mainLight.directionX = -0.5;
    mainLight.directionY = -0.5;
    mainLight.directionZ = -1.0;
    mainLight.color = wxColour(255, 255, 255);
    mainLight.intensity = 1.0;
    mainLight.enabled = true;
    m_lights.push_back(mainLight);
    
    RenderLightSettings fillLight;
    fillLight.name = "Fill Light";
    fillLight.type = "directional";
    fillLight.positionX = -3.0;
    fillLight.positionY = 2.0;
    fillLight.positionZ = 8.0;
    fillLight.directionX = 0.3;
    fillLight.directionY = -0.2;
    fillLight.directionZ = -1.0;
    fillLight.color = wxColour(240, 240, 240);
    fillLight.intensity = 0.5;
    fillLight.enabled = true;
    m_lights.push_back(fillLight);
    
    RenderLightSettings backLight;
    backLight.name = "Back Light";
    backLight.type = "directional";
    backLight.positionX = 0.0;
    backLight.positionY = -5.0;
    backLight.positionZ = 5.0;
    backLight.directionX = 0.0;
    backLight.directionY = 0.5;
    backLight.directionZ = -1.0;
    backLight.color = wxColour(255, 255, 255);
    backLight.intensity = 0.3;
    backLight.enabled = true;
    m_lights.push_back(backLight);
    
    updateLightList();
    m_currentPresetLabel->SetLabel("FreeCAD Lighting");
    onLightingChanged(event);
}

void GlobalSettingsPanel::onNavcubePreset(wxCommandEvent& event)
{
    // Clear existing lights
    m_lights.clear();
    
    // Add NavigationCube-style lighting
    RenderLightSettings frontLight;
    frontLight.name = "Front Light";
    frontLight.type = "directional";
    frontLight.positionX = 0.0;
    frontLight.positionY = 0.0;
    frontLight.positionZ = 10.0;
    frontLight.directionX = 0.0;
    frontLight.directionY = 0.0;
    frontLight.directionZ = -1.0;
    frontLight.color = wxColour(255, 255, 255);
    frontLight.intensity = 0.8;
    frontLight.enabled = true;
    m_lights.push_back(frontLight);
    
    RenderLightSettings topLight;
    topLight.name = "Top Light";
    topLight.type = "directional";
    topLight.positionX = 0.0;
    topLight.positionY = 0.0;
    topLight.positionZ = 15.0;
    topLight.directionX = 0.0;
    topLight.directionY = 0.0;
    topLight.directionZ = -1.0;
    topLight.color = wxColour(240, 240, 240);
    topLight.intensity = 0.6;
    topLight.enabled = true;
    m_lights.push_back(topLight);
    
    RenderLightSettings sideLight;
    sideLight.name = "Side Light";
    sideLight.type = "directional";
    sideLight.positionX = 8.0;
    sideLight.positionY = 0.0;
    sideLight.positionZ = 8.0;
    sideLight.directionX = -0.8;
    sideLight.directionY = 0.0;
    sideLight.directionZ = -1.0;
    sideLight.color = wxColour(220, 220, 220);
    sideLight.intensity = 0.4;
    sideLight.enabled = true;
    m_lights.push_back(sideLight);
    
    updateLightList();
    m_currentPresetLabel->SetLabel("Navcube Lighting");
    onLightingChanged(event);
}

void GlobalSettingsPanel::updateControlStates()
{
    bool hasValidName = false;
    wxColour currentColor = wxColour(255, 255, 255);
    std::string lightType = "directional";
    
    // Check if we have a valid light selected
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        const auto& light = m_lights[m_currentLightIndex];
        hasValidName = !light.name.empty() && light.name.length() > 0;
        currentColor = light.color;
        lightType = light.type;
        
        LOG_INF_S("GlobalSettingsPanel::updateControlStates: Light '" + light.name + "' - Type: " + lightType + 
                  " - Valid name: " + std::string(hasValidName ? "true" : "false"));
    } else {
        LOG_INF_S("GlobalSettingsPanel::updateControlStates: No light selected");
    }
    
    // Enable/disable controls based on light name validity and type
    if (m_lightTypeChoice) {
        m_lightTypeChoice->Enable(hasValidName);
        LOG_INF_S("GlobalSettingsPanel::updateControlStates: Light type choice " + std::string(hasValidName ? "enabled" : "disabled"));
    }
    
    // Position controls: only enabled for point and spot lights
    bool positionEnabled = hasValidName && (lightType == "point" || lightType == "spot");
    if (m_positionXSpin) {
        m_positionXSpin->Enable(positionEnabled);
    }
    if (m_positionYSpin) {
        m_positionYSpin->Enable(positionEnabled);
    }
    if (m_positionZSpin) {
        m_positionZSpin->Enable(positionEnabled);
    }
    
    // Direction controls: enabled for all light types
    bool directionEnabled = hasValidName;
    if (m_directionXSpin) {
        m_directionXSpin->Enable(directionEnabled);
    }
    if (m_directionYSpin) {
        m_directionYSpin->Enable(directionEnabled);
    }
    if (m_directionZSpin) {
        m_directionZSpin->Enable(directionEnabled);
    }
    
    // Other controls: enabled if name is valid
    if (m_intensitySpin) {
        m_intensitySpin->Enable(hasValidName);
    }
    if (m_lightColorButton) {
        m_lightColorButton->Enable(hasValidName);
    }
    if (m_lightEnabledCheckBox) {
        m_lightEnabledCheckBox->Enable(hasValidName);
    }
    
    LOG_INF_S("GlobalSettingsPanel::updateControlStates: Position controls " + std::string(positionEnabled ? "enabled" : "disabled") + 
              " for light type: " + lightType);
    
    // Update color button appearance to indicate disabled state
    if (m_lightColorButton) {
        if (!hasValidName) {
            m_lightColorButton->SetBackgroundColour(wxColour(200, 200, 200)); // Gray out
            m_lightColorButton->SetLabel("RGB(200,200,200)");
            LOG_INF_S("GlobalSettingsPanel::updateControlStates: Color button grayed out");
        } else {
            // Restore original color
            m_lightColorButton->SetBackgroundColour(currentColor);
            m_lightColorButton->SetLabel(wxString::Format("RGB(%d,%d,%d)", 
                currentColor.Red(), currentColor.Green(), currentColor.Blue()));
            LOG_INF_S("GlobalSettingsPanel::updateControlStates: Color button restored to original color");
        }
    }
    
    // Force refresh of the UI
    if (m_lightTypeChoice) {
        m_lightTypeChoice->Refresh();
    }
    if (m_positionXSpin) {
        m_positionXSpin->Refresh();
    }
    if (m_positionYSpin) {
        m_positionYSpin->Refresh();
    }
    if (m_positionZSpin) {
        m_positionZSpin->Refresh();
    }
    if (m_directionXSpin) {
        m_directionXSpin->Refresh();
    }
    if (m_directionYSpin) {
        m_directionYSpin->Refresh();
    }
    if (m_directionZSpin) {
        m_directionZSpin->Refresh();
    }
    if (m_intensitySpin) {
        m_intensitySpin->Refresh();
    }
    if (m_lightColorButton) {
        m_lightColorButton->Refresh();
    }
    if (m_lightEnabledCheckBox) {
        m_lightEnabledCheckBox->Refresh();
    }
    
    LOG_INF_S("GlobalSettingsPanel::updateControlStates: All controls updated - Valid name: " + 
              std::string(hasValidName ? "true" : "false"));
}

// Global Settings button event handlers
void GlobalSettingsPanel::OnGlobalApply(wxCommandEvent& event)
{
    // Apply only global settings (lighting, anti-aliasing, rendering mode)
    if (m_parentDialog && m_parentDialog->getRenderCanvas()) {
        auto canvas = m_parentDialog->getRenderCanvas();
        
        // Apply lighting settings
        auto lights = getLights();
        if (!lights.empty()) {
            canvas->updateMultiLighting(lights);
        }
        
        // Apply anti-aliasing settings
        int antiAliasingMethod = getAntiAliasingMethod();
        int msaaSamples = getMSAASamples();
        bool fxaaEnabled = isFXAAEnabled();
        canvas->updateAntiAliasing(antiAliasingMethod, msaaSamples, fxaaEnabled);
        
        // Apply rendering mode
        int renderingMode = getRenderingMode();
        canvas->updateRenderingMode(renderingMode);
        
        // Force canvas refresh
        canvas->render(true);
        canvas->Refresh();
        canvas->Update();
        
        wxMessageBox(wxT("Global settings applied to preview successfully!"), wxT("Apply Global"), wxOK | wxICON_INFORMATION);
        LOG_INF_S("GlobalSettingsPanel::OnGlobalApply: Global settings applied");
    }
}

void GlobalSettingsPanel::OnGlobalSave(wxCommandEvent& event)
{
    saveSettings();
    wxMessageBox(wxT("Global settings saved successfully!"), wxT("Save Global"), wxOK | wxICON_INFORMATION);
    LOG_INF_S("GlobalSettingsPanel::OnGlobalSave: Global settings saved");
}

void GlobalSettingsPanel::OnGlobalReset(wxCommandEvent& event)
{
    resetToDefaults();
    wxMessageBox(wxT("Global settings reset to defaults!"), wxT("Reset Global"), wxOK | wxICON_INFORMATION);
    LOG_INF_S("GlobalSettingsPanel::OnGlobalReset: Global settings reset");
}

void GlobalSettingsPanel::OnGlobalUndo(wxCommandEvent& event)
{
    // TODO: Implement undo functionality for global settings
    wxMessageBox(wxT("Global Undo: Not implemented yet"), wxT("Undo Global"), wxOK | wxICON_INFORMATION);
    LOG_INF_S("GlobalSettingsPanel::OnGlobalUndo: Global undo requested");
}

void GlobalSettingsPanel::OnGlobalRedo(wxCommandEvent& event)
{
    // TODO: Implement redo functionality for global settings
    wxMessageBox(wxT("Global Redo: Not implemented yet"), wxT("Redo Global"), wxOK | wxICON_INFORMATION);
    LOG_INF_S("GlobalSettingsPanel::OnGlobalRedo: Global redo requested");
}

void GlobalSettingsPanel::OnGlobalAutoApply(wxCommandEvent& event)
{
    m_autoApply = event.IsChecked();
    LOG_INF_S("GlobalSettingsPanel::OnGlobalAutoApply: Global auto apply " + std::string(m_autoApply ? "enabled" : "disabled"));
} 