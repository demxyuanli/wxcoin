#include "VisualSettingsDialog.h"
#include <wx/notebook.h>
#include <wx/filedlg.h>
#include <wx/valnum.h>
#include <wx/textctrl.h>
#include <sstream>
#include <iomanip>

BEGIN_EVENT_TABLE(VisualSettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, VisualSettingsDialog::OnOkButton)
    EVT_BUTTON(wxID_CANCEL, VisualSettingsDialog::OnCancelButton)
    EVT_BUTTON(wxID_ANY, VisualSettingsDialog::OnBrowseTexture)
    EVT_BUTTON(wxID_APPLY, VisualSettingsDialog::OnApplyButton)
END_EVENT_TABLE()

VisualSettingsDialog::VisualSettingsDialog(wxWindow* parent, const wxString& title, 
                                             const BasicGeometryParameters& basicParams)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(600, 700), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_basicParams(basicParams)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for tabs
    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);
    
    // Create tabs
    CreateBasicInfoPanel();
    CreateMaterialPanel();
    CreateTexturePanel();
    CreateRenderingPanel();
    CreateDisplayPanel();
    CreateSubdivisionPanel();
    CreateEdgeSettingsPanel();
    
    // Add panels to notebook
    wxPanel* basicInfoPanel = new wxPanel(notebook);
    basicInfoPanel->SetSizer(mainSizer);
    notebook->AddPage(basicInfoPanel, "Basic Info");
    
    wxPanel* materialPanel = new wxPanel(notebook);
    materialPanel->SetSizer(mainSizer);
    notebook->AddPage(materialPanel, "Material");
    
    wxPanel* texturePanel = new wxPanel(notebook);
    texturePanel->SetSizer(mainSizer);
    notebook->AddPage(texturePanel, "Texture");
    
    wxPanel* renderingPanel = new wxPanel(notebook);
    renderingPanel->SetSizer(mainSizer);
    notebook->AddPage(renderingPanel, "Rendering");
    
    wxPanel* displayPanel = new wxPanel(notebook);
    displayPanel->SetSizer(mainSizer);
    notebook->AddPage(displayPanel, "Display");
    
    wxPanel* subdivisionPanel = new wxPanel(notebook);
    subdivisionPanel->SetSizer(mainSizer);
    notebook->AddPage(subdivisionPanel, "Subdivision");
    
    wxPanel* edgeSettingsPanel = new wxPanel(notebook);
    edgeSettingsPanel->SetSizer(mainSizer);
    notebook->AddPage(edgeSettingsPanel, "Edge Settings");
    
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
}

void VisualSettingsDialog::CreateBasicInfoPanel()
{
    wxPanel* panel = new wxPanel(this);
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

void VisualSettingsDialog::CreateMaterialPanel()
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Diffuse color
    wxStaticText* diffuseLabel = new wxStaticText(panel, wxID_ANY, "Diffuse Color:");
    sizer->Add(diffuseLabel, 0, wxALL, 5);
    
    wxFlexGridSizer* diffuseSizer = new wxFlexGridSizer(3, 2, 5, 5);
    m_diffuseRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.8");
    m_diffuseGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.8");
    m_diffuseBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.8");
    
    diffuseSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL);
    diffuseSizer->Add(m_diffuseRTextCtrl, 1, wxEXPAND);
    diffuseSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL);
    diffuseSizer->Add(m_diffuseGTextCtrl, 1, wxEXPAND);
    diffuseSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL);
    diffuseSizer->Add(m_diffuseBTextCtrl, 1, wxEXPAND);
    sizer->Add(diffuseSizer, 0, wxEXPAND | wxALL, 5);
    
    // Ambient color
    wxStaticText* ambientLabel = new wxStaticText(panel, wxID_ANY, "Ambient Color:");
    sizer->Add(ambientLabel, 0, wxALL, 5);
    
    wxFlexGridSizer* ambientSizer = new wxFlexGridSizer(3, 2, 5, 5);
    m_ambientRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.2");
    m_ambientGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.2");
    m_ambientBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.2");
    
    ambientSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL);
    ambientSizer->Add(m_ambientRTextCtrl, 1, wxEXPAND);
    ambientSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL);
    ambientSizer->Add(m_ambientGTextCtrl, 1, wxEXPAND);
    ambientSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL);
    ambientSizer->Add(m_ambientBTextCtrl, 1, wxEXPAND);
    sizer->Add(ambientSizer, 0, wxEXPAND | wxALL, 5);
    
    // Specular color
    wxStaticText* specularLabel = new wxStaticText(panel, wxID_ANY, "Specular Color:");
    sizer->Add(specularLabel, 0, wxALL, 5);
    
    wxFlexGridSizer* specularSizer = new wxFlexGridSizer(3, 2, 5, 5);
    m_specularRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "1.0");
    m_specularGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "1.0");
    m_specularBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "1.0");
    
    specularSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL);
    specularSizer->Add(m_specularRTextCtrl, 1, wxEXPAND);
    specularSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL);
    specularSizer->Add(m_specularGTextCtrl, 1, wxEXPAND);
    specularSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL);
    specularSizer->Add(m_specularBTextCtrl, 1, wxEXPAND);
    sizer->Add(specularSizer, 0, wxEXPAND | wxALL, 5);
    
    // Emissive color
    wxStaticText* emissiveLabel = new wxStaticText(panel, wxID_ANY, "Emissive Color:");
    sizer->Add(emissiveLabel, 0, wxALL, 5);
    
    wxFlexGridSizer* emissiveSizer = new wxFlexGridSizer(3, 2, 5, 5);
    m_emissiveRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0");
    m_emissiveGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0");
    m_emissiveBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0");
    
    emissiveSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL);
    emissiveSizer->Add(m_emissiveRTextCtrl, 1, wxEXPAND);
    emissiveSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL);
    emissiveSizer->Add(m_emissiveGTextCtrl, 1, wxEXPAND);
    emissiveSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL);
    emissiveSizer->Add(m_emissiveBTextCtrl, 1, wxEXPAND);
    sizer->Add(emissiveSizer, 0, wxEXPAND | wxALL, 5);
    
    // Shininess and transparency
    wxFlexGridSizer* materialSizer = new wxFlexGridSizer(2, 2, 5, 5);
    
    wxStaticText* shininessLabel = new wxStaticText(panel, wxID_ANY, "Shininess:");
    m_shininessTextCtrl = new wxTextCtrl(panel, wxID_ANY, "50.0");
    materialSizer->Add(shininessLabel, 0, wxALIGN_CENTER_VERTICAL);
    materialSizer->Add(m_shininessTextCtrl, 1, wxEXPAND);
    
    wxStaticText* transparencyLabel = new wxStaticText(panel, wxID_ANY, "Transparency:");
    m_transparencyTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0");
    materialSizer->Add(transparencyLabel, 0, wxALIGN_CENTER_VERTICAL);
    materialSizer->Add(m_transparencyTextCtrl, 1, wxEXPAND);
    
    sizer->Add(materialSizer, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void VisualSettingsDialog::CreateTexturePanel()
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Texture enabled checkbox
    m_textureEnabledCheckBox = new wxCheckBox(panel, wxID_ANY, "Enable Texture");
    sizer->Add(m_textureEnabledCheckBox, 0, wxALL, 5);
    
    // Texture path
    wxStaticText* pathLabel = new wxStaticText(panel, wxID_ANY, "Texture Path:");
    sizer->Add(pathLabel, 0, wxALL, 5);
    
    wxBoxSizer* pathSizer = new wxBoxSizer(wxHORIZONTAL);
    m_texturePathTextCtrl = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    m_browseTextureButton = new wxButton(panel, wxID_ANY, "Browse");
    pathSizer->Add(m_texturePathTextCtrl, 1, wxEXPAND);
    pathSizer->Add(m_browseTextureButton, 0, wxLEFT, 5);
    sizer->Add(pathSizer, 0, wxEXPAND | wxALL, 5);
    
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

void VisualSettingsDialog::CreateRenderingPanel()
{
    wxPanel* panel = new wxPanel(this);
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

void VisualSettingsDialog::CreateDisplayPanel()
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Display options
    m_showNormalsCheckBox = new wxCheckBox(panel, wxID_ANY, "Show Normals");
    sizer->Add(m_showNormalsCheckBox, 0, wxALL, 5);
    
    m_showEdgesCheckBox = new wxCheckBox(panel, wxID_ANY, "Show Edges");
    sizer->Add(m_showEdgesCheckBox, 0, wxALL, 5);
    
    m_showWireframeCheckBox = new wxCheckBox(panel, wxID_ANY, "Show Wireframe");
    sizer->Add(m_showWireframeCheckBox, 0, wxALL, 5);
    
    m_showSilhouetteCheckBox = new wxCheckBox(panel, wxID_ANY, "Show Silhouette");
    sizer->Add(m_showSilhouetteCheckBox, 0, wxALL, 5);
    
    m_showFeatureEdgesCheckBox = new wxCheckBox(panel, wxID_ANY, "Show Feature Edges");
    sizer->Add(m_showFeatureEdgesCheckBox, 0, wxALL, 5);
    
    m_showMeshEdgesCheckBox = new wxCheckBox(panel, wxID_ANY, "Show Mesh Edges");
    sizer->Add(m_showMeshEdgesCheckBox, 0, wxALL, 5);
    
    m_showOriginalEdgesCheckBox = new wxCheckBox(panel, wxID_ANY, "Show Original Edges");
    sizer->Add(m_showOriginalEdgesCheckBox, 0, wxALL, 5);
    
    m_showFaceNormalsCheckBox = new wxCheckBox(panel, wxID_ANY, "Show Face Normals");
    sizer->Add(m_showFaceNormalsCheckBox, 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void VisualSettingsDialog::CreateSubdivisionPanel()
{
    wxPanel* panel = new wxPanel(this);
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

void VisualSettingsDialog::CreateEdgeSettingsPanel()
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Edge enabled
    m_edgeEnabledCheckBox = new wxCheckBox(panel, wxID_ANY, "Enable Edges");
    m_edgeEnabledCheckBox->SetValue(true);
    sizer->Add(m_edgeEnabledCheckBox, 0, wxALL, 5);
    
    // Edge type
    wxStaticText* typeLabel = new wxStaticText(panel, wxID_ANY, "Edge Type:");
    sizer->Add(typeLabel, 0, wxALL, 5);
    
    m_edgeTypeChoice = new wxChoice(panel, wxID_ANY);
    m_edgeTypeChoice->Append("Solid");
    m_edgeTypeChoice->Append("Dashed");
    m_edgeTypeChoice->Append("Dotted");
    m_edgeTypeChoice->SetSelection(0);
    sizer->Add(m_edgeTypeChoice, 0, wxEXPAND | wxALL, 5);
    
    // Edge width
    wxStaticText* widthLabel = new wxStaticText(panel, wxID_ANY, "Edge Width:");
    sizer->Add(widthLabel, 0, wxALL, 5);
    
    m_edgeWidthTextCtrl = new wxTextCtrl(panel, wxID_ANY, "1.0");
    sizer->Add(m_edgeWidthTextCtrl, 0, wxEXPAND | wxALL, 5);
    
    // Edge color
    wxStaticText* colorLabel = new wxStaticText(panel, wxID_ANY, "Edge Color:");
    sizer->Add(colorLabel, 0, wxALL, 5);
    
    wxFlexGridSizer* colorSizer = new wxFlexGridSizer(3, 2, 5, 5);
    m_edgeColorRTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0");
    m_edgeColorGTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0");
    m_edgeColorBTextCtrl = new wxTextCtrl(panel, wxID_ANY, "0.0");
    
    colorSizer->Add(new wxStaticText(panel, wxID_ANY, "R:"), 0, wxALIGN_CENTER_VERTICAL);
    colorSizer->Add(m_edgeColorRTextCtrl, 1, wxEXPAND);
    colorSizer->Add(new wxStaticText(panel, wxID_ANY, "G:"), 0, wxALIGN_CENTER_VERTICAL);
    colorSizer->Add(m_edgeColorGTextCtrl, 1, wxEXPAND);
    colorSizer->Add(new wxStaticText(panel, wxID_ANY, "B:"), 0, wxALIGN_CENTER_VERTICAL);
    colorSizer->Add(m_edgeColorBTextCtrl, 1, wxEXPAND);
    sizer->Add(colorSizer, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void VisualSettingsDialog::LoadAdvancedParametersFromControls()
{
    // Load material parameters
    double r, g, b, shininess, transparency;
    
    m_diffuseRTextCtrl->GetValue().ToDouble(&r);
    m_diffuseGTextCtrl->GetValue().ToDouble(&g);
    m_diffuseBTextCtrl->GetValue().ToDouble(&b);
    m_advancedParams.materialDiffuseColor = SbColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    m_ambientRTextCtrl->GetValue().ToDouble(&r);
    m_ambientGTextCtrl->GetValue().ToDouble(&g);
    m_ambientBTextCtrl->GetValue().ToDouble(&b);
    m_advancedParams.materialAmbientColor = SbColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    m_specularRTextCtrl->GetValue().ToDouble(&r);
    m_specularGTextCtrl->GetValue().ToDouble(&g);
    m_specularBTextCtrl->GetValue().ToDouble(&b);
    m_advancedParams.materialSpecularColor = SbColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    m_emissiveRTextCtrl->GetValue().ToDouble(&r);
    m_emissiveGTextCtrl->GetValue().ToDouble(&g);
    m_emissiveBTextCtrl->GetValue().ToDouble(&b);
    m_advancedParams.materialEmissiveColor = SbColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    
    m_shininessTextCtrl->GetValue().ToDouble(&shininess);
    m_advancedParams.materialShininess = static_cast<float>(shininess);
    
    m_transparencyTextCtrl->GetValue().ToDouble(&transparency);
    m_advancedParams.materialTransparency = static_cast<float>(transparency);
    
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
    m_advancedParams.showNormals = m_showNormalsCheckBox->GetValue();
    m_advancedParams.showEdges = m_showEdgesCheckBox->GetValue();
    m_advancedParams.showWireframe = m_showWireframeCheckBox->GetValue();
    m_advancedParams.showSilhouette = m_showSilhouetteCheckBox->GetValue();
    m_advancedParams.showFeatureEdges = m_showFeatureEdgesCheckBox->GetValue();
    m_advancedParams.showMeshEdges = m_showMeshEdgesCheckBox->GetValue();
    m_advancedParams.showOriginalEdges = m_showOriginalEdgesCheckBox->GetValue();
    m_advancedParams.showFaceNormals = m_showFaceNormalsCheckBox->GetValue();
    
    // Load subdivision parameters
    m_advancedParams.subdivisionEnabled = m_subdivisionEnabledCheckBox->GetValue();
    m_advancedParams.subdivisionLevels = m_subdivisionLevelsSpinCtrl->GetValue();
    
    // Load edge settings
    m_advancedParams.edgeEnabled = m_edgeEnabledCheckBox->GetValue();
    
    int edgeTypeIndex = m_edgeTypeChoice->GetSelection();
    switch (edgeTypeIndex) {
        case 0: m_advancedParams.edgeStyle = 0; break; // Solid
        case 1: m_advancedParams.edgeStyle = 1; break; // Dashed
        case 2: m_advancedParams.edgeStyle = 2; break; // Dotted
    }
    
    double edgeWidth;
    m_edgeWidthTextCtrl->GetValue().ToDouble(&edgeWidth);
    m_advancedParams.edgeWidth = static_cast<float>(edgeWidth);
    
    m_edgeColorRTextCtrl->GetValue().ToDouble(&r);
    m_edgeColorGTextCtrl->GetValue().ToDouble(&g);
    m_edgeColorBTextCtrl->GetValue().ToDouble(&b);
    m_advancedParams.edgeColor = SbColor(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
}

void VisualSettingsDialog::SaveAdvancedParametersToControls()
{
    // Save material parameters
    m_diffuseRTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialDiffuseColor[0]));
    m_diffuseGTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialDiffuseColor[1]));
    m_diffuseBTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialDiffuseColor[2]));
    
    m_ambientRTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialAmbientColor[0]));
    m_ambientGTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialAmbientColor[1]));
    m_ambientBTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialAmbientColor[2]));
    
    m_specularRTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialSpecularColor[0]));
    m_specularGTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialSpecularColor[1]));
    m_specularBTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialSpecularColor[2]));
    
    m_emissiveRTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialEmissiveColor[0]));
    m_emissiveGTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialEmissiveColor[1]));
    m_emissiveBTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialEmissiveColor[2]));
    
    m_shininessTextCtrl->SetValue(wxString::Format("%.1f", m_advancedParams.materialShininess));
    m_transparencyTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.materialTransparency));
    
    // Save texture parameters
    m_texturePathTextCtrl->SetValue(m_advancedParams.texturePath);
    m_textureEnabledCheckBox->SetValue(m_advancedParams.textureEnabled);
    
    switch (m_advancedParams.textureMode) {
        case RenderingConfig::TextureMode::Modulate: m_textureModeChoice->SetSelection(0); break;
        case RenderingConfig::TextureMode::Decal: m_textureModeChoice->SetSelection(1); break;
        case RenderingConfig::TextureMode::Blend: m_textureModeChoice->SetSelection(2); break;
        case RenderingConfig::TextureMode::Replace: m_textureModeChoice->SetSelection(3); break;
    }
    
    // Save rendering parameters
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
    
    // Save display parameters
    m_showNormalsCheckBox->SetValue(m_advancedParams.showNormals);
    m_showEdgesCheckBox->SetValue(m_advancedParams.showEdges);
    m_showWireframeCheckBox->SetValue(m_advancedParams.showWireframe);
    m_showSilhouetteCheckBox->SetValue(m_advancedParams.showSilhouette);
    m_showFeatureEdgesCheckBox->SetValue(m_advancedParams.showFeatureEdges);
    m_showMeshEdgesCheckBox->SetValue(m_advancedParams.showMeshEdges);
    m_showOriginalEdgesCheckBox->SetValue(m_advancedParams.showOriginalEdges);
    m_showFaceNormalsCheckBox->SetValue(m_advancedParams.showFaceNormals);
    
    // Save subdivision parameters
    m_subdivisionEnabledCheckBox->SetValue(m_advancedParams.subdivisionEnabled);
    m_subdivisionLevelsSpinCtrl->SetValue(m_advancedParams.subdivisionLevels);
    
    // Save edge settings
    m_edgeEnabledCheckBox->SetValue(m_advancedParams.edgeEnabled);
    
    switch (m_advancedParams.edgeStyle) {
        case 0: m_edgeTypeChoice->SetSelection(0); break; // Solid
        case 1: m_edgeTypeChoice->SetSelection(1); break; // Dashed
        case 2: m_edgeTypeChoice->SetSelection(2); break; // Dotted
    }
    
    m_edgeWidthTextCtrl->SetValue(wxString::Format("%.1f", m_advancedParams.edgeWidth));
    
    m_edgeColorRTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.edgeColor[0]));
    m_edgeColorGTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.edgeColor[1]));
    m_edgeColorBTextCtrl->SetValue(wxString::Format("%.2f", m_advancedParams.edgeColor[2]));
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
    // TODO: Apply changes without closing dialog
} 