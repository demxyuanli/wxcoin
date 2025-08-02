#include "renderpreview/ObjectSettingsPanel.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>

BEGIN_EVENT_TABLE(ObjectSettingsPanel, wxPanel)
    EVT_SLIDER(wxID_ANY, ObjectSettingsPanel::onMaterialChanged)
    EVT_CHECKBOX(wxID_ANY, ObjectSettingsPanel::onTextureChanged)
    EVT_CHOICE(wxID_ANY, ObjectSettingsPanel::onTextureChanged)
    EVT_SLIDER(wxID_ANY, ObjectSettingsPanel::onTextureChanged)
END_EVENT_TABLE()

ObjectSettingsPanel::ObjectSettingsPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    LOG_INF_S("ObjectSettingsPanel::ObjectSettingsPanel: Initializing");
    createUI();
    bindEvents();
    loadSettings();
    LOG_INF_S("ObjectSettingsPanel::ObjectSettingsPanel: Initialized successfully");
}

ObjectSettingsPanel::~ObjectSettingsPanel()
{
    LOG_INF_S("ObjectSettingsPanel::~ObjectSettingsPanel: Destroying");
}

void ObjectSettingsPanel::createUI()
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    auto* titleLabel = new wxStaticText(this, wxID_ANY, "Object Settings", 
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    mainSizer->Add(titleLabel, 0, wxALIGN_CENTER | wxALL, 10);
    
    // Description
    auto* descLabel = new wxStaticText(this, wxID_ANY, 
        "These settings affect individual geometric objects.\n"
        "Changes apply only to selected objects.", 
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    mainSizer->Add(descLabel, 0, wxALIGN_CENTER | wxALL, 10);
    
    // Create notebook for tabs
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    // Create tab panels
    auto* materialPanel = new wxPanel(m_notebook, wxID_ANY);
    auto* texturePanel = new wxPanel(m_notebook, wxID_ANY);
    
    // Set up tab panels with their content
    materialPanel->SetSizer(createMaterialTab(materialPanel));
    texturePanel->SetSizer(createTextureTab(texturePanel));
    
    // Add tabs to notebook
    m_notebook->AddPage(materialPanel, "Material");
    m_notebook->AddPage(texturePanel, "Texture");
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);
    
    SetSizer(mainSizer);
}

wxSizer* ObjectSettingsPanel::createMaterialTab(wxWindow* parent)
{
    auto* materialSizer = new wxBoxSizer(wxVERTICAL);
    
    // Ambient
    auto* ambientSizer = new wxBoxSizer(wxHORIZONTAL);
    ambientSizer->Add(new wxStaticText(parent, wxID_ANY, "Ambient:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_ambientSlider = new wxSlider(parent, wxID_ANY, 30, 0, 100, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    ambientSizer->Add(m_ambientSlider, 1, wxEXPAND);
    materialSizer->Add(ambientSizer, 0, wxEXPAND | wxALL, 8);
    
    // Diffuse
    auto* diffuseSizer = new wxBoxSizer(wxHORIZONTAL);
    diffuseSizer->Add(new wxStaticText(parent, wxID_ANY, "Diffuse:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_diffuseSlider = new wxSlider(parent, wxID_ANY, 70, 0, 100, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    diffuseSizer->Add(m_diffuseSlider, 1, wxEXPAND);
    materialSizer->Add(diffuseSizer, 0, wxEXPAND | wxALL, 8);
    
    // Specular
    auto* specularSizer = new wxBoxSizer(wxHORIZONTAL);
    specularSizer->Add(new wxStaticText(parent, wxID_ANY, "Specular:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_specularSlider = new wxSlider(parent, wxID_ANY, 50, 0, 100, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    specularSizer->Add(m_specularSlider, 1, wxEXPAND);
    materialSizer->Add(specularSizer, 0, wxEXPAND | wxALL, 8);
    
    // Shininess
    auto* shininessSizer = new wxBoxSizer(wxHORIZONTAL);
    shininessSizer->Add(new wxStaticText(parent, wxID_ANY, "Shininess:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_shininessSpin = new wxSpinCtrl(parent, wxID_ANY, "32", wxDefaultPosition, wxSize(100, -1), 
        wxSP_ARROW_KEYS, 1, 128, 32);
    shininessSizer->Add(m_shininessSpin, 0);
    materialSizer->Add(shininessSizer, 0, wxEXPAND | wxALL, 8);
    
    // Transparency
    auto* transparencySizer = new wxBoxSizer(wxHORIZONTAL);
    transparencySizer->Add(new wxStaticText(parent, wxID_ANY, "Transparency:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_transparencySlider = new wxSlider(parent, wxID_ANY, 0, 0, 100, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    transparencySizer->Add(m_transparencySlider, 1, wxEXPAND);
    materialSizer->Add(transparencySizer, 0, wxEXPAND | wxALL, 8);
    
    return materialSizer;
}

wxSizer* ObjectSettingsPanel::createTextureTab(wxWindow* parent)
{
    auto* textureSizer = new wxBoxSizer(wxVERTICAL);
    
    // Enable Texture
    m_textureCheckBox = new wxCheckBox(parent, wxID_ANY, "Enable Texture");
    textureSizer->Add(m_textureCheckBox, 0, wxALL, 8);
    
    // Texture Mode
    auto* modeSizer = new wxBoxSizer(wxHORIZONTAL);
    modeSizer->Add(new wxStaticText(parent, wxID_ANY, "Texture Mode:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_textureModeChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
    m_textureModeChoice->Append("Replace");
    m_textureModeChoice->Append("Modulate");
    m_textureModeChoice->Append("Decal");
    m_textureModeChoice->Append("Blend");
    m_textureModeChoice->SetSelection(0);
    modeSizer->Add(m_textureModeChoice, 1, wxEXPAND);
    textureSizer->Add(modeSizer, 0, wxEXPAND | wxALL, 8);
    
    // Texture Scale
    auto* scaleSizer = new wxBoxSizer(wxHORIZONTAL);
    scaleSizer->Add(new wxStaticText(parent, wxID_ANY, "Texture Scale:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    m_textureScaleSlider = new wxSlider(parent, wxID_ANY, 100, 10, 500, 
        wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    scaleSizer->Add(m_textureScaleSlider, 1, wxEXPAND);
    textureSizer->Add(scaleSizer, 0, wxEXPAND | wxALL, 8);
    
    return textureSizer;
}

void ObjectSettingsPanel::bindEvents()
{
    // Material events
    m_ambientSlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onMaterialChanged, this);
    m_diffuseSlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onMaterialChanged, this);
    m_specularSlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onMaterialChanged, this);
    m_shininessSpin->Bind(wxEVT_SPINCTRL, &ObjectSettingsPanel::onMaterialChanged, this);
    m_transparencySlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onMaterialChanged, this);
    
    // Texture events
    m_textureCheckBox->Bind(wxEVT_CHECKBOX, &ObjectSettingsPanel::onTextureChanged, this);
    m_textureModeChoice->Bind(wxEVT_CHOICE, &ObjectSettingsPanel::onTextureChanged, this);
    m_textureScaleSlider->Bind(wxEVT_SLIDER, &ObjectSettingsPanel::onTextureChanged, this);
}

// Material methods
float ObjectSettingsPanel::getAmbient() const
{
    return m_ambientSlider ? m_ambientSlider->GetValue() / 100.0f : 0.3f;
}

float ObjectSettingsPanel::getDiffuse() const
{
    return m_diffuseSlider ? m_diffuseSlider->GetValue() / 100.0f : 0.7f;
}

float ObjectSettingsPanel::getSpecular() const
{
    return m_specularSlider ? m_specularSlider->GetValue() / 100.0f : 0.5f;
}

float ObjectSettingsPanel::getShininess() const
{
    return m_shininessSpin ? static_cast<float>(m_shininessSpin->GetValue()) : 32.0f;
}

float ObjectSettingsPanel::getTransparency() const
{
    return m_transparencySlider ? m_transparencySlider->GetValue() / 100.0f : 0.0f;
}

// Texture methods
bool ObjectSettingsPanel::isTextureEnabled() const
{
    return m_textureCheckBox ? m_textureCheckBox->GetValue() : false;
}

int ObjectSettingsPanel::getTextureMode() const
{
    return m_textureModeChoice ? m_textureModeChoice->GetSelection() : 0;
}

float ObjectSettingsPanel::getTextureScale() const
{
    return m_textureScaleSlider ? m_textureScaleSlider->GetValue() / 100.0f : 1.0f;
}

void ObjectSettingsPanel::onMaterialChanged(wxCommandEvent& event)
{
    // This will be handled by the parent dialog
    // The parent dialog can call getAmbient(), getDiffuse(), etc.
}

void ObjectSettingsPanel::onMaterialChangedSpin(wxSpinEvent& event)
{
    // This will be handled by the parent dialog
    // The parent dialog can call getShininess()
}

void ObjectSettingsPanel::onTextureChanged(wxCommandEvent& event)
{
    // This will be handled by the parent dialog
    // The parent dialog can call isTextureEnabled(), getTextureMode(), getTextureScale()
}

void ObjectSettingsPanel::loadSettings()
{
    try {
        auto& configManager = ConfigManager::getInstance();
        
        // Load material settings
        if (m_ambientSlider) {
            m_ambientSlider->SetValue(configManager.getInt("RenderPreview", "Object.Material.Ambient", 30));
        }
        if (m_diffuseSlider) {
            m_diffuseSlider->SetValue(configManager.getInt("RenderPreview", "Object.Material.Diffuse", 70));
        }
        if (m_specularSlider) {
            m_specularSlider->SetValue(configManager.getInt("RenderPreview", "Object.Material.Specular", 50));
        }
        if (m_shininessSpin) {
            m_shininessSpin->SetValue(configManager.getInt("RenderPreview", "Object.Material.Shininess", 32));
        }
        if (m_transparencySlider) {
            m_transparencySlider->SetValue(configManager.getInt("RenderPreview", "Object.Material.Transparency", 0));
        }
        
        // Load texture settings
        if (m_textureCheckBox) {
            m_textureCheckBox->SetValue(configManager.getBool("RenderPreview", "Object.Texture.Enabled", false));
        }
        if (m_textureModeChoice) {
            m_textureModeChoice->SetSelection(configManager.getInt("RenderPreview", "Object.Texture.Mode", 0));
        }
        if (m_textureScaleSlider) {
            m_textureScaleSlider->SetValue(configManager.getInt("RenderPreview", "Object.Texture.Scale", 100));
        }
        
        LOG_INF_S("ObjectSettingsPanel::loadSettings: Settings loaded successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("ObjectSettingsPanel::loadSettings: Failed to load settings: " + std::string(e.what()));
    }
}

void ObjectSettingsPanel::saveSettings()
{
    try {
        auto& configManager = ConfigManager::getInstance();
        
        // Save material settings
        if (m_ambientSlider) {
            configManager.setInt("RenderPreview", "Object.Material.Ambient", m_ambientSlider->GetValue());
        }
        if (m_diffuseSlider) {
            configManager.setInt("RenderPreview", "Object.Material.Diffuse", m_diffuseSlider->GetValue());
        }
        if (m_specularSlider) {
            configManager.setInt("RenderPreview", "Object.Material.Specular", m_specularSlider->GetValue());
        }
        if (m_shininessSpin) {
            configManager.setInt("RenderPreview", "Object.Material.Shininess", m_shininessSpin->GetValue());
        }
        if (m_transparencySlider) {
            configManager.setInt("RenderPreview", "Object.Material.Transparency", m_transparencySlider->GetValue());
        }
        
        // Save texture settings
        if (m_textureCheckBox) {
            configManager.setBool("RenderPreview", "Object.Texture.Enabled", m_textureCheckBox->GetValue());
        }
        if (m_textureModeChoice) {
            configManager.setInt("RenderPreview", "Object.Texture.Mode", m_textureModeChoice->GetSelection());
        }
        if (m_textureScaleSlider) {
            configManager.setInt("RenderPreview", "Object.Texture.Scale", m_textureScaleSlider->GetValue());
        }
        
        configManager.save();
        LOG_INF_S("ObjectSettingsPanel::saveSettings: Settings saved successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("ObjectSettingsPanel::saveSettings: Failed to save settings: " + std::string(e.what()));
    }
}

void ObjectSettingsPanel::resetToDefaults()
{
    // Reset material settings
    if (m_ambientSlider) {
        m_ambientSlider->SetValue(30);
    }
    if (m_diffuseSlider) {
        m_diffuseSlider->SetValue(70);
    }
    if (m_specularSlider) {
        m_specularSlider->SetValue(50);
    }
    if (m_shininessSpin) {
        m_shininessSpin->SetValue(32);
    }
    if (m_transparencySlider) {
        m_transparencySlider->SetValue(0);
    }
    
    // Reset texture settings
    if (m_textureCheckBox) {
        m_textureCheckBox->SetValue(false);
    }
    if (m_textureModeChoice) {
        m_textureModeChoice->SetSelection(0);
    }
    if (m_textureScaleSlider) {
        m_textureScaleSlider->SetValue(100);
    }
    
    LOG_INF_S("ObjectSettingsPanel::resetToDefaults: Settings reset to defaults");
} 