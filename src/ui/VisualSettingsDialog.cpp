#include "VisualSettingsDialog.h"
#include <wx/notebook.h>
#include <wx/filedlg.h>
#include <wx/valnum.h>
#include <wx/textctrl.h>
#include <wx/colour.h>
#include <wx/colordlg.h>
#include <wx/statbmp.h>
#include <wx/image.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/brush.h>
#include <wx/pen.h>
#include "logger/Logger.h"
#include <sstream>
#include <iomanip>

// Custom event IDs
enum {
	ID_DIFFUSE_COLOR_BUTTON = wxID_HIGHEST + 2000,
	ID_AMBIENT_COLOR_BUTTON,
	ID_SPECULAR_COLOR_BUTTON,
	ID_EMISSIVE_COLOR_BUTTON,
	ID_BROWSE_TEXTURE_BUTTON
};

BEGIN_EVENT_TABLE(VisualSettingsDialog, wxDialog)
EVT_BUTTON(wxID_OK, VisualSettingsDialog::OnOkButton)
EVT_BUTTON(wxID_CANCEL, VisualSettingsDialog::OnCancelButton)
EVT_BUTTON(ID_BROWSE_TEXTURE_BUTTON, VisualSettingsDialog::OnBrowseTexture)
EVT_BUTTON(wxID_APPLY, VisualSettingsDialog::OnApplyButton)
EVT_BUTTON(ID_DIFFUSE_COLOR_BUTTON, VisualSettingsDialog::OnDiffuseColorButton)
EVT_BUTTON(ID_AMBIENT_COLOR_BUTTON, VisualSettingsDialog::OnAmbientColorButton)
EVT_BUTTON(ID_SPECULAR_COLOR_BUTTON, VisualSettingsDialog::OnSpecularColorButton)
EVT_BUTTON(ID_EMISSIVE_COLOR_BUTTON, VisualSettingsDialog::OnEmissiveColorButton)
END_EVENT_TABLE()

VisualSettingsDialog::VisualSettingsDialog(wxWindow* parent, const wxString& title,
	const BasicGeometryParameters& basicParams)
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(600, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	m_basicParams(basicParams)
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Create notebook for tabs
	wxNotebook* notebook = new wxNotebook(this, wxID_ANY);

	// Create panels with their own sizers
	wxPanel* basicInfoPanel = new wxPanel(notebook, wxID_ANY);
	CreateBasicInfoPanel(basicInfoPanel);
	notebook->AddPage(basicInfoPanel, "Basic Info");

	wxPanel* materialPanel = new wxPanel(notebook, wxID_ANY);
	CreateMaterialPanel(materialPanel);
	notebook->AddPage(materialPanel, "Material");

	wxPanel* texturePanel = new wxPanel(notebook, wxID_ANY);
	CreateTexturePanel(texturePanel);
	notebook->AddPage(texturePanel, "Texture");

	wxPanel* renderingPanel = new wxPanel(notebook, wxID_ANY);
	CreateRenderingPanel(renderingPanel);
	notebook->AddPage(renderingPanel, "Rendering");

	wxPanel* subdivisionPanel = new wxPanel(notebook, wxID_ANY);
	CreateSubdivisionPanel(subdivisionPanel);
	notebook->AddPage(subdivisionPanel, "Subdivision");

	mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 5);

	// Add buttons
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* applyButton = new wxButton(this, wxID_APPLY, "Apply");
	wxButton* okButton = new wxButton(this, wxID_OK, "OK");
	wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");

	buttonSizer->Add(applyButton, 0, wxALL, 5);
	buttonSizer->Add(okButton, 0, wxALL, 5);
	buttonSizer->Add(cancelButton, 0, wxALL, 5);

	mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

	SetSizer(mainSizer);

	// Initialize controls with default values
	SaveAdvancedParametersToControls();
	UpdateBasicInfoDisplay();

	// Initialize color buttons and texture preview
	UpdateColorButton(m_diffuseColorButton, m_advancedParams.materialDiffuseColor);
	UpdateColorButton(m_ambientColorButton, m_advancedParams.materialAmbientColor);
	UpdateColorButton(m_specularColorButton, m_advancedParams.materialSpecularColor);
	UpdateColorButton(m_emissiveColorButton, m_advancedParams.materialEmissiveColor);
	UpdateTexturePreview();
}

void VisualSettingsDialog::CreateBasicInfoPanel(wxPanel* panel)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Basic info display (read-only)
	wxStaticText* infoLabel = new wxStaticText(panel, wxID_ANY, "Basic Geometry Information (Read-only):");
	sizer->Add(infoLabel, 0, wxALL, 5);

	wxStaticText* typeLabel = new wxStaticText(panel, wxID_ANY, "Geometry Type:");
	m_geometryTypeLabel = new wxStaticText(panel, wxID_ANY, "");
	sizer->Add(typeLabel, 0, wxALL, 5);
	sizer->Add(m_geometryTypeLabel, 0, wxALL, 5);

	wxStaticText* positionLabel = new wxStaticText(panel, wxID_ANY, "Position:");
	m_positionLabel = new wxStaticText(panel, wxID_ANY, "");
	sizer->Add(positionLabel, 0, wxALL, 5);
	sizer->Add(m_positionLabel, 0, wxALL, 5);

	wxStaticText* dimensionsLabel = new wxStaticText(panel, wxID_ANY, "Dimensions:");
	m_dimensionsLabel = new wxStaticText(panel, wxID_ANY, "");
	sizer->Add(dimensionsLabel, 0, wxALL, 5);
	sizer->Add(m_dimensionsLabel, 0, wxALL, 5);

	panel->SetSizer(sizer);
}

void VisualSettingsDialog::CreateMaterialPanel(wxPanel* panel)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Diffuse color - horizontal layout
	wxStaticText* diffuseLabel = new wxStaticText(panel, wxID_ANY, "Diffuse Color:");
	sizer->Add(diffuseLabel, 0, wxALL, 5);

	wxBoxSizer* diffuseSizer = new wxBoxSizer(wxHORIZONTAL);

	// Color picker button
	m_diffuseColorButton = new wxButton(panel, ID_DIFFUSE_COLOR_BUTTON, "Color", wxDefaultPosition, wxSize(60, 25));
	diffuseSizer->Add(m_diffuseColorButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	// RGB text controls in horizontal layout
	diffuseSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_diffuseRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.8", wxDefaultPosition, wxSize(50, 25));
	diffuseSizer->Add(m_diffuseRTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	diffuseSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_diffuseGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.8", wxDefaultPosition, wxSize(50, 25));
	diffuseSizer->Add(m_diffuseGTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	diffuseSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_diffuseBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.8", wxDefaultPosition, wxSize(50, 25));
	diffuseSizer->Add(m_diffuseBTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	sizer->Add(diffuseSizer, 0, wxEXPAND | wxALL, 5);

	// Ambient color - horizontal layout
	wxStaticText* ambientLabel = new wxStaticText(panel, wxID_ANY, "Ambient Color:");
	sizer->Add(ambientLabel, 0, wxALL, 5);

	wxBoxSizer* ambientSizer = new wxBoxSizer(wxHORIZONTAL);

	// Color picker button
	m_ambientColorButton = new wxButton(panel, ID_AMBIENT_COLOR_BUTTON, "Color", wxDefaultPosition, wxSize(60, 25));
	ambientSizer->Add(m_ambientColorButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	// RGB text controls in horizontal layout
	ambientSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_ambientRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.2", wxDefaultPosition, wxSize(50, 25));
	ambientSizer->Add(m_ambientRTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	ambientSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_ambientGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.2", wxDefaultPosition, wxSize(50, 25));
	ambientSizer->Add(m_ambientGTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	ambientSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_ambientBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.2", wxDefaultPosition, wxSize(50, 25));
	ambientSizer->Add(m_ambientBTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	sizer->Add(ambientSizer, 0, wxEXPAND | wxALL, 5);

	// Specular color - horizontal layout
	wxStaticText* specularLabel = new wxStaticText(panel, wxID_ANY, "Specular Color:");
	sizer->Add(specularLabel, 0, wxALL, 5);

	wxBoxSizer* specularSizer = new wxBoxSizer(wxHORIZONTAL);

	// Color picker button
	m_specularColorButton = new wxButton(panel, ID_SPECULAR_COLOR_BUTTON, "Color", wxDefaultPosition, wxSize(60, 25));
	specularSizer->Add(m_specularColorButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	// RGB text controls in horizontal layout
	specularSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_specularRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "1.0", wxDefaultPosition, wxSize(50, 25));
	specularSizer->Add(m_specularRTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	specularSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_specularGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "1.0", wxDefaultPosition, wxSize(50, 25));
	specularSizer->Add(m_specularGTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	specularSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_specularBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "1.0", wxDefaultPosition, wxSize(50, 25));
	specularSizer->Add(m_specularBTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	sizer->Add(specularSizer, 0, wxEXPAND | wxALL, 5);

	// Emissive color - horizontal layout
	wxStaticText* emissiveLabel = new wxStaticText(panel, wxID_ANY, "Emissive Color:");
	sizer->Add(emissiveLabel, 0, wxALL, 5);

	wxBoxSizer* emissiveSizer = new wxBoxSizer(wxHORIZONTAL);

	// Color picker button
	m_emissiveColorButton = new wxButton(panel, ID_EMISSIVE_COLOR_BUTTON, "Color", wxDefaultPosition, wxSize(60, 25));
	emissiveSizer->Add(m_emissiveColorButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	// RGB text controls in horizontal layout
	emissiveSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_emissiveRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0", wxDefaultPosition, wxSize(50, 25));
	emissiveSizer->Add(m_emissiveRTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	emissiveSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_emissiveGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0", wxDefaultPosition, wxSize(50, 25));
	emissiveSizer->Add(m_emissiveGTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	emissiveSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	m_emissiveBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0", wxDefaultPosition, wxSize(50, 25));
	emissiveSizer->Add(m_emissiveBTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	sizer->Add(emissiveSizer, 0, wxEXPAND | wxALL, 5);

	// Material properties - horizontal layout
	wxStaticText* propertiesLabel = new wxStaticText(panel, wxID_ANY, "Material Properties:");
	sizer->Add(propertiesLabel, 0, wxALL, 5);

	wxBoxSizer* propertiesSizer = new wxBoxSizer(wxHORIZONTAL);

	propertiesSizer->Add(new wxStaticText(panel, wxID_ANY, "Shininess:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
	m_shininessTextCtrl = new wxTextCtrl(panel, wxID_ANY, "50.0", wxDefaultPosition, wxSize(80, 25));
	propertiesSizer->Add(m_shininessTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	propertiesSizer->Add(new wxStaticText(panel, wxID_ANY, "Transparency:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 20);
	m_transparencyTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0", wxDefaultPosition, wxSize(80, 25));
	propertiesSizer->Add(m_transparencyTextCtrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	sizer->Add(propertiesSizer, 0, wxEXPAND | wxALL, 5);

	panel->SetSizer(sizer);
}

void VisualSettingsDialog::CreateTexturePanel(wxPanel* panel)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Texture enabled checkbox
	m_textureEnabledCheckBox = new wxCheckBox(panel, wxID_ANY, "Enable Texture");
	sizer->Add(m_textureEnabledCheckBox, 0, wxALL, 5);

	// Texture path
	wxStaticText* pathLabel = new wxStaticText(panel, wxID_ANY, "Texture Path:");
	sizer->Add(pathLabel, 0, wxALL, 5);

	wxBoxSizer* pathSizer = new wxBoxSizer(wxHORIZONTAL);
	m_texturePathTextCtrl = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	m_browseTextureButton = new wxButton(panel, ID_BROWSE_TEXTURE_BUTTON, "Browse");
	pathSizer->Add(m_texturePathTextCtrl, 1, wxEXPAND);
	pathSizer->Add(m_browseTextureButton, 0, wxLEFT, 5);
	sizer->Add(pathSizer, 0, wxEXPAND | wxALL, 5);

	// Texture preview
	wxStaticText* previewLabel = new wxStaticText(panel, wxID_ANY, "Texture Preview:");
	sizer->Add(previewLabel, 0, wxALL, 5);

	// Create a default bitmap for preview (white square with border)
	wxBitmap defaultBitmap(128, 128);
	wxMemoryDC dc;
	dc.SelectObject(defaultBitmap);
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();

	// Draw a border to make it visible
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.DrawRectangle(0, 0, 128, 128);

	dc.SelectObject(wxNullBitmap);

	m_texturePreview = new wxStaticBitmap(panel, wxID_ANY, defaultBitmap, wxDefaultPosition, wxSize(128, 128));
	sizer->Add(m_texturePreview, 0, wxALIGN_CENTER | wxALL, 5);

	// Texture mode
	wxStaticText* modeLabel = new wxStaticText(panel, wxID_ANY, "Texture Mode:");
	sizer->Add(modeLabel, 0, wxALL, 5);

	m_textureModeChoice = new wxChoice(panel, wxID_ANY);
	m_textureModeChoice->Append("Modulate");
	m_textureModeChoice->Append("Decal");
	m_textureModeChoice->Append("Blend");
	m_textureModeChoice->Append("Replace");
	m_textureModeChoice->SetSelection(0);
	sizer->Add(m_textureModeChoice, 0, wxEXPAND | wxALL, 5);

	panel->SetSizer(sizer);
}

void VisualSettingsDialog::CreateRenderingPanel(wxPanel* panel)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Rendering quality
	wxStaticText* qualityLabel = new wxStaticText(panel, wxID_ANY, "Rendering Quality:");
	sizer->Add(qualityLabel, 0, wxALL, 5);

	m_renderingQualityChoice = new wxChoice(panel, wxID_ANY);
	m_renderingQualityChoice->Append("Draft");
	m_renderingQualityChoice->Append("Normal");
	m_renderingQualityChoice->Append("High");
	m_renderingQualityChoice->SetSelection(1);
	sizer->Add(m_renderingQualityChoice, 0, wxEXPAND | wxALL, 5);

	// Blend mode
	wxStaticText* blendLabel = new wxStaticText(panel, wxID_ANY, "Blend Mode:");
	sizer->Add(blendLabel, 0, wxALL, 5);

	m_blendModeChoice = new wxChoice(panel, wxID_ANY);
	m_blendModeChoice->Append("None");
	m_blendModeChoice->Append("Alpha");
	m_blendModeChoice->Append("Additive");
	m_blendModeChoice->Append("Multiply");
	m_blendModeChoice->Append("Screen");
	m_blendModeChoice->Append("Overlay");
	m_blendModeChoice->SetSelection(0);
	sizer->Add(m_blendModeChoice, 0, wxEXPAND | wxALL, 5);

	// Lighting model
	wxStaticText* lightingLabel = new wxStaticText(panel, wxID_ANY, "Lighting Model:");
	sizer->Add(lightingLabel, 0, wxALL, 5);

	m_lightingModelChoice = new wxChoice(panel, wxID_ANY);
	m_lightingModelChoice->Append("Lambert");
	m_lightingModelChoice->Append("BlinnPhong");
	m_lightingModelChoice->Append("CookTorrance");
	m_lightingModelChoice->Append("OrenNayar");
	m_lightingModelChoice->Append("Minnaert");
	m_lightingModelChoice->Append("Fresnel");
	m_lightingModelChoice->SetSelection(0);
	sizer->Add(m_lightingModelChoice, 0, wxEXPAND | wxALL, 5);

	// Rendering options
	m_backfaceCullingCheckBox = new wxCheckBox(panel, wxID_ANY, "Backface Culling");
	m_backfaceCullingCheckBox->SetValue(true);
	sizer->Add(m_backfaceCullingCheckBox, 0, wxALL, 5);

	m_depthTestCheckBox = new wxCheckBox(panel, wxID_ANY, "Depth Test");
	m_depthTestCheckBox->SetValue(true);
	sizer->Add(m_depthTestCheckBox, 0, wxALL, 5);

	panel->SetSizer(sizer);
}

void VisualSettingsDialog::CreateSubdivisionPanel(wxPanel* panel)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Subdivision enabled
	m_subdivisionEnabledCheckBox = new wxCheckBox(panel, wxID_ANY, "Enable Subdivision");
	sizer->Add(m_subdivisionEnabledCheckBox, 0, wxALL, 5);

	// Subdivision levels
	wxStaticText* levelsLabel = new wxStaticText(panel, wxID_ANY, "Subdivision Levels:");
	sizer->Add(levelsLabel, 0, wxALL, 5);

	m_subdivisionLevelsSpinCtrl = new wxSpinCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 1);
	sizer->Add(m_subdivisionLevelsSpinCtrl, 0, wxEXPAND | wxALL, 5);

	panel->SetSizer(sizer);
}

void VisualSettingsDialog::LoadAdvancedParametersFromControls()
{
	// Load material parameters
	double r, g, b, shininess, transparency;

	m_diffuseRTextCtrl->GetValue().ToDouble(&r);
	m_diffuseGTextCtrl->GetValue().ToDouble(&g);
	m_diffuseBTextCtrl->GetValue().ToDouble(&b);
	m_advancedParams.materialDiffuseColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);

	m_ambientRTextCtrl->GetValue().ToDouble(&r);
	m_ambientGTextCtrl->GetValue().ToDouble(&g);
	m_ambientBTextCtrl->GetValue().ToDouble(&b);
	m_advancedParams.materialAmbientColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);

	m_specularRTextCtrl->GetValue().ToDouble(&r);
	m_specularGTextCtrl->GetValue().ToDouble(&g);
	m_specularBTextCtrl->GetValue().ToDouble(&b);
	m_advancedParams.materialSpecularColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);

	m_emissiveRTextCtrl->GetValue().ToDouble(&r);
	m_emissiveGTextCtrl->GetValue().ToDouble(&g);
	m_emissiveBTextCtrl->GetValue().ToDouble(&b);
	m_advancedParams.materialEmissiveColor = Quantity_Color(r, g, b, Quantity_TOC_RGB);

	m_shininessTextCtrl->GetValue().ToDouble(&shininess);
	m_advancedParams.materialShininess = shininess;

	m_transparencyTextCtrl->GetValue().ToDouble(&transparency);
	m_advancedParams.materialTransparency = transparency;

	// Load texture parameters
	m_advancedParams.texturePath = m_texturePathTextCtrl->GetValue().ToStdString();
	m_advancedParams.textureEnabled = m_textureEnabledCheckBox->GetValue();

	int textureModeIndex = m_textureModeChoice->GetSelection();
	switch (textureModeIndex) {
	case 0: m_advancedParams.textureMode = RenderingConfig::TextureMode::Modulate; break;
	case 1: m_advancedParams.textureMode = RenderingConfig::TextureMode::Decal; break;
	case 2: m_advancedParams.textureMode = RenderingConfig::TextureMode::Blend; break;
	case 3: m_advancedParams.textureMode = RenderingConfig::TextureMode::Replace; break;
	}

	// Load rendering parameters
	int qualityIndex = m_renderingQualityChoice->GetSelection();
	switch (qualityIndex) {
	case 0: m_advancedParams.renderingQuality = RenderingConfig::RenderingQuality::Draft; break;
	case 1: m_advancedParams.renderingQuality = RenderingConfig::RenderingQuality::Normal; break;
	case 2: m_advancedParams.renderingQuality = RenderingConfig::RenderingQuality::High; break;
	}

	int blendIndex = m_blendModeChoice->GetSelection();
	switch (blendIndex) {
	case 0: m_advancedParams.blendMode = RenderingConfig::BlendMode::None; break;
	case 1: m_advancedParams.blendMode = RenderingConfig::BlendMode::Alpha; break;
	case 2: m_advancedParams.blendMode = RenderingConfig::BlendMode::Additive; break;
	case 3: m_advancedParams.blendMode = RenderingConfig::BlendMode::Multiply; break;
	case 4: m_advancedParams.blendMode = RenderingConfig::BlendMode::Screen; break;
	case 5: m_advancedParams.blendMode = RenderingConfig::BlendMode::Overlay; break;
	}

	int lightingIndex = m_lightingModelChoice->GetSelection();
	switch (lightingIndex) {
	case 0: m_advancedParams.lightingModel = RenderingConfig::LightingModel::Lambert; break;
	case 1: m_advancedParams.lightingModel = RenderingConfig::LightingModel::BlinnPhong; break;
	case 2: m_advancedParams.lightingModel = RenderingConfig::LightingModel::CookTorrance; break;
	case 3: m_advancedParams.lightingModel = RenderingConfig::LightingModel::OrenNayar; break;
	case 4: m_advancedParams.lightingModel = RenderingConfig::LightingModel::Minnaert; break;
	case 5: m_advancedParams.lightingModel = RenderingConfig::LightingModel::Fresnel; break;
	}

	m_advancedParams.backfaceCulling = m_backfaceCullingCheckBox->GetValue();
	m_advancedParams.depthTest = m_depthTestCheckBox->GetValue();

	// Load display parameters
	// m_advancedParams.showNormals = m_showNormalsCheckBox->GetValue(); // Removed as per edit hint
	// m_advancedParams.showEdges = m_showEdgesCheckBox->GetValue(); // Removed as per edit hint
	// m_advancedParams.showWireframe = m_showWireframeCheckBox->GetValue(); // Removed as per edit hint
	// m_advancedParams.showSilhouette = m_showSilhouetteCheckBox->GetValue(); // Removed as per edit hint
	// m_advancedParams.showFeatureEdges = m_showFeatureEdgesCheckBox->GetValue(); // Removed as per edit hint
	// m_advancedParams.showMeshEdges = m_showMeshEdgesCheckBox->GetValue(); // Removed as per edit hint
	// m_advancedParams.showOriginalEdges = m_showOriginalEdgesCheckBox->GetValue(); // Removed as per edit hint
	// m_advancedParams.showFaceNormals = m_showFaceNormalsCheckBox->GetValue(); // Removed as per edit hint

	// Load subdivision parameters
	m_advancedParams.subdivisionEnabled = m_subdivisionEnabledCheckBox->GetValue();
	m_advancedParams.subdivisionLevels = m_subdivisionLevelsSpinCtrl->GetValue();

	// Load edge settings
	// m_advancedParams.edgeEnabled = m_edgeEnabledCheckBox->GetValue(); // Removed as per edit hint

	// int edgeTypeIndex = m_edgeTypeChoice->GetSelection(); // Removed as per edit hint
	// switch (edgeTypeIndex) { // Removed as per edit hint
	//     case 0: m_advancedParams.edgeStyle = 0; break; // Solid // Removed as per edit hint
	//     case 1: m_advancedParams.edgeStyle = 1; break; // Dashed // Removed as per edit hint
	//     case 2: m_advancedParams.edgeStyle = 2; break; // Dotted // Removed as per edit hint
	// } // Removed as per edit hint

	// double edgeWidth; // Removed as per edit hint
	// m_edgeWidthTextCtrl->GetValue().ToDouble(&edgeWidth); // Removed as per edit hint
	// m_advancedParams.edgeWidth = edgeWidth; // Removed as per edit hint

	// m_edgeColorRTextCtrl->GetValue().ToDouble(&r); // Removed as per edit hint
	// m_edgeColorGTextCtrl->GetValue().ToDouble(&g); // Removed as per edit hint
	// m_edgeColorBTextCtrl->GetValue().ToDouble(&b); // Removed as per edit hint
	// m_advancedParams.edgeColor = Quantity_Color(r, g, b, Quantity_TOC_RGB); // Removed as per edit hint
}

void VisualSettingsDialog::SaveAdvancedParametersToControls()
{
	Standard_Real r, g, b;

	// Save material colors
	m_advancedParams.materialDiffuseColor.Values(r, g, b, Quantity_TOC_RGB);
	m_diffuseRTextCtrl->SetValue(wxString::Format("%.2f", r));
	m_diffuseGTextCtrl->SetValue(wxString::Format("%.2f", g));
	m_diffuseBTextCtrl->SetValue(wxString::Format("%.2f", b));
	UpdateColorButton(m_diffuseColorButton, m_advancedParams.materialDiffuseColor);

	m_advancedParams.materialAmbientColor.Values(r, g, b, Quantity_TOC_RGB);
	m_ambientRTextCtrl->SetValue(wxString::Format("%.2f", r));
	m_ambientGTextCtrl->SetValue(wxString::Format("%.2f", g));
	m_ambientBTextCtrl->SetValue(wxString::Format("%.2f", b));
	UpdateColorButton(m_ambientColorButton, m_advancedParams.materialAmbientColor);

	m_advancedParams.materialSpecularColor.Values(r, g, b, Quantity_TOC_RGB);
	m_specularRTextCtrl->SetValue(wxString::Format("%.2f", r));
	m_specularGTextCtrl->SetValue(wxString::Format("%.2f", g));
	m_specularBTextCtrl->SetValue(wxString::Format("%.2f", b));
	UpdateColorButton(m_specularColorButton, m_advancedParams.materialSpecularColor);

	m_advancedParams.materialEmissiveColor.Values(r, g, b, Quantity_TOC_RGB);
	m_emissiveRTextCtrl->SetValue(wxString::Format("%.2f", r));
	m_emissiveGTextCtrl->SetValue(wxString::Format("%.2f", g));
	m_emissiveBTextCtrl->SetValue(wxString::Format("%.2f", b));
	UpdateColorButton(m_emissiveColorButton, m_advancedParams.materialEmissiveColor);

	// Save material properties
	m_shininessTextCtrl->SetValue(wxString::Format("%.1f", m_advancedParams.materialShininess));
	m_transparencyTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialTransparency));

	// Save texture settings
	m_textureEnabledCheckBox->SetValue(m_advancedParams.textureEnabled);
	m_texturePathTextCtrl->SetValue(m_advancedParams.texturePath);

	switch (m_advancedParams.textureMode) {
	case RenderingConfig::TextureMode::Modulate: m_textureModeChoice->SetSelection(0); break;
	case RenderingConfig::TextureMode::Decal: m_textureModeChoice->SetSelection(1); break;
	case RenderingConfig::TextureMode::Blend: m_textureModeChoice->SetSelection(2); break;
	case RenderingConfig::TextureMode::Replace: m_textureModeChoice->SetSelection(3); break;
	}

	// Update texture preview
	UpdateTexturePreview();

	// Save rendering settings
	switch (m_advancedParams.renderingQuality) {
	case RenderingConfig::RenderingQuality::Draft: m_renderingQualityChoice->SetSelection(0); break;
	case RenderingConfig::RenderingQuality::Normal: m_renderingQualityChoice->SetSelection(1); break;
	case RenderingConfig::RenderingQuality::High: m_renderingQualityChoice->SetSelection(2); break;
	}

	switch (m_advancedParams.blendMode) {
	case RenderingConfig::BlendMode::None: m_blendModeChoice->SetSelection(0); break;
	case RenderingConfig::BlendMode::Alpha: m_blendModeChoice->SetSelection(1); break;
	case RenderingConfig::BlendMode::Additive: m_blendModeChoice->SetSelection(2); break;
	case RenderingConfig::BlendMode::Multiply: m_blendModeChoice->SetSelection(3); break;
	case RenderingConfig::BlendMode::Screen: m_blendModeChoice->SetSelection(4); break;
	case RenderingConfig::BlendMode::Overlay: m_blendModeChoice->SetSelection(5); break;
	}

	switch (m_advancedParams.lightingModel) {
	case RenderingConfig::LightingModel::Lambert: m_lightingModelChoice->SetSelection(0); break;
	case RenderingConfig::LightingModel::BlinnPhong: m_lightingModelChoice->SetSelection(1); break;
	case RenderingConfig::LightingModel::CookTorrance: m_lightingModelChoice->SetSelection(2); break;
	case RenderingConfig::LightingModel::OrenNayar: m_lightingModelChoice->SetSelection(3); break;
	case RenderingConfig::LightingModel::Minnaert: m_lightingModelChoice->SetSelection(4); break;
	case RenderingConfig::LightingModel::Fresnel: m_lightingModelChoice->SetSelection(5); break;
	}

	m_backfaceCullingCheckBox->SetValue(m_advancedParams.backfaceCulling);
	m_depthTestCheckBox->SetValue(m_advancedParams.depthTest);

	// Save display settings
	// m_showNormalsCheckBox->SetValue(m_advancedParams.showNormals); // Removed as per edit hint
	// m_showEdgesCheckBox->SetValue(m_advancedParams.showEdges); // Removed as per edit hint
	// m_showWireframeCheckBox->SetValue(m_advancedParams.showWireframe); // Removed as per edit hint
	// m_showSilhouetteCheckBox->SetValue(m_advancedParams.showSilhouette); // Removed as per edit hint
	// m_showFeatureEdgesCheckBox->SetValue(m_advancedParams.showFeatureEdges); // Removed as per edit hint
	// m_showMeshEdgesCheckBox->SetValue(m_advancedParams.showMeshEdges); // Removed as per edit hint
	// m_showOriginalEdgesCheckBox->SetValue(m_advancedParams.showOriginalEdges); // Removed as per edit hint
	// m_showFaceNormalsCheckBox->SetValue(m_advancedParams.showFaceNormals); // Removed as per edit hint

	// Save subdivision settings
	m_subdivisionEnabledCheckBox->SetValue(m_advancedParams.subdivisionEnabled);
	m_subdivisionLevelsSpinCtrl->SetValue(m_advancedParams.subdivisionLevels);
}

void VisualSettingsDialog::UpdateBasicInfoDisplay()
{
	m_geometryTypeLabel->SetLabel(m_basicParams.geometryType);

	// Format position (assuming position is stored elsewhere or passed separately)
	m_positionLabel->SetLabel("X: 0.00, Y: 0.00, Z: 0.00");

	// Format dimensions based on geometry type
	std::string dimensions;
	if (m_basicParams.geometryType == "Box") {
		dimensions = wxString::Format("W: %.2f, H: %.2f, D: %.2f",
			m_basicParams.width, m_basicParams.height, m_basicParams.depth).ToStdString();
	}
	else if (m_basicParams.geometryType == "Sphere") {
		dimensions = wxString::Format("Radius: %.2f", m_basicParams.radius).ToStdString();
	}
	else if (m_basicParams.geometryType == "Cylinder") {
		dimensions = wxString::Format("Radius: %.2f, Height: %.2f",
			m_basicParams.cylinderRadius, m_basicParams.cylinderHeight).ToStdString();
	}
	else if (m_basicParams.geometryType == "Cone") {
		dimensions = wxString::Format("Bottom: %.2f, Top: %.2f, Height: %.2f",
			m_basicParams.bottomRadius, m_basicParams.topRadius, m_basicParams.coneHeight).ToStdString();
	}
	else if (m_basicParams.geometryType == "Torus") {
		dimensions = wxString::Format("Major: %.2f, Minor: %.2f",
			m_basicParams.majorRadius, m_basicParams.minorRadius).ToStdString();
	}
	else if (m_basicParams.geometryType == "TruncatedCylinder") {
		dimensions = wxString::Format("Bottom: %.2f, Top: %.2f, Height: %.2f",
			m_basicParams.truncatedBottomRadius, m_basicParams.truncatedTopRadius, m_basicParams.truncatedHeight).ToStdString();
	}

	m_dimensionsLabel->SetLabel(dimensions);
}

void VisualSettingsDialog::SetBasicParameters(const BasicGeometryParameters& basicParams)
{
	m_basicParams = basicParams;
	UpdateBasicInfoDisplay();
}

void VisualSettingsDialog::SetAdvancedParameters(const AdvancedGeometryParameters& advancedParams)
{
	m_advancedParams = advancedParams;
	SaveAdvancedParametersToControls();
}

AdvancedGeometryParameters VisualSettingsDialog::GetAdvancedParameters() const
{
	return m_advancedParams;
}

BasicGeometryParameters VisualSettingsDialog::GetBasicParameters() const
{
	return m_basicParams;
}

void VisualSettingsDialog::OnBrowseTexture(wxCommandEvent& event)
{
	wxFileDialog dialog(this, "Select Texture File", "", "",
		"Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tga)|*.png;*.jpg;*.jpeg;*.bmp;*.tga|All files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dialog.ShowModal() == wxID_OK) {
		m_texturePathTextCtrl->SetValue(dialog.GetPath());
		UpdateTexturePreview(); // Update preview after selecting file
	}
}

void VisualSettingsDialog::OnOkButton(wxCommandEvent& event)
{
	LoadAdvancedParametersFromControls();
	EndModal(wxID_OK);
}

void VisualSettingsDialog::OnCancelButton(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void VisualSettingsDialog::OnApplyButton(wxCommandEvent& event)
{
	LoadAdvancedParametersFromControls();
	LOG_INF_S("Advanced parameters applied");
}

// Color picker event handlers
void VisualSettingsDialog::OnDiffuseColorButton(wxCommandEvent& event)
{
	wxColourDialog dialog(this);
	if (dialog.ShowModal() == wxID_OK) {
		wxColour color = dialog.GetColourData().GetColour();
		Quantity_Color quantityColor = WxColourToQuantityColor(color);
		m_advancedParams.materialDiffuseColor = quantityColor;

		// Update text controls
		Standard_Real r, g, b;
		quantityColor.Values(r, g, b, Quantity_TOC_RGB);
		m_diffuseRTextCtrl->SetValue(wxString::Format("%.2f", r));
		m_diffuseGTextCtrl->SetValue(wxString::Format("%.2f", g));
		m_diffuseBTextCtrl->SetValue(wxString::Format("%.2f", b));

		// Update button color
		UpdateColorButton(m_diffuseColorButton, quantityColor);
	}
}

void VisualSettingsDialog::OnAmbientColorButton(wxCommandEvent& event)
{
	wxColourDialog dialog(this);
	if (dialog.ShowModal() == wxID_OK) {
		wxColour color = dialog.GetColourData().GetColour();
		Quantity_Color quantityColor = WxColourToQuantityColor(color);
		m_advancedParams.materialAmbientColor = quantityColor;

		// Update text controls
		Standard_Real r, g, b;
		quantityColor.Values(r, g, b, Quantity_TOC_RGB);
		m_ambientRTextCtrl->SetValue(wxString::Format("%.2f", r));
		m_ambientGTextCtrl->SetValue(wxString::Format("%.2f", g));
		m_ambientBTextCtrl->SetValue(wxString::Format("%.2f", b));

		// Update button color
		UpdateColorButton(m_ambientColorButton, quantityColor);
	}
}

void VisualSettingsDialog::OnSpecularColorButton(wxCommandEvent& event)
{
	wxColourDialog dialog(this);
	if (dialog.ShowModal() == wxID_OK) {
		wxColour color = dialog.GetColourData().GetColour();
		Quantity_Color quantityColor = WxColourToQuantityColor(color);
		m_advancedParams.materialSpecularColor = quantityColor;

		// Update text controls
		Standard_Real r, g, b;
		quantityColor.Values(r, g, b, Quantity_TOC_RGB);
		m_specularRTextCtrl->SetValue(wxString::Format("%.2f", r));
		m_specularGTextCtrl->SetValue(wxString::Format("%.2f", g));
		m_specularBTextCtrl->SetValue(wxString::Format("%.2f", b));

		// Update button color
		UpdateColorButton(m_specularColorButton, quantityColor);
	}
}

void VisualSettingsDialog::OnEmissiveColorButton(wxCommandEvent& event)
{
	wxColourDialog dialog(this);
	if (dialog.ShowModal() == wxID_OK) {
		wxColour color = dialog.GetColourData().GetColour();
		Quantity_Color quantityColor = WxColourToQuantityColor(color);
		m_advancedParams.materialEmissiveColor = quantityColor;

		// Update text controls
		Standard_Real r, g, b;
		quantityColor.Values(r, g, b, Quantity_TOC_RGB);
		m_emissiveRTextCtrl->SetValue(wxString::Format("%.2f", r));
		m_emissiveGTextCtrl->SetValue(wxString::Format("%.2f", g));
		m_emissiveBTextCtrl->SetValue(wxString::Format("%.2f", b));

		// Update button color
		UpdateColorButton(m_emissiveColorButton, quantityColor);
	}
}

// Helper methods
void VisualSettingsDialog::UpdateColorButton(wxButton* button, const Quantity_Color& color)
{
	wxColour wxColor = QuantityColorToWxColour(color);
	button->SetBackgroundColour(wxColor);

	// Set text color to contrast with background
	int brightness = (wxColor.Red() * 299 + wxColor.Green() * 587 + wxColor.Blue() * 114) / 1000;
	if (brightness > 128) {
		button->SetForegroundColour(*wxBLACK);
	}
	else {
		button->SetForegroundColour(*wxWHITE);
	}

	button->Refresh();
}

void VisualSettingsDialog::UpdateTexturePreview()
{
	if (m_textureEnabledCheckBox && m_textureEnabledCheckBox->GetValue() &&
		m_texturePathTextCtrl && !m_texturePathTextCtrl->GetValue().IsEmpty()) {
		wxString texturePath = m_texturePathTextCtrl->GetValue();

		if (wxFileExists(texturePath)) {
			wxImage image;
			if (image.LoadFile(texturePath)) {
				// Scale image to fit preview size
				wxSize previewSize(128, 128);
				image.Rescale(previewSize.GetWidth(), previewSize.GetHeight(), wxIMAGE_QUALITY_HIGH);

				wxBitmap bitmap(image);
				if (m_texturePreview) {
					m_texturePreview->SetBitmap(bitmap);
					m_texturePreview->Refresh();
				}
				return;
			}
		}
	}

	// Show default preview (white square)
	wxBitmap defaultBitmap(128, 128);
	wxMemoryDC dc;
	dc.SelectObject(defaultBitmap);
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();

	// Draw a border to make it visible
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.DrawRectangle(0, 0, 128, 128);

	dc.SelectObject(wxNullBitmap);

	if (m_texturePreview) {
		m_texturePreview->SetBitmap(defaultBitmap);
		m_texturePreview->Refresh();
	}
}

wxColour VisualSettingsDialog::QuantityColorToWxColour(const Quantity_Color& color)
{
	Standard_Real r, g, b;
	color.Values(r, g, b, Quantity_TOC_RGB);
	return wxColour(static_cast<unsigned char>(r * 255),
		static_cast<unsigned char>(g * 255),
		static_cast<unsigned char>(b * 255));
}

Quantity_Color VisualSettingsDialog::WxColourToQuantityColor(const wxColour& color)
{
	return Quantity_Color(color.Red() / 255.0,
		color.Green() / 255.0,
		color.Blue() / 255.0,
		Quantity_TOC_RGB);
}