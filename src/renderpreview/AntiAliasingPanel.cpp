#include "renderpreview/AntiAliasingPanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/AntiAliasingManager.h"
#include "logger/Logger.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/config.h>

BEGIN_EVENT_TABLE(AntiAliasingPanel, wxPanel)
    EVT_CHOICE(wxID_ANY, AntiAliasingPanel::onAntiAliasingChanged)
    EVT_SLIDER(wxID_ANY, AntiAliasingPanel::onAntiAliasingChanged)
    EVT_CHECKBOX(wxID_ANY, AntiAliasingPanel::onAntiAliasingChanged)
END_EVENT_TABLE()

AntiAliasingPanel::AntiAliasingPanel(wxWindow* parent, RenderPreviewDialog* dialog)
    : wxPanel(parent, wxID_ANY), m_parentDialog(dialog), m_antiAliasingManager(nullptr)
{
    createUI();
    bindEvents();
    loadSettings();
}

AntiAliasingPanel::~AntiAliasingPanel()
{
}

void AntiAliasingPanel::createUI()
{
    auto* antiAliasingSizer = new wxBoxSizer(wxVERTICAL);

    auto* presetsBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Anti-aliasing Presets");
    auto* presetsLabel = new wxStaticText(this, wxID_ANY, "Choose an anti-aliasing preset:");
    presetsBoxSizer->Add(presetsLabel, 0, wxALL, 8);
    m_antiAliasingChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(300, -1));
    m_antiAliasingChoice->Append("None");
    m_antiAliasingChoice->Append("MSAA 2x");
    m_antiAliasingChoice->Append("MSAA 4x");
    m_antiAliasingChoice->Append("MSAA 8x");
    m_antiAliasingChoice->Append("MSAA 16x");
    m_antiAliasingChoice->Append("FXAA Low");
    m_antiAliasingChoice->Append("FXAA Medium");
    m_antiAliasingChoice->Append("FXAA High");
    m_antiAliasingChoice->Append("FXAA Ultra");
    m_antiAliasingChoice->Append("SSAA 2x");
    m_antiAliasingChoice->Append("SSAA 4x");
    m_antiAliasingChoice->Append("TAA Low");
    m_antiAliasingChoice->Append("TAA Medium");
    m_antiAliasingChoice->Append("TAA High");
    m_antiAliasingChoice->Append("MSAA 4x + FXAA");
    m_antiAliasingChoice->Append("MSAA 4x + TAA");
    m_antiAliasingChoice->Append("Performance Low");
    m_antiAliasingChoice->Append("Performance Medium");
    m_antiAliasingChoice->Append("Quality High");
    m_antiAliasingChoice->Append("Quality Ultra");
    m_antiAliasingChoice->Append("CAD Standard");
    m_antiAliasingChoice->Append("CAD High Quality");
    m_antiAliasingChoice->Append("Gaming Fast");
    m_antiAliasingChoice->Append("Gaming Balanced");
    m_antiAliasingChoice->Append("Mobile Low");
    m_antiAliasingChoice->Append("Mobile Medium");
    m_antiAliasingChoice->SetSelection(2);
    presetsBoxSizer->Add(m_antiAliasingChoice, 0, wxEXPAND | wxALL, 8);
    antiAliasingSizer->Add(presetsBoxSizer, 0, wxEXPAND | wxALL, 8);

    auto* performanceBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Performance Impact");
    auto* impactLabel = new wxStaticText(this, wxID_ANY, "Performance Impact: Low");
    impactLabel->SetForegroundColour(wxColour(0, 128, 0));
    performanceBoxSizer->Add(impactLabel, 0, wxALL, 8);
    auto* qualityLabel = new wxStaticText(this, wxID_ANY, "Quality: MSAA 4x");
    qualityLabel->SetForegroundColour(wxColour(0, 0, 128));
    performanceBoxSizer->Add(qualityLabel, 0, wxALL, 8);
    auto* fpsLabel = new wxStaticText(this, wxID_ANY, "Estimated FPS: 60");
    fpsLabel->SetForegroundColour(wxColour(0, 128, 0));
    performanceBoxSizer->Add(fpsLabel, 0, wxALL, 8);
    antiAliasingSizer->Add(performanceBoxSizer, 0, wxEXPAND | wxALL, 8);

    auto* legacyBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Advanced Settings");
    auto* msaaLabel = new wxStaticText(this, wxID_ANY, "MSAA Samples:");
    legacyBoxSizer->Add(msaaLabel, 0, wxALL, 8);
    m_msaaSamplesSlider = new wxSlider(this, wxID_ANY, 4, 2, 16, wxDefaultPosition, wxSize(250, -1), wxSL_HORIZONTAL | wxSL_LABELS);
    legacyBoxSizer->Add(m_msaaSamplesSlider, 0, wxEXPAND | wxALL, 8);
    m_fxaaCheckBox = new wxCheckBox(this, wxID_ANY, "Enable FXAA");
    legacyBoxSizer->Add(m_fxaaCheckBox, 0, wxALL, 8);
    antiAliasingSizer->Add(legacyBoxSizer, 0, wxEXPAND | wxALL, 8);

    SetSizer(antiAliasingSizer);
}

void AntiAliasingPanel::bindEvents()
{
    m_antiAliasingChoice->Bind(wxEVT_CHOICE, &AntiAliasingPanel::onAntiAliasingChanged, this);
    m_msaaSamplesSlider->Bind(wxEVT_SLIDER, &AntiAliasingPanel::onAntiAliasingChanged, this);
    m_fxaaCheckBox->Bind(wxEVT_CHECKBOX, &AntiAliasingPanel::onAntiAliasingChanged, this);
}

int AntiAliasingPanel::getAntiAliasingMethod() const
{
    return m_antiAliasingChoice ? m_antiAliasingChoice->GetSelection() : 1;
}

int AntiAliasingPanel::getMSAASamples() const
{
    return m_msaaSamplesSlider ? m_msaaSamplesSlider->GetValue() : 4;
}

bool AntiAliasingPanel::isFXAAEnabled() const
{
    return m_fxaaCheckBox ? m_fxaaCheckBox->GetValue() : false;
}

void AntiAliasingPanel::setAntiAliasingMethod(int method)
{
    if (m_antiAliasingChoice && method >= 0 && method < m_antiAliasingChoice->GetCount()) {
        m_antiAliasingChoice->SetSelection(method);
    }
}

void AntiAliasingPanel::setMSAASamples(int samples)
{
    if (m_msaaSamplesSlider) {
        m_msaaSamplesSlider->SetValue(samples);
    }
}

void AntiAliasingPanel::setFXAAEnabled(bool enabled)
{
    if (m_fxaaCheckBox) {
        m_fxaaCheckBox->SetValue(enabled);
    }
}

void AntiAliasingPanel::setAntiAliasingManager(AntiAliasingManager* manager)
{
    m_antiAliasingManager = manager;
}

void AntiAliasingPanel::onAntiAliasingChanged(wxCommandEvent& event)
{
    if (m_antiAliasingManager && m_antiAliasingChoice) {
        int selection = m_antiAliasingChoice->GetSelection();
        wxString presetName = m_antiAliasingChoice->GetString(selection);
        m_antiAliasingManager->applyPreset(presetName.ToStdString());
        float impact = m_antiAliasingManager->getPerformanceImpact();
        std::string qualityDesc = m_antiAliasingManager->getQualityDescription();
        wxWindow* parent = m_antiAliasingChoice->GetParent();
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
                        int estimatedFPS = static_cast<int>(60.0f / impact);
                        if (estimatedFPS < 15) estimatedFPS = 15;
                        if (estimatedFPS > 120) estimatedFPS = 120;
                        label->SetLabel("Estimated FPS: " + std::to_string(estimatedFPS));
                    }
                }
            }
        }
        LOG_INF_S("AntiAliasingPanel::onAntiAliasingChanged: Applied preset '" + presetName.ToStdString() + "'");
    }
    if (m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void AntiAliasingPanel::loadSettings()
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString exeDir = exeFile.GetPath();
        wxString renderConfigPath = exeDir + wxFileName::GetPathSeparator() + "render_settings.ini";
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, renderConfigPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);

        if (this->m_antiAliasingChoice) {
            int method;
            if (renderConfig.Read("Global.AntiAliasing.Method", &method, 2)) {
                if (method >= 0 && method < this->m_antiAliasingChoice->GetCount()) {
                    this->m_antiAliasingChoice->SetSelection(method);
                } else {
                    this->m_antiAliasingChoice->SetSelection(2);
                    LOG_WRN_S("AntiAliasingPanel::loadSettings: Invalid anti-aliasing method " + std::to_string(method) + ", defaulting to MSAA 4x");
                }
            }
        }
        if (this->m_msaaSamplesSlider) {
            int samples;
            if (renderConfig.Read("Global.AntiAliasing.MSAASamples", &samples, 4)) {
                this->m_msaaSamplesSlider->SetValue(samples);
            }
        }
        if (this->m_fxaaCheckBox) {
            bool enabled;
            if (renderConfig.Read("Global.AntiAliasing.FXAAEnabled", &enabled, false)) {
                this->m_fxaaCheckBox->SetValue(enabled);
            }
        }
        LOG_INF_S("AntiAliasingPanel::loadSettings: Settings loaded successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("AntiAliasingPanel::loadSettings: Failed to load settings: " + std::string(e.what()));
    }
}

void AntiAliasingPanel::saveSettings()
{
    try {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxFileName exeFile(exePath);
        wxString exeDir = exeFile.GetPath();
        wxString renderConfigPath = exeDir + wxFileName::GetPathSeparator() + "render_settings.ini";
        wxFileConfig renderConfig(wxEmptyString, wxEmptyString, renderConfigPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);

        if (this->m_antiAliasingChoice) {
            renderConfig.Write("Global.AntiAliasing.Method", this->m_antiAliasingChoice->GetSelection());
        }
        if (this->m_msaaSamplesSlider) {
            renderConfig.Write("Global.AntiAliasing.MSAASamples", this->m_msaaSamplesSlider->GetValue());
        }
        if (this->m_fxaaCheckBox) {
            renderConfig.Write("Global.AntiAliasing.FXAAEnabled", this->m_fxaaCheckBox->GetValue());
        }
        renderConfig.Flush();
        LOG_INF_S("AntiAliasingPanel::saveSettings: Settings saved successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("AntiAliasingPanel::saveSettings: Failed to save settings: " + std::string(e.what()));
    }
}

void AntiAliasingPanel::resetToDefaults()
{
    if (this->m_antiAliasingChoice) {
        this->m_antiAliasingChoice->SetSelection(1);
    }
    if (this->m_msaaSamplesSlider) {
        this->m_msaaSamplesSlider->SetValue(4);
    }
    if (this->m_fxaaCheckBox) {
        this->m_fxaaCheckBox->SetValue(false);
    }
    LOG_INF_S("AntiAliasingPanel::resetToDefaults: Settings reset to defaults");
}