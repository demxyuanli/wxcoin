#include "opencascade/geometry/helper/DisplayModeConfigDialog.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "config/FontManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/scrolwin.h>
#include <sstream>
#include <iomanip>

DisplayModeConfigDialog::DisplayModeConfigDialog(wxWindow* parent)
    : FramelessModalPopup(parent, "Display Mode Configuration", wxSize(1200, 800))
    , m_notebook(nullptr)
    , m_customModeKey(RenderingConfig::DisplayMode::Custom)
    , m_applyButton(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_resetButton(nullptr)
    , m_splitter(nullptr)
    , m_previewCanvas(nullptr)
{
    m_defaultContext.material.ambientColor = Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB);
    m_defaultContext.material.diffuseColor = Quantity_Color(0.95, 0.95, 0.95, Quantity_TOC_RGB);
    m_defaultContext.material.specularColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    m_defaultContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    m_defaultContext.material.shininess = 50.0;
    m_defaultContext.material.transparency = 0.0;
    m_defaultContext.display.wireframeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    m_defaultContext.display.wireframeWidth = 1.0;
    m_defaultContext.display.showSolidWithPointView = true;
    
    SetTitleIcon("render", wxSize(20, 20));
    ShowTitleIcon(true);
    
    createControls();
    layoutControls();
    bindEvents();
    
    for (auto& pair : m_modeControls) {
        loadConfigForMode(pair.first);
    }
    updateControls();
    
    applyThemeAndFonts();
    
    for (auto& pair : m_modeControls) {
        updateModeVisibility(pair.first);
    }
}

DisplayModeConfigDialog::~DisplayModeConfigDialog()
{
}

DisplayModeConfig DisplayModeConfigDialog::getConfig(RenderingConfig::DisplayMode mode) const
{
    auto it = m_modeControls.find(mode);
    if (it != m_modeControls.end()) {
        return it->second.config;
    }
    return DisplayModeConfigFactory::getConfig(mode, m_defaultContext);
}

void DisplayModeConfigDialog::createControls()
{
    m_applyButton = new wxButton(m_contentPanel, wxID_APPLY, "Apply");
    m_okButton = new wxButton(m_contentPanel, wxID_OK, "OK");
    m_cancelButton = new wxButton(m_contentPanel, wxID_CANCEL, "Cancel");
    m_resetButton = new wxButton(m_contentPanel, wxID_ANY, "Reset to Defaults");
}

void DisplayModeConfigDialog::createModePage(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    controls.page = new wxPanel(m_notebook);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxScrolledWindow* scrolled = new wxScrolledWindow(controls.page);
    scrolled->SetScrollRate(10, 10);
    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    scrolled->SetSizer(scrolledSizer);
    
    createNodeRequirementsPanel(scrolled, scrolledSizer, mode);
    createRenderingPropertiesPanel(scrolled, scrolledSizer, mode);
    createEdgeConfigPanel(scrolled, scrolledSizer, mode);
    createPostProcessingPanel(scrolled, scrolledSizer, mode);
    scrolledSizer->Fit(scrolled);
    
    mainSizer->Add(scrolled, 1, wxEXPAND | wxALL, 5);
    controls.page->SetSizer(mainSizer);
    
    wxString modeName;
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        modeName = "No Shading";
        break;
    case RenderingConfig::DisplayMode::Points:
        modeName = "Points";
        break;
    case RenderingConfig::DisplayMode::Wireframe:
        modeName = "Wireframe";
        break;
    case RenderingConfig::DisplayMode::Solid:
        modeName = "Solid";
        break;
    case RenderingConfig::DisplayMode::FlatLines:
        modeName = "Flat Lines";
        break;
    case RenderingConfig::DisplayMode::Transparent:
        modeName = "Transparent";
        break;
    case RenderingConfig::DisplayMode::HiddenLine:
        modeName = "Hidden Line";
        break;
    }
    
    m_notebook->AddPage(controls.page, modeName);
}

void DisplayModeConfigDialog::createNodeRequirementsPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    wxStaticBoxSizer* boxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Node Requirements");
    controls.nodeRequirementsBox = boxSizer->GetStaticBox();
    
    controls.requireSurface = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "Require Surface");
    controls.requireOriginalEdges = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "Require Original Edges");
    controls.requireMeshEdges = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "Require Mesh Edges");
    controls.requirePoints = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "Require Points");
    
    boxSizer->Add(controls.requireSurface, 0, wxALL, 5);
    boxSizer->Add(controls.requireOriginalEdges, 0, wxALL, 5);
    boxSizer->Add(controls.requireMeshEdges, 0, wxALL, 5);
    boxSizer->Add(controls.requirePoints, 0, wxALL, 5);
    
    sizer->Add(boxSizer, 0, wxEXPAND | wxALL, 5);
}

void DisplayModeConfigDialog::createRenderingPropertiesPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    wxStaticBoxSizer* boxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Rendering Properties");
    controls.renderingPropertiesBox = boxSizer->GetStaticBox();
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 5, 5);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Light Model:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.lightModel = new wxChoice(boxSizer->GetStaticBox(), wxID_ANY);
    controls.lightModel->Append("BASE_COLOR");
    controls.lightModel->Append("PHONG");
    controls.lightModel->SetSelection(1);
    gridSizer->Add(controls.lightModel, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Texture Enabled:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.textureEnabled = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "");
    gridSizer->Add(controls.textureEnabled, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Blend Mode:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.blendMode = new wxChoice(boxSizer->GetStaticBox(), wxID_ANY);
    controls.blendMode->Append("None");
    controls.blendMode->Append("Alpha");
    controls.blendMode->Append("Additive");
    controls.blendMode->Append("Multiply");
    controls.blendMode->Append("Screen");
    controls.blendMode->Append("Overlay");
    controls.blendMode->SetSelection(0);
    gridSizer->Add(controls.blendMode, 0, wxEXPAND);
    
    boxSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);
    
    wxStaticLine* line = new wxStaticLine(boxSizer->GetStaticBox());
    boxSizer->Add(line, 0, wxEXPAND | wxALL, 5);
    
    wxStaticText* materialLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Material Override:");
    boxSizer->Add(materialLabel, 0, wxALL, 5);
    
    controls.materialOverrideEnabled = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "Enable Material Override");
    boxSizer->Add(controls.materialOverrideEnabled, 0, wxALL, 5);
    
    wxFlexGridSizer* materialGrid = new wxFlexGridSizer(2, 5, 5);
    materialGrid->AddGrowableCol(1);
    
    materialGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Ambient Color:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.materialAmbientColor = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 25));
    materialGrid->Add(controls.materialAmbientColor, 0, wxEXPAND);
    
    materialGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Diffuse Color:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.materialDiffuseColor = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 25));
    materialGrid->Add(controls.materialDiffuseColor, 0, wxEXPAND);
    
    materialGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Specular Color:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.materialSpecularColor = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 25));
    materialGrid->Add(controls.materialSpecularColor, 0, wxEXPAND);
    
    materialGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Emissive Color:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.materialEmissiveColor = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 25));
    materialGrid->Add(controls.materialEmissiveColor, 0, wxEXPAND);
    
    materialGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Shininess:"), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* shininessSizer = new wxBoxSizer(wxHORIZONTAL);
    controls.materialShininess = new wxSlider(boxSizer->GetStaticBox(), wxID_ANY, 0, 0, 1280, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    controls.materialShininessLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "0.0");
    controls.materialShininessLabel->SetMinSize(wxSize(50, -1));
    shininessSizer->Add(controls.materialShininess, 1, wxEXPAND | wxRIGHT, 5);
    shininessSizer->Add(controls.materialShininessLabel, 0, wxALIGN_CENTER_VERTICAL);
    materialGrid->Add(shininessSizer, 0, wxEXPAND);
    
    materialGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Transparency:"), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* transparencySizer = new wxBoxSizer(wxHORIZONTAL);
    controls.materialTransparency = new wxSlider(boxSizer->GetStaticBox(), wxID_ANY, 0, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    controls.materialTransparencyLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "0.00");
    controls.materialTransparencyLabel->SetMinSize(wxSize(50, -1));
    transparencySizer->Add(controls.materialTransparency, 1, wxEXPAND | wxRIGHT, 5);
    transparencySizer->Add(controls.materialTransparencyLabel, 0, wxALIGN_CENTER_VERTICAL);
    materialGrid->Add(transparencySizer, 0, wxEXPAND);
    
    boxSizer->Add(materialGrid, 0, wxEXPAND | wxALL, 5);
    
    sizer->Add(boxSizer, 0, wxEXPAND | wxALL, 5);
}

void DisplayModeConfigDialog::createEdgeConfigPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    wxStaticBoxSizer* boxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Edge Configuration");
    controls.edgeConfigBox = boxSizer->GetStaticBox();
    
    wxStaticText* originalLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Original Edge:");
    boxSizer->Add(originalLabel, 0, wxALL, 5);
    
    controls.originalEdgeEnabled = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "Enable Original Edge");
    boxSizer->Add(controls.originalEdgeEnabled, 0, wxALL, 5);
    
    wxFlexGridSizer* originalGrid = new wxFlexGridSizer(2, 5, 5);
    originalGrid->AddGrowableCol(1);
    
    originalGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Color:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.originalEdgeColor = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 25));
    originalGrid->Add(controls.originalEdgeColor, 0, wxEXPAND);
    
    originalGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* originalWidthSizer = new wxBoxSizer(wxHORIZONTAL);
    controls.originalEdgeWidth = new wxSlider(boxSizer->GetStaticBox(), wxID_ANY, 10, 1, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    controls.originalEdgeWidthLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "1.0");
    controls.originalEdgeWidthLabel->SetMinSize(wxSize(50, -1));
    originalWidthSizer->Add(controls.originalEdgeWidth, 1, wxEXPAND | wxRIGHT, 5);
    originalWidthSizer->Add(controls.originalEdgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
    originalGrid->Add(originalWidthSizer, 0, wxEXPAND);
    
    boxSizer->Add(originalGrid, 0, wxEXPAND | wxALL, 5);
    
    controls.meshEdgeSeparator = new wxStaticLine(boxSizer->GetStaticBox());
    boxSizer->Add(controls.meshEdgeSeparator, 0, wxEXPAND | wxALL, 5);
    
    controls.meshEdgeLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Mesh Edge:");
    boxSizer->Add(controls.meshEdgeLabel, 0, wxALL, 5);
    
    controls.meshEdgeEnabled = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "Enable Mesh Edge");
    boxSizer->Add(controls.meshEdgeEnabled, 0, wxALL, 5);
    
    wxFlexGridSizer* meshGrid = new wxFlexGridSizer(2, 5, 5);
    meshGrid->AddGrowableCol(1);
    
    meshGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Color:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.meshEdgeColor = new wxButton(boxSizer->GetStaticBox(), wxID_ANY, "Choose Color", wxDefaultPosition, wxSize(100, 25));
    meshGrid->Add(controls.meshEdgeColor, 0, wxEXPAND);
    
    meshGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* meshWidthSizer = new wxBoxSizer(wxHORIZONTAL);
    controls.meshEdgeWidth = new wxSlider(boxSizer->GetStaticBox(), wxID_ANY, 10, 1, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    controls.meshEdgeWidthLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "1.0");
    controls.meshEdgeWidthLabel->SetMinSize(wxSize(50, -1));
    meshWidthSizer->Add(controls.meshEdgeWidth, 1, wxEXPAND | wxRIGHT, 5);
    meshWidthSizer->Add(controls.meshEdgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
    meshGrid->Add(meshWidthSizer, 0, wxEXPAND);
    
    meshGrid->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Use Effective Color:"), 0, wxALIGN_CENTER_VERTICAL);
    controls.meshEdgeUseEffectiveColor = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "");
    meshGrid->Add(controls.meshEdgeUseEffectiveColor, 0, wxEXPAND);
    
    boxSizer->Add(meshGrid, 0, wxEXPAND | wxALL, 5);
    
    sizer->Add(boxSizer, 0, wxEXPAND | wxALL, 5);
}

void DisplayModeConfigDialog::createPostProcessingPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    wxStaticBoxSizer* boxSizer = new wxStaticBoxSizer(wxVERTICAL, parent, "Post-Processing");
    controls.postProcessingBox = boxSizer->GetStaticBox();
    
    controls.polygonOffsetEnabled = new wxCheckBox(boxSizer->GetStaticBox(), wxID_ANY, "Enable Polygon Offset");
    boxSizer->Add(controls.polygonOffsetEnabled, 0, wxALL, 5);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 5, 5);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Factor:"), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* factorSizer = new wxBoxSizer(wxHORIZONTAL);
    controls.polygonOffsetFactor = new wxSlider(boxSizer->GetStaticBox(), wxID_ANY, 0, -100, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    controls.polygonOffsetFactorLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "0.0");
    controls.polygonOffsetFactorLabel->SetMinSize(wxSize(50, -1));
    factorSizer->Add(controls.polygonOffsetFactor, 1, wxEXPAND | wxRIGHT, 5);
    factorSizer->Add(controls.polygonOffsetFactorLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(factorSizer, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "Units:"), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* unitsSizer = new wxBoxSizer(wxHORIZONTAL);
    controls.polygonOffsetUnits = new wxSlider(boxSizer->GetStaticBox(), wxID_ANY, 0, -100, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
    controls.polygonOffsetUnitsLabel = new wxStaticText(boxSizer->GetStaticBox(), wxID_ANY, "0.0");
    controls.polygonOffsetUnitsLabel->SetMinSize(wxSize(50, -1));
    unitsSizer->Add(controls.polygonOffsetUnits, 1, wxEXPAND | wxRIGHT, 5);
    unitsSizer->Add(controls.polygonOffsetUnitsLabel, 0, wxALIGN_CENTER_VERTICAL);
    gridSizer->Add(unitsSizer, 0, wxEXPAND);
    
    boxSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);
    
    sizer->Add(boxSizer, 0, wxEXPAND | wxALL, 5);
}

void DisplayModeConfigDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxPanel* leftPanel = new wxPanel(m_contentPanel);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    
    m_notebook = new wxNotebook(leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_LEFT);
    m_notebook->SetMinSize(wxSize(360, -1));
    
    createModePage(RenderingConfig::DisplayMode::NoShading);
    createModePage(RenderingConfig::DisplayMode::Points);
    createModePage(RenderingConfig::DisplayMode::Wireframe);
    createModePage(RenderingConfig::DisplayMode::Solid);
    createModePage(RenderingConfig::DisplayMode::FlatLines);
    createModePage(RenderingConfig::DisplayMode::Transparent);
    createModePage(RenderingConfig::DisplayMode::HiddenLine);
    createCustomModePage();
    
    leftSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
    leftPanel->SetSizer(leftSizer);
    leftPanel->SetMinSize(wxSize(360, -1));
    
    wxPanel* rightPanel = new wxPanel(m_contentPanel);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* previewLabel = new wxStaticText(rightPanel, wxID_ANY, "Preview");
    wxFont labelFont = previewLabel->GetFont();
    labelFont.SetPointSize(labelFont.GetPointSize() + 1);
    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    previewLabel->SetFont(labelFont);
    rightSizer->Add(previewLabel, 0, wxALL, 10);
    
    m_previewCanvas = new DisplayModePreviewCanvas(rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    rightSizer->Add(m_previewCanvas, 1, wxEXPAND | wxALL, 5);
    
    rightPanel->SetSizer(rightSizer);
    
    contentSizer->Add(leftPanel, 0, wxEXPAND | wxALL, 5);
    contentSizer->Add(rightPanel, 1, wxEXPAND | wxALL, 5);
    
    mainSizer->Add(contentSizer, 1, wxEXPAND | wxALL, 5);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(m_resetButton, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_applyButton, 0, wxALL, 5);
    buttonSizer->Add(m_okButton, 0, wxALL, 5);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    m_contentPanel->SetSizer(mainSizer);
    
    updatePreview();
}

RenderingConfig::DisplayMode DisplayModeConfigDialog::getModeFromPageIndex(int pageIndex) const
{
    static const RenderingConfig::DisplayMode modeOrder[] = {
        RenderingConfig::DisplayMode::NoShading,
        RenderingConfig::DisplayMode::Points,
        RenderingConfig::DisplayMode::Wireframe,
        RenderingConfig::DisplayMode::Solid,
        RenderingConfig::DisplayMode::FlatLines,
        RenderingConfig::DisplayMode::Transparent,
        RenderingConfig::DisplayMode::HiddenLine,
        RenderingConfig::DisplayMode::Custom
    };
    static const int modeOrderSize = 8;
    
    if (pageIndex >= 0 && pageIndex < modeOrderSize) {
        return modeOrder[pageIndex];
    }
    return RenderingConfig::DisplayMode::Solid;
}

void DisplayModeConfigDialog::bindEvents()
{
    m_applyButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onApply, this);
    m_okButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onOK, this);
    m_cancelButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onCancel, this);
    m_resetButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onReset, this);
    
    m_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& event) {
        int currentPage = m_notebook->GetSelection();
        if (currentPage >= 0) {
            RenderingConfig::DisplayMode mode = this->getModeFromPageIndex(currentPage);
            updateModeVisibility(mode);
            ModeControls& controls = m_modeControls[mode];

            // Update all controls from config for this mode
            controls.requireSurface->SetValue(controls.config.nodes.requireSurface);
            controls.requireOriginalEdges->SetValue(controls.config.nodes.requireOriginalEdges);
            controls.requireMeshEdges->SetValue(controls.config.nodes.requireMeshEdges);
            controls.requirePoints->SetValue(controls.config.nodes.requirePoints);
            
            controls.lightModel->SetSelection(static_cast<int>(controls.config.rendering.lightModel));
            controls.textureEnabled->SetValue(controls.config.rendering.textureEnabled);
            
            int blendModeIndex = 0;
            switch (controls.config.rendering.blendMode) {
            case RenderingConfig::BlendMode::None: blendModeIndex = 0; break;
            case RenderingConfig::BlendMode::Alpha: blendModeIndex = 1; break;
            case RenderingConfig::BlendMode::Additive: blendModeIndex = 2; break;
            case RenderingConfig::BlendMode::Multiply: blendModeIndex = 3; break;
            case RenderingConfig::BlendMode::Screen: blendModeIndex = 4; break;
            case RenderingConfig::BlendMode::Overlay: blendModeIndex = 5; break;
            }
            controls.blendMode->SetSelection(blendModeIndex);
            controls.blendMode->Refresh();
            
            controls.materialOverrideEnabled->SetValue(controls.config.rendering.materialOverride.enabled);
            updateColorButton(controls.materialAmbientColor, quantityColorToWxColour(controls.config.rendering.materialOverride.ambientColor));
            updateColorButton(controls.materialDiffuseColor, quantityColorToWxColour(controls.config.rendering.materialOverride.diffuseColor));
            updateColorButton(controls.materialSpecularColor, quantityColorToWxColour(controls.config.rendering.materialOverride.specularColor));
            updateColorButton(controls.materialEmissiveColor, quantityColorToWxColour(controls.config.rendering.materialOverride.emissiveColor));
            if (controls.materialShininess) {
                controls.materialShininess->SetValue(static_cast<int>(controls.config.rendering.materialOverride.shininess * 10.0));
                if (controls.materialShininessLabel) {
                    controls.materialShininessLabel->SetLabel(wxString::Format("%.1f", controls.config.rendering.materialOverride.shininess));
                }
            }
            if (controls.materialTransparency) {
                int transparencyValue = static_cast<int>(controls.config.rendering.materialOverride.transparency * 100.0);
                controls.materialTransparency->SetValue(transparencyValue);
                if (controls.materialTransparencyLabel) {
                    controls.materialTransparencyLabel->SetLabel(wxString::Format("%.2f", controls.config.rendering.materialOverride.transparency));
                    controls.materialTransparencyLabel->Refresh();
                }
                controls.materialTransparency->Refresh();
            }
            
            if (controls.originalEdgeEnabled && controls.originalEdgeEnabled->IsShown()) {
                controls.originalEdgeEnabled->SetValue(controls.config.edges.originalEdge.enabled);
            }
            updateColorButton(controls.originalEdgeColor, quantityColorToWxColour(controls.config.edges.originalEdge.color));
            if (controls.originalEdgeWidth) {
                controls.originalEdgeWidth->SetValue(static_cast<int>(controls.config.edges.originalEdge.width * 10.0));
                if (controls.originalEdgeWidthLabel) {
                    controls.originalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.originalEdge.width));
                }
            }
            
            controls.meshEdgeEnabled->SetValue(controls.config.edges.meshEdge.enabled);
            updateColorButton(controls.meshEdgeColor, quantityColorToWxColour(controls.config.edges.meshEdge.color));
            if (controls.meshEdgeWidth) {
                controls.meshEdgeWidth->SetValue(static_cast<int>(controls.config.edges.meshEdge.width * 10.0));
                if (controls.meshEdgeWidthLabel) {
                    controls.meshEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.meshEdge.width));
                }
            }
            controls.meshEdgeUseEffectiveColor->SetValue(controls.config.edges.meshEdge.useEffectiveColor);
            
            controls.polygonOffsetEnabled->SetValue(controls.config.postProcessing.polygonOffset.enabled);
            if (controls.polygonOffsetFactor) {
                controls.polygonOffsetFactor->SetValue(static_cast<int>(controls.config.postProcessing.polygonOffset.factor * 10.0));
                if (controls.polygonOffsetFactorLabel) {
                    controls.polygonOffsetFactorLabel->SetLabel(wxString::Format("%.1f", controls.config.postProcessing.polygonOffset.factor));
                }
            }
            if (controls.polygonOffsetUnits) {
                controls.polygonOffsetUnits->SetValue(static_cast<int>(controls.config.postProcessing.polygonOffset.units * 10.0));
                if (controls.polygonOffsetUnitsLabel) {
                    controls.polygonOffsetUnitsLabel->SetLabel(wxString::Format("%.1f", controls.config.postProcessing.polygonOffset.units));
                }
            }
            
            // Force layout update to ensure all controls are properly displayed
            if (controls.page) {
                controls.page->Layout();
            }
        }
        updatePreview();
    });
    
    for (auto& pair : m_modeControls) {
        RenderingConfig::DisplayMode mode = pair.first;
        ModeControls& controls = pair.second;
        
        controls.materialAmbientColor->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.materialDiffuseColor->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.materialSpecularColor->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.materialEmissiveColor->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.originalEdgeColor->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.meshEdgeColor->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        
        controls.requireSurface->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updateConfigFromControls(currentMode);
                updatePreview();
            }
        });
        controls.requireOriginalEdges->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                if (currentMode == RenderingConfig::DisplayMode::FlatLines || currentMode == RenderingConfig::DisplayMode::Solid) {
                    ModeControls& ctrl = m_modeControls[currentMode];
                    bool requireOriginalEdges = ctrl.requireOriginalEdges->GetValue();
                    if (ctrl.originalEdgeEnabled && ctrl.originalEdgeEnabled->IsShown()) {
                        ctrl.originalEdgeEnabled->SetValue(requireOriginalEdges);
                    }
                }
                updateConfigFromControls(currentMode);
                updatePreview();
            }
        });
        controls.requireMeshEdges->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) { updatePreview(); });
        controls.requirePoints->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) { updatePreview(); });
        controls.lightModel->Bind(wxEVT_COMMAND_CHOICE_SELECTED, [this](wxCommandEvent&) { updatePreview(); });
        controls.blendMode->Bind(wxEVT_COMMAND_CHOICE_SELECTED, [this](wxCommandEvent&) { updatePreview(); });
        controls.materialOverrideEnabled->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) { updatePreview(); });
        controls.materialShininess->Bind(wxEVT_SLIDER, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.materialShininess->GetValue()) / 10.0;
                ctrl.materialShininessLabel->SetLabel(wxString::Format("%.1f", value));
                updatePreview();
            }
        });
        controls.materialTransparency->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                if (ctrl.materialTransparency && ctrl.materialTransparencyLabel) {
                    int sliderValue = ctrl.materialTransparency->GetValue();
                    double value = static_cast<double>(sliderValue) / 100.0;
                    ctrl.materialTransparencyLabel->SetLabel(wxString::Format("%.2f", value));
                }
                updatePreview();
            }
        });

        controls.originalEdgeEnabled->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) {
            // Update preview normally, but this control is disabled in Solid and FlatLines modes
            updatePreview();
        });
        controls.originalEdgeWidth->Bind(wxEVT_SLIDER, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.originalEdgeWidth->GetValue()) / 10.0;
                ctrl.originalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", value));
                updatePreview();
            }
        });
        controls.meshEdgeEnabled->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) { updatePreview(); });
        controls.meshEdgeWidth->Bind(wxEVT_SLIDER, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.meshEdgeWidth->GetValue()) / 10.0;
                ctrl.meshEdgeWidthLabel->SetLabel(wxString::Format("%.1f", value));
                updatePreview();
            }
        });
        controls.meshEdgeUseEffectiveColor->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) { updatePreview(); });
        controls.polygonOffsetEnabled->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [this](wxCommandEvent&) { updatePreview(); });
        controls.polygonOffsetFactor->Bind(wxEVT_SLIDER, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.polygonOffsetFactor->GetValue()) / 10.0;
                ctrl.polygonOffsetFactorLabel->SetLabel(wxString::Format("%.1f", value));
                updatePreview();
            }
        });
        controls.polygonOffsetUnits->Bind(wxEVT_SLIDER, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.polygonOffsetUnits->GetValue()) / 10.0;
                ctrl.polygonOffsetUnitsLabel->SetLabel(wxString::Format("%.1f", value));
                updatePreview();
            }
        });
    }
}

void DisplayModeConfigDialog::updateControls()
{
    for (auto& pair : m_modeControls) {
        RenderingConfig::DisplayMode mode = pair.first;
        ModeControls& controls = pair.second;
        
        controls.requireSurface->SetValue(controls.config.nodes.requireSurface);
        controls.requireOriginalEdges->SetValue(controls.config.nodes.requireOriginalEdges);
        controls.requireMeshEdges->SetValue(controls.config.nodes.requireMeshEdges);
        controls.requirePoints->SetValue(controls.config.nodes.requirePoints);
        
        controls.lightModel->SetSelection(static_cast<int>(controls.config.rendering.lightModel));
        controls.textureEnabled->SetValue(controls.config.rendering.textureEnabled);
        
        int blendModeIndex = 0;
        switch (controls.config.rendering.blendMode) {
        case RenderingConfig::BlendMode::None: blendModeIndex = 0; break;
        case RenderingConfig::BlendMode::Alpha: blendModeIndex = 1; break;
        case RenderingConfig::BlendMode::Additive: blendModeIndex = 2; break;
        case RenderingConfig::BlendMode::Multiply: blendModeIndex = 3; break;
        case RenderingConfig::BlendMode::Screen: blendModeIndex = 4; break;
        case RenderingConfig::BlendMode::Overlay: blendModeIndex = 5; break;
        }
        controls.blendMode->SetSelection(blendModeIndex);
        
        controls.materialOverrideEnabled->SetValue(controls.config.rendering.materialOverride.enabled);
        updateColorButton(controls.materialAmbientColor, quantityColorToWxColour(controls.config.rendering.materialOverride.ambientColor));
        updateColorButton(controls.materialDiffuseColor, quantityColorToWxColour(controls.config.rendering.materialOverride.diffuseColor));
        updateColorButton(controls.materialSpecularColor, quantityColorToWxColour(controls.config.rendering.materialOverride.specularColor));
        updateColorButton(controls.materialEmissiveColor, quantityColorToWxColour(controls.config.rendering.materialOverride.emissiveColor));
        controls.materialShininess->SetValue(static_cast<int>(controls.config.rendering.materialOverride.shininess * 10.0));
        controls.materialShininessLabel->SetLabel(wxString::Format("%.1f", controls.config.rendering.materialOverride.shininess));
        controls.materialTransparency->SetValue(static_cast<int>(controls.config.rendering.materialOverride.transparency * 100.0));
        controls.materialTransparencyLabel->SetLabel(wxString::Format("%.2f", controls.config.rendering.materialOverride.transparency));
        
        // Apply force sync for Solid and FlatLines modes: originalEdge.enabled must match requireOriginalEdges
        if (mode == RenderingConfig::DisplayMode::FlatLines || mode == RenderingConfig::DisplayMode::Solid) {
            controls.config.edges.originalEdge.enabled = controls.config.nodes.requireOriginalEdges;
        }
        controls.originalEdgeEnabled->SetValue(controls.config.edges.originalEdge.enabled);
        updateColorButton(controls.originalEdgeColor, quantityColorToWxColour(controls.config.edges.originalEdge.color));
        controls.originalEdgeWidth->SetValue(static_cast<int>(controls.config.edges.originalEdge.width * 10.0));
        controls.originalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.originalEdge.width));
        
        controls.meshEdgeEnabled->SetValue(controls.config.edges.meshEdge.enabled);
        updateColorButton(controls.meshEdgeColor, quantityColorToWxColour(controls.config.edges.meshEdge.color));
        controls.meshEdgeWidth->SetValue(static_cast<int>(controls.config.edges.meshEdge.width * 10.0));
        controls.meshEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.meshEdge.width));
        controls.meshEdgeUseEffectiveColor->SetValue(controls.config.edges.meshEdge.useEffectiveColor);
        
        controls.polygonOffsetEnabled->SetValue(controls.config.postProcessing.polygonOffset.enabled);
        controls.polygonOffsetFactor->SetValue(static_cast<int>(controls.config.postProcessing.polygonOffset.factor * 10.0));
        controls.polygonOffsetFactorLabel->SetLabel(wxString::Format("%.1f", controls.config.postProcessing.polygonOffset.factor));
        controls.polygonOffsetUnits->SetValue(static_cast<int>(controls.config.postProcessing.polygonOffset.units * 10.0));
        controls.polygonOffsetUnitsLabel->SetLabel(wxString::Format("%.1f", controls.config.postProcessing.polygonOffset.units));
    }
}

void DisplayModeConfigDialog::loadConfigForMode(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    controls.config = DisplayModeConfigFactory::getConfig(mode, m_defaultContext);
}

void DisplayModeConfigDialog::saveConfigForMode(RenderingConfig::DisplayMode mode)
{
    updateConfigFromControls(mode);
}

void DisplayModeConfigDialog::updateConfigFromControls(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    if (controls.requireSurface && controls.requireSurface->IsShown()) {
        controls.config.nodes.requireSurface = controls.requireSurface->GetValue();
    }
    if (controls.requireOriginalEdges && controls.requireOriginalEdges->IsShown()) {
        controls.config.nodes.requireOriginalEdges = controls.requireOriginalEdges->GetValue();
    }
    if (controls.requireMeshEdges && controls.requireMeshEdges->IsShown()) {
        controls.config.nodes.requireMeshEdges = controls.requireMeshEdges->GetValue();
    }
    if (controls.requirePoints && controls.requirePoints->IsShown()) {
        controls.config.nodes.requirePoints = controls.requirePoints->GetValue();
    }
    
    if (controls.lightModel && controls.lightModel->IsShown()) {
        controls.config.rendering.lightModel = static_cast<DisplayModeConfig::RenderingProperties::LightModel>(controls.lightModel->GetSelection());
    }
    if (controls.textureEnabled && controls.textureEnabled->IsShown()) {
        controls.config.rendering.textureEnabled = controls.textureEnabled->GetValue();
    }
    
    int blendModeIndex = 0;
    if (controls.blendMode && controls.blendMode->IsShown()) {
        blendModeIndex = controls.blendMode->GetSelection();
    }
    switch (blendModeIndex) {
    case 0: controls.config.rendering.blendMode = RenderingConfig::BlendMode::None; break;
    case 1: controls.config.rendering.blendMode = RenderingConfig::BlendMode::Alpha; break;
    case 2: controls.config.rendering.blendMode = RenderingConfig::BlendMode::Additive; break;
    case 3: controls.config.rendering.blendMode = RenderingConfig::BlendMode::Multiply; break;
    case 4: controls.config.rendering.blendMode = RenderingConfig::BlendMode::Screen; break;
    case 5: controls.config.rendering.blendMode = RenderingConfig::BlendMode::Overlay; break;
    }
    
    if (controls.materialOverrideEnabled && controls.materialOverrideEnabled->IsShown()) {
        controls.config.rendering.materialOverride.enabled = controls.materialOverrideEnabled->GetValue();
    }
    wxColour ambientColour = controls.materialAmbientColor->GetBackgroundColour();
    if (ambientColour.IsOk()) {
        controls.config.rendering.materialOverride.ambientColor = wxColourToQuantityColor(ambientColour);
    }
    wxColour diffuseColour = controls.materialDiffuseColor->GetBackgroundColour();
    if (diffuseColour.IsOk()) {
        controls.config.rendering.materialOverride.diffuseColor = wxColourToQuantityColor(diffuseColour);
    }
    wxColour specularColour = controls.materialSpecularColor->GetBackgroundColour();
    if (specularColour.IsOk()) {
        controls.config.rendering.materialOverride.specularColor = wxColourToQuantityColor(specularColour);
    }
    wxColour emissiveColour = controls.materialEmissiveColor->GetBackgroundColour();
    if (emissiveColour.IsOk()) {
        controls.config.rendering.materialOverride.emissiveColor = wxColourToQuantityColor(emissiveColour);
    }
    if (controls.materialShininess && controls.materialShininess->IsShown()) {
        controls.config.rendering.materialOverride.shininess = static_cast<double>(controls.materialShininess->GetValue()) / 10.0;
    }
    if (controls.materialTransparency && controls.materialTransparency->IsShown()) {
        int sliderValue = controls.materialTransparency->GetValue();
        controls.config.rendering.materialOverride.transparency = static_cast<double>(sliderValue) / 100.0;
    }
    
    if (mode == RenderingConfig::DisplayMode::FlatLines || mode == RenderingConfig::DisplayMode::Solid) {
        // Force sync: original edge display state must match the data requirement exactly
        controls.config.edges.originalEdge.enabled = controls.config.nodes.requireOriginalEdges;
        if (controls.originalEdgeEnabled && controls.originalEdgeEnabled->IsShown()) {
            controls.originalEdgeEnabled->SetValue(controls.config.nodes.requireOriginalEdges);
        }
    } else {
        if (controls.originalEdgeEnabled && controls.originalEdgeEnabled->IsShown()) {
            controls.config.edges.originalEdge.enabled = controls.originalEdgeEnabled->GetValue();
        } else {
            controls.config.edges.originalEdge.enabled = false;
        }
    }
    wxColour originalEdgeColour = controls.originalEdgeColor->GetBackgroundColour();
    if (originalEdgeColour.IsOk()) {
        controls.config.edges.originalEdge.color = wxColourToQuantityColor(originalEdgeColour);
    }
    controls.config.edges.originalEdge.width = static_cast<double>(controls.originalEdgeWidth->GetValue()) / 10.0;
    
    controls.config.edges.meshEdge.enabled = controls.meshEdgeEnabled->GetValue();
    wxColour meshEdgeColour = controls.meshEdgeColor->GetBackgroundColour();
    if (meshEdgeColour.IsOk()) {
        controls.config.edges.meshEdge.color = wxColourToQuantityColor(meshEdgeColour);
    }
    controls.config.edges.meshEdge.width = static_cast<double>(controls.meshEdgeWidth->GetValue()) / 10.0;
    controls.config.edges.meshEdge.useEffectiveColor = controls.meshEdgeUseEffectiveColor->GetValue();
    
    controls.config.postProcessing.polygonOffset.enabled = controls.polygonOffsetEnabled->GetValue();
    controls.config.postProcessing.polygonOffset.factor = static_cast<double>(controls.polygonOffsetFactor->GetValue()) / 10.0;
    controls.config.postProcessing.polygonOffset.units = static_cast<double>(controls.polygonOffsetUnits->GetValue()) / 10.0;
}

wxColour DisplayModeConfigDialog::quantityColorToWxColour(const Quantity_Color& color) const
{
    return wxColour(static_cast<unsigned char>(color.Red() * 255),
                    static_cast<unsigned char>(color.Green() * 255),
                    static_cast<unsigned char>(color.Blue() * 255));
}

Quantity_Color DisplayModeConfigDialog::wxColourToQuantityColor(const wxColour& color) const
{
    return Quantity_Color(color.Red() / 255.0, color.Green() / 255.0, color.Blue() / 255.0, Quantity_TOC_RGB);
}

void DisplayModeConfigDialog::updateColorButton(wxButton* button, const wxColour& color)
{
    button->SetBackgroundColour(color);
    button->SetForegroundColour(wxColour(255 - color.Red(), 255 - color.Green(), 255 - color.Blue()));
    button->Refresh();
}

void DisplayModeConfigDialog::onColorButtonClicked(wxCommandEvent& event)
{
    wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
    if (!button) return;
    
    wxColour currentColor = button->GetBackgroundColour();
    wxColour newColor = getColorFromDialog(currentColor);
    
    if (newColor.IsOk()) {
        updateColorButton(button, newColor);
        updatePreview();
    }
}

void DisplayModeConfigDialog::onApply(wxCommandEvent& event)
{
    for (auto& pair : m_modeControls) {
        saveConfigForMode(pair.first);
    }
}

void DisplayModeConfigDialog::onOK(wxCommandEvent& event)
{
    onApply(event);
    EndModal(wxID_OK);
}

void DisplayModeConfigDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void DisplayModeConfigDialog::onReset(wxCommandEvent& event)
{
    for (auto& pair : m_modeControls) {
        loadConfigForMode(pair.first);
    }
    updateControls();
    updatePreview();
}

wxColour DisplayModeConfigDialog::getColorFromDialog(const wxColour& initialColor)
{
    wxColourData colorData;
    colorData.SetColour(initialColor);
    
    wxColourDialog dialog(this, &colorData);
    if (dialog.ShowModal() == wxID_OK) {
        wxColour newColor = dialog.GetColourData().GetColour();
        updatePreview();
        return newColor;
    }
    return wxColour();
}

void DisplayModeConfigDialog::updatePreview()
{
    if (!m_previewCanvas) return;
    
    int currentPage = m_notebook->GetSelection();
    if (currentPage < 0) return;
    
    RenderingConfig::DisplayMode mode = this->getModeFromPageIndex(currentPage);
    
    updateConfigFromControls(mode);
    DisplayModeConfig config = m_modeControls[mode].config;
    
    m_previewCanvas->updateDisplayMode(mode, config);
}

void DisplayModeConfigDialog::applyThemeAndFonts()
{
    FontManager& fontManager = FontManager::getInstance();
    fontManager.initialize();
    fontManager.applyFontToWindowAndChildren(this, "Default");
}

void DisplayModeConfigDialog::createCustomModePage()
{
    ModeControls& controls = m_modeControls[m_customModeKey];
    
    wxPanel* customPage = new wxPanel(m_notebook);
    controls.page = customPage;
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxScrolledWindow* scrolled = new wxScrolledWindow(customPage);
    scrolled->SetScrollRate(10, 10);
    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    scrolled->SetSizer(scrolledSizer);
    
    createNodeRequirementsPanel(scrolled, scrolledSizer, m_customModeKey);
    createRenderingPropertiesPanel(scrolled, scrolledSizer, m_customModeKey);
    createEdgeConfigPanel(scrolled, scrolledSizer, m_customModeKey);
    createPostProcessingPanel(scrolled, scrolledSizer, m_customModeKey);
    
    scrolledSizer->Fit(scrolled);
    mainSizer->Add(scrolled, 1, wxEXPAND | wxALL, 5);
    customPage->SetSizer(mainSizer);
    
    m_notebook->AddPage(customPage, "Custom");
}

void DisplayModeConfigDialog::updateModeVisibility(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    bool showAll = (mode == RenderingConfig::DisplayMode::Solid);
    
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        if (controls.requireSurface) controls.requireSurface->Show(true);
        if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(true);
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(false);
        if (controls.requirePoints) controls.requirePoints->Show(false);
        
        if (controls.lightModel) controls.lightModel->Show(true);
        if (controls.textureEnabled) controls.textureEnabled->Show(false);
        if (controls.blendMode) controls.blendMode->Show(false);
        
        if (controls.materialOverrideEnabled) controls.materialOverrideEnabled->Show(true);
        if (controls.materialAmbientColor) controls.materialAmbientColor->Show(false);
        if (controls.materialDiffuseColor) controls.materialDiffuseColor->Show(true);
        if (controls.materialSpecularColor) controls.materialSpecularColor->Show(false);
        if (controls.materialEmissiveColor) controls.materialEmissiveColor->Show(false);
        if (controls.materialShininess) controls.materialShininess->Show(false);
        if (controls.materialShininessLabel) controls.materialShininessLabel->Show(false);
        if (controls.materialTransparency) controls.materialTransparency->Show(false);
        if (controls.materialTransparencyLabel) controls.materialTransparencyLabel->Show(false);
        
        if (controls.originalEdgeEnabled) controls.originalEdgeEnabled->Show(true);
        if (controls.originalEdgeColor) controls.originalEdgeColor->Show(true);
        if (controls.originalEdgeWidth) controls.originalEdgeWidth->Show(true);
        if (controls.originalEdgeWidthLabel) controls.originalEdgeWidthLabel->Show(true);
        
        if (controls.meshEdgeEnabled) controls.meshEdgeEnabled->Show(false);
        if (controls.meshEdgeColor) controls.meshEdgeColor->Show(false);
        if (controls.meshEdgeWidth) controls.meshEdgeWidth->Show(false);
        if (controls.meshEdgeWidthLabel) controls.meshEdgeWidthLabel->Show(false);
        if (controls.meshEdgeUseEffectiveColor) controls.meshEdgeUseEffectiveColor->Show(false);
        
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(false);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(false);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(false);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(false);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(false);
        break;
        
    case RenderingConfig::DisplayMode::Points:
        if (controls.requireSurface) controls.requireSurface->Show(true);
        if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(false);
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(false);
        if (controls.requirePoints) controls.requirePoints->Show(true);
        
        if (controls.lightModel) controls.lightModel->Show(true);
        if (controls.textureEnabled) controls.textureEnabled->Show(false);
        if (controls.blendMode) controls.blendMode->Show(false);
        
        if (controls.materialOverrideEnabled) controls.materialOverrideEnabled->Show(false);
        if (controls.materialAmbientColor) controls.materialAmbientColor->Show(false);
        if (controls.materialDiffuseColor) controls.materialDiffuseColor->Show(false);
        if (controls.materialSpecularColor) controls.materialSpecularColor->Show(false);
        if (controls.materialEmissiveColor) controls.materialEmissiveColor->Show(false);
        if (controls.materialShininess) controls.materialShininess->Show(false);
        if (controls.materialShininessLabel) controls.materialShininessLabel->Show(false);
        if (controls.materialTransparency) controls.materialTransparency->Show(false);
        if (controls.materialTransparencyLabel) controls.materialTransparencyLabel->Show(false);
        
        if (controls.originalEdgeEnabled) controls.originalEdgeEnabled->Show(false);
        if (controls.originalEdgeColor) controls.originalEdgeColor->Show(false);
        if (controls.originalEdgeWidth) controls.originalEdgeWidth->Show(false);
        if (controls.originalEdgeWidthLabel) controls.originalEdgeWidthLabel->Show(false);
        
        if (controls.meshEdgeEnabled) controls.meshEdgeEnabled->Show(false);
        if (controls.meshEdgeColor) controls.meshEdgeColor->Show(false);
        if (controls.meshEdgeWidth) controls.meshEdgeWidth->Show(false);
        if (controls.meshEdgeWidthLabel) controls.meshEdgeWidthLabel->Show(false);
        if (controls.meshEdgeUseEffectiveColor) controls.meshEdgeUseEffectiveColor->Show(false);
        
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(false);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(false);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(false);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(false);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(false);
        break;
        
    case RenderingConfig::DisplayMode::Wireframe:
        if (controls.requireSurface) controls.requireSurface->Show(true);
        if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(true);
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(false);
        if (controls.requirePoints) controls.requirePoints->Show(false);
        
        if (controls.lightModel) controls.lightModel->Show(true);
        if (controls.textureEnabled) controls.textureEnabled->Show(false);
        if (controls.blendMode) controls.blendMode->Show(false);
        
        if (controls.materialOverrideEnabled) controls.materialOverrideEnabled->Show(true);
        if (controls.materialAmbientColor) controls.materialAmbientColor->Show(false);
        if (controls.materialDiffuseColor) controls.materialDiffuseColor->Show(false);
        if (controls.materialSpecularColor) controls.materialSpecularColor->Show(false);
        if (controls.materialEmissiveColor) controls.materialEmissiveColor->Show(false);
        if (controls.materialShininess) controls.materialShininess->Show(false);
        if (controls.materialShininessLabel) controls.materialShininessLabel->Show(false);
        if (controls.materialTransparency) controls.materialTransparency->Show(false);
        if (controls.materialTransparencyLabel) controls.materialTransparencyLabel->Show(false);
        
        if (controls.originalEdgeEnabled) controls.originalEdgeEnabled->Show(true);
        if (controls.originalEdgeColor) controls.originalEdgeColor->Show(true);
        if (controls.originalEdgeWidth) controls.originalEdgeWidth->Show(true);
        if (controls.originalEdgeWidthLabel) controls.originalEdgeWidthLabel->Show(true);
        
        if (controls.meshEdgeEnabled) controls.meshEdgeEnabled->Show(false);
        if (controls.meshEdgeColor) controls.meshEdgeColor->Show(false);
        if (controls.meshEdgeWidth) controls.meshEdgeWidth->Show(false);
        if (controls.meshEdgeWidthLabel) controls.meshEdgeWidthLabel->Show(false);
        if (controls.meshEdgeUseEffectiveColor) controls.meshEdgeUseEffectiveColor->Show(false);
        
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(false);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(false);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(false);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(false);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(false);
        break;
        
    case RenderingConfig::DisplayMode::Solid:
        if (controls.nodeRequirementsBox) controls.nodeRequirementsBox->Show(true);
        if (controls.requireSurface) controls.requireSurface->Show(true);
        if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(true);
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(false);
        if (controls.requirePoints) controls.requirePoints->Show(false);
        
        if (controls.renderingPropertiesBox) controls.renderingPropertiesBox->Show(true);
        if (controls.lightModel) controls.lightModel->Show(true);
        if (controls.textureEnabled) controls.textureEnabled->Show(true);
        if (controls.blendMode) controls.blendMode->Show(true);
        
        if (controls.materialOverrideEnabled) controls.materialOverrideEnabled->Show(true);
        if (controls.materialAmbientColor) controls.materialAmbientColor->Show(true);
        if (controls.materialDiffuseColor) controls.materialDiffuseColor->Show(true);
        if (controls.materialSpecularColor) controls.materialSpecularColor->Show(true);
        if (controls.materialEmissiveColor) controls.materialEmissiveColor->Show(true);
        if (controls.materialShininess) controls.materialShininess->Show(true);
        if (controls.materialShininessLabel) controls.materialShininessLabel->Show(true);
        if (controls.materialTransparency) controls.materialTransparency->Show(true);
        if (controls.materialTransparencyLabel) controls.materialTransparencyLabel->Show(true);
        
        if (controls.edgeConfigBox) controls.edgeConfigBox->Show(true);
        if (controls.originalEdgeEnabled) {
            controls.originalEdgeEnabled->Show(true);
            controls.originalEdgeEnabled->Enable(false);  // Disabled in Solid mode, controlled by requireOriginalEdges
        }
        if (controls.originalEdgeColor) controls.originalEdgeColor->Show(true);
        if (controls.originalEdgeWidth) controls.originalEdgeWidth->Show(true);
        if (controls.originalEdgeWidthLabel) controls.originalEdgeWidthLabel->Show(true);
        
        if (controls.meshEdgeSeparator) controls.meshEdgeSeparator->Show(false);
        if (controls.meshEdgeLabel) controls.meshEdgeLabel->Show(false);
        if (controls.meshEdgeEnabled) controls.meshEdgeEnabled->Show(false);
        if (controls.meshEdgeColor) controls.meshEdgeColor->Show(false);
        if (controls.meshEdgeWidth) controls.meshEdgeWidth->Show(false);
        if (controls.meshEdgeWidthLabel) controls.meshEdgeWidthLabel->Show(false);
        if (controls.meshEdgeUseEffectiveColor) controls.meshEdgeUseEffectiveColor->Show(false);
        
        if (controls.postProcessingBox) controls.postProcessingBox->Show(true);
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(true);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(true);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(true);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(true);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(true);
        break;
        
    case RenderingConfig::DisplayMode::FlatLines:
        if (controls.requireSurface) controls.requireSurface->Show(true);
        if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(true);
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(false);
        if (controls.requirePoints) controls.requirePoints->Show(false);

        if (controls.lightModel) controls.lightModel->Show(true);
        if (controls.textureEnabled) controls.textureEnabled->Show(false);
        if (controls.blendMode) controls.blendMode->Show(false);

        if (controls.materialOverrideEnabled) controls.materialOverrideEnabled->Show(true);
        if (controls.materialAmbientColor) controls.materialAmbientColor->Show(false);
        if (controls.materialDiffuseColor) controls.materialDiffuseColor->Show(false);
        if (controls.materialSpecularColor) controls.materialSpecularColor->Show(false);
        if (controls.materialEmissiveColor) controls.materialEmissiveColor->Show(false);
        if (controls.materialShininess) controls.materialShininess->Show(true);
        if (controls.materialTransparency) controls.materialTransparency->Show(false);

        if (controls.originalEdgeEnabled) {
            controls.originalEdgeEnabled->Show(true);
            controls.originalEdgeEnabled->Enable(false);  // Disabled in FlatLines mode, controlled by requireOriginalEdges
        }
        if (controls.originalEdgeColor) controls.originalEdgeColor->Show(true);
        if (controls.originalEdgeWidth) controls.originalEdgeWidth->Show(true);
        if (controls.originalEdgeWidthLabel) controls.originalEdgeWidthLabel->Show(true);
        
        if (controls.meshEdgeEnabled) controls.meshEdgeEnabled->Show(false);
        if (controls.meshEdgeColor) controls.meshEdgeColor->Show(false);
        if (controls.meshEdgeWidth) controls.meshEdgeWidth->Show(false);
        if (controls.meshEdgeWidthLabel) controls.meshEdgeWidthLabel->Show(false);
        if (controls.meshEdgeUseEffectiveColor) controls.meshEdgeUseEffectiveColor->Show(false);
        
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(false);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(false);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(false);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(false);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(false);
        break;
        
    case RenderingConfig::DisplayMode::Transparent:
        if (controls.requireSurface) controls.requireSurface->Show(true);
        if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(false);
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(false);
        if (controls.requirePoints) controls.requirePoints->Show(false);
        
        if (controls.lightModel) controls.lightModel->Show(true);
        if (controls.textureEnabled) controls.textureEnabled->Show(false);
        if (controls.blendMode) controls.blendMode->Show(true);
        
        if (controls.materialOverrideEnabled) controls.materialOverrideEnabled->Show(true);
        if (controls.materialAmbientColor) controls.materialAmbientColor->Show(false);
        if (controls.materialDiffuseColor) controls.materialDiffuseColor->Show(false);
        if (controls.materialSpecularColor) controls.materialSpecularColor->Show(false);
        if (controls.materialEmissiveColor) controls.materialEmissiveColor->Show(false);
        if (controls.materialShininess) controls.materialShininess->Show(false);
        if (controls.materialShininessLabel) controls.materialShininessLabel->Show(false);
        if (controls.materialTransparency) controls.materialTransparency->Show(true);
        if (controls.materialTransparencyLabel) controls.materialTransparencyLabel->Show(true);
        
        if (controls.originalEdgeEnabled) controls.originalEdgeEnabled->Show(false);
        if (controls.originalEdgeColor) controls.originalEdgeColor->Show(false);
        if (controls.originalEdgeWidth) controls.originalEdgeWidth->Show(false);
        if (controls.originalEdgeWidthLabel) controls.originalEdgeWidthLabel->Show(false);
        
        if (controls.meshEdgeEnabled) controls.meshEdgeEnabled->Show(false);
        if (controls.meshEdgeColor) controls.meshEdgeColor->Show(false);
        if (controls.meshEdgeWidth) controls.meshEdgeWidth->Show(false);
        if (controls.meshEdgeWidthLabel) controls.meshEdgeWidthLabel->Show(false);
        if (controls.meshEdgeUseEffectiveColor) controls.meshEdgeUseEffectiveColor->Show(false);
        
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(false);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(false);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(false);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(false);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(false);
        break;
        
    case RenderingConfig::DisplayMode::HiddenLine:
        if (controls.requireSurface) controls.requireSurface->Show(true);
        if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(false);
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(true);
        if (controls.requirePoints) controls.requirePoints->Show(false);
        
        if (controls.lightModel) controls.lightModel->Show(true);
        if (controls.textureEnabled) controls.textureEnabled->Show(false);
        if (controls.blendMode) controls.blendMode->Show(true);
        
        if (controls.materialOverrideEnabled) controls.materialOverrideEnabled->Show(true);
        if (controls.materialAmbientColor) controls.materialAmbientColor->Show(true);
        if (controls.materialDiffuseColor) controls.materialDiffuseColor->Show(true);
        if (controls.materialSpecularColor) controls.materialSpecularColor->Show(false);
        if (controls.materialEmissiveColor) controls.materialEmissiveColor->Show(false);
        if (controls.materialShininess) controls.materialShininess->Show(false);
        if (controls.materialShininessLabel) controls.materialShininessLabel->Show(false);
        if (controls.materialTransparency) controls.materialTransparency->Show(false);
        if (controls.materialTransparencyLabel) controls.materialTransparencyLabel->Show(false);
        
        if (controls.originalEdgeEnabled) controls.originalEdgeEnabled->Show(false);
        if (controls.originalEdgeColor) controls.originalEdgeColor->Show(false);
        if (controls.originalEdgeWidth) controls.originalEdgeWidth->Show(false);
        if (controls.originalEdgeWidthLabel) controls.originalEdgeWidthLabel->Show(false);
        
        if (controls.meshEdgeEnabled) controls.meshEdgeEnabled->Show(true);
        if (controls.meshEdgeColor) controls.meshEdgeColor->Show(true);
        if (controls.meshEdgeWidth) controls.meshEdgeWidth->Show(true);
        if (controls.meshEdgeWidthLabel) controls.meshEdgeWidthLabel->Show(true);
        if (controls.meshEdgeUseEffectiveColor) controls.meshEdgeUseEffectiveColor->Show(true);
        
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(true);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(true);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(true);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(true);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(true);
        break;
    }
    
    if (mode == m_customModeKey) {
        if (controls.requireSurface) controls.requireSurface->Show(true);
        if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(true);
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(true);
        if (controls.requirePoints) controls.requirePoints->Show(true);
        
        if (controls.lightModel) controls.lightModel->Show(true);
        if (controls.textureEnabled) controls.textureEnabled->Show(true);
        if (controls.blendMode) controls.blendMode->Show(true);
        
        if (controls.materialOverrideEnabled) controls.materialOverrideEnabled->Show(true);
        if (controls.materialAmbientColor) controls.materialAmbientColor->Show(true);
        if (controls.materialDiffuseColor) controls.materialDiffuseColor->Show(true);
        if (controls.materialSpecularColor) controls.materialSpecularColor->Show(true);
        if (controls.materialEmissiveColor) controls.materialEmissiveColor->Show(true);
        if (controls.materialShininess) controls.materialShininess->Show(true);
        if (controls.materialShininessLabel) controls.materialShininessLabel->Show(true);
        if (controls.materialTransparency) controls.materialTransparency->Show(true);
        if (controls.materialTransparencyLabel) controls.materialTransparencyLabel->Show(true);
        
        if (controls.originalEdgeEnabled) controls.originalEdgeEnabled->Show(true);
        if (controls.originalEdgeColor) controls.originalEdgeColor->Show(true);
        if (controls.originalEdgeWidth) controls.originalEdgeWidth->Show(true);
        if (controls.originalEdgeWidthLabel) controls.originalEdgeWidthLabel->Show(true);
        
        if (controls.meshEdgeEnabled) controls.meshEdgeEnabled->Show(true);
        if (controls.meshEdgeColor) controls.meshEdgeColor->Show(true);
        if (controls.meshEdgeWidth) controls.meshEdgeWidth->Show(true);
        if (controls.meshEdgeWidthLabel) controls.meshEdgeWidthLabel->Show(true);
        if (controls.meshEdgeUseEffectiveColor) controls.meshEdgeUseEffectiveColor->Show(true);
        
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(true);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(true);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(true);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(true);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(true);
    }
    
    if (controls.page) {
        controls.page->Layout();
    }
}

