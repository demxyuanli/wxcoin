#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/PreviewCanvas.h"
#include "renderpreview/GlobalSettingsPanel.h"
#include "renderpreview/ObjectSettingsPanel.h"
#include "renderpreview/ConfigValidator.h"
#include "renderpreview/UndoManager.h"
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
    EVT_BUTTON(wxID_UNDO, RenderPreviewDialog::OnUndo)
    EVT_BUTTON(wxID_REDO, RenderPreviewDialog::OnRedo)
wxEND_EVENT_TABLE()

RenderPreviewDialog::RenderPreviewDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Render Preview System"), wxDefaultPosition, wxSize(1200, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLOSE_BOX | wxCAPTION)
    , m_currentLightIndex(-1)
    , m_undoManager(std::make_unique<UndoManager>())
    , m_autoApply(false)
    , m_validationEnabled(true)
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
    
    // Bottom section: Buttons and options in a row (fixed 40px height)
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_RESET, wxT("Reset")), 0, wxALL, 2);
    buttonSizer->Add(new wxButton(this, wxID_UNDO, wxT("Undo")), 0, wxALL, 2);
    buttonSizer->Add(new wxButton(this, wxID_REDO, wxT("Redo")), 0, wxALL, 2);
    buttonSizer->AddStretchSpacer();
    
    // Auto-apply checkbox
    auto* autoApplyCheckBox = new wxCheckBox(this, wxID_ANY, wxT("Auto Apply"));
    autoApplyCheckBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
        setAutoApply(event.IsChecked());
    });
    buttonSizer->Add(autoApplyCheckBox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2);
    
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
    // Save current state before applying changes
    saveCurrentState("Apply Settings");
    
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
    
    // Validate settings if validation is enabled
    if (m_validationEnabled) {
        if (!validateCurrentSettings()) {
            return;
        }
    }
    
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
        // Use multi-light support
        m_renderCanvas->updateMultiLighting(lights);
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
    
    // Apply lighting from global settings panel using multi-light support
    auto lights = m_globalSettingsPanel->getLights();
    m_renderCanvas->updateMultiLighting(lights);
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

// New feature implementations
void RenderPreviewDialog::OnUndo(wxCommandEvent& event)
{
    if (m_undoManager && m_undoManager->canUndo()) {
        auto snapshot = m_undoManager->undo();
        applySnapshot(snapshot);
        wxMessageBox(wxT("Undo: ") + wxString(m_undoManager->getUndoDescription()), wxT("Undo"), wxOK | wxICON_INFORMATION);
    }
}

void RenderPreviewDialog::OnRedo(wxCommandEvent& event)
{
    if (m_undoManager && m_undoManager->canRedo()) {
        auto snapshot = m_undoManager->redo();
        applySnapshot(snapshot);
        wxMessageBox(wxT("Redo: ") + wxString(m_undoManager->getRedoDescription()), wxT("Redo"), wxOK | wxICON_INFORMATION);
    }
}

void RenderPreviewDialog::saveCurrentState(const std::string& description)
{
    if (m_undoManager) {
        auto snapshot = createSnapshot();
        m_undoManager->saveState(snapshot, description);
    }
}

void RenderPreviewDialog::applySnapshot(const ConfigSnapshot& snapshot)
{
    // Apply lighting settings
    if (m_globalSettingsPanel) {
        m_globalSettingsPanel->setLights(snapshot.lights);
    }
    
    // Apply anti-aliasing settings
    if (m_globalSettingsPanel) {
        m_globalSettingsPanel->setAntiAliasingMethod(snapshot.antiAliasingMethod);
        m_globalSettingsPanel->setMSAASamples(snapshot.msaaSamples);
        m_globalSettingsPanel->setFXAAEnabled(snapshot.fxaaEnabled);
    }
    
    // Apply rendering mode
    if (m_globalSettingsPanel) {
        m_globalSettingsPanel->setRenderingMode(snapshot.renderingMode);
    }
    
    // Apply object settings
    if (m_objectSettingsPanel) {
        // Note: These would need to be added to ObjectSettingsPanel
        // m_objectSettingsPanel->setAmbient(snapshot.materialAmbient);
        // m_objectSettingsPanel->setDiffuse(snapshot.materialDiffuse);
        // m_objectSettingsPanel->setSpecular(snapshot.materialSpecular);
        // m_objectSettingsPanel->setShininess(snapshot.materialShininess);
        // m_objectSettingsPanel->setTransparency(snapshot.materialTransparency);
        // m_objectSettingsPanel->setTextureEnabled(snapshot.textureEnabled);
        // m_objectSettingsPanel->setTextureMode(snapshot.textureMode);
        // m_objectSettingsPanel->setTextureScale(snapshot.textureScale);
    }
    
    // Apply to canvas
    applyGlobalSettingsToCanvas();
    applyObjectSettingsToCanvas();
}

ConfigSnapshot RenderPreviewDialog::createSnapshot() const
{
    ConfigSnapshot snapshot;
    
    // Get lighting settings
    if (m_globalSettingsPanel) {
        snapshot.lights = m_globalSettingsPanel->getLights();
        snapshot.antiAliasingMethod = m_globalSettingsPanel->getAntiAliasingMethod();
        snapshot.msaaSamples = m_globalSettingsPanel->getMSAASamples();
        snapshot.fxaaEnabled = m_globalSettingsPanel->isFXAAEnabled();
        snapshot.renderingMode = m_globalSettingsPanel->getRenderingMode();
    }
    
    // Get object settings
    if (m_objectSettingsPanel) {
        snapshot.materialAmbient = m_objectSettingsPanel->getAmbient();
        snapshot.materialDiffuse = m_objectSettingsPanel->getDiffuse();
        snapshot.materialSpecular = m_objectSettingsPanel->getSpecular();
        snapshot.materialShininess = m_objectSettingsPanel->getShininess();
        snapshot.materialTransparency = m_objectSettingsPanel->getTransparency();
        snapshot.textureEnabled = m_objectSettingsPanel->isTextureEnabled();
        snapshot.textureMode = m_objectSettingsPanel->getTextureMode();
        snapshot.textureScale = m_objectSettingsPanel->getTextureScale();
    }
    
    return snapshot;
}

bool RenderPreviewDialog::validateCurrentSettings()
{
    if (!m_globalSettingsPanel) return false;
    
    // Validate lighting settings
    auto lights = m_globalSettingsPanel->getLights();
    auto lightValidation = ConfigValidator::validateLightSettings(lights);
    if (!lightValidation.isValid) {
        wxMessageBox(wxString(lightValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
        return false;
    }
    
    // Validate anti-aliasing settings
    int method = m_globalSettingsPanel->getAntiAliasingMethod();
    int msaaSamples = m_globalSettingsPanel->getMSAASamples();
    bool fxaaEnabled = m_globalSettingsPanel->isFXAAEnabled();
    auto aaValidation = ConfigValidator::validateAntiAliasingSettings(method, msaaSamples, fxaaEnabled);
    if (!aaValidation.isValid) {
        wxMessageBox(wxString(aaValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
        return false;
    }
    
    // Validate rendering mode
    int mode = m_globalSettingsPanel->getRenderingMode();
    auto modeValidation = ConfigValidator::validateRenderingMode(mode);
    if (!modeValidation.isValid) {
        wxMessageBox(wxString(modeValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
        return false;
    }
    
    // Validate object settings
    if (m_objectSettingsPanel) {
        float ambient = m_objectSettingsPanel->getAmbient();
        float diffuse = m_objectSettingsPanel->getDiffuse();
        float specular = m_objectSettingsPanel->getSpecular();
        float shininess = m_objectSettingsPanel->getShininess();
        float transparency = m_objectSettingsPanel->getTransparency();
        auto materialValidation = ConfigValidator::validateMaterialSettings(ambient, diffuse, specular, shininess, transparency);
        if (!materialValidation.isValid) {
            wxMessageBox(wxString(materialValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
            return false;
        }
        
        bool textureEnabled = m_objectSettingsPanel->isTextureEnabled();
        int textureMode = m_objectSettingsPanel->getTextureMode();
        float textureScale = m_objectSettingsPanel->getTextureScale();
        auto textureValidation = ConfigValidator::validateTextureSettings(textureEnabled, textureMode, textureScale);
        if (!textureValidation.isValid) {
            wxMessageBox(wxString(textureValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
            return false;
        }
    }
    
    return true;
}

void RenderPreviewDialog::setAutoApply(bool enabled)
{
    m_autoApply = enabled;
    
    // Set auto-apply mode for panels
    if (m_globalSettingsPanel) {
        m_globalSettingsPanel->setAutoApply(enabled);
    }
    if (m_objectSettingsPanel) {
        // Note: ObjectSettingsPanel would need similar setAutoApply method
        // m_objectSettingsPanel->setAutoApply(enabled);
    }
}

void RenderPreviewDialog::setValidationEnabled(bool enabled)
{
    m_validationEnabled = enabled;
}
