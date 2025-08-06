#include "renderpreview/LightingPanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "config/FontManager.h"
#include "logger/Logger.h"
#include <wx/scrolwin.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/config.h>

BEGIN_EVENT_TABLE(LightingPanel, wxPanel)
    EVT_BUTTON(wxID_ANY, LightingPanel::onAddLight)
    EVT_BUTTON(wxID_ANY, LightingPanel::onRemoveLight)
    EVT_TEXT(wxID_ANY, LightingPanel::onLightPropertyChanged)
    EVT_CHOICE(wxID_ANY, LightingPanel::onLightPropertyChanged)
    EVT_CHECKBOX(wxID_ANY, LightingPanel::onLightPropertyChanged)
END_EVENT_TABLE()

LightingPanel::LightingPanel(wxWindow* parent, RenderPreviewDialog* dialog)
    : wxPanel(parent, wxID_ANY), m_parentDialog(dialog), m_currentLightIndex(-1)
{
    createUI();
    bindEvents();
    loadSettings();
}

LightingPanel::~LightingPanel()
{
}

void LightingPanel::createUI()
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    m_notebook = new wxNotebook(this, wxID_ANY);

    auto* lightingPanel = new wxPanel(m_notebook, wxID_ANY);
    lightingPanel->SetSizer(createLightingTab(lightingPanel));

    auto* lightPresetsPanel = new wxPanel(m_notebook, wxID_ANY);
    lightPresetsPanel->SetSizer(createLightPresetsTab(lightPresetsPanel));

    m_notebook->AddPage(lightingPanel, "Lighting");
    m_notebook->AddPage(lightPresetsPanel, "Light Presets");

    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 2);
    SetSizer(mainSizer);
}

wxSizer* LightingPanel::createLightingTab(wxWindow* parent)
{
    auto* lightingSizer = new wxBoxSizer(wxVERTICAL);
    
    auto* listBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Light List");
    
    auto* lightListPanel = new wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
    lightListPanel->SetScrollRate(10, 10);
    m_lightListSizer = new wxBoxSizer(wxVERTICAL);
    lightListPanel->SetSizer(m_lightListSizer);
    
    listBoxSizer->Add(lightListPanel, 1, wxEXPAND | wxALL, 8);
    
    auto* listButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_addLightButton = new wxButton(parent, wxID_ANY, "Add Light");
    m_removeLightButton = new wxButton(parent, wxID_ANY, "Remove Light");
    listButtonSizer->Add(m_addLightButton, 1, wxEXPAND | wxRIGHT, 5);
    listButtonSizer->Add(m_removeLightButton, 1, wxEXPAND | wxLEFT, 5);
    listBoxSizer->Add(listButtonSizer, 0, wxEXPAND | wxALL, 8);
    
    lightingSizer->Add(listBoxSizer, 0, wxEXPAND | wxALL, 10);
    
    auto* propertiesBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Light Properties");
    
    auto* basicGridSizer = new wxFlexGridSizer(2, 2, 10, 15);
    basicGridSizer->AddGrowableCol(1, 1);
    
    basicGridSizer->Add(new wxStaticText(parent, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_lightNameText = new wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, -1));
    basicGridSizer->Add(m_lightNameText, 1, wxEXPAND);
    
    basicGridSizer->Add(new wxStaticText(parent, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL);
    m_lightTypeChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, -1));
    m_lightTypeChoice->Append("directional");
    m_lightTypeChoice->Append("point");
    m_lightTypeChoice->Append("spot");
    basicGridSizer->Add(m_lightTypeChoice, 1, wxEXPAND);
    
    propertiesBoxSizer->Add(basicGridSizer, 0, wxEXPAND | wxALL, 10);
    
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
    
    m_lightEnabledCheckBox = new wxCheckBox(parent, wxID_ANY, "Enabled");
    m_lightEnabledCheckBox->SetValue(true);
    propertiesBoxSizer->Add(m_lightEnabledCheckBox, 0, wxALL, 10);
    
    lightingSizer->Add(propertiesBoxSizer, 1, wxEXPAND | wxALL, 10);
    
    return lightingSizer;
}

wxSizer* LightingPanel::createLightPresetsTab(wxWindow* parent)
{
    auto* presetsSizer = new wxBoxSizer(wxVERTICAL);
    
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* headerText = new wxStaticText(parent, wxID_ANY, "Choose a lighting preset to apply to your scene");
    headerText->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    headerSizer->Add(headerText, 1, wxALIGN_CENTER_VERTICAL);
    presetsSizer->Add(headerSizer, 0, wxALL, 15);
    
    auto* presetsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Lighting Presets");
    
    auto* gridSizer = new wxGridSizer(4, 2, 10, 10);
    
    m_studioButton = new wxButton(parent, wxID_ANY, "Studio\nLighting", wxDefaultPosition, wxSize(120, 60));
    m_studioButton->SetBackgroundColour(wxColour(240, 248, 255));
    m_studioButton->SetToolTip("Professional studio lighting with soft shadows");
    gridSizer->Add(m_studioButton, 0, wxEXPAND);
    
    m_outdoorButton = new wxButton(parent, wxID_ANY, "Outdoor\nNatural", wxDefaultPosition, wxSize(120, 60));
    m_outdoorButton->SetBackgroundColour(wxColour(255, 255, 224));
    m_outdoorButton->SetToolTip("Natural outdoor lighting with sunlight");
    gridSizer->Add(m_outdoorButton, 0, wxEXPAND);
    
    m_dramaticButton = new wxButton(parent, wxID_ANY, "Dramatic\nShadows", wxDefaultPosition, wxSize(120, 60));
    m_dramaticButton->SetBackgroundColour(wxColour(255, 228, 225));
    m_dramaticButton->SetToolTip("Dramatic lighting with strong contrasts");
    gridSizer->Add(m_dramaticButton, 0, wxEXPAND);
    
    m_warmButton = new wxButton(parent, wxID_ANY, "Warm\nTones", wxDefaultPosition, wxSize(120, 60));
    m_warmButton->SetBackgroundColour(wxColour(255, 240, 245));
    m_warmButton->SetToolTip("Warm, cozy lighting atmosphere");
    gridSizer->Add(m_warmButton, 0, wxEXPAND);
    
    m_coolButton = new wxButton(parent, wxID_ANY, "Cool\nBlues", wxDefaultPosition, wxSize(120, 60));
    m_coolButton->SetBackgroundColour(wxColour(240, 255, 255));
    m_coolButton->SetToolTip("Cool, blue-tinted lighting");
    gridSizer->Add(m_coolButton, 0, wxEXPAND);
    
    m_minimalButton = new wxButton(parent, wxID_ANY, "Minimal\nClean", wxDefaultPosition, wxSize(120, 60));
    m_minimalButton->SetBackgroundColour(wxColour(245, 245, 245));
    m_minimalButton->SetToolTip("Minimal, clean lighting setup");
    gridSizer->Add(m_minimalButton, 0, wxEXPAND);
    
    m_freeCADButton = new wxButton(parent, wxID_ANY, "FreeCAD\nClassic", wxDefaultPosition, wxSize(120, 60));
    m_freeCADButton->SetBackgroundColour(wxColour(255, 248, 220));
    m_freeCADButton->SetToolTip("Classic FreeCAD lighting style");
    gridSizer->Add(m_freeCADButton, 0, wxEXPAND);
    
    m_navcubeButton = new wxButton(parent, wxID_ANY, "Navcube\nStyle", wxDefaultPosition, wxSize(120, 60));
    m_navcubeButton->SetBackgroundColour(wxColour(240, 248, 255));
    m_navcubeButton->SetToolTip("NavigationCube-style lighting");
    gridSizer->Add(m_navcubeButton, 0, wxEXPAND);
    
    presetsBoxSizer->Add(gridSizer, 0, wxALL, 15);
    presetsSizer->Add(presetsBoxSizer, 1, wxEXPAND | wxALL, 10);
    
    auto* statusBoxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Current Status");
    
    auto* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    m_currentPresetLabel = new wxStaticText(parent, wxID_ANY, "No preset applied");
    m_currentPresetLabel->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    statusSizer->Add(m_currentPresetLabel, 1, wxALIGN_CENTER_VERTICAL);
    
    statusBoxSizer->Add(statusSizer, 0, wxALL, 10);
    presetsSizer->Add(statusBoxSizer, 0, wxEXPAND | wxALL, 10);
    
    return presetsSizer;
}

void LightingPanel::bindEvents()
{
    m_addLightButton->Bind(wxEVT_BUTTON, &LightingPanel::onAddLight, this);
    m_removeLightButton->Bind(wxEVT_BUTTON, &LightingPanel::onRemoveLight, this);
    m_lightNameText->Bind(wxEVT_TEXT, &LightingPanel::onLightPropertyChanged, this);
    m_lightTypeChoice->Bind(wxEVT_CHOICE, &LightingPanel::onLightPropertyChanged, this);
    m_positionXSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingPanel::onLightPropertyChangedSpin, this);
    m_positionYSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingPanel::onLightPropertyChangedSpin, this);
    m_positionZSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingPanel::onLightPropertyChangedSpin, this);
    m_directionXSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingPanel::onLightPropertyChangedSpin, this);
    m_directionYSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingPanel::onLightPropertyChangedSpin, this);
    m_directionZSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingPanel::onLightPropertyChangedSpin, this);
    m_intensitySpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingPanel::onLightPropertyChangedSpin, this);
    m_lightColorButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        wxColourDialog dialog(this);
        if (dialog.ShowModal() == wxID_OK) {
            wxColour color = dialog.GetColourData().GetColour();
            m_lightColorButton->SetBackgroundColour(color);
            m_lightColorButton->SetLabel(wxString::Format("RGB(%d,%d,%d)", color.Red(), color.Green(), color.Blue()));
            onLightPropertyChanged(event);
        }
    });
    m_lightEnabledCheckBox->Bind(wxEVT_CHECKBOX, &LightingPanel::onLightPropertyChanged, this);

    m_studioButton->Bind(wxEVT_BUTTON, &LightingPanel::onStudioPreset, this);
    m_outdoorButton->Bind(wxEVT_BUTTON, &LightingPanel::onOutdoorPreset, this);
    m_dramaticButton->Bind(wxEVT_BUTTON, &LightingPanel::onDramaticPreset, this);
    m_warmButton->Bind(wxEVT_BUTTON, &LightingPanel::onWarmPreset, this);
    m_coolButton->Bind(wxEVT_BUTTON, &LightingPanel::onCoolPreset, this);
    m_minimalButton->Bind(wxEVT_BUTTON, &LightingPanel::onMinimalPreset, this);
    m_freeCADButton->Bind(wxEVT_BUTTON, &LightingPanel::onFreeCADPreset, this);
    m_navcubeButton->Bind(wxEVT_BUTTON, &LightingPanel::onNavcubePreset, this);
}

std::vector<RenderLightSettings> LightingPanel::getLights() const
{
    return m_lights;
}

void LightingPanel::setLights(const std::vector<RenderLightSettings>& lights)
{
    m_lights = lights;
    updateLightList();
}

void LightingPanel::updateLightList()
{
    m_lightListSizer->Clear(true);
    FontManager& fontManager = FontManager::getInstance();
    for (size_t i = 0; i < m_lights.size(); ++i) {
        auto* lightItemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* nameButton = new wxButton(m_lightListSizer->GetContainingWindow(), wxID_ANY, m_lights[i].name, wxDefaultPosition, wxSize(-1, 25));
        nameButton->SetToolTip("Click to select this light");
        nameButton->SetFont(fontManager.getButtonFont());
        auto* colorButton = new wxButton(m_lightListSizer->GetContainingWindow(), wxID_ANY, "", wxDefaultPosition, wxSize(30, 25));
        colorButton->SetBackgroundColour(m_lights[i].color);
        colorButton->SetToolTip("Click to change light color");
        colorButton->SetFont(fontManager.getButtonFont());
        nameButton->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent& event) {
            m_currentLightIndex = static_cast<int>(i);
            onLightSelected(event);
        });
        colorButton->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent& event) {
            wxColourDialog dialog(this);
            if (dialog.ShowModal() == wxID_OK) {
                wxColour color = dialog.GetColourData().GetColour();
                m_lights[i].color = color;
                wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
                if (button) {
                    button->SetBackgroundColour(color);
                }
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
    m_lightListSizer->Layout();
    m_lightListSizer->GetContainingWindow()->Layout();
}

void LightingPanel::onLightSelected(wxCommandEvent& event)
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
        updateControlStates();
    }
}

void LightingPanel::onAddLight(wxCommandEvent& event)
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

void LightingPanel::onRemoveLight(wxCommandEvent& event)
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

void LightingPanel::onLightPropertyChanged(wxCommandEvent& event)
{
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        auto& light = m_lights[m_currentLightIndex];
        light.name = m_lightNameText->GetValue().ToStdString();
        int selection = m_lightTypeChoice->GetSelection();
        if (selection != wxNOT_FOUND && selection >= 0 && selection < static_cast<int>(m_lightTypeChoice->GetCount())) {
            light.type = m_lightTypeChoice->GetString(selection).ToStdString();
        }
        light.enabled = m_lightEnabledCheckBox->GetValue();
        updateLightList();
        updateControlStates();
        if (m_parentDialog) {
            m_parentDialog->applyGlobalSettingsToCanvas();
        }
        onLightingChanged(event);
    }
}

void LightingPanel::onLightPropertyChangedSpin(wxSpinDoubleEvent& event)
{
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        auto& light = m_lights[m_currentLightIndex];
        wxSpinCtrlDouble* sourceControl = dynamic_cast<wxSpinCtrlDouble*>(event.GetEventObject());
        bool shouldApplyChanges = true;
        light.positionX = m_positionXSpin->GetValue();
        light.positionY = m_positionYSpin->GetValue();
        light.positionZ = m_positionZSpin->GetValue();
        light.directionX = m_directionXSpin->GetValue();
        light.directionY = m_directionYSpin->GetValue();
        light.directionZ = m_directionZSpin->GetValue();
        light.intensity = m_intensitySpin->GetValue();
        if (sourceControl == m_positionXSpin || sourceControl == m_positionYSpin || sourceControl == m_positionZSpin) {
            if (light.type == "directional") {
                shouldApplyChanges = false;
                LOG_INF_S("LightingPanel::onLightPropertyChangedSpin: Position change ignored for directional light");
            }
        }
        updateControlStates();
        if (shouldApplyChanges && m_parentDialog) {
            m_parentDialog->applyGlobalSettingsToCanvas();
            LOG_INF_S("LightingPanel::onLightPropertyChangedSpin: Applied changes to canvas");
        }
        onLightingChanged(wxCommandEvent());
    }
}

void LightingPanel::onLightingChanged(wxCommandEvent& event)
{
    if (m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void LightingPanel::onStudioPreset(wxCommandEvent& event)
{
    m_lights.clear();
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

void LightingPanel::onOutdoorPreset(wxCommandEvent& event)
{
    m_lights.clear();
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

void LightingPanel::onDramaticPreset(wxCommandEvent& event)
{
    m_lights.clear();
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

void LightingPanel::onWarmPreset(wxCommandEvent& event)
{
    m_lights.clear();
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

void LightingPanel::onCoolPreset(wxCommandEvent& event)
{
    m_lights.clear();
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

void LightingPanel::onMinimalPreset(wxCommandEvent& event)
{
    m_lights.clear();
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

void LightingPanel::onFreeCADPreset(wxCommandEvent& event)
{
    m_lights.clear();
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

void LightingPanel::onNavcubePreset(wxCommandEvent& event)
{
    m_lights.clear();
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

void LightingPanel::updateControlStates()
{
    bool hasValidName = false;
    wxColour currentColor = wxColour(255, 255, 255);
    std::string lightType = "directional";
    if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_lights.size())) {
        const auto& light = m_lights[m_currentLightIndex];
        hasValidName = !light.name.empty() && light.name.length() > 0;
        currentColor = light.color;
        lightType = light.type;
        LOG_INF_S("LightingPanel::updateControlStates: Light '" + light.name + "' - Type: " + lightType + " - Valid name: " + std::string(hasValidName ? "true" : "false"));
    } else {
        LOG_INF_S("LightingPanel::updateControlStates: No light selected");
    }
    if (m_lightTypeChoice) {
        m_lightTypeChoice->Enable(hasValidName);
        LOG_INF_S("LightingPanel::updateControlStates: Light type choice " + std::string(hasValidName ? "enabled" : "disabled"));
    }
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
    if (m_intensitySpin) {
        m_intensitySpin->Enable(hasValidName);
    }
    if (m_lightColorButton) {
        m_lightColorButton->Enable(hasValidName);
    }
    if (m_lightEnabledCheckBox) {
        m_lightEnabledCheckBox->Enable(hasValidName);
    }
    LOG_INF_S("LightingPanel::updateControlStates: Position controls " + std::string(positionEnabled ? "enabled" : "disabled") + " for light type: " + lightType);
    if (m_lightColorButton) {
        if (!hasValidName) {
            m_lightColorButton->SetBackgroundColour(wxColour(200, 200, 200));
            m_lightColorButton->SetLabel("RGB(200,200,200)");
            LOG_INF_S("LightingPanel::updateControlStates: Color button grayed out");
        } else {
            m_lightColorButton->SetBackgroundColour(currentColor);
            m_lightColorButton->SetLabel(wxString::Format("RGB(%d,%d,%d)", currentColor.Red(), currentColor.Green(), currentColor.Blue()));
            LOG_INF_S("LightingPanel::updateControlStates: Color button restored to original color");
        }
    }
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
    LOG_INF_S("LightingPanel::updateControlStates: All controls updated - Valid name: " + std::string(hasValidName ? "true" : "false"));
}

void LightingPanel::loadSettings()
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString exeDir = exeFile.GetPath();
        wxString renderConfigPath = exeDir + wxFileName::GetPathSeparator() + "render_settings.ini";
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, renderConfigPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);

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
                this->m_lights.push_back(light);
            }
        } else {
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
            this->m_lights.push_back(defaultLight);
        }
        this->updateLightList();
        this->updateControlStates();
        LOG_INF_S("LightingPanel::loadSettings: Settings loaded successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("LightingPanel::loadSettings: Failed to load settings: " + std::string(e.what()));
    }
}

void LightingPanel::saveSettings()
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString exeDir = exeFile.GetPath();
        wxString renderConfigPath = exeDir + wxFileName::GetPathSeparator() + "render_settings.ini";
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, renderConfigPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);

        renderConfig.Write("Global.Lighting.Count", static_cast<int>(this->m_lights.size()));
        for (size_t i = 0; i < this->m_lights.size(); ++i) {
            const auto& light = this->m_lights[i];
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
        LOG_INF_S("LightingPanel::saveSettings: Settings saved successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("LightingPanel::saveSettings: Failed to save settings: " + std::string(e.what()));
    }
}

void LightingPanel::resetToDefaults()
{
    this->m_lights.clear();
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
    this->m_lights.push_back(defaultLight);
    this->updateLightList();
    this->updateControlStates();
    LOG_INF_S("LightingPanel::resetToDefaults: Settings reset to defaults");
}