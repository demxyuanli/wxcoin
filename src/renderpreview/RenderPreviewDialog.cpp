#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/PreviewCanvas.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/colour.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/colordlg.h>
#include <fstream>

wxBEGIN_EVENT_TABLE(RenderPreviewDialog, wxDialog)
    EVT_BUTTON(wxID_RESET, RenderPreviewDialog::OnReset)
    EVT_BUTTON(wxID_SAVE, RenderPreviewDialog::OnSave)
    EVT_BUTTON(wxID_CANCEL, RenderPreviewDialog::OnCancel)
    EVT_CLOSE(RenderPreviewDialog::OnClose)
wxEND_EVENT_TABLE()

RenderPreviewDialog::RenderPreviewDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Render Preview System", wxDefaultPosition, wxSize(1100, 640), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLOSE_BOX | wxCAPTION)
    , m_lightColor(255, 255, 255)
{
    LOG_INF_S("RenderPreviewDialog::RenderPreviewDialog: Initializing");
    createUI();
    loadConfiguration();
    SetSizer(GetSizer());
    
    // Set fixed size to ensure 1000x500
    SetSize(1100, 640);
    SetMinSize(wxSize(1100, 640));
    
    // Center the dialog on parent window
    if (parent) {
        CenterOnParent();
    }
    
    LOG_INF_S("RenderPreviewDialog::RenderPreviewDialog: Initialized successfully");
}

void RenderPreviewDialog::createUI()
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Top section: Configuration and Preview
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left panel: Configuration tabs (fixed 320px width)
    auto* leftPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(400, -1), wxBORDER_SUNKEN);
    m_configNotebook = new wxNotebook(leftPanel, wxID_ANY);
    
    createLightingPanel(m_configNotebook);
    createMaterialPanel(m_configNotebook);
    createTexturePanel(m_configNotebook);
    createAntiAliasingPanel(m_configNotebook);
    
    auto* leftSizer = new wxBoxSizer(wxVERTICAL);
    leftSizer->Add(m_configNotebook, 1, wxEXPAND | wxALL, 2);
    leftPanel->SetSizer(leftSizer);
    
    // Right panel: Render preview canvas (adaptive width)
    m_renderCanvas = new PreviewCanvas(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    
    topSizer->Add(leftPanel, 0, wxEXPAND | wxALL, 2);
    topSizer->Add(m_renderCanvas, 1, wxEXPAND | wxALL, 2);
    
    mainSizer->Add(topSizer, 1, wxEXPAND | wxALL, 2);
    
    // Bottom section: Buttons in a row (fixed 40px height)
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_RESET, "Reset"), 0, wxALL, 2);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(new wxButton(this, wxID_SAVE, "Save"), 0, wxALL, 2);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 2);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 2);
    
    SetSizer(mainSizer);
}

void RenderPreviewDialog::createLightingPanel(wxNotebook* notebook)
{
    auto* panel = new wxPanel(notebook);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Ambient Light
    auto* ambientGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Ambient Light");
    m_ambientLightSlider = new wxSlider(panel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    ambientGroup->Add(m_ambientLightSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(ambientGroup, 0, wxEXPAND | wxALL, 5);
    
    // Diffuse Light
    auto* diffuseGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Diffuse Light");
    m_diffuseLightSlider = new wxSlider(panel, wxID_ANY, 80, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    diffuseGroup->Add(m_diffuseLightSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(diffuseGroup, 0, wxEXPAND | wxALL, 5);
    
    // Specular Light
    auto* specularGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Specular Light");
    m_specularLightSlider = new wxSlider(panel, wxID_ANY, 60, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    specularGroup->Add(m_specularLightSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(specularGroup, 0, wxEXPAND | wxALL, 5);
    
    // Light Color
    auto* colorGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Light Color");
    m_lightColorButton = new wxButton(panel, wxID_ANY, "Select Color");
    m_lightColorButton->SetBackgroundColour(m_lightColor);
    colorGroup->Add(m_lightColorButton, 0, wxEXPAND | wxALL, 5);
    sizer->Add(colorGroup, 0, wxEXPAND | wxALL, 5);
    
    // Light Intensity
    auto* intensityGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Light Intensity");
    m_lightIntensitySlider = new wxSlider(panel, wxID_ANY, 100, 0, 200, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    intensityGroup->Add(m_lightIntensitySlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(intensityGroup, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
    notebook->AddPage(panel, "Lighting");
    
    // Bind events
    m_ambientLightSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onLightingChanged, this);
    m_diffuseLightSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onLightingChanged, this);
    m_specularLightSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onLightingChanged, this);
    m_lightColorButton->Bind(wxEVT_BUTTON, &RenderPreviewDialog::onLightColorButton, this);
    m_lightIntensitySlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onLightingChanged, this);
}

void RenderPreviewDialog::createMaterialPanel(wxNotebook* notebook)
{
    auto* panel = new wxPanel(notebook);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Ambient Material
    auto* ambientGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Ambient Material");
    m_ambientMaterialSlider = new wxSlider(panel, wxID_ANY, 30, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    ambientGroup->Add(m_ambientMaterialSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(ambientGroup, 0, wxEXPAND | wxALL, 5);
    
    // Diffuse Material
    auto* diffuseGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Diffuse Material");
    m_diffuseMaterialSlider = new wxSlider(panel, wxID_ANY, 70, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    diffuseGroup->Add(m_diffuseMaterialSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(diffuseGroup, 0, wxEXPAND | wxALL, 5);
    
    // Specular Material
    auto* specularGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Specular Material");
    m_specularMaterialSlider = new wxSlider(panel, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    specularGroup->Add(m_specularMaterialSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(specularGroup, 0, wxEXPAND | wxALL, 5);
    
    // Shininess
    auto* shininessGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Shininess");
    m_shininessSlider = new wxSlider(panel, wxID_ANY, 32, 1, 128, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    shininessGroup->Add(m_shininessSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(shininessGroup, 0, wxEXPAND | wxALL, 5);
    
    // Transparency
    auto* transparencyGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Transparency");
    m_transparencySlider = new wxSlider(panel, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    transparencyGroup->Add(m_transparencySlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(transparencyGroup, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
    notebook->AddPage(panel, "Material");
    
    // Bind events
    m_ambientMaterialSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onMaterialChanged, this);
    m_diffuseMaterialSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onMaterialChanged, this);
    m_specularMaterialSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onMaterialChanged, this);
    m_shininessSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onMaterialChanged, this);
    m_transparencySlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onMaterialChanged, this);
}

void RenderPreviewDialog::createTexturePanel(wxNotebook* notebook)
{
    auto* panel = new wxPanel(notebook);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Enable Texture
    m_enableTextureCheckBox = new wxCheckBox(panel, wxID_ANY, "Enable Texture");
    sizer->Add(m_enableTextureCheckBox, 0, wxALL, 5);
    
    // Texture Mode
    auto* modeGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Texture Mode");
    m_textureModeChoice = new wxChoice(panel, wxID_ANY);
    m_textureModeChoice->Append("Replace");
    m_textureModeChoice->Append("Modulate");
    m_textureModeChoice->Append("Decal");
    m_textureModeChoice->Append("Blend");
    m_textureModeChoice->SetSelection(0);
    modeGroup->Add(m_textureModeChoice, 0, wxEXPAND | wxALL, 5);
    sizer->Add(modeGroup, 0, wxEXPAND | wxALL, 5);
    
    // Texture Scale
    auto* scaleGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Texture Scale");
    m_textureScaleSlider = new wxSlider(panel, wxID_ANY, 100, 10, 500, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    scaleGroup->Add(m_textureScaleSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(scaleGroup, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
    notebook->AddPage(panel, "Texture");
    
    // Bind events
    m_enableTextureCheckBox->Bind(wxEVT_CHECKBOX, &RenderPreviewDialog::onTextureChanged, this);
    m_textureModeChoice->Bind(wxEVT_CHOICE, &RenderPreviewDialog::onTextureChanged, this);
    m_textureScaleSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onTextureChanged, this);
}

void RenderPreviewDialog::createAntiAliasingPanel(wxNotebook* notebook)
{
    auto* panel = new wxPanel(notebook);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Anti-aliasing Method
    auto* methodGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Anti-aliasing Method");
    m_antiAliasingChoice = new wxChoice(panel, wxID_ANY);
    m_antiAliasingChoice->Append("None");
    m_antiAliasingChoice->Append("MSAA");
    m_antiAliasingChoice->Append("FXAA");
    m_antiAliasingChoice->Append("MSAA + FXAA");
    m_antiAliasingChoice->SetSelection(1);
    methodGroup->Add(m_antiAliasingChoice, 0, wxEXPAND | wxALL, 5);
    sizer->Add(methodGroup, 0, wxEXPAND | wxALL, 5);
    
    // MSAA Samples
    auto* msaaGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "MSAA Samples");
    m_msaaSamplesSlider = new wxSlider(panel, wxID_ANY, 4, 2, 16, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    msaaGroup->Add(m_msaaSamplesSlider, 0, wxEXPAND | wxALL, 5);
    sizer->Add(msaaGroup, 0, wxEXPAND | wxALL, 5);
    
    // Enable FXAA
    m_enableFXAACheckBox = new wxCheckBox(panel, wxID_ANY, "Enable FXAA");
    sizer->Add(m_enableFXAACheckBox, 0, wxALL, 5);
    
    panel->SetSizer(sizer);
    notebook->AddPage(panel, "Anti-aliasing");
    
    // Bind events
    m_antiAliasingChoice->Bind(wxEVT_CHOICE, &RenderPreviewDialog::onAntiAliasingChanged, this);
    m_msaaSamplesSlider->Bind(wxEVT_SLIDER, &RenderPreviewDialog::onAntiAliasingChanged, this);
    m_enableFXAACheckBox->Bind(wxEVT_CHECKBOX, &RenderPreviewDialog::onAntiAliasingChanged, this);
}

void RenderPreviewDialog::onLightColorButton(wxCommandEvent& event)
{
    wxColourDialog dialog(this);
    wxColourData& data = dialog.GetColourData();
    data.SetColour(m_lightColor);
    
    if (dialog.ShowModal() == wxID_OK) {
        m_lightColor = data.GetColour();
        m_lightColorButton->SetBackgroundColour(m_lightColor);
        onLightingChanged(event);
    }
}

void RenderPreviewDialog::onLightingChanged(wxCommandEvent& event)
{
    if (m_renderCanvas) {
        float ambient = m_ambientLightSlider->GetValue() / 100.0f;
        float diffuse = m_diffuseLightSlider->GetValue() / 100.0f;
        float specular = m_specularLightSlider->GetValue() / 100.0f;
        float intensity = m_lightIntensitySlider->GetValue() / 100.0f;
        
        m_renderCanvas->updateLighting(ambient, diffuse, specular, m_lightColor, intensity);
    }
}

void RenderPreviewDialog::onMaterialChanged(wxCommandEvent& event)
{
    if (m_renderCanvas) {
        float ambient = m_ambientMaterialSlider->GetValue() / 100.0f;
        float diffuse = m_diffuseMaterialSlider->GetValue() / 100.0f;
        float specular = m_specularMaterialSlider->GetValue() / 100.0f;
        float shininess = m_shininessSlider->GetValue() / 128.0f;
        float transparency = m_transparencySlider->GetValue() / 100.0f;
        
        m_renderCanvas->updateMaterial(ambient, diffuse, specular, shininess, transparency);
    }
}

void RenderPreviewDialog::onTextureChanged(wxCommandEvent& event)
{
    if (m_renderCanvas) {
        bool enabled = m_enableTextureCheckBox->GetValue();
        int mode = m_textureModeChoice->GetSelection();
        float scale = m_textureScaleSlider->GetValue() / 100.0f;
        
        m_renderCanvas->updateTexture(enabled, mode, scale);
    }
}

void RenderPreviewDialog::onAntiAliasingChanged(wxCommandEvent& event)
{
    if (m_renderCanvas) {
        int method = m_antiAliasingChoice->GetSelection();
        int msaaSamples = m_msaaSamplesSlider->GetValue();
        bool fxaaEnabled = m_enableFXAACheckBox->GetValue();
        
        m_renderCanvas->updateAntiAliasing(method, msaaSamples, fxaaEnabled);
    }
}

void RenderPreviewDialog::OnReset(wxCommandEvent& event)
{
    resetToDefaults();
    if (m_renderCanvas) {
        m_renderCanvas->resetView();
        m_renderCanvas->render(false);
    }
}

void RenderPreviewDialog::OnSave(wxCommandEvent& event)
{
    saveConfiguration();
    EndModal(wxID_OK);
}

void RenderPreviewDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void RenderPreviewDialog::OnClose(wxCloseEvent& event)
{
    EndModal(wxID_CANCEL);
}

void RenderPreviewDialog::saveConfiguration()
{
    try {
        auto& configManager = ConfigManager::getInstance();
        
        // Save lighting settings
        configManager.setInt("RenderPreview", "Lighting.Ambient", m_ambientLightSlider->GetValue());
        configManager.setInt("RenderPreview", "Lighting.Diffuse", m_diffuseLightSlider->GetValue());
        configManager.setInt("RenderPreview", "Lighting.Specular", m_specularLightSlider->GetValue());
        configManager.setInt("RenderPreview", "Lighting.Intensity", m_lightIntensitySlider->GetValue());
        
        configManager.setInt("RenderPreview", "Lighting.Color.R", m_lightColor.Red());
        configManager.setInt("RenderPreview", "Lighting.Color.G", m_lightColor.Green());
        configManager.setInt("RenderPreview", "Lighting.Color.B", m_lightColor.Blue());
        
        // Save material settings
        configManager.setInt("RenderPreview", "Material.Ambient", m_ambientMaterialSlider->GetValue());
        configManager.setInt("RenderPreview", "Material.Diffuse", m_diffuseMaterialSlider->GetValue());
        configManager.setInt("RenderPreview", "Material.Specular", m_specularMaterialSlider->GetValue());
        configManager.setInt("RenderPreview", "Material.Shininess", m_shininessSlider->GetValue());
        configManager.setInt("RenderPreview", "Material.Transparency", m_transparencySlider->GetValue());
        
        // Save texture settings
        configManager.setBool("RenderPreview", "Texture.Enabled", m_enableTextureCheckBox->GetValue());
        configManager.setInt("RenderPreview", "Texture.Mode", m_textureModeChoice->GetSelection());
        configManager.setInt("RenderPreview", "Texture.Scale", m_textureScaleSlider->GetValue());
        
        // Save anti-aliasing settings
        configManager.setInt("RenderPreview", "AntiAliasing.Method", m_antiAliasingChoice->GetSelection());
        configManager.setInt("RenderPreview", "AntiAliasing.MSAASamples", m_msaaSamplesSlider->GetValue());
        configManager.setBool("RenderPreview", "AntiAliasing.FXAAEnabled", m_enableFXAACheckBox->GetValue());
        
        configManager.save();
        LOG_INF_S("RenderPreviewDialog::saveConfiguration: Configuration saved successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("RenderPreviewDialog::saveConfiguration: Failed to save configuration: " + std::string(e.what()));
    }
}

void RenderPreviewDialog::loadConfiguration()
{
    try {
        auto& configManager = ConfigManager::getInstance();
        
        // Load lighting settings
        m_ambientLightSlider->SetValue(configManager.getInt("RenderPreview", "Lighting.Ambient", 50));
        m_diffuseLightSlider->SetValue(configManager.getInt("RenderPreview", "Lighting.Diffuse", 80));
        m_specularLightSlider->SetValue(configManager.getInt("RenderPreview", "Lighting.Specular", 60));
        m_lightIntensitySlider->SetValue(configManager.getInt("RenderPreview", "Lighting.Intensity", 100));
        
        int r = configManager.getInt("RenderPreview", "Lighting.Color.R", 255);
        int g = configManager.getInt("RenderPreview", "Lighting.Color.G", 255);
        int b = configManager.getInt("RenderPreview", "Lighting.Color.B", 255);
        m_lightColor = wxColour(r, g, b);
        if (m_lightColorButton) {
            m_lightColorButton->SetBackgroundColour(m_lightColor);
        }
        
        // Load material settings
        m_ambientMaterialSlider->SetValue(configManager.getInt("RenderPreview", "Material.Ambient", 30));
        m_diffuseMaterialSlider->SetValue(configManager.getInt("RenderPreview", "Material.Diffuse", 70));
        m_specularMaterialSlider->SetValue(configManager.getInt("RenderPreview", "Material.Specular", 50));
        m_shininessSlider->SetValue(configManager.getInt("RenderPreview", "Material.Shininess", 32));
        m_transparencySlider->SetValue(configManager.getInt("RenderPreview", "Material.Transparency", 0));
        
        // Load texture settings
        m_enableTextureCheckBox->SetValue(configManager.getBool("RenderPreview", "Texture.Enabled", false));
        m_textureModeChoice->SetSelection(configManager.getInt("RenderPreview", "Texture.Mode", 0));
        m_textureScaleSlider->SetValue(configManager.getInt("RenderPreview", "Texture.Scale", 100));
        
        // Load anti-aliasing settings
        m_antiAliasingChoice->SetSelection(configManager.getInt("RenderPreview", "AntiAliasing.Method", 1));
        m_msaaSamplesSlider->SetValue(configManager.getInt("RenderPreview", "AntiAliasing.MSAASamples", 4));
        m_enableFXAACheckBox->SetValue(configManager.getBool("RenderPreview", "AntiAliasing.FXAAEnabled", false));
        
        LOG_INF_S("RenderPreviewDialog::loadConfiguration: Configuration loaded successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("RenderPreviewDialog::loadConfiguration: Failed to load configuration: " + std::string(e.what()));
    }
}

void RenderPreviewDialog::resetToDefaults()
{
    // Reset lighting
    m_ambientLightSlider->SetValue(50);
    m_diffuseLightSlider->SetValue(80);
    m_specularLightSlider->SetValue(60);
    m_lightColor = wxColour(255, 255, 255);
    if (m_lightColorButton) {
        m_lightColorButton->SetBackgroundColour(m_lightColor);
    }
    m_lightIntensitySlider->SetValue(100);
    
    // Reset material
    m_ambientMaterialSlider->SetValue(30);
    m_diffuseMaterialSlider->SetValue(70);
    m_specularMaterialSlider->SetValue(50);
    m_shininessSlider->SetValue(32);
    m_transparencySlider->SetValue(0);
    
    // Reset texture
    m_enableTextureCheckBox->SetValue(false);
    m_textureModeChoice->SetSelection(0);
    m_textureScaleSlider->SetValue(100);
    
    // Reset anti-aliasing
    m_antiAliasingChoice->SetSelection(1);
    m_msaaSamplesSlider->SetValue(4);
    m_enableFXAACheckBox->SetValue(false);
    
    LOG_INF_S("RenderPreviewDialog::resetToDefaults: Settings reset to defaults");
}
