#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/PreviewCanvas.h"
#include "renderpreview/GlobalSettingsPanel.h"
#include "renderpreview/ObjectSettingsPanel.h"
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
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/scrolwin.h>
#include <wx/msgdlg.h>
#include <wx/grid.h>
#include <wx/font.h>
#include <fstream>
#include <sstream>

wxBEGIN_EVENT_TABLE(RenderPreviewDialog, wxDialog)
    EVT_BUTTON(wxID_RESET, RenderPreviewDialog::OnReset)
    EVT_BUTTON(wxID_APPLY, RenderPreviewDialog::OnApply)
    EVT_BUTTON(wxID_SAVE, RenderPreviewDialog::OnSave)
    EVT_BUTTON(wxID_CANCEL, RenderPreviewDialog::OnCancel)
    EVT_CLOSE(RenderPreviewDialog::OnClose)
wxEND_EVENT_TABLE()

RenderPreviewDialog::RenderPreviewDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Render Preview System"), wxDefaultPosition, wxSize(1200, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLOSE_BOX | wxCAPTION)
    , m_currentLightIndex(-1)
{
    LOG_INF_S("RenderPreviewDialog::RenderPreviewDialog: Initializing");
    createUI();
    loadConfiguration();
    SetSizer(GetSizer());
    
    // Set fixed size to ensure proper layout
    SetSize(1200, 700);
    SetMinSize(wxSize(1200, 700));
    
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
    
    // Left panel: Configuration tabs (fixed 450px width)
    auto* leftPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(450, -1), wxBORDER_SUNKEN);
    auto* configNotebook = new wxNotebook(leftPanel, wxID_ANY);
    
    // Create panel instances
    m_globalSettingsPanel = new GlobalSettingsPanel(configNotebook, this);
    m_objectSettingsPanel = new ObjectSettingsPanel(configNotebook);
    
    // Add panels to notebook with clear categorization
    configNotebook->AddPage(m_globalSettingsPanel, wxT("Global Settings"));
    configNotebook->AddPage(m_objectSettingsPanel, wxT("Object Settings"));
    
    auto* leftSizer = new wxBoxSizer(wxVERTICAL);
    leftSizer->Add(configNotebook, 1, wxEXPAND | wxALL, 2);
    leftPanel->SetSizer(leftSizer);
    
    // Right panel: Render preview canvas (adaptive width)
    m_renderCanvas = new PreviewCanvas(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    
    topSizer->Add(leftPanel, 0, wxEXPAND | wxALL, 2);
    topSizer->Add(m_renderCanvas, 1, wxEXPAND | wxALL, 2);
    
    mainSizer->Add(topSizer, 1, wxEXPAND | wxALL, 2);
    
    // Bottom section: Buttons in a row (fixed 40px height)
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_RESET, wxT("Reset")), 0, wxALL, 2);
    buttonSizer->AddStretchSpacer();
    m_applyButton = new wxButton(this, wxID_APPLY, wxT("Apply"));
    buttonSizer->Add(m_applyButton, 0, wxALL, 2);
    buttonSizer->Add(new wxButton(this, wxID_SAVE, wxT("Save")), 0, wxALL, 2);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, wxT("Cancel")), 0, wxALL, 2);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 2);
    
    SetSizer(mainSizer);
}

















void RenderPreviewDialog::OnReset(wxCommandEvent& event)
{
    resetToDefaults();
    if (m_renderCanvas) {
        m_renderCanvas->resetView();
        m_renderCanvas->render(false);
    }
}

void RenderPreviewDialog::OnApply(wxCommandEvent& event)
{
    // Apply all current settings to the preview canvas
    applyGlobalSettingsToCanvas();
    applyObjectSettingsToCanvas();
    
    // Show feedback to user
    wxMessageBox(wxT("Settings applied to preview successfully!"), wxT("Apply"), wxOK | wxICON_INFORMATION);
    
    LOG_INF_S("RenderPreviewDialog::OnApply: Settings applied to preview");
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
        
        // Save global settings
        if (m_globalSettingsPanel) {
            m_globalSettingsPanel->saveSettings();
        }
        
        // Save object settings
        if (m_objectSettingsPanel) {
            m_objectSettingsPanel->saveSettings();
        }
        
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
        
        // Load global settings
        if (m_globalSettingsPanel) {
            m_globalSettingsPanel->loadSettings();
        }
        
        // Load object settings
        if (m_objectSettingsPanel) {
            m_objectSettingsPanel->loadSettings();
        }
        
        LOG_INF_S("RenderPreviewDialog::loadConfiguration: Configuration loaded successfully");
        
        // Initialize light management if not already done
        if (m_lights.empty()) {
            RenderLightSettings defaultLight;
            m_lights.push_back(defaultLight);
            updateLightList();
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("RenderPreviewDialog::loadConfiguration: Failed to load configuration: " + std::string(e.what()));
    }
}

void RenderPreviewDialog::resetToDefaults()
{
    // Reset global settings
    if (m_globalSettingsPanel) {
        m_globalSettingsPanel->resetToDefaults();
    }
    
    // Reset object settings
    if (m_objectSettingsPanel) {
        m_objectSettingsPanel->resetToDefaults();
    }
    
    LOG_INF_S("RenderPreviewDialog::resetToDefaults: Configuration reset to defaults");
}

// Global settings methods
void RenderPreviewDialog::applyGlobalSettingsToCanvas()
{
    if (!m_globalSettingsPanel || !m_renderCanvas) return;
    
    // Get lighting settings from global panel
    auto lights = m_globalSettingsPanel->getLights();
    
    // Apply anti-aliasing settings
    int antiAliasingMethod = m_globalSettingsPanel->getAntiAliasingMethod();
    int msaaSamples = m_globalSettingsPanel->getMSAASamples();
    bool fxaaEnabled = m_globalSettingsPanel->isFXAAEnabled();
    
    // Apply rendering mode
    int renderingMode = m_globalSettingsPanel->getRenderingMode();
    
    // Apply to canvas using update methods
    if (!lights.empty()) {
        const auto& firstLight = lights[0];
        m_renderCanvas->updateLighting(0.2f, 0.8f, 0.6f, firstLight.color, static_cast<float>(firstLight.intensity));
    }
    m_renderCanvas->updateAntiAliasing(antiAliasingMethod, msaaSamples, fxaaEnabled);
    m_renderCanvas->updateRenderingMode(renderingMode);
    
    LOG_INF_S("RenderPreviewDialog::applyGlobalSettingsToCanvas: Applied global settings to canvas");
}

void RenderPreviewDialog::updateLightList()
{
    // This method is now handled by GlobalSettingsPanel
    // The GlobalSettingsPanel manages its own light list
    LOG_INF_S("RenderPreviewDialog::updateLightList: Light list management moved to GlobalSettingsPanel");
}

void RenderPreviewDialog::onGlobalLightingChanged(wxCommandEvent& event)
{
    updateGlobalLighting();
}

void RenderPreviewDialog::onGlobalAntiAliasingChanged(wxCommandEvent& event)
{
    updateGlobalAntiAliasing();
}

void RenderPreviewDialog::onGlobalRenderingModeChanged(wxCommandEvent& event)
{
    updateGlobalRenderingMode();
}

void RenderPreviewDialog::onObjectMaterialChanged(wxCommandEvent& event)
{
    applyObjectSettingsToCanvas();
}

void RenderPreviewDialog::onObjectTextureChanged(wxCommandEvent& event)
{
    applyObjectSettingsToCanvas();
}

void RenderPreviewDialog::updateGlobalLighting()
{
    if (!m_renderCanvas || !m_globalSettingsPanel) return;
    
    // Apply lighting from global settings panel
    auto lights = m_globalSettingsPanel->getLights();
    for (const auto& light : lights) {
        if (light.enabled) {
            float intensity = static_cast<float>(light.intensity);
            m_renderCanvas->updateLighting(0.2f, 0.8f, 0.6f, light.color, intensity);
            break; // Use first enabled light for now
        }
    }
}

void RenderPreviewDialog::updateGlobalAntiAliasing()
{
    if (!m_renderCanvas || !m_globalSettingsPanel) return;
    
    int method = m_globalSettingsPanel->getAntiAliasingMethod();
    int msaaSamples = m_globalSettingsPanel->getMSAASamples();
    bool fxaaEnabled = m_globalSettingsPanel->isFXAAEnabled();
    
    m_renderCanvas->updateAntiAliasing(method, msaaSamples, fxaaEnabled);
}

void RenderPreviewDialog::updateGlobalRenderingMode()
{
    if (!m_renderCanvas || !m_globalSettingsPanel) return;
    
    int mode = m_globalSettingsPanel->getRenderingMode();
    m_renderCanvas->updateRenderingMode(mode);
}

void RenderPreviewDialog::applyObjectSettingsToCanvas()
{
    if (!m_renderCanvas) return;
    
    // Apply material settings
    if (m_objectSettingsPanel) {
        float ambient = m_objectSettingsPanel->getAmbient();
        float diffuse = m_objectSettingsPanel->getDiffuse();
        float specular = m_objectSettingsPanel->getSpecular();
        float shininess = m_objectSettingsPanel->getShininess();
        float transparency = m_objectSettingsPanel->getTransparency();
        
        m_renderCanvas->updateMaterial(ambient, diffuse, specular, shininess, transparency);
    }
    
    // Apply texture settings
    if (m_objectSettingsPanel) {
        bool enabled = m_objectSettingsPanel->isTextureEnabled();
        int mode = m_objectSettingsPanel->getTextureMode();
        float scale = m_objectSettingsPanel->getTextureScale();
        
        m_renderCanvas->updateTexture(enabled, mode, scale);
    }
    
    if (m_renderCanvas) {
        m_renderCanvas->render(false);
    }
}
