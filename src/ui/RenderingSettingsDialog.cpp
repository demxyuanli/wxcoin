#include "RenderingSettingsDialog.h"
#include "OCCViewer.h"
#include "RenderingEngine.h"
#include "config/RenderingConfig.h"
#include <wx/wx.h>
#include <wx/colour.h>
#include <wx/colordlg.h>

RenderingSettingsDialog::RenderingSettingsDialog(wxWindow* parent, OCCViewer* occViewer, RenderingEngine* renderingEngine)
    : wxDialog(parent, wxID_ANY, "Rendering Settings", wxDefaultPosition, wxSize(500, 450), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_occViewer(occViewer)
    , m_renderingEngine(renderingEngine)
    , m_notebook(nullptr)
    , m_materialPage(nullptr)
    , m_lightingPage(nullptr)
    , m_texturePage(nullptr)
{
    // Load current settings from configuration
    RenderingConfig& config = RenderingConfig::getInstance();
    const auto& materialSettings = config.getMaterialSettings();
    const auto& lightingSettings = config.getLightingSettings();
    const auto& textureSettings = config.getTextureSettings();
    
    // Initialize with current configuration values
    m_materialAmbientColor = materialSettings.ambientColor;
    m_materialDiffuseColor = materialSettings.diffuseColor;
    m_materialSpecularColor = materialSettings.specularColor;
    m_materialShininess = materialSettings.shininess;
    m_materialTransparency = materialSettings.transparency;
    
    m_lightAmbientColor = lightingSettings.ambientColor;
    m_lightDiffuseColor = lightingSettings.diffuseColor;
    m_lightSpecularColor = lightingSettings.specularColor;
    m_lightIntensity = lightingSettings.intensity;
    m_lightAmbientIntensity = lightingSettings.ambientIntensity;
    
    m_textureColor = textureSettings.color;
    m_textureIntensity = textureSettings.intensity;
    m_textureEnabled = textureSettings.enabled;
    
    createControls();
    layoutControls();
    bindEvents();
    updateControls();
}

RenderingSettingsDialog::~RenderingSettingsDialog()
{
}

void RenderingSettingsDialog::createControls()
{
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    createMaterialPage();
    createLightingPage();
    createTexturePage();
    
    // Create dialog buttons
    m_applyButton = new wxButton(this, wxID_APPLY, "Apply");
    m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    m_okButton = new wxButton(this, wxID_OK, "OK");
    m_resetButton = new wxButton(this, wxID_ANY, "Reset to Defaults");
}

void RenderingSettingsDialog::createMaterialPage()
{
    m_materialPage = new wxPanel(m_notebook);
    
    // Material ambient color
    new wxStaticText(m_materialPage, wxID_ANY, "Ambient Color:");
    m_materialAmbientColorButton = new wxButton(m_materialPage, wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 30));
    updateColorButton(m_materialAmbientColorButton, quantityColorToWxColour(m_materialAmbientColor));
    
    // Material diffuse color
    new wxStaticText(m_materialPage, wxID_ANY, "Diffuse Color:");
    m_materialDiffuseColorButton = new wxButton(m_materialPage, wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 30));
    updateColorButton(m_materialDiffuseColorButton, quantityColorToWxColour(m_materialDiffuseColor));
    
    // Material specular color
    new wxStaticText(m_materialPage, wxID_ANY, "Specular Color:");
    m_materialSpecularColorButton = new wxButton(m_materialPage, wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 30));
    updateColorButton(m_materialSpecularColorButton, quantityColorToWxColour(m_materialSpecularColor));
    
    // Material shininess
    new wxStaticText(m_materialPage, wxID_ANY, "Shininess:");
    m_materialShininessSlider = new wxSlider(m_materialPage, wxID_ANY, 30, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_materialShininessLabel = new wxStaticText(m_materialPage, wxID_ANY, "30");
    
    // Material transparency
    new wxStaticText(m_materialPage, wxID_ANY, "Transparency:");
    m_materialTransparencySlider = new wxSlider(m_materialPage, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_materialTransparencyLabel = new wxStaticText(m_materialPage, wxID_ANY, "0%");
    
    // Layout material page
    wxBoxSizer* materialSizer = new wxBoxSizer(wxVERTICAL);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 3, 10, 10);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, "Ambient Color:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_materialAmbientColorButton, 0, wxEXPAND);
    gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, ""), 0);
    
    gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, "Diffuse Color:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_materialDiffuseColorButton, 0, wxEXPAND);
    gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, ""), 0);
    
    gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, "Specular Color:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_materialSpecularColorButton, 0, wxEXPAND);
    gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, ""), 0);
    
    gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, "Shininess:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_materialShininessSlider, 0, wxEXPAND);
    gridSizer->Add(m_materialShininessLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, "Transparency:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_materialTransparencySlider, 0, wxEXPAND);
    gridSizer->Add(m_materialTransparencyLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    materialSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
    m_materialPage->SetSizer(materialSizer);
    
    m_notebook->AddPage(m_materialPage, "Material");
}

void RenderingSettingsDialog::createLightingPage()
{
    m_lightingPage = new wxPanel(m_notebook);
    
    // Lighting ambient color
    m_lightAmbientColorButton = new wxButton(m_lightingPage, wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 30));
    updateColorButton(m_lightAmbientColorButton, quantityColorToWxColour(m_lightAmbientColor));
    
    // Lighting diffuse color
    m_lightDiffuseColorButton = new wxButton(m_lightingPage, wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 30));
    updateColorButton(m_lightDiffuseColorButton, quantityColorToWxColour(m_lightDiffuseColor));
    
    // Lighting specular color
    m_lightSpecularColorButton = new wxButton(m_lightingPage, wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 30));
    updateColorButton(m_lightSpecularColorButton, quantityColorToWxColour(m_lightSpecularColor));
    
    // Light intensity
    m_lightIntensitySlider = new wxSlider(m_lightingPage, wxID_ANY, 80, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_lightIntensityLabel = new wxStaticText(m_lightingPage, wxID_ANY, "80%");
    
    // Light ambient intensity
    m_lightAmbientIntensitySlider = new wxSlider(m_lightingPage, wxID_ANY, 30, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_lightAmbientIntensityLabel = new wxStaticText(m_lightingPage, wxID_ANY, "30%");
    
    // Layout lighting page
    wxBoxSizer* lightingSizer = new wxBoxSizer(wxVERTICAL);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 3, 10, 10);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(m_lightingPage, wxID_ANY, "Ambient Color:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_lightAmbientColorButton, 0, wxEXPAND);
    gridSizer->Add(new wxStaticText(m_lightingPage, wxID_ANY, ""), 0);
    
    gridSizer->Add(new wxStaticText(m_lightingPage, wxID_ANY, "Diffuse Color:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_lightDiffuseColorButton, 0, wxEXPAND);
    gridSizer->Add(new wxStaticText(m_lightingPage, wxID_ANY, ""), 0);
    
    gridSizer->Add(new wxStaticText(m_lightingPage, wxID_ANY, "Specular Color:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_lightSpecularColorButton, 0, wxEXPAND);
    gridSizer->Add(new wxStaticText(m_lightingPage, wxID_ANY, ""), 0);
    
    gridSizer->Add(new wxStaticText(m_lightingPage, wxID_ANY, "Light Intensity:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_lightIntensitySlider, 0, wxEXPAND);
    gridSizer->Add(m_lightIntensityLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    gridSizer->Add(new wxStaticText(m_lightingPage, wxID_ANY, "Ambient Intensity:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_lightAmbientIntensitySlider, 0, wxEXPAND);
    gridSizer->Add(m_lightAmbientIntensityLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    lightingSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
    m_lightingPage->SetSizer(lightingSizer);
    
    m_notebook->AddPage(m_lightingPage, "Lighting");
}

void RenderingSettingsDialog::createTexturePage()
{
    m_texturePage = new wxPanel(m_notebook);
    
    // Texture enabled checkbox
    m_textureEnabledCheckbox = new wxCheckBox(m_texturePage, wxID_ANY, "Enable Texture");
    
    // Texture color
    m_textureColorButton = new wxButton(m_texturePage, wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 30));
    updateColorButton(m_textureColorButton, quantityColorToWxColour(m_textureColor));
    
    // Texture intensity
    m_textureIntensitySlider = new wxSlider(m_texturePage, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_textureIntensityLabel = new wxStaticText(m_texturePage, wxID_ANY, "50%");
    
    // Layout texture page
    wxBoxSizer* textureSizer = new wxBoxSizer(wxVERTICAL);
    
    textureSizer->Add(m_textureEnabledCheckbox, 0, wxALL, 10);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 3, 10, 10);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, "Texture Color:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_textureColorButton, 0, wxEXPAND);
    gridSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, ""), 0);
    
    gridSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, "Texture Intensity:"), 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(m_textureIntensitySlider, 0, wxEXPAND);
    gridSizer->Add(m_textureIntensityLabel, 0, wxALIGN_CENTER_VERTICAL);
    
    textureSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
    m_texturePage->SetSizer(textureSizer);
    
    m_notebook->AddPage(m_texturePage, "Texture");
}

void RenderingSettingsDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Add notebook
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);
    
    // Add dialog buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(m_resetButton, 0, wxRIGHT, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_applyButton, 0, wxRIGHT, 5);
    buttonSizer->Add(m_cancelButton, 0, wxRIGHT, 5);
    buttonSizer->Add(m_okButton, 0);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(mainSizer);
}

void RenderingSettingsDialog::bindEvents()
{
    // Material events
    m_materialAmbientColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onMaterialAmbientColorButton, this);
    m_materialDiffuseColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onMaterialDiffuseColorButton, this);
    m_materialSpecularColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onMaterialSpecularColorButton, this);
    m_materialShininessSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onMaterialShininessSlider, this);
    m_materialTransparencySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onMaterialTransparencySlider, this);
    
    // Lighting events
    m_lightAmbientColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onLightAmbientColorButton, this);
    m_lightDiffuseColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onLightDiffuseColorButton, this);
    m_lightSpecularColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onLightSpecularColorButton, this);
    m_lightIntensitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onLightIntensitySlider, this);
    m_lightAmbientIntensitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onLightAmbientIntensitySlider, this);
    
    // Texture events
    m_textureColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onTextureColorButton, this);
    m_textureIntensitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onTextureIntensitySlider, this);
    m_textureEnabledCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &RenderingSettingsDialog::onTextureEnabledCheckbox, this);
    
    // Dialog events
    m_applyButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onApply, this);
    m_cancelButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onCancel, this);
    m_okButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onOK, this);
    m_resetButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onReset, this);
}

void RenderingSettingsDialog::updateControls()
{
    // Update material controls
    m_materialShininessSlider->SetValue(static_cast<int>(m_materialShininess));
    m_materialShininessLabel->SetLabel(wxString::Format("%.0f", m_materialShininess));
    
    m_materialTransparencySlider->SetValue(static_cast<int>(m_materialTransparency * 100));
    m_materialTransparencyLabel->SetLabel(wxString::Format("%.0f%%", m_materialTransparency * 100));
    
    // Update lighting controls
    m_lightIntensitySlider->SetValue(static_cast<int>(m_lightIntensity * 100));
    m_lightIntensityLabel->SetLabel(wxString::Format("%.0f%%", m_lightIntensity * 100));
    
    m_lightAmbientIntensitySlider->SetValue(static_cast<int>(m_lightAmbientIntensity * 100));
    m_lightAmbientIntensityLabel->SetLabel(wxString::Format("%.0f%%", m_lightAmbientIntensity * 100));
    
    // Update texture controls
    m_textureIntensitySlider->SetValue(static_cast<int>(m_textureIntensity * 100));
    m_textureIntensityLabel->SetLabel(wxString::Format("%.0f%%", m_textureIntensity * 100));
    
    m_textureEnabledCheckbox->SetValue(m_textureEnabled);
}

// Material event handlers
void RenderingSettingsDialog::onMaterialAmbientColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_materialAmbientColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    wxColourDialog dialog(this, &colorData);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxColour newColor = dialog.GetColourData().GetColour();
        m_materialAmbientColor = wxColourToQuantityColor(newColor);
        updateColorButton(m_materialAmbientColorButton, newColor);
    }
}

void RenderingSettingsDialog::onMaterialDiffuseColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_materialDiffuseColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    wxColourDialog dialog(this, &colorData);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxColour newColor = dialog.GetColourData().GetColour();
        m_materialDiffuseColor = wxColourToQuantityColor(newColor);
        updateColorButton(m_materialDiffuseColorButton, newColor);
    }
}

void RenderingSettingsDialog::onMaterialSpecularColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_materialSpecularColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    wxColourDialog dialog(this, &colorData);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxColour newColor = dialog.GetColourData().GetColour();
        m_materialSpecularColor = wxColourToQuantityColor(newColor);
        updateColorButton(m_materialSpecularColorButton, newColor);
    }
}

void RenderingSettingsDialog::onMaterialShininessSlider(wxCommandEvent& event)
{
    m_materialShininess = static_cast<double>(m_materialShininessSlider->GetValue());
    m_materialShininessLabel->SetLabel(wxString::Format("%.0f", m_materialShininess));
}

void RenderingSettingsDialog::onMaterialTransparencySlider(wxCommandEvent& event)
{
    m_materialTransparency = static_cast<double>(m_materialTransparencySlider->GetValue()) / 100.0;
    m_materialTransparencyLabel->SetLabel(wxString::Format("%.0f%%", m_materialTransparency * 100));
}

// Lighting event handlers
void RenderingSettingsDialog::onLightAmbientColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_lightAmbientColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    wxColourDialog dialog(this, &colorData);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxColour newColor = dialog.GetColourData().GetColour();
        m_lightAmbientColor = wxColourToQuantityColor(newColor);
        updateColorButton(m_lightAmbientColorButton, newColor);
    }
}

void RenderingSettingsDialog::onLightDiffuseColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_lightDiffuseColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    wxColourDialog dialog(this, &colorData);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxColour newColor = dialog.GetColourData().GetColour();
        m_lightDiffuseColor = wxColourToQuantityColor(newColor);
        updateColorButton(m_lightDiffuseColorButton, newColor);
    }
}

void RenderingSettingsDialog::onLightSpecularColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_lightSpecularColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    wxColourDialog dialog(this, &colorData);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxColour newColor = dialog.GetColourData().GetColour();
        m_lightSpecularColor = wxColourToQuantityColor(newColor);
        updateColorButton(m_lightSpecularColorButton, newColor);
    }
}

void RenderingSettingsDialog::onLightIntensitySlider(wxCommandEvent& event)
{
    m_lightIntensity = static_cast<double>(m_lightIntensitySlider->GetValue()) / 100.0;
    m_lightIntensityLabel->SetLabel(wxString::Format("%.0f%%", m_lightIntensity * 100));
}

void RenderingSettingsDialog::onLightAmbientIntensitySlider(wxCommandEvent& event)
{
    m_lightAmbientIntensity = static_cast<double>(m_lightAmbientIntensitySlider->GetValue()) / 100.0;
    m_lightAmbientIntensityLabel->SetLabel(wxString::Format("%.0f%%", m_lightAmbientIntensity * 100));
}

// Texture event handlers
void RenderingSettingsDialog::onTextureColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_textureColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    wxColourDialog dialog(this, &colorData);
    
    if (dialog.ShowModal() == wxID_OK) {
        wxColour newColor = dialog.GetColourData().GetColour();
        m_textureColor = wxColourToQuantityColor(newColor);
        updateColorButton(m_textureColorButton, newColor);
    }
}

void RenderingSettingsDialog::onTextureIntensitySlider(wxCommandEvent& event)
{
    m_textureIntensity = static_cast<double>(m_textureIntensitySlider->GetValue()) / 100.0;
    m_textureIntensityLabel->SetLabel(wxString::Format("%.0f%%", m_textureIntensity * 100));
}

void RenderingSettingsDialog::onTextureEnabledCheckbox(wxCommandEvent& event)
{
    m_textureEnabled = m_textureEnabledCheckbox->GetValue();
}

// Dialog event handlers
void RenderingSettingsDialog::onApply(wxCommandEvent& event)
{
    applySettings();
}

void RenderingSettingsDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void RenderingSettingsDialog::onOK(wxCommandEvent& event)
{
    applySettings();
    EndModal(wxID_OK);
}

void RenderingSettingsDialog::onReset(wxCommandEvent& event)
{
    resetToDefaults();
}

void RenderingSettingsDialog::applySettings()
{
    // Save settings to configuration file
    RenderingConfig& config = RenderingConfig::getInstance();
    
    // Update material settings
    RenderingConfig::MaterialSettings materialSettings;
    materialSettings.ambientColor = m_materialAmbientColor;
    materialSettings.diffuseColor = m_materialDiffuseColor;
    materialSettings.specularColor = m_materialSpecularColor;
    materialSettings.shininess = m_materialShininess;
    materialSettings.transparency = m_materialTransparency;
    config.setMaterialSettings(materialSettings);
    
    // Update lighting settings
    RenderingConfig::LightingSettings lightingSettings;
    lightingSettings.ambientColor = m_lightAmbientColor;
    lightingSettings.diffuseColor = m_lightDiffuseColor;
    lightingSettings.specularColor = m_lightSpecularColor;
    lightingSettings.intensity = m_lightIntensity;
    lightingSettings.ambientIntensity = m_lightAmbientIntensity;
    config.setLightingSettings(lightingSettings);
    
    // Update texture settings
    RenderingConfig::TextureSettings textureSettings;
    textureSettings.color = m_textureColor;
    textureSettings.intensity = m_textureIntensity;
    textureSettings.enabled = m_textureEnabled;
    config.setTextureSettings(textureSettings);
    
    if (m_occViewer) {
        // Apply settings to all geometries
        auto geometries = m_occViewer->getAllGeometry();
        for (auto& geometry : geometries) {
            // Apply material settings
            geometry->setMaterialAmbientColor(m_materialAmbientColor);
            geometry->setMaterialDiffuseColor(m_materialDiffuseColor);
            geometry->setMaterialSpecularColor(m_materialSpecularColor);
            geometry->setMaterialShininess(m_materialShininess);
            geometry->setTransparency(m_materialTransparency);
            
            // Apply texture settings
            geometry->setTextureColor(m_textureColor);
            geometry->setTextureIntensity(m_textureIntensity);
            geometry->setTextureEnabled(m_textureEnabled);
        }
        
        // Apply lighting settings to rendering engine
        if (m_renderingEngine) {
            m_renderingEngine->setLightAmbientColor(m_lightAmbientColor);
            m_renderingEngine->setLightDiffuseColor(m_lightDiffuseColor);
            m_renderingEngine->setLightSpecularColor(m_lightSpecularColor);
            m_renderingEngine->setLightIntensity(m_lightIntensity);
            m_renderingEngine->setLightAmbientIntensity(m_lightAmbientIntensity);
        }
        
        // Request view refresh to apply changes
        m_occViewer->requestViewRefresh();
    }
}

void RenderingSettingsDialog::resetToDefaults()
{
    // Reset configuration to defaults
    RenderingConfig& config = RenderingConfig::getInstance();
    config.resetToDefaults();
    
    // Load the default values
    const auto& materialSettings = config.getMaterialSettings();
    const auto& lightingSettings = config.getLightingSettings();
    const auto& textureSettings = config.getTextureSettings();
    
    // Reset dialog values
    m_materialAmbientColor = materialSettings.ambientColor;
    m_materialDiffuseColor = materialSettings.diffuseColor;
    m_materialSpecularColor = materialSettings.specularColor;
    m_materialShininess = materialSettings.shininess;
    m_materialTransparency = materialSettings.transparency;
    
    m_lightAmbientColor = lightingSettings.ambientColor;
    m_lightDiffuseColor = lightingSettings.diffuseColor;
    m_lightSpecularColor = lightingSettings.specularColor;
    m_lightIntensity = lightingSettings.intensity;
    m_lightAmbientIntensity = lightingSettings.ambientIntensity;
    
    m_textureColor = textureSettings.color;
    m_textureIntensity = textureSettings.intensity;
    m_textureEnabled = textureSettings.enabled;
    
    // Update color buttons
    updateColorButton(m_materialAmbientColorButton, quantityColorToWxColour(m_materialAmbientColor));
    updateColorButton(m_materialDiffuseColorButton, quantityColorToWxColour(m_materialDiffuseColor));
    updateColorButton(m_materialSpecularColorButton, quantityColorToWxColour(m_materialSpecularColor));
    updateColorButton(m_lightAmbientColorButton, quantityColorToWxColour(m_lightAmbientColor));
    updateColorButton(m_lightDiffuseColorButton, quantityColorToWxColour(m_lightDiffuseColor));
    updateColorButton(m_lightSpecularColorButton, quantityColorToWxColour(m_lightSpecularColor));
    updateColorButton(m_textureColorButton, quantityColorToWxColour(m_textureColor));
    
    // Update controls
    updateControls();
}

wxColour RenderingSettingsDialog::quantityColorToWxColour(const Quantity_Color& color)
{
    return wxColour(static_cast<unsigned char>(color.Red() * 255),
                   static_cast<unsigned char>(color.Green() * 255),
                   static_cast<unsigned char>(color.Blue() * 255));
}

Quantity_Color RenderingSettingsDialog::wxColourToQuantityColor(const wxColour& color)
{
    return Quantity_Color(static_cast<double>(color.Red()) / 255.0,
                         static_cast<double>(color.Green()) / 255.0,
                         static_cast<double>(color.Blue()) / 255.0,
                         Quantity_TOC_RGB);
}

void RenderingSettingsDialog::updateColorButton(wxButton* button, const wxColour& color)
{
    button->SetBackgroundColour(color);
    button->SetForegroundColour(color.Red() + color.Green() + color.Blue() < 382 ? *wxWHITE : *wxBLACK);
    button->Refresh();
} 