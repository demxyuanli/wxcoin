#include "renderpreview/RenderingModePanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/RenderingManager.h"
#include "renderpreview/PreviewCanvas.h"
#include "logger/Logger.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/config.h>
#include <wx/fileconf.h>

BEGIN_EVENT_TABLE(RenderingModePanel, wxPanel)
    EVT_CHOICE(wxID_ANY, RenderingModePanel::onRenderingModeChanged)
END_EVENT_TABLE()

RenderingModePanel::RenderingModePanel(wxWindow* parent, RenderPreviewDialog* dialog)
    : wxPanel(parent, wxID_ANY), m_parentDialog(dialog), m_renderingManager(nullptr)
{
    createUI();
    bindEvents();
    loadSettings();
}

RenderingModePanel::~RenderingModePanel()
{
}

void RenderingModePanel::createUI()
{
    auto* renderingSizer = new wxBoxSizer(wxVERTICAL);

    auto* presetsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Rendering Presets");
    auto* presetsLabel = new wxStaticText(this, wxID_ANY, "Choose a rendering preset:");
    presetsBoxSizer->Add(presetsLabel, 0, wxALL, 8);
    m_renderingModeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(300, -1));
    m_renderingModeChoice->Append("None");
    m_renderingModeChoice->Append("Performance");
    m_renderingModeChoice->Append("Balanced");
    m_renderingModeChoice->Append("Quality");
    m_renderingModeChoice->Append("Ultra");
    m_renderingModeChoice->Append("Wireframe");
    m_renderingModeChoice->Append("CAD Standard");
    m_renderingModeChoice->Append("CAD High Quality");
    m_renderingModeChoice->Append("CAD Wireframe");
    m_renderingModeChoice->Append("Gaming Fast");
    m_renderingModeChoice->Append("Gaming Balanced");
    m_renderingModeChoice->Append("Gaming Quality");
    m_renderingModeChoice->Append("Mobile Low");
    m_renderingModeChoice->Append("Mobile Medium");
    m_renderingModeChoice->Append("Presentation Standard");
    m_renderingModeChoice->Append("Presentation High");
    m_renderingModeChoice->Append("Debug Wireframe");
    m_renderingModeChoice->Append("Debug Points");
    m_renderingModeChoice->Append("Legacy Solid");
    m_renderingModeChoice->Append("Legacy Hidden Line");
    m_renderingModeChoice->SetSelection(0);
    presetsBoxSizer->Add(m_renderingModeChoice, 0, wxEXPAND | wxALL, 8);
    renderingSizer->Add(presetsBoxSizer, 0, wxEXPAND | wxALL, 8);

    auto* performanceBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Performance Impact");
    auto* impactLabel = new wxStaticText(this, wxID_ANY, "Performance Impact: Medium");
    impactLabel->SetForegroundColour(wxColour(255, 165, 0));
    performanceBoxSizer->Add(impactLabel, 0, wxALL, 8);
    auto* qualityLabel = new wxStaticText(this, wxID_ANY, "Quality: Balanced");
    qualityLabel->SetForegroundColour(wxColour(0, 0, 128));
    performanceBoxSizer->Add(qualityLabel, 0, wxALL, 8);
    auto* fpsLabel = new wxStaticText(this, wxID_ANY, "Estimated FPS: 60");
    fpsLabel->SetForegroundColour(wxColour(0, 128, 0));
    performanceBoxSizer->Add(fpsLabel, 0, wxALL, 8);
    auto* featuresLabel = new wxStaticText(this, wxID_ANY, "Features: Smooth Shading, Phong Shading");
    featuresLabel->SetForegroundColour(wxColour(128, 128, 128));
    performanceBoxSizer->Add(featuresLabel, 0, wxALL, 8);
    renderingSizer->Add(performanceBoxSizer, 0, wxEXPAND | wxALL, 8);

    auto* legacyBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Legacy Mode Settings");
    auto* modeLabel = new wxStaticText(this, wxID_ANY, "Legacy Rendering Mode:");
    legacyBoxSizer->Add(modeLabel, 0, wxALL, 8);
    m_legacyChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
    m_legacyChoice->Append("Solid");
    m_legacyChoice->Append("Wireframe");
    m_legacyChoice->Append("Points");
    m_legacyChoice->Append("Hidden Line");
    m_legacyChoice->Append("Shaded");
    m_legacyChoice->SetSelection(4);
    m_legacyChoice->Enable(false);
    legacyBoxSizer->Add(m_legacyChoice, 0, wxEXPAND | wxALL, 8);
    renderingSizer->Add(legacyBoxSizer, 0, wxEXPAND | wxALL, 8);

    SetSizer(renderingSizer);
}

void RenderingModePanel::bindEvents()
{
    m_renderingModeChoice->Bind(wxEVT_CHOICE, &RenderingModePanel::onRenderingModeChanged, this);
    if (m_legacyChoice) {
        m_legacyChoice->Bind(wxEVT_CHOICE, &RenderingModePanel::onLegacyModeChanged, this);
    }
}

int RenderingModePanel::getRenderingMode() const
{
    return m_renderingModeChoice ? m_renderingModeChoice->GetSelection() : 4;
}

void RenderingModePanel::setRenderingMode(int mode)
{
    if (m_renderingModeChoice && mode >= 0 && mode < m_renderingModeChoice->GetCount()) {
        m_renderingModeChoice->SetSelection(mode);
    }
}

void RenderingModePanel::setRenderingManager(RenderingManager* manager)
{
    m_renderingManager = manager;
}

void RenderingModePanel::onRenderingModeChanged(wxCommandEvent& event)
{
    if (m_renderingManager && m_renderingModeChoice) {
        int selection = m_renderingModeChoice->GetSelection();
        wxString presetName = m_renderingModeChoice->GetString(selection);
        if (presetName == "None") {
            if (m_legacyChoice) {
                m_legacyChoice->Enable(true);
            }
            m_renderingManager->setActiveConfiguration(-1);
            LOG_INF_S("RenderingModePanel::onRenderingModeChanged: Selected None - Legacy mode enabled");
        } else {
            m_renderingManager->applyPreset(presetName.ToStdString());
            m_renderingManager->setupRenderingState();
            updateLegacyChoiceFromCurrentMode();
            if (m_legacyChoice) {
                m_legacyChoice->Enable(false);
            }
        }
        float impact = m_renderingManager->getPerformanceImpact();
        std::string qualityDesc = m_renderingManager->getQualityDescription();
        int estimatedFPS = m_renderingManager->getEstimatedFPS();
        wxWindow* parent = m_renderingModeChoice->GetParent();
        if (parent) {
            wxWindowList children = parent->GetChildren();
            for (auto child : children) {
                if (wxStaticText* label = dynamic_cast<wxStaticText*>(child)) {
                    wxString labelText = label->GetLabel();
                    if (labelText.Contains("Performance Impact:")) {
                        std::string impactText = "Performance Impact: ";
                        if (impact < 1.2f) {
                            impactText += "Low";
                            label->SetForegroundColour(wxColour(0, 128, 0));
                        } else if (impact < 1.8f) {
                            impactText += "Medium";
                            label->SetForegroundColour(wxColour(255, 165, 0));
                        } else {
                            impactText += "High";
                            label->SetForegroundColour(wxColour(255, 0, 0));
                        }
                        label->SetLabel(impactText);
                    } else if (labelText.Contains("Quality:")) {
                        label->SetLabel("Quality: " + qualityDesc);
                    } else if (labelText.Contains("Estimated FPS:")) {
                        label->SetLabel("Estimated FPS: " + std::to_string(estimatedFPS));
                    } else if (labelText.Contains("Features:")) {
                        std::string featuresText = "Features: ";
                        if (presetName.Contains("CAD")) {
                            featuresText += "CAD Optimized, High Precision";
                        } else if (presetName.Contains("Gaming")) {
                            featuresText += "Real-time Optimized, Fast Rendering";
                        } else if (presetName.Contains("Mobile")) {
                            featuresText += "Mobile Optimized, Battery Efficient";
                        } else if (presetName.Contains("Presentation")) {
                            featuresText += "High Quality, Smooth Shading";
                        } else if (presetName.Contains("Debug")) {
                            featuresText += "Debug Mode, Wireframe/Points";
                        } else {
                            featuresText += "Standard Rendering Features";
                        }
                        label->SetLabel(featuresText);
                    }
                }
            }
        }
        LOG_INF_S("RenderingModePanel::onRenderingModeChanged: Applied preset '" + presetName.ToStdString() + "'");
    }
    if (m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void RenderingModePanel::onLegacyModeChanged(wxCommandEvent& event)
{
    if (!m_legacyChoice) {
        return;
    }
    if (!m_legacyChoice->IsEnabled()) {
        LOG_INF_S("RenderingModePanel::onLegacyModeChanged: Legacy choice is disabled - ignoring change");
        return;
    }
    int selection = m_legacyChoice->GetSelection();
    LOG_INF_S("RenderingModePanel::onLegacyModeChanged: Legacy mode changed to selection " + std::to_string(selection));
    int renderingMode = 0;
    switch (selection) {
        case 0: renderingMode = 0; break;
        case 1: renderingMode = 1; break;
        case 2: renderingMode = 2; break;
        case 3: renderingMode = 3; break;
        case 4: renderingMode = 4; break;
        default: renderingMode = 0; break;
    }
    if (m_renderingManager) {
        if (m_renderingManager->hasActiveConfiguration()) {
            int activeConfigId = m_renderingManager->getActiveConfigurationId();
            RenderingSettings settings = m_renderingManager->getConfiguration(activeConfigId);
            settings.mode = renderingMode;
            switch (renderingMode) {
                case 0: settings.polygonMode = 0; break;
                case 1: settings.polygonMode = 1; break;
                case 2: settings.polygonMode = 2; break;
                case 3: settings.polygonMode = 0; break;
                case 4: settings.polygonMode = 0; break;
            }
            m_renderingManager->updateConfiguration(activeConfigId, settings);
            m_renderingManager->setupRenderingState();
        }
    }
    if (m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void RenderingModePanel::updateLegacyChoiceFromCurrentMode()
{
    if (m_renderingManager && m_legacyChoice && m_renderingManager->hasActiveConfiguration()) {
        int activeConfigId = m_renderingManager->getActiveConfigurationId();
        RenderingSettings settings = m_renderingManager->getConfiguration(activeConfigId);
        int selection = 0;
        switch (settings.mode) {
            case 0: selection = 0; break;
            case 1: selection = 1; break;
            case 2: selection = 2; break;
            case 3: selection = 3; break;
            case 4: selection = 4; break;
        }
        m_legacyChoice->SetSelection(selection);
    }
}

void RenderingModePanel::loadSettings()
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString exeDir = exeFile.GetPath();
        wxString renderConfigPath = exeDir + wxFileName::GetPathSeparator() + "render_settings.ini";
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, renderConfigPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);

        if (this->m_renderingModeChoice) {
            int mode;
            if (renderConfig.Read("Global.RenderingMode", &mode, 1)) {
                if (mode >= 0 && mode < this->m_renderingModeChoice->GetCount()) {
                    this->m_renderingModeChoice->SetSelection(mode);
                } else {
                    this->m_renderingModeChoice->SetSelection(1);
                    LOG_WRN_S("RenderingModePanel::loadSettings: Invalid rendering mode " + std::to_string(mode) + ", defaulting to Balanced");
                }
            }
        }
        LOG_INF_S("RenderingModePanel::loadSettings: Settings loaded successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("RenderingModePanel::loadSettings: Failed to load settings: " + std::string(e.what()));
    }
}

void RenderingModePanel::saveSettings()
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString exeDir = exeFile.GetPath();
        wxString renderConfigPath = exeDir + wxFileName::GetPathSeparator() + "render_settings.ini";
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, renderConfigPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);

        if (this->m_renderingModeChoice) {
            renderConfig.Write("Global.RenderingMode", this->m_renderingModeChoice->GetSelection());
        }
        renderConfig.Flush();
        LOG_INF_S("RenderingModePanel::saveSettings: Settings saved successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("RenderingModePanel::saveSettings: Failed to save settings: " + std::string(e.what()));
    }
}

void RenderingModePanel::resetToDefaults()
{
    if (this->m_renderingModeChoice) {
        this->m_renderingModeChoice->SetSelection(4);
    }
    LOG_INF_S("RenderingModePanel::resetToDefaults: Settings reset to defaults");
}