#include "RenderingSettingsDialog.h"
#include "OCCViewer.h"
#include "RenderingEngine.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"
#include <wx/wx.h>
#include <wx/colour.h>
#include <wx/colordlg.h>
#include <wx/filename.h>

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
	const auto& blendSettings = config.getBlendSettings();
	const auto& shadingSettings = config.getShadingSettings();
	const auto& displaySettings = config.getDisplaySettings();
	const auto& qualitySettings = config.getQualitySettings();
	const auto& shadowSettings = config.getShadowSettings();
	const auto& lightingModelSettings = config.getLightingModelSettings();

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
	m_textureImagePath = textureSettings.imagePath;
	m_textureMode = textureSettings.textureMode;

	m_blendMode = blendSettings.blendMode;
	m_depthTest = blendSettings.depthTest;
	m_depthWrite = blendSettings.depthWrite;
	m_cullFace = blendSettings.cullFace;
	m_alphaThreshold = blendSettings.alphaThreshold;

	// Initialize new rendering mode settings
	// Removed m_shadingMode initialization - functionality not needed
	// Shading page removed; ensure defaults are still initialized safely
	m_smoothNormals = shadingSettings.smoothNormals;
	m_wireframeWidth = shadingSettings.wireframeWidth;
	m_pointSize = shadingSettings.pointSize;

	m_displayMode = displaySettings.displayMode;
	m_showEdges = displaySettings.showEdges;
	m_showVertices = displaySettings.showVertices;
	m_edgeWidth = displaySettings.edgeWidth;
	m_vertexSize = displaySettings.vertexSize;
	m_edgeColor = displaySettings.edgeColor;
	m_vertexColor = displaySettings.vertexColor;

	m_renderingQuality = qualitySettings.quality;
	m_tessellationLevel = qualitySettings.tessellationLevel;
	m_antiAliasingSamples = qualitySettings.antiAliasingSamples;
	m_enableLOD = qualitySettings.enableLOD;
	m_lodDistance = qualitySettings.lodDistance;

	m_shadowMode = shadowSettings.shadowMode;
	m_shadowIntensity = shadowSettings.shadowIntensity;
	m_shadowSoftness = shadowSettings.shadowSoftness;
	m_shadowMapSize = shadowSettings.shadowMapSize;
	m_shadowBias = shadowSettings.shadowBias;

	m_lightingModel = lightingModelSettings.lightingModel;
	m_roughness = lightingModelSettings.roughness;
	m_metallic = lightingModelSettings.metallic;
	m_fresnel = lightingModelSettings.fresnel;
	m_subsurfaceScattering = lightingModelSettings.subsurfaceScattering;

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
	createBlendPage();
	// Removed createShadingPage call - functionality not needed
	createDisplayPage();
	createQualityPage();
	createShadowPage();
	createLightingModelPage();

	// Create dialog buttons
	m_applyButton = new wxButton(this, wxID_APPLY, "Apply");
	m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
	m_okButton = new wxButton(this, wxID_OK, "OK");
	m_resetButton = new wxButton(this, wxID_ANY, "Reset to Defaults");
}

void RenderingSettingsDialog::createMaterialPage()
{
	m_materialPage = new wxPanel(m_notebook);

	// Material preset choice
	new wxStaticText(m_materialPage, wxID_ANY, "Material Preset:");
	m_materialPresetChoice = new wxChoice(m_materialPage, wxID_ANY);
	auto presets = RenderingConfig::getAvailablePresets();
	for (const auto& preset : presets) {
		m_materialPresetChoice->Append(preset);
	}
	m_materialPresetChoice->SetSelection(0); // Default to "Custom"

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

	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(6, 3, 10, 10);
	gridSizer->AddGrowableCol(1);

	gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, "Material Preset:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_materialPresetChoice, 0, wxEXPAND);
	gridSizer->Add(new wxStaticText(m_materialPage, wxID_ANY, ""), 0);

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

	// Texture image selection
	m_textureImageButton = new wxButton(m_texturePage, wxID_ANY, "Select Image...", wxDefaultPosition, wxSize(120, 30));

	// Texture preview
	wxBitmap defaultBitmap(64, 64);
	m_texturePreview = new wxStaticBitmap(m_texturePage, wxID_ANY, defaultBitmap);

	// Texture path label
	m_texturePathLabel = new wxStaticText(m_texturePage, wxID_ANY,
		m_textureImagePath.empty() ? "No image selected" : m_textureImagePath);

	// Texture color
	m_textureColorButton = new wxButton(m_texturePage, wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 30));
	updateColorButton(m_textureColorButton, quantityColorToWxColour(m_textureColor));

	// Texture intensity
	m_textureIntensitySlider = new wxSlider(m_texturePage, wxID_ANY, 50, 0, 100, wxDefaultPosition, wxSize(200, -1));
	m_textureIntensityLabel = new wxStaticText(m_texturePage, wxID_ANY, "50%");

	// Texture mode choice
	m_textureModeChoice = new wxChoice(m_texturePage, wxID_ANY);
	auto textureModes = RenderingConfig::getAvailableTextureModes();
	for (const auto& mode : textureModes) {
		m_textureModeChoice->Append(mode);
	}
	m_textureModeChoice->SetSelection(static_cast<int>(m_textureMode));

	// Layout texture page
	wxBoxSizer* textureSizer = new wxBoxSizer(wxVERTICAL);

	textureSizer->Add(m_textureEnabledCheckbox, 0, wxALL, 10);

	// Image selection section
	wxBoxSizer* imageSizer = new wxBoxSizer(wxHORIZONTAL);
	imageSizer->Add(m_textureImageButton, 0, wxRIGHT, 10);
	imageSizer->Add(m_texturePreview, 0, wxRIGHT, 10);

	wxBoxSizer* imageInfoSizer = new wxBoxSizer(wxVERTICAL);
	imageInfoSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, "Image File:"), 0, wxBOTTOM, 5);
	imageInfoSizer->Add(m_texturePathLabel, 0, wxEXPAND);
	imageSizer->Add(imageInfoSizer, 1, wxEXPAND);

	textureSizer->Add(imageSizer, 0, wxEXPAND | wxALL, 10);

	// Settings section
	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(3, 3, 10, 10);
	gridSizer->AddGrowableCol(1);

	gridSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, "Texture Color:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_textureColorButton, 0, wxEXPAND);
	gridSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, ""), 0);

	gridSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, "Texture Intensity:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_textureIntensitySlider, 0, wxEXPAND);
	gridSizer->Add(m_textureIntensityLabel, 0, wxALIGN_CENTER_VERTICAL);

	gridSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, "Texture Mode:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_textureModeChoice, 0, wxEXPAND);
	gridSizer->Add(new wxStaticText(m_texturePage, wxID_ANY, ""), 0);

	textureSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
	m_texturePage->SetSizer(textureSizer);

	m_notebook->AddPage(m_texturePage, "Texture");
}

void RenderingSettingsDialog::createBlendPage()
{
	m_blendPage = new wxPanel(m_notebook);

	// Blend mode choice
	m_blendModeChoice = new wxChoice(m_blendPage, wxID_ANY);
	auto blendModes = RenderingConfig::getAvailableBlendModes();
	for (const auto& mode : blendModes) {
		m_blendModeChoice->Append(mode);
	}
	m_blendModeChoice->SetSelection(static_cast<int>(m_blendMode));

	// Depth test checkbox
	m_depthTestCheckbox = new wxCheckBox(m_blendPage, wxID_ANY, "Enable Depth Test");
	m_depthTestCheckbox->SetValue(m_depthTest);

	// Depth write checkbox
	m_depthWriteCheckbox = new wxCheckBox(m_blendPage, wxID_ANY, "Enable Depth Write");
	m_depthWriteCheckbox->SetValue(m_depthWrite);

	// Cull face checkbox
	m_cullFaceCheckbox = new wxCheckBox(m_blendPage, wxID_ANY, "Enable Face Culling");
	m_cullFaceCheckbox->SetValue(m_cullFace);

	// Alpha threshold slider
	m_alphaThresholdSlider = new wxSlider(m_blendPage, wxID_ANY,
		static_cast<int>(m_alphaThreshold * 100), 0, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_alphaThresholdLabel = new wxStaticText(m_blendPage, wxID_ANY,
		wxString::Format("%.2f", m_alphaThreshold));

	// Layout blend page
	wxBoxSizer* blendSizer = new wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 2, 10, 10);
	gridSizer->AddGrowableCol(1);

	gridSizer->Add(new wxStaticText(m_blendPage, wxID_ANY, "Blend Mode:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_blendModeChoice, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_blendPage, wxID_ANY, "Alpha Threshold:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* thresholdSizer = new wxBoxSizer(wxHORIZONTAL);
	thresholdSizer->Add(m_alphaThresholdSlider, 1, wxEXPAND | wxRIGHT, 5);
	thresholdSizer->Add(m_alphaThresholdLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(thresholdSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_blendPage, wxID_ANY, ""), 0);
	gridSizer->Add(m_depthTestCheckbox, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_blendPage, wxID_ANY, ""), 0);
	gridSizer->Add(m_depthWriteCheckbox, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_blendPage, wxID_ANY, ""), 0);
	gridSizer->Add(m_cullFaceCheckbox, 0, wxEXPAND);

	blendSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
	m_blendPage->SetSizer(blendSizer);

	m_notebook->AddPage(m_blendPage, "Blend");
}

// Removed createShadingPage method - functionality not needed

void RenderingSettingsDialog::createDisplayPage()
{
	m_displayPage = new wxPanel(m_notebook);

	// Display mode choice
	m_displayModeChoice = new wxChoice(m_displayPage, wxID_ANY);
	auto displayModes = RenderingConfig::getAvailableDisplayModes();
	for (const auto& mode : displayModes) {
		m_displayModeChoice->Append(mode);
	}
	m_displayModeChoice->SetSelection(static_cast<int>(m_displayMode));

	// Show edges checkbox
	m_showEdgesCheckbox = new wxCheckBox(m_displayPage, wxID_ANY, "Show Edges");
	m_showEdgesCheckbox->SetValue(m_showEdges);

	// Show vertices checkbox
	m_showVerticesCheckbox = new wxCheckBox(m_displayPage, wxID_ANY, "Show Vertices");
	m_showVerticesCheckbox->SetValue(m_showVertices);

	// Edge width slider
	m_edgeWidthSlider = new wxSlider(m_displayPage, wxID_ANY,
		static_cast<int>(m_edgeWidth * 10), 1, 50,
		wxDefaultPosition, wxSize(200, -1));
	m_edgeWidthLabel = new wxStaticText(m_displayPage, wxID_ANY,
		wxString::Format("%.1f", m_edgeWidth));

	// Vertex size slider
	m_vertexSizeSlider = new wxSlider(m_displayPage, wxID_ANY,
		static_cast<int>(m_vertexSize * 10), 1, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_vertexSizeLabel = new wxStaticText(m_displayPage, wxID_ANY,
		wxString::Format("%.1f", m_vertexSize));

	// Edge color button
	m_edgeColorButton = new wxButton(m_displayPage, wxID_ANY, "Edge Color");
	updateColorButton(m_edgeColorButton, quantityColorToWxColour(m_edgeColor));

	// Vertex color button
	m_vertexColorButton = new wxButton(m_displayPage, wxID_ANY, "Vertex Color");
	updateColorButton(m_vertexColorButton, quantityColorToWxColour(m_vertexColor));

	// Layout display page
	wxBoxSizer* displaySizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(7, 2, 10, 10);
	gridSizer->AddGrowableCol(1);

	gridSizer->Add(new wxStaticText(m_displayPage, wxID_ANY, "Display Mode:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_displayModeChoice, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_displayPage, wxID_ANY, "Edge Width:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* edgeWidthSizer = new wxBoxSizer(wxHORIZONTAL);
	edgeWidthSizer->Add(m_edgeWidthSlider, 1, wxEXPAND | wxRIGHT, 5);
	edgeWidthSizer->Add(m_edgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(edgeWidthSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_displayPage, wxID_ANY, "Vertex Size:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* vertexSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	vertexSizeSizer->Add(m_vertexSizeSlider, 1, wxEXPAND | wxRIGHT, 5);
	vertexSizeSizer->Add(m_vertexSizeLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(vertexSizeSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_displayPage, wxID_ANY, ""), 0);
	gridSizer->Add(m_edgeColorButton, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_displayPage, wxID_ANY, ""), 0);
	gridSizer->Add(m_vertexColorButton, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_displayPage, wxID_ANY, ""), 0);
	gridSizer->Add(m_showEdgesCheckbox, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_displayPage, wxID_ANY, ""), 0);
	gridSizer->Add(m_showVerticesCheckbox, 0, wxEXPAND);

	displaySizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
	m_displayPage->SetSizer(displaySizer);

	m_notebook->AddPage(m_displayPage, "Display");
}

void RenderingSettingsDialog::createQualityPage()
{
	m_qualityPage = new wxPanel(m_notebook);

	// Quality choice
	m_renderingQualityChoice = new wxChoice(m_qualityPage, wxID_ANY);
	auto qualityModes = RenderingConfig::getAvailableQualityModes();
	for (const auto& mode : qualityModes) {
		m_renderingQualityChoice->Append(mode);
	}
	m_renderingQualityChoice->SetSelection(static_cast<int>(m_renderingQuality));

	// Tessellation level slider
	m_tessellationLevelSlider = new wxSlider(m_qualityPage, wxID_ANY,
		m_tessellationLevel, 1, 10,
		wxDefaultPosition, wxSize(200, -1));
	m_tessellationLevelLabel = new wxStaticText(m_qualityPage, wxID_ANY,
		wxString::Format("%d", m_tessellationLevel));

	// Anti-aliasing samples slider
	m_antiAliasingSamplesSlider = new wxSlider(m_qualityPage, wxID_ANY,
		m_antiAliasingSamples, 1, 16,
		wxDefaultPosition, wxSize(200, -1));
	m_antiAliasingSamplesLabel = new wxStaticText(m_qualityPage, wxID_ANY,
		wxString::Format("%d", m_antiAliasingSamples));

	// Enable LOD checkbox
	m_enableLODCheckbox = new wxCheckBox(m_qualityPage, wxID_ANY, "Enable Level of Detail");
	m_enableLODCheckbox->SetValue(m_enableLOD);

	// LOD distance slider
	m_lodDistanceSlider = new wxSlider(m_qualityPage, wxID_ANY,
		static_cast<int>(m_lodDistance), 10, 1000,
		wxDefaultPosition, wxSize(200, -1));
	m_lodDistanceLabel = new wxStaticText(m_qualityPage, wxID_ANY,
		wxString::Format("%.0f", m_lodDistance));

	// Layout quality page
	wxBoxSizer* qualitySizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 2, 10, 10);
	gridSizer->AddGrowableCol(1);

	gridSizer->Add(new wxStaticText(m_qualityPage, wxID_ANY, "Quality:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_renderingQualityChoice, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_qualityPage, wxID_ANY, "Tessellation Level:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* tessellationSizer = new wxBoxSizer(wxHORIZONTAL);
	tessellationSizer->Add(m_tessellationLevelSlider, 1, wxEXPAND | wxRIGHT, 5);
	tessellationSizer->Add(m_tessellationLevelLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(tessellationSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_qualityPage, wxID_ANY, "Anti-aliasing Samples:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* aaSizer = new wxBoxSizer(wxHORIZONTAL);
	aaSizer->Add(m_antiAliasingSamplesSlider, 1, wxEXPAND | wxRIGHT, 5);
	aaSizer->Add(m_antiAliasingSamplesLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(aaSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_qualityPage, wxID_ANY, "LOD Distance:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* lodSizer = new wxBoxSizer(wxHORIZONTAL);
	lodSizer->Add(m_lodDistanceSlider, 1, wxEXPAND | wxRIGHT, 5);
	lodSizer->Add(m_lodDistanceLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(lodSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_qualityPage, wxID_ANY, ""), 0);
	gridSizer->Add(m_enableLODCheckbox, 0, wxEXPAND);

	qualitySizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
	m_qualityPage->SetSizer(qualitySizer);

	m_notebook->AddPage(m_qualityPage, "Quality");
}

void RenderingSettingsDialog::createShadowPage()
{
	m_shadowPage = new wxPanel(m_notebook);

	// Shadow mode choice
	m_shadowModeChoice = new wxChoice(m_shadowPage, wxID_ANY);
	auto shadowModes = RenderingConfig::getAvailableShadowModes();
	for (const auto& mode : shadowModes) {
		m_shadowModeChoice->Append(mode);
	}
	m_shadowModeChoice->SetSelection(static_cast<int>(m_shadowMode));

	// Shadow intensity slider
	m_shadowIntensitySlider = new wxSlider(m_shadowPage, wxID_ANY,
		static_cast<int>(m_shadowIntensity * 100), 0, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_shadowIntensityLabel = new wxStaticText(m_shadowPage, wxID_ANY,
		wxString::Format("%.2f", m_shadowIntensity));

	// Shadow softness slider
	m_shadowSoftnessSlider = new wxSlider(m_shadowPage, wxID_ANY,
		static_cast<int>(m_shadowSoftness * 100), 0, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_shadowSoftnessLabel = new wxStaticText(m_shadowPage, wxID_ANY,
		wxString::Format("%.2f", m_shadowSoftness));

	// Shadow map size slider
	m_shadowMapSizeSlider = new wxSlider(m_shadowPage, wxID_ANY,
		m_shadowMapSize, 256, 4096,
		wxDefaultPosition, wxSize(200, -1));
	m_shadowMapSizeLabel = new wxStaticText(m_shadowPage, wxID_ANY,
		wxString::Format("%d", m_shadowMapSize));

	// Shadow bias slider
	m_shadowBiasSlider = new wxSlider(m_shadowPage, wxID_ANY,
		static_cast<int>(m_shadowBias * 10000), 1, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_shadowBiasLabel = new wxStaticText(m_shadowPage, wxID_ANY,
		wxString::Format("%.4f", m_shadowBias));

	// Layout shadow page
	wxBoxSizer* shadowSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 2, 10, 10);
	gridSizer->AddGrowableCol(1);

	gridSizer->Add(new wxStaticText(m_shadowPage, wxID_ANY, "Shadow Mode:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_shadowModeChoice, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_shadowPage, wxID_ANY, "Shadow Intensity:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* intensitySizer = new wxBoxSizer(wxHORIZONTAL);
	intensitySizer->Add(m_shadowIntensitySlider, 1, wxEXPAND | wxRIGHT, 5);
	intensitySizer->Add(m_shadowIntensityLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(intensitySizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_shadowPage, wxID_ANY, "Shadow Softness:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* softnessSizer = new wxBoxSizer(wxHORIZONTAL);
	softnessSizer->Add(m_shadowSoftnessSlider, 1, wxEXPAND | wxRIGHT, 5);
	softnessSizer->Add(m_shadowSoftnessLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(softnessSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_shadowPage, wxID_ANY, "Shadow Map Size:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* mapSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	mapSizeSizer->Add(m_shadowMapSizeSlider, 1, wxEXPAND | wxRIGHT, 5);
	mapSizeSizer->Add(m_shadowMapSizeLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(mapSizeSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_shadowPage, wxID_ANY, "Shadow Bias:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* biasSizer = new wxBoxSizer(wxHORIZONTAL);
	biasSizer->Add(m_shadowBiasSlider, 1, wxEXPAND | wxRIGHT, 5);
	biasSizer->Add(m_shadowBiasLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(biasSizer, 0, wxEXPAND);

	shadowSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
	m_shadowPage->SetSizer(shadowSizer);

	m_notebook->AddPage(m_shadowPage, "Shadow");
}

void RenderingSettingsDialog::createLightingModelPage()
{
	m_lightingModelPage = new wxPanel(m_notebook);

	// Lighting model choice
	m_lightingModelChoice = new wxChoice(m_lightingModelPage, wxID_ANY);
	auto lightingModels = RenderingConfig::getAvailableLightingModels();
	for (const auto& model : lightingModels) {
		m_lightingModelChoice->Append(model);
	}
	m_lightingModelChoice->SetSelection(static_cast<int>(m_lightingModel));

	// Roughness slider
	m_roughnessSlider = new wxSlider(m_lightingModelPage, wxID_ANY,
		static_cast<int>(m_roughness * 100), 0, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_roughnessLabel = new wxStaticText(m_lightingModelPage, wxID_ANY,
		wxString::Format("%.2f", m_roughness));

	// Metallic slider
	m_metallicSlider = new wxSlider(m_lightingModelPage, wxID_ANY,
		static_cast<int>(m_metallic * 100), 0, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_metallicLabel = new wxStaticText(m_lightingModelPage, wxID_ANY,
		wxString::Format("%.2f", m_metallic));

	// Fresnel slider
	m_fresnelSlider = new wxSlider(m_lightingModelPage, wxID_ANY,
		static_cast<int>(m_fresnel * 100), 0, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_fresnelLabel = new wxStaticText(m_lightingModelPage, wxID_ANY,
		wxString::Format("%.2f", m_fresnel));

	// Subsurface scattering slider
	m_subsurfaceScatteringSlider = new wxSlider(m_lightingModelPage, wxID_ANY,
		static_cast<int>(m_subsurfaceScattering * 100), 0, 100,
		wxDefaultPosition, wxSize(200, -1));
	m_subsurfaceScatteringLabel = new wxStaticText(m_lightingModelPage, wxID_ANY,
		wxString::Format("%.2f", m_subsurfaceScattering));

	// Layout lighting model page
	wxBoxSizer* lightingModelSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(5, 2, 10, 10);
	gridSizer->AddGrowableCol(1);

	gridSizer->Add(new wxStaticText(m_lightingModelPage, wxID_ANY, "Lighting Model:"), 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(m_lightingModelChoice, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_lightingModelPage, wxID_ANY, "Roughness:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* roughnessSizer = new wxBoxSizer(wxHORIZONTAL);
	roughnessSizer->Add(m_roughnessSlider, 1, wxEXPAND | wxRIGHT, 5);
	roughnessSizer->Add(m_roughnessLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(roughnessSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_lightingModelPage, wxID_ANY, "Metallic:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* metallicSizer = new wxBoxSizer(wxHORIZONTAL);
	metallicSizer->Add(m_metallicSlider, 1, wxEXPAND | wxRIGHT, 5);
	metallicSizer->Add(m_metallicLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(metallicSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_lightingModelPage, wxID_ANY, "Fresnel:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* fresnelSizer = new wxBoxSizer(wxHORIZONTAL);
	fresnelSizer->Add(m_fresnelSlider, 1, wxEXPAND | wxRIGHT, 5);
	fresnelSizer->Add(m_fresnelLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(fresnelSizer, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(m_lightingModelPage, wxID_ANY, "Subsurface Scattering:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* subsurfaceSizer = new wxBoxSizer(wxHORIZONTAL);
	subsurfaceSizer->Add(m_subsurfaceScatteringSlider, 1, wxEXPAND | wxRIGHT, 5);
	subsurfaceSizer->Add(m_subsurfaceScatteringLabel, 0, wxALIGN_CENTER_VERTICAL);
	gridSizer->Add(subsurfaceSizer, 0, wxEXPAND);

	lightingModelSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
	m_lightingModelPage->SetSizer(lightingModelSizer);

	m_notebook->AddPage(m_lightingModelPage, "Lighting Model");
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
	m_materialPresetChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &RenderingSettingsDialog::onMaterialPresetChoice, this);
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
	m_textureImageButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onTextureImageButton, this);
	m_textureModeChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &RenderingSettingsDialog::onTextureModeChoice, this);

	// Blend events
	m_blendModeChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &RenderingSettingsDialog::onBlendModeChoice, this);
	m_depthTestCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &RenderingSettingsDialog::onDepthTestCheckbox, this);
	m_depthWriteCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &RenderingSettingsDialog::onDepthWriteCheckbox, this);
	m_cullFaceCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &RenderingSettingsDialog::onCullFaceCheckbox, this);
	m_alphaThresholdSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onAlphaThresholdSlider, this);

	// Shading events (page removed) - skip binding non-existent controls

	// Display events
	m_displayModeChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &RenderingSettingsDialog::onDisplayModeChoice, this);
	m_showEdgesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &RenderingSettingsDialog::onShowEdgesCheckbox, this);
	m_showVerticesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &RenderingSettingsDialog::onShowVerticesCheckbox, this);
	m_edgeWidthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onEdgeWidthSlider, this);
	m_vertexSizeSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onVertexSizeSlider, this);
	m_edgeColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onEdgeColorButton, this);
	m_vertexColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &RenderingSettingsDialog::onVertexColorButton, this);

	// Quality events
	m_renderingQualityChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &RenderingSettingsDialog::onRenderingQualityChoice, this);
	m_tessellationLevelSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onTessellationLevelSlider, this);
	m_antiAliasingSamplesSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onAntiAliasingSamplesSlider, this);
	m_enableLODCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &RenderingSettingsDialog::onEnableLODCheckbox, this);
	m_lodDistanceSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onLODDistanceSlider, this);

	// Shadow events
	m_shadowModeChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &RenderingSettingsDialog::onShadowModeChoice, this);
	m_shadowIntensitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onShadowIntensitySlider, this);
	m_shadowSoftnessSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onShadowSoftnessSlider, this);
	m_shadowMapSizeSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onShadowMapSizeSlider, this);
	m_shadowBiasSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onShadowBiasSlider, this);

	// Lighting Model events
	m_lightingModelChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &RenderingSettingsDialog::onLightingModelChoice, this);
	m_roughnessSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onRoughnessSlider, this);
	m_metallicSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onMetallicSlider, this);
	m_fresnelSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onFresnelSlider, this);
	m_subsurfaceScatteringSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &RenderingSettingsDialog::onSubsurfaceScatteringSlider, this);

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
	m_textureModeChoice->SetSelection(static_cast<int>(m_textureMode));
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

void RenderingSettingsDialog::onTextureModeChoice(wxCommandEvent& event)
{
	int selection = m_textureModeChoice->GetSelection();
	m_textureMode = static_cast<RenderingConfig::TextureMode>(selection);
}

// Blend events
void RenderingSettingsDialog::onBlendModeChoice(wxCommandEvent& event)
{
	int selection = m_blendModeChoice->GetSelection();
	m_blendMode = static_cast<RenderingConfig::BlendMode>(selection);
}

void RenderingSettingsDialog::onDepthTestCheckbox(wxCommandEvent& event)
{
	m_depthTest = m_depthTestCheckbox->GetValue();
}

void RenderingSettingsDialog::onDepthWriteCheckbox(wxCommandEvent& event)
{
	m_depthWrite = m_depthWriteCheckbox->GetValue();
}

void RenderingSettingsDialog::onCullFaceCheckbox(wxCommandEvent& event)
{
	m_cullFace = m_cullFaceCheckbox->GetValue();
}

void RenderingSettingsDialog::onAlphaThresholdSlider(wxCommandEvent& event)
{
	m_alphaThreshold = static_cast<double>(m_alphaThresholdSlider->GetValue()) / 100.0;
	m_alphaThresholdLabel->SetLabel(wxString::Format("%.2f", m_alphaThreshold));
}

// Removed onShadingModeChoice method - functionality not needed

void RenderingSettingsDialog::onSmoothNormalsCheckbox(wxCommandEvent& event)
{
	m_smoothNormals = m_smoothNormalsCheckbox->GetValue();
}

void RenderingSettingsDialog::onWireframeWidthSlider(wxCommandEvent& event)
{
	m_wireframeWidth = static_cast<double>(m_wireframeWidthSlider->GetValue()) / 10.0;
	m_wireframeWidthLabel->SetLabel(wxString::Format("%.1f", m_wireframeWidth));
}

void RenderingSettingsDialog::onPointSizeSlider(wxCommandEvent& event)
{
	m_pointSize = static_cast<double>(m_pointSizeSlider->GetValue()) / 10.0;
	m_pointSizeLabel->SetLabel(wxString::Format("%.1f", m_pointSize));
}

// Display events
void RenderingSettingsDialog::onDisplayModeChoice(wxCommandEvent& event)
{
	int selection = m_displayModeChoice->GetSelection();
	m_displayMode = static_cast<RenderingConfig::DisplayMode>(selection);
}

void RenderingSettingsDialog::onShowEdgesCheckbox(wxCommandEvent& event)
{
	m_showEdges = m_showEdgesCheckbox->GetValue();
}

void RenderingSettingsDialog::onShowVerticesCheckbox(wxCommandEvent& event)
{
	m_showVertices = m_showVerticesCheckbox->GetValue();
}

void RenderingSettingsDialog::onEdgeWidthSlider(wxCommandEvent& event)
{
	m_edgeWidth = static_cast<double>(m_edgeWidthSlider->GetValue()) / 10.0;
	m_edgeWidthLabel->SetLabel(wxString::Format("%.1f", m_edgeWidth));
}

void RenderingSettingsDialog::onVertexSizeSlider(wxCommandEvent& event)
{
	m_vertexSize = static_cast<double>(m_vertexSizeSlider->GetValue()) / 10.0;
	m_vertexSizeLabel->SetLabel(wxString::Format("%.1f", m_vertexSize));
}

void RenderingSettingsDialog::onEdgeColorButton(wxCommandEvent& event)
{
	wxColour currentColor = quantityColorToWxColour(m_edgeColor);
	wxColourData colorData;
	colorData.SetColour(currentColor);
	wxColourDialog dialog(this, &colorData);

	if (dialog.ShowModal() == wxID_OK) {
		wxColour newColor = dialog.GetColourData().GetColour();
		m_edgeColor = wxColourToQuantityColor(newColor);
		updateColorButton(m_edgeColorButton, newColor);
	}
}

void RenderingSettingsDialog::onVertexColorButton(wxCommandEvent& event)
{
	wxColour currentColor = quantityColorToWxColour(m_vertexColor);
	wxColourData colorData;
	colorData.SetColour(currentColor);
	wxColourDialog dialog(this, &colorData);

	if (dialog.ShowModal() == wxID_OK) {
		wxColour newColor = dialog.GetColourData().GetColour();
		m_vertexColor = wxColourToQuantityColor(newColor);
		updateColorButton(m_vertexColorButton, newColor);
	}
}

// Quality events
void RenderingSettingsDialog::onRenderingQualityChoice(wxCommandEvent& event)
{
	int selection = m_renderingQualityChoice->GetSelection();
	m_renderingQuality = static_cast<RenderingConfig::RenderingQuality>(selection);
}

void RenderingSettingsDialog::onTessellationLevelSlider(wxCommandEvent& event)
{
	m_tessellationLevel = m_tessellationLevelSlider->GetValue();
	m_tessellationLevelLabel->SetLabel(wxString::Format("%d", m_tessellationLevel));
}

void RenderingSettingsDialog::onAntiAliasingSamplesSlider(wxCommandEvent& event)
{
	m_antiAliasingSamples = m_antiAliasingSamplesSlider->GetValue();
	m_antiAliasingSamplesLabel->SetLabel(wxString::Format("%d", m_antiAliasingSamples));
}

void RenderingSettingsDialog::onEnableLODCheckbox(wxCommandEvent& event)
{
	m_enableLOD = m_enableLODCheckbox->GetValue();
}

void RenderingSettingsDialog::onLODDistanceSlider(wxCommandEvent& event)
{
	m_lodDistance = static_cast<double>(m_lodDistanceSlider->GetValue());
	m_lodDistanceLabel->SetLabel(wxString::Format("%.0f", m_lodDistance));
}

// Shadow events
void RenderingSettingsDialog::onShadowModeChoice(wxCommandEvent& event)
{
	int selection = m_shadowModeChoice->GetSelection();
	m_shadowMode = static_cast<RenderingConfig::ShadowMode>(selection);
}

void RenderingSettingsDialog::onShadowIntensitySlider(wxCommandEvent& event)
{
	m_shadowIntensity = static_cast<double>(m_shadowIntensitySlider->GetValue()) / 100.0;
	m_shadowIntensityLabel->SetLabel(wxString::Format("%.2f", m_shadowIntensity));
}

void RenderingSettingsDialog::onShadowSoftnessSlider(wxCommandEvent& event)
{
	m_shadowSoftness = static_cast<double>(m_shadowSoftnessSlider->GetValue()) / 100.0;
	m_shadowSoftnessLabel->SetLabel(wxString::Format("%.2f", m_shadowSoftness));
}

void RenderingSettingsDialog::onShadowMapSizeSlider(wxCommandEvent& event)
{
	m_shadowMapSize = m_shadowMapSizeSlider->GetValue();
	m_shadowMapSizeLabel->SetLabel(wxString::Format("%d", m_shadowMapSize));
}

void RenderingSettingsDialog::onShadowBiasSlider(wxCommandEvent& event)
{
	m_shadowBias = static_cast<double>(m_shadowBiasSlider->GetValue()) / 10000.0;
	m_shadowBiasLabel->SetLabel(wxString::Format("%.4f", m_shadowBias));
}

// Lighting Model events
void RenderingSettingsDialog::onLightingModelChoice(wxCommandEvent& event)
{
	int selection = m_lightingModelChoice->GetSelection();
	m_lightingModel = static_cast<RenderingConfig::LightingModel>(selection);
}

void RenderingSettingsDialog::onRoughnessSlider(wxCommandEvent& event)
{
	m_roughness = static_cast<double>(m_roughnessSlider->GetValue()) / 100.0;
	m_roughnessLabel->SetLabel(wxString::Format("%.2f", m_roughness));
}

void RenderingSettingsDialog::onMetallicSlider(wxCommandEvent& event)
{
	m_metallic = static_cast<double>(m_metallicSlider->GetValue()) / 100.0;
	m_metallicLabel->SetLabel(wxString::Format("%.2f", m_metallic));
}

void RenderingSettingsDialog::onFresnelSlider(wxCommandEvent& event)
{
	m_fresnel = static_cast<double>(m_fresnelSlider->GetValue()) / 100.0;
	m_fresnelLabel->SetLabel(wxString::Format("%.2f", m_fresnel));
}

void RenderingSettingsDialog::onSubsurfaceScatteringSlider(wxCommandEvent& event)
{
	m_subsurfaceScattering = static_cast<double>(m_subsurfaceScatteringSlider->GetValue()) / 100.0;
	m_subsurfaceScatteringLabel->SetLabel(wxString::Format("%.2f", m_subsurfaceScattering));
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
	textureSettings.imagePath = m_textureImagePath;
	textureSettings.textureMode = m_textureMode;
	config.setTextureSettings(textureSettings);

	// Update blend settings
	RenderingConfig::BlendSettings blendSettings;
	blendSettings.blendMode = m_blendMode;
	blendSettings.depthTest = m_depthTest;
	blendSettings.depthWrite = m_depthWrite;
	blendSettings.cullFace = m_cullFace;
	blendSettings.alphaThreshold = m_alphaThreshold;
	config.setBlendSettings(blendSettings);

	// Update shading settings
	RenderingConfig::ShadingSettings shadingSettings;
	// Removed shadingMode assignment - functionality not needed
	shadingSettings.smoothNormals = m_smoothNormals;
	shadingSettings.wireframeWidth = m_wireframeWidth;
	shadingSettings.pointSize = m_pointSize;
	config.setShadingSettings(shadingSettings);

	// Update display settings
	RenderingConfig::DisplaySettings displaySettings;
	displaySettings.displayMode = m_displayMode;
	displaySettings.showEdges = m_showEdges;
	displaySettings.showVertices = m_showVertices;
	displaySettings.edgeWidth = m_edgeWidth;
	displaySettings.vertexSize = m_vertexSize;
	displaySettings.edgeColor = m_edgeColor;
	displaySettings.vertexColor = m_vertexColor;
	config.setDisplaySettings(displaySettings);

	// Update quality settings
	RenderingConfig::QualitySettings qualitySettings;
	qualitySettings.quality = m_renderingQuality;
	qualitySettings.tessellationLevel = m_tessellationLevel;
	qualitySettings.antiAliasingSamples = m_antiAliasingSamples;
	qualitySettings.enableLOD = m_enableLOD;
	qualitySettings.lodDistance = m_lodDistance;
	config.setQualitySettings(qualitySettings);

	// Update shadow settings
	RenderingConfig::ShadowSettings shadowSettings;
	shadowSettings.shadowMode = m_shadowMode;
	shadowSettings.shadowIntensity = m_shadowIntensity;
	shadowSettings.shadowSoftness = m_shadowSoftness;
	shadowSettings.shadowMapSize = m_shadowMapSize;
	shadowSettings.shadowBias = m_shadowBias;
	config.setShadowSettings(shadowSettings);

	// Update lighting model settings
	RenderingConfig::LightingModelSettings lightingModelSettings;
	lightingModelSettings.lightingModel = m_lightingModel;
	lightingModelSettings.roughness = m_roughness;
	lightingModelSettings.metallic = m_metallic;
	lightingModelSettings.fresnel = m_fresnel;
	lightingModelSettings.subsurfaceScattering = m_subsurfaceScattering;
	config.setLightingModelSettings(lightingModelSettings);

	if (m_occViewer) {
		// Note: Individual geometry settings are now handled by PositionBasicDialog and VisualSettingsDialog
		// RenderingSettingsDialog only manages global rendering configuration
		LOG_INF_S("Global rendering settings applied. Individual geometry settings are managed by PositionBasicDialog and VisualSettingsDialog.");

		// Apply lighting settings to rendering engine
		if (m_renderingEngine) {
			// Lighting is now handled by SceneManager through LightingConfig
			// No need to call RenderingEngine lighting methods
			LOG_INF_S("Lighting settings applied through SceneManager");
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
	const auto& blendSettings = config.getBlendSettings();
	const auto& shadingSettings = config.getShadingSettings();
	const auto& displaySettings = config.getDisplaySettings();
	const auto& qualitySettings = config.getQualitySettings();
	const auto& shadowSettings = config.getShadowSettings();
	const auto& lightingModelSettings = config.getLightingModelSettings();

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
	m_textureImagePath = textureSettings.imagePath;
	m_textureMode = textureSettings.textureMode;

	m_blendMode = blendSettings.blendMode;
	m_depthTest = blendSettings.depthTest;
	m_depthWrite = blendSettings.depthWrite;
	m_cullFace = blendSettings.cullFace;
	m_alphaThreshold = blendSettings.alphaThreshold;

	// Initialize new rendering mode settings
	// Removed m_shadingMode initialization - functionality not needed
	m_smoothNormals = shadingSettings.smoothNormals;
	m_wireframeWidth = shadingSettings.wireframeWidth;
	m_pointSize = shadingSettings.pointSize;

	m_displayMode = displaySettings.displayMode;
	m_showEdges = displaySettings.showEdges;
	m_showVertices = displaySettings.showVertices;
	m_edgeWidth = displaySettings.edgeWidth;
	m_vertexSize = displaySettings.vertexSize;
	m_edgeColor = displaySettings.edgeColor;
	m_vertexColor = displaySettings.vertexColor;

	m_renderingQuality = qualitySettings.quality;
	m_tessellationLevel = qualitySettings.tessellationLevel;
	m_antiAliasingSamples = qualitySettings.antiAliasingSamples;
	m_enableLOD = qualitySettings.enableLOD;
	m_lodDistance = qualitySettings.lodDistance;

	m_shadowMode = shadowSettings.shadowMode;
	m_shadowIntensity = shadowSettings.shadowIntensity;
	m_shadowSoftness = shadowSettings.shadowSoftness;
	m_shadowMapSize = shadowSettings.shadowMapSize;
	m_shadowBias = shadowSettings.shadowBias;

	m_lightingModel = lightingModelSettings.lightingModel;
	m_roughness = lightingModelSettings.roughness;
	m_metallic = lightingModelSettings.metallic;
	m_fresnel = lightingModelSettings.fresnel;
	m_subsurfaceScattering = lightingModelSettings.subsurfaceScattering;

	// Update texture preview and path label
	updateTexturePreview();
	m_texturePathLabel->SetLabel(m_textureImagePath.empty() ? "No image selected" : m_textureImagePath);

	// Update blend controls
	m_blendModeChoice->SetSelection(static_cast<int>(m_blendMode));
	m_depthTestCheckbox->SetValue(m_depthTest);
	m_depthWriteCheckbox->SetValue(m_depthWrite);
	m_cullFaceCheckbox->SetValue(m_cullFace);
	m_alphaThresholdSlider->SetValue(static_cast<int>(m_alphaThreshold * 100));
	m_alphaThresholdLabel->SetLabel(wxString::Format("%.2f", m_alphaThreshold));

	// Reset material preset to Custom
	m_materialPresetChoice->SetSelection(0);

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

void RenderingSettingsDialog::onMaterialPresetChoice(wxCommandEvent& event)
{
	wxString selectedPreset = m_materialPresetChoice->GetStringSelection();
	applyMaterialPreset(selectedPreset.ToStdString());
}

void RenderingSettingsDialog::applyMaterialPreset(const std::string& presetName)
{
	RenderingConfig::MaterialPreset preset = RenderingConfig::getPresetFromName(presetName);
	if (preset != RenderingConfig::MaterialPreset::Custom) {
		RenderingConfig& config = RenderingConfig::getInstance();
		RenderingConfig::MaterialSettings presetMaterial = config.getPresetMaterial(preset);

		// Update internal values
		m_materialAmbientColor = presetMaterial.ambientColor;
		m_materialDiffuseColor = presetMaterial.diffuseColor;
		m_materialSpecularColor = presetMaterial.specularColor;
		m_materialShininess = presetMaterial.shininess;
		m_materialTransparency = presetMaterial.transparency;

		// Update UI controls
		updateMaterialControls();
	}
}

void RenderingSettingsDialog::updateMaterialControls()
{
	// Update color buttons
	updateColorButton(m_materialAmbientColorButton, quantityColorToWxColour(m_materialAmbientColor));
	updateColorButton(m_materialDiffuseColorButton, quantityColorToWxColour(m_materialDiffuseColor));
	updateColorButton(m_materialSpecularColorButton, quantityColorToWxColour(m_materialSpecularColor));

	// Update sliders and labels
	m_materialShininessSlider->SetValue(static_cast<int>(m_materialShininess));
	m_materialShininessLabel->SetLabel(wxString::Format("%.0f", m_materialShininess));

	m_materialTransparencySlider->SetValue(static_cast<int>(m_materialTransparency * 100));
	m_materialTransparencyLabel->SetLabel(wxString::Format("%.0f%%", m_materialTransparency * 100));
}

void RenderingSettingsDialog::onTextureImageButton(wxCommandEvent& event)
{
	wxFileDialog fileDialog(this, "Select Texture Image", "", "",
		"Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.tiff)|*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.tiff",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (fileDialog.ShowModal() == wxID_OK) {
		m_textureImagePath = fileDialog.GetPath().ToStdString();

		// Update path label
		wxFileName fileName(fileDialog.GetPath());
		m_texturePathLabel->SetLabel(fileName.GetFullName());

		// Update texture preview
		updateTexturePreview();
	}
}

void RenderingSettingsDialog::updateTexturePreview()
{
	if (!m_textureImagePath.empty()) {
		wxImage image(m_textureImagePath);
		if (image.IsOk()) {
			// Scale image to fit preview size (64x64)
			image = image.Scale(64, 64, wxIMAGE_QUALITY_HIGH);
			wxBitmap bitmap(image);
			m_texturePreview->SetBitmap(bitmap);
		}
	}
	else {
		// Set default empty bitmap
		wxBitmap defaultBitmap(64, 64);
		m_texturePreview->SetBitmap(defaultBitmap);
	}

	// Refresh the preview control
	m_texturePreview->Refresh();
}