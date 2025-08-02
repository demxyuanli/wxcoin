#include "renderpreview/GlobalSettingsPanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/colordlg.h>

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
{
    LOG_INF_S("GlobalSettingsPanel::GlobalSettingsPanel: Initializing");
    createUI();
    bindEvents();
    loadSettings();
    LOG_INF_S("GlobalSettingsPanel::GlobalSettingsPanel: Initialized successfully");
}

GlobalSettingsPanel::~GlobalSettingsPanel()
{
    LOG_INF_S("GlobalSettingsPanel::~GlobalSettingsPanel: Destroying");
}

void GlobalSettingsPanel::createUI()
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    auto* titleLabel = new wxStaticText(this, wxID_ANY, "Global Settings", 
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    mainSizer->Add(titleLabel, 0, wxALIGN_CENTER | wxALL, 10);
    
    // Description
    auto* descLabel = new wxStaticText(this, wxID_ANY, 
        "These settings affect the entire scene and view.\n"
        "Changes apply to all objects in the scene.", 
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    mainSizer->Add(descLabel, 0, wxALIGN_CENTER | wxALL, 10);
    
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
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);
    
    SetSizer(mainSizer);
}

wxSizer* GlobalSettingsPanel::createLightingTab(wxWindow* parent)
{
    auto* lightingSizer = new wxBoxSizer(wxVERTICAL);
    
    // Light list section with better styling
    auto* listBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Light List");
    
    m_lightListBox = new wxListBox(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
    listBoxSizer->Add(m_lightListBox, 1, wxEXPAND | wxALL, 8);
    
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
    
    // Anti-aliasing Method
    auto* methodLabel = new wxStaticText(parent, wxID_ANY, "Anti-aliasing Method:");
    antiAliasingSizer->Add(methodLabel, 0, wxALL, 8);
    
    m_antiAliasingChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
    m_antiAliasingChoice->Append("None");
    m_antiAliasingChoice->Append("MSAA");
    m_antiAliasingChoice->Append("FXAA");
    m_antiAliasingChoice->Append("MSAA + FXAA");
    m_antiAliasingChoice->SetSelection(1);
    antiAliasingSizer->Add(m_antiAliasingChoice, 0, wxEXPAND | wxALL, 8);
    
    // MSAA Samples
    auto* msaaLabel = new wxStaticText(parent, wxID_ANY, "MSAA Samples:");
    antiAliasingSizer->Add(msaaLabel, 0, wxALL, 8);
    
    m_msaaSamplesSlider = new wxSlider(parent, wxID_ANY, 4, 2, 16, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    antiAliasingSizer->Add(m_msaaSamplesSlider, 0, wxEXPAND | wxALL, 8);
    
    // Enable FXAA
    m_fxaaCheckBox = new wxCheckBox(parent, wxID_ANY, "Enable FXAA");
    antiAliasingSizer->Add(m_fxaaCheckBox, 0, wxALL, 8);
    
    return antiAliasingSizer;
}

wxSizer* GlobalSettingsPanel::createRenderingModeTab(wxWindow* parent)
{
    auto* renderingSizer = new wxBoxSizer(wxVERTICAL);
    
    auto* modeLabel = new wxStaticText(parent, wxID_ANY, "Rendering Mode:");
    renderingSizer->Add(modeLabel, 0, wxALL, 8);
    
    m_renderingModeChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
    m_renderingModeChoice->Append("Solid");
    m_renderingModeChoice->Append("Wireframe");
    m_renderingModeChoice->Append("Points");
    m_renderingModeChoice->Append("Hidden Line");
    m_renderingModeChoice->Append("Shaded");
    m_renderingModeChoice->SetSelection(4);
    renderingSizer->Add(m_renderingModeChoice, 0, wxEXPAND | wxALL, 8);
    
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
    m_lightListBox->Clear();
    for (const auto& light : m_lights) {
        m_lightListBox->Append(light.name);
    }
}

void GlobalSettingsPanel::bindEvents()
{
    // Lighting events
    m_lightListBox->Bind(wxEVT_LISTBOX, &GlobalSettingsPanel::onLightSelected, this);
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
    m_lightListBox->SetSelection(m_lights.size() - 1);
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
            m_lightListBox->SetSelection(m_currentLightIndex);
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
    int selection = m_lightListBox->GetSelection();
    if (selection >= 0 && selection < static_cast<int>(m_lights.size())) {
        m_currentLightIndex = selection;
        const auto& light = m_lights[selection];
        
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
    
    addLight(newLight);
}

void GlobalSettingsPanel::onRemoveLight(wxCommandEvent& event)
{
    removeLight(m_currentLightIndex);
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
        
        updateLightList();
        onLightingChanged(event);
    }
}

void GlobalSettingsPanel::onLightPropertyChangedSpin(wxSpinDoubleEvent& event)
{
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        auto& light = m_lights[m_currentLightIndex];
        light.positionX = m_positionXSpin->GetValue();
        light.positionY = m_positionYSpin->GetValue();
        light.positionZ = m_positionZSpin->GetValue();
        light.directionX = m_directionXSpin->GetValue();
        light.directionY = m_directionYSpin->GetValue();
        light.directionZ = m_directionZSpin->GetValue();
        light.intensity = m_intensitySpin->GetValue();
        
        onLightingChanged(wxCommandEvent());
    }
}

void GlobalSettingsPanel::onLightingChanged(wxCommandEvent& event)
{
    // This will be handled by the parent dialog
    // The parent dialog can call getLights()
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
    // This will be handled by the parent dialog
    // The parent dialog can call getAntiAliasingMethod(), getMSAASamples(), etc.
}

void GlobalSettingsPanel::onRenderingModeChanged(wxCommandEvent& event)
{
    // This will be handled by the parent dialog
    // The parent dialog can call getRenderingMode()
}

void GlobalSettingsPanel::loadSettings()
{
    try {
        auto& configManager = ConfigManager::getInstance();
        
        // Load anti-aliasing settings
        if (m_antiAliasingChoice) {
            m_antiAliasingChoice->SetSelection(configManager.getInt("RenderPreview", "Global.AntiAliasing.Method", 1));
        }
        if (m_msaaSamplesSlider) {
            m_msaaSamplesSlider->SetValue(configManager.getInt("RenderPreview", "Global.AntiAliasing.MSAASamples", 4));
        }
        if (m_fxaaCheckBox) {
            m_fxaaCheckBox->SetValue(configManager.getBool("RenderPreview", "Global.AntiAliasing.FXAAEnabled", false));
        }
        if (m_renderingModeChoice) {
            m_renderingModeChoice->SetSelection(configManager.getInt("RenderPreview", "Global.RenderingMode", 4));
        }
        
        // Initialize default light if none exists
        if (m_lights.empty()) {
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
        }
        
        LOG_INF_S("GlobalSettingsPanel::loadSettings: Settings loaded successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("GlobalSettingsPanel::loadSettings: Failed to load settings: " + std::string(e.what()));
    }
}

void GlobalSettingsPanel::saveSettings()
{
    try {
        auto& configManager = ConfigManager::getInstance();
        
        // Save anti-aliasing settings
        if (m_antiAliasingChoice) {
            configManager.setInt("RenderPreview", "Global.AntiAliasing.Method", m_antiAliasingChoice->GetSelection());
        }
        if (m_msaaSamplesSlider) {
            configManager.setInt("RenderPreview", "Global.AntiAliasing.MSAASamples", m_msaaSamplesSlider->GetValue());
        }
        if (m_fxaaCheckBox) {
            configManager.setBool("RenderPreview", "Global.AntiAliasing.FXAAEnabled", m_fxaaCheckBox->GetValue());
        }
        if (m_renderingModeChoice) {
            configManager.setInt("RenderPreview", "Global.RenderingMode", m_renderingModeChoice->GetSelection());
        }
        
        configManager.save();
        LOG_INF_S("GlobalSettingsPanel::saveSettings: Settings saved successfully");
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