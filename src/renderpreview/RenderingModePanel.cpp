#include "renderpreview/RenderingModePanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/RenderingManager.h"
#include "renderpreview/PreviewCanvas.h"
#include "config/FontManager.h"
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
	presetsBoxSizer->Add(presetsLabel, 0, wxALL, 4);
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
	presetsBoxSizer->Add(m_renderingModeChoice, 0, wxEXPAND | wxALL, 4);
	renderingSizer->Add(presetsBoxSizer, 0, wxEXPAND | wxALL, 4);

	auto* performanceBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Performance Impact");
	auto* impactLabel = new wxStaticText(this, wxID_ANY, "Performance Impact: Medium");
	impactLabel->SetForegroundColour(wxColour(255, 165, 0));
	performanceBoxSizer->Add(impactLabel, 0, wxALL, 4);
	auto* qualityLabel = new wxStaticText(this, wxID_ANY, "Quality: Balanced");
	qualityLabel->SetForegroundColour(wxColour(0, 0, 128));
	performanceBoxSizer->Add(qualityLabel, 0, wxALL, 4);
	auto* fpsLabel = new wxStaticText(this, wxID_ANY, "Estimated FPS: 60");
	fpsLabel->SetForegroundColour(wxColour(0, 128, 0));
	performanceBoxSizer->Add(fpsLabel, 0, wxALL, 4);
	auto* featuresLabel = new wxStaticText(this, wxID_ANY, "Features: Smooth Shading, Phong Shading");
	featuresLabel->SetForegroundColour(wxColour(128, 128, 128));
	performanceBoxSizer->Add(featuresLabel, 0, wxALL, 4);
	renderingSizer->Add(performanceBoxSizer, 0, wxEXPAND | wxALL, 4);

	auto* legacyBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Legacy Mode Settings");
	auto* modeLabel = new wxStaticText(this, wxID_ANY, "Legacy Rendering Mode:");
	legacyBoxSizer->Add(modeLabel, 0, wxALL, 4);
	m_legacyChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(250, -1));
	m_legacyChoice->Append("Solid");
	m_legacyChoice->Append("Wireframe");
	m_legacyChoice->Append("Points");
	m_legacyChoice->Append("Hidden Line");
	m_legacyChoice->Append("Shaded");
	m_legacyChoice->SetSelection(4);
	m_legacyChoice->Enable(false);
	legacyBoxSizer->Add(m_legacyChoice, 0, wxEXPAND | wxALL, 4);
	renderingSizer->Add(legacyBoxSizer, 0, wxEXPAND | wxALL, 4);

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

void RenderingModePanel::notifyParameterChanged()
{
	if (m_parameterChangeCallback) {
		m_parameterChangeCallback();
	}
}

void RenderingModePanel::onRenderingModeChanged(wxCommandEvent& event)
{
	if (m_renderingManager && m_renderingModeChoice) {
		int selection = m_renderingModeChoice->GetSelection();
		wxString presetName = m_renderingModeChoice->GetString(selection);
		m_renderingManager->applyPreset(presetName.ToStdString());
		updateLegacyChoiceFromCurrentMode();

		notifyParameterChanged();

		LOG_INF_S("RenderingModePanel::onRenderingModeChanged: Applied preset '" + presetName.ToStdString() + "'");
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void RenderingModePanel::onLegacyModeChanged(wxCommandEvent& event)
{
	if (m_renderingManager && m_legacyChoice) {
		int selection = m_legacyChoice->GetSelection();
		wxString modeName = m_legacyChoice->GetString(selection);

		// Use the existing setRenderingMode method
		if (m_renderingManager->hasActiveConfiguration()) {
			int activeId = m_renderingManager->getActiveConfigurationId();
			m_renderingManager->setRenderingMode(activeId, selection);
		}

		notifyParameterChanged();

		LOG_INF_S("RenderingModePanel::onLegacyModeChanged: Applied legacy mode '" + modeName.ToStdString() + "'");
	}
	if (m_parentDialog && m_parentDialog->getRenderCanvas()) {
		auto canvas = m_parentDialog->getRenderCanvas();
		canvas->render(false);
		canvas->Refresh();
		canvas->Update();
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
				}
				else {
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

void RenderingModePanel::applyFonts()
{
	FontManager& fontManager = FontManager::getInstance();

	// Apply fonts to choice controls
	if (m_renderingModeChoice) m_renderingModeChoice->SetFont(fontManager.getChoiceFont());
	if (m_legacyChoice) m_legacyChoice->SetFont(fontManager.getChoiceFont());

	// Apply fonts to all static texts in the panel
	wxWindowList& children = GetChildren();
	for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
		wxWindow* child = *it;
		if (child) {
			if (dynamic_cast<wxStaticText*>(child)) {
				child->SetFont(fontManager.getLabelFont());
			}
		}
	}
}