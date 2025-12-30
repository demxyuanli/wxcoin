#include "opencascade/geometry/helper/DisplayModeConfigDialog.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "config/FontManager.h"
#include "config/ThemeManager.h"
#include "widgets/FlatProgressBar.h"
#include "widgets/FlatStaticBoxSizer.h"
#include "widgets/FlatNotebook.h"
#include "logger/Logger.h"
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/scrolwin.h>
#include <sstream>
#include <iomanip>

namespace {
    // Layout constants
    const int CONTROL_WIDTH = 180;
    const int BUTTON_WIDTH = 100;
    const int COMBOBOX_WIDTH = 140;
    const int SLIDER_WIDTH = 140;
    const int CONTROL_HEIGHT = 22;
    const int LABEL_WIDTH = 50;
    const int GRID_COL_GAP = 4;
    const int GRID_ROW_GAP = 3;
    const int BOX_PADDING_TOP = 4;
    const int BOX_PADDING_SIDE = 4;
    const int BOX_PADDING_BOTTOM = 4;
    const int BOX_PADDING_MIDDLE = 3;
    const int BOX_PADDING_OUTER = 3;
}

DisplayModeConfigDialog::DisplayModeConfigDialog(wxWindow* parent, RenderingConfig::DisplayMode initialMode,
                                                   const std::map<RenderingConfig::DisplayMode, DisplayModeConfig>& preloadedConfigs,
                                                   ProgressCallback progressCallback)
    : FramelessModalPopup(parent, "Display Mode Configuration", wxSize(1200, 800))
    , m_notebook(nullptr)
    , m_customModeKey(RenderingConfig::DisplayMode::Custom)
    , m_applyButton(nullptr)
    , m_okButton(nullptr)
    , m_cancelButton(nullptr)
    , m_resetButton(nullptr)
    , m_splitter(nullptr)
    , m_previewCanvas(nullptr)
    , m_progressCallback(progressCallback)
    , m_previewUpdateTimer(this)
    , m_pendingPreviewMode(RenderingConfig::DisplayMode::Solid)
    , m_previewUpdatePending(false)
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
    
    // Report progress: Creating controls
    if (m_progressCallback) {
        m_progressCallback(1, 6, "Creating controls...");
        wxSafeYield();
    }
    
    createControls();
    
    // Report progress: Layout controls
    if (m_progressCallback) {
        m_progressCallback(2, 6, "Layout controls...");
        wxSafeYield();
    }
    
    layoutControls();
    
    // Report progress: Binding events
    if (m_progressCallback) {
        m_progressCallback(3, 6, "Binding events...");
        wxSafeYield();
    }
    
    bindEvents();

    // Bind timer event for debounced preview updates
    Bind(wxEVT_TIMER, &DisplayModeConfigDialog::onPreviewUpdateTimer, this, m_previewUpdateTimer.GetId());

    // Report progress: Loading configurations
    if (m_progressCallback) {
        m_progressCallback(4, 6, "Loading configurations...");
        wxSafeYield();
    }
    
    // Use preloaded configurations if provided, otherwise load them
    if (!preloadedConfigs.empty()) {
        for (const auto& pair : preloadedConfigs) {
            m_modeControls[pair.first].config = pair.second;
        }
    } else {
        // Fallback: load configurations directly (for backward compatibility)
        static const RenderingConfig::DisplayMode modes[] = {
            RenderingConfig::DisplayMode::NoShading,
            RenderingConfig::DisplayMode::Points,
            RenderingConfig::DisplayMode::Wireframe,
            RenderingConfig::DisplayMode::Solid,
            RenderingConfig::DisplayMode::FlatLines,
            RenderingConfig::DisplayMode::Transparent,
            RenderingConfig::DisplayMode::HiddenLine,
            RenderingConfig::DisplayMode::Custom
        };
        for (auto mode : modes) {
            loadConfigForMode(mode);
        }
    }
    
    // Report progress: Updating controls
    if (m_progressCallback) {
        m_progressCallback(5, 6, "Updating controls...");
        wxSafeYield();
    }
    
    updateControls();
    
    applyThemeAndFonts();
    
    for (auto& pair : m_modeControls) {
        updateModeVisibility(pair.first);
    }
    
    // Set the initial selected tab based on the provided display mode
    if (m_notebook) {
        int pageIndex = getPageIndexFromMode(initialMode);
        if (pageIndex >= 0 && pageIndex < m_notebook->GetPageCount()) {
            // SetSelection should trigger the page changed event, but ensure initialization is correct
            m_notebook->SetSelection(pageIndex);

            // Manually trigger the page change logic for initial setup (in case event doesn't fire)
            RenderingConfig::DisplayMode mode = getModeFromPageIndex(pageIndex);
            updateModeVisibility(mode);
            // Note: updatePreviewForMode will be called by the page changed event or manually below
        }
    }
    
    // Report progress: Initialization complete
    if (m_progressCallback) {
        m_progressCallback(6, 6, "Initialization complete");
        wxSafeYield();
    }
    
    // Show the main window after loading is complete (centered by FramelessModalPopup)
    Show();
    wxSafeYield();  // Allow window to render

    // Perform initial view all after dialog is fully shown
    if (m_previewCanvas) {
        CallAfter([this]() {
            if (m_previewCanvas) {
                wxSize canvasSize = m_previewCanvas->GetSize();
                if (canvasSize.GetWidth() > 0 && canvasSize.GetHeight() > 0) {
                    // Only call performViewAll once after dialog is properly sized
                    static bool initialViewAllDone = false;
                    if (!initialViewAllDone) {
                        initialViewAllDone = true;
                        m_previewCanvas->performViewAll();
                    }
                }
            }
        });
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
    m_applyButton = new FlatButton(m_contentPanel, wxID_APPLY, "Apply", 
                                    wxDefaultPosition, wxDefaultSize, 
                                    FlatButton::ButtonStyle::PRIMARY);
    m_okButton = new FlatButton(m_contentPanel, wxID_OK, "OK", 
                                 wxDefaultPosition, wxDefaultSize, 
                                 FlatButton::ButtonStyle::PRIMARY);
    m_cancelButton = new FlatButton(m_contentPanel, wxID_CANCEL, "Cancel", 
                                     wxDefaultPosition, wxDefaultSize, 
                                     FlatButton::ButtonStyle::OUTLINE);
    m_resetButton = new FlatButton(m_contentPanel, wxID_ANY, "Reset to Defaults", 
                                    wxDefaultPosition, wxDefaultSize, 
                                    FlatButton::ButtonStyle::OUTLINE);
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
    
    mainSizer->Add(scrolled, 1, wxEXPAND | wxALL, 3);
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
    
    FlatStaticBoxSizer* boxSizer = new FlatStaticBoxSizer(wxVERTICAL, parent, "Node Requirements");
    wxStaticBox* staticBox = boxSizer->GetStaticBox();
    controls.nodeRequirementsBox = staticBox;
    
    controls.requireSurface = new FlatCheckBox(staticBox, wxID_ANY, "Require Surface");
    controls.requireOriginalEdges = new FlatCheckBox(staticBox, wxID_ANY, "Require Original Edges");
    controls.requireMeshEdges = new FlatCheckBox(staticBox, wxID_ANY, "Require Mesh Edges");
    controls.requirePoints = new FlatCheckBox(staticBox, wxID_ANY, "Require Points");

    std::vector<wxString> drawStyleItems = {
        "FILLED - Surface only",
        "LINES - Edges only",
        "POINTS - Points only",
        "FILLED+LINES - Surface+Edges",
        "FILLED+POINTS - Surface+Points",
        "LINES+POINTS - Edges+Points",
        "FILLED+LINES+POINTS - Surface+Edges+Points"
    };
    controls.drawStyle = createComboBox(staticBox, "Draw Style:", drawStyleItems, 0);

    addCheckBox(boxSizer, controls.requireSurface, wxLEFT | wxRIGHT | wxTOP, BOX_PADDING_TOP);
    addCheckBox(boxSizer, controls.requireOriginalEdges, wxLEFT | wxRIGHT, BOX_PADDING_MIDDLE);
    addCheckBox(boxSizer, controls.requireMeshEdges, wxLEFT | wxRIGHT, BOX_PADDING_MIDDLE);
    addCheckBox(boxSizer, controls.requirePoints, wxLEFT | wxRIGHT, BOX_PADDING_MIDDLE);

    wxFlexGridSizer* drawStyleGrid = new wxFlexGridSizer(2, GRID_COL_GAP, GRID_ROW_GAP);
    addGridRow(drawStyleGrid, staticBox, "Draw Style:", controls.drawStyle);
    boxSizer->Add(drawStyleGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, BOX_PADDING_SIDE);
    
    sizer->Add(boxSizer, 0, wxEXPAND | wxALL, BOX_PADDING_OUTER);
}

void DisplayModeConfigDialog::createRenderingPropertiesPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    FlatStaticBoxSizer* boxSizer = new FlatStaticBoxSizer(wxVERTICAL, parent, "Rendering Properties");
    wxStaticBox* staticBox = boxSizer->GetStaticBox();
    controls.renderingPropertiesBox = staticBox;
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, GRID_COL_GAP, GRID_ROW_GAP);
    
    std::vector<wxString> lightModelItems = { "BASE_COLOR", "PHONG" };
    controls.lightModel = createComboBox(staticBox, "Light Model:", lightModelItems, 1);
    addGridRow(gridSizer, staticBox, "Light Model:", controls.lightModel);
    
    controls.textureEnabled = new FlatCheckBox(staticBox, wxID_ANY, "Texture Enabled");
    addGridRow(gridSizer, staticBox, "", controls.textureEnabled);
    
    std::vector<wxString> blendModeItems = { "None", "Alpha", "Additive", "Multiply", "Screen", "Overlay" };
    controls.blendMode = createComboBox(staticBox, "Blend Mode:", blendModeItems, 0);
    addGridRow(gridSizer, staticBox, "Blend Mode:", controls.blendMode);
    
    boxSizer->Add(gridSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, BOX_PADDING_SIDE);
    
    wxStaticLine* line = new wxStaticLine(staticBox);
    boxSizer->Add(line, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, BOX_PADDING_SIDE);
    
    wxStaticText* materialLabel = new wxStaticText(staticBox, wxID_ANY, "Material Override:");
    boxSizer->Add(materialLabel, 0, wxLEFT | wxRIGHT, BOX_PADDING_SIDE);
    
    controls.materialOverrideEnabled = new FlatCheckBox(staticBox, wxID_ANY, "Enable Material Override");
    addCheckBox(boxSizer, controls.materialOverrideEnabled, wxLEFT | wxRIGHT, BOX_PADDING_MIDDLE);
    
    wxFlexGridSizer* materialGrid = new wxFlexGridSizer(2, GRID_COL_GAP, GRID_ROW_GAP);
    
    controls.materialAmbientColor = createColorButton(staticBox, "Ambient Color:");
    addGridRow(materialGrid, staticBox, "Ambient Color:", controls.materialAmbientColor);
    
    controls.materialDiffuseColor = createColorButton(staticBox, "Diffuse Color:");
    addGridRow(materialGrid, staticBox, "Diffuse Color:", controls.materialDiffuseColor);
    
    controls.materialSpecularColor = createColorButton(staticBox, "Specular Color:");
    addGridRow(materialGrid, staticBox, "Specular Color:", controls.materialSpecularColor);
    
    controls.materialEmissiveColor = createColorButton(staticBox, "Emissive Color:");
    addGridRow(materialGrid, staticBox, "Emissive Color:", controls.materialEmissiveColor);
    
    wxBoxSizer* shininessSizer = createSliderWithLabel(staticBox, controls.materialShininess, 
                                                       controls.materialShininessLabel, 0, 0, 1280, "%.1f");
    addGridRow(materialGrid, staticBox, "Shininess:", shininessSizer);
    
    wxBoxSizer* transparencySizer = createSliderWithLabel(staticBox, controls.materialTransparency, 
                                                          controls.materialTransparencyLabel, 0, 0, 100, "%.2f");
    addGridRow(materialGrid, staticBox, "Transparency:", transparencySizer);
    
    boxSizer->Add(materialGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, BOX_PADDING_SIDE);
    
    sizer->Add(boxSizer, 0, wxEXPAND | wxALL, BOX_PADDING_OUTER);
}

void DisplayModeConfigDialog::createEdgeConfigPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    FlatStaticBoxSizer* boxSizer = new FlatStaticBoxSizer(wxVERTICAL, parent, "Edge Configuration");
    wxStaticBox* staticBox = boxSizer->GetStaticBox();
    controls.edgeConfigBox = staticBox;
    
    wxStaticText* originalLabel = new wxStaticText(staticBox, wxID_ANY, "Original Edge:");
    boxSizer->Add(originalLabel, 0, wxLEFT | wxRIGHT | wxTOP, BOX_PADDING_TOP);
    
    controls.originalEdgeEnabled = new FlatCheckBox(staticBox, wxID_ANY, "Enable Original Edge");
    addCheckBox(boxSizer, controls.originalEdgeEnabled, wxLEFT | wxRIGHT, BOX_PADDING_MIDDLE);
    
    wxFlexGridSizer* originalGrid = new wxFlexGridSizer(2, GRID_COL_GAP, GRID_ROW_GAP);
    
    controls.originalEdgeColor = createColorButton(staticBox, "Color:");
    addGridRow(originalGrid, staticBox, "Color:", controls.originalEdgeColor);
    
    wxBoxSizer* originalWidthSizer = createSliderWithLabel(staticBox, controls.originalEdgeWidth, 
                                                           controls.originalEdgeWidthLabel, 10, 1, 100, "%.1f");
    addGridRow(originalGrid, staticBox, "Width:", originalWidthSizer);
    
    boxSizer->Add(originalGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, BOX_PADDING_SIDE);
    
    controls.meshEdgeSeparator = new wxStaticLine(staticBox);
    boxSizer->Add(controls.meshEdgeSeparator, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, BOX_PADDING_SIDE);
    
    controls.meshEdgeLabel = new wxStaticText(staticBox, wxID_ANY, "Mesh Edge:");
    boxSizer->Add(controls.meshEdgeLabel, 0, wxLEFT | wxRIGHT, BOX_PADDING_SIDE);
    
    controls.meshEdgeEnabled = new FlatCheckBox(staticBox, wxID_ANY, "Enable Mesh Edge");
    addCheckBox(boxSizer, controls.meshEdgeEnabled, wxLEFT | wxRIGHT, BOX_PADDING_MIDDLE);
    
    wxFlexGridSizer* meshGrid = new wxFlexGridSizer(2, GRID_COL_GAP, GRID_ROW_GAP);
    
    controls.meshEdgeColor = createColorButton(staticBox, "Color:");
    addGridRow(meshGrid, staticBox, "Color:", controls.meshEdgeColor);
    
    wxBoxSizer* meshWidthSizer = createSliderWithLabel(staticBox, controls.meshEdgeWidth, 
                                                      controls.meshEdgeWidthLabel, 10, 1, 100, "%.1f");
    addGridRow(meshGrid, staticBox, "Width:", meshWidthSizer);
    
    controls.meshEdgeUseEffectiveColor = new FlatCheckBox(staticBox, wxID_ANY, "");
    addGridRow(meshGrid, staticBox, "Effect Color:", controls.meshEdgeUseEffectiveColor);

    boxSizer->Add(meshGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, BOX_PADDING_SIDE);

    controls.silhouetteEdgeSeparator = new wxStaticLine(staticBox);
    boxSizer->Add(controls.silhouetteEdgeSeparator, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, BOX_PADDING_SIDE);

    controls.silhouetteEdgeLabel = new wxStaticText(staticBox, wxID_ANY, "Silhouette Edge:");
    boxSizer->Add(controls.silhouetteEdgeLabel, 0, wxLEFT | wxRIGHT, BOX_PADDING_SIDE);

    controls.silhouetteEdgeEnabled = new FlatCheckBox(staticBox, wxID_ANY, "Enable Silhouette Edge");
    addCheckBox(boxSizer, controls.silhouetteEdgeEnabled, wxLEFT | wxRIGHT, BOX_PADDING_MIDDLE);

    wxFlexGridSizer* silhouetteGrid = new wxFlexGridSizer(2, GRID_COL_GAP, GRID_ROW_GAP);

    controls.silhouetteEdgeColor = createColorButton(staticBox, "Color:");
    addGridRow(silhouetteGrid, staticBox, "Color:", controls.silhouetteEdgeColor);

    wxBoxSizer* silhouetteWidthSizer = createSliderWithLabel(staticBox, controls.silhouetteEdgeWidth,
                                                           controls.silhouetteEdgeWidthLabel, 10, 1, 100, "%.1f");
    addGridRow(silhouetteGrid, staticBox, "Width:", silhouetteWidthSizer);

    boxSizer->Add(silhouetteGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, BOX_PADDING_SIDE);

    sizer->Add(boxSizer, 0, wxEXPAND | wxALL, BOX_PADDING_OUTER);
}

void DisplayModeConfigDialog::createPostProcessingPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    FlatStaticBoxSizer* boxSizer = new FlatStaticBoxSizer(wxVERTICAL, parent, "Post-Processing");
    wxStaticBox* staticBox = boxSizer->GetStaticBox();
    controls.postProcessingBox = staticBox;
    
    controls.polygonOffsetEnabled = new FlatCheckBox(staticBox, wxID_ANY, "Enable Polygon Offset");
    addCheckBox(boxSizer, controls.polygonOffsetEnabled, wxLEFT | wxRIGHT | wxTOP, BOX_PADDING_TOP);
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, GRID_COL_GAP, GRID_ROW_GAP);
    
    wxBoxSizer* factorSizer = createSliderWithLabel(staticBox, controls.polygonOffsetFactor, 
                                                    controls.polygonOffsetFactorLabel, 0, -100, 100, "%.1f");
    addGridRow(gridSizer, staticBox, "Factor:", factorSizer);
    
    wxBoxSizer* unitsSizer = createSliderWithLabel(staticBox, controls.polygonOffsetUnits, 
                                                   controls.polygonOffsetUnitsLabel, 0, -100, 100, "%.1f");
    addGridRow(gridSizer, staticBox, "Units:", unitsSizer);
    
    boxSizer->Add(gridSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, BOX_PADDING_SIDE);
    
    sizer->Add(boxSizer, 0, wxEXPAND | wxALL, BOX_PADDING_OUTER);
}

void DisplayModeConfigDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxBoxSizer* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxPanel* leftPanel = new wxPanel(m_contentPanel);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    
    m_notebook = new FlatNotebook(leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_LEFT, wxNotebookNameStr);
    m_notebook->SetMinSize(wxSize(360, -1));
    
    createModePage(RenderingConfig::DisplayMode::NoShading);
    createModePage(RenderingConfig::DisplayMode::Points);
    createModePage(RenderingConfig::DisplayMode::Wireframe);
    createModePage(RenderingConfig::DisplayMode::Solid);
    createModePage(RenderingConfig::DisplayMode::FlatLines);
    createModePage(RenderingConfig::DisplayMode::Transparent);
    createModePage(RenderingConfig::DisplayMode::HiddenLine);
    createCustomModePage();
    
    leftSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 3);
    leftPanel->SetSizer(leftSizer);
    leftPanel->SetMinSize(wxSize(360, -1));
    
    wxPanel* rightPanel = new wxPanel(m_contentPanel);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* previewLabel = new wxStaticText(rightPanel, wxID_ANY, "Preview");
    wxFont labelFont = previewLabel->GetFont();
    labelFont.SetPointSize(labelFont.GetPointSize() + 1);
    labelFont.SetWeight(wxFONTWEIGHT_BOLD);
    previewLabel->SetFont(labelFont);
    rightSizer->Add(previewLabel, 0, wxALL, 5);
    
    m_previewCanvas = new DisplayModePreviewCanvas(rightPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    rightSizer->Add(m_previewCanvas, 1, wxEXPAND | wxALL, 3);
    
    rightPanel->SetSizer(rightSizer);
    
    contentSizer->Add(leftPanel, 0, wxEXPAND | wxALL, 3);
    contentSizer->Add(rightPanel, 1, wxEXPAND | wxALL, 3);
    
    mainSizer->Add(contentSizer, 1, wxEXPAND | wxALL, 3);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(m_resetButton, 0, wxALL, 3);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_applyButton, 0, wxALL, 3);
    buttonSizer->Add(m_okButton, 0, wxALL, 3);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 3);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 3);
    
    m_contentPanel->SetSizer(mainSizer);

    // Note: Preview will be initialized when notebook page changes in constructor
    // Don't call updatePreview() here to avoid double initialization
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

int DisplayModeConfigDialog::getPageIndexFromMode(RenderingConfig::DisplayMode mode) const
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
    
    for (int i = 0; i < modeOrderSize; ++i) {
        if (modeOrder[i] == mode) {
            return i;
        }
    }
    // Default to Solid mode (index 3) if mode not found
    return 3;
}

void DisplayModeConfigDialog::bindEvents()
{
    m_applyButton->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onApply, this);
    m_okButton->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onOK, this);
    m_cancelButton->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onCancel, this);
    m_resetButton->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onReset, this);
    
    m_notebook->Bind(wxEVT_FLATNOTEBOOK_PAGE_CHANGED, [this](wxCommandEvent& event) {
        int currentPage = event.GetInt();
        int oldPage = event.GetExtraLong();
        LOG_INF_S("DisplayModeConfigDialog: Notebook page changed from " + std::to_string(oldPage) + " to " + std::to_string(currentPage));

        // Refresh notebook to ensure selected tab is properly highlighted
        // wxNotebook with wxNB_LEFT style has built-in highlighting for selected tab
        // Calling Refresh ensures the highlight is properly displayed
        if (currentPage >= 0 && m_notebook) {
            m_notebook->Refresh();
        }
        
        if (currentPage >= 0) {
            RenderingConfig::DisplayMode mode = this->getModeFromPageIndex(currentPage);
            updateModeVisibility(mode);
            ModeControls& controls = m_modeControls[mode];

            // Ensure config matches mode requirements before updating controls
            enforceModeRequirements(mode);
            
            // Update all controls from config for this mode (use config values, don't read from controls)
            // Note: We don't call updateConfigFromControls here because controls may still have old mode values
            // The config has been correctly set by enforceModeRequirements, so we use config to update controls
            controls.requireSurface->SetValue(controls.config.nodes.requireSurface);
            controls.requireOriginalEdges->SetValue(controls.config.nodes.requireOriginalEdges);
            controls.requireMeshEdges->SetValue(controls.config.nodes.requireMeshEdges);
            controls.requirePoints->SetValue(controls.config.nodes.requirePoints);
            
            // Update DrawStyle based on config
            if (controls.drawStyle && controls.drawStyle->IsShown()) {
                bool showSurface = controls.config.nodes.requireSurface;
                bool showEdges = (controls.config.nodes.requireOriginalEdges && controls.config.edges.originalEdge.enabled) ||
                               (controls.config.nodes.requireMeshEdges && controls.config.edges.meshEdge.enabled);
                bool showPoints = controls.config.nodes.requirePoints;

                int drawStyleIndex = 0;
                if (showPoints && !showEdges && !showSurface) {
                    drawStyleIndex = 2; // POINTS
                } else if (showEdges && !showPoints && !showSurface) {
                    drawStyleIndex = 1; // LINES
                } else if (showSurface && !showEdges && !showPoints) {
                    drawStyleIndex = 0; // FILLED
                } else if (showSurface && showEdges && !showPoints) {
                    drawStyleIndex = 3; // FILLED+LINES
                } else if (showSurface && showPoints && !showEdges) {
                    drawStyleIndex = 4; // FILLED+POINTS
                } else if (showEdges && showPoints && !showSurface) {
                    drawStyleIndex = 5; // LINES+POINTS
                } else if (showSurface && showEdges && showPoints) {
                    drawStyleIndex = 6; // FILLED+LINES+POINTS
                }
                controls.drawStyle->SetSelection(drawStyleIndex);
            }
            
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
                // For Solid and FlatLines modes, sync with requireOriginalEdges
                if (mode == RenderingConfig::DisplayMode::Solid || mode == RenderingConfig::DisplayMode::FlatLines) {
                    controls.originalEdgeEnabled->SetValue(controls.config.nodes.requireOriginalEdges);
                } else {
                    controls.originalEdgeEnabled->SetValue(controls.config.edges.originalEdge.enabled);
                }
            }
            if (controls.originalEdgeColor && controls.originalEdgeColor->IsShown()) {
                updateColorButton(controls.originalEdgeColor, quantityColorToWxColour(controls.config.edges.originalEdge.color));
            }
            if (controls.originalEdgeWidth && controls.originalEdgeWidth->IsShown()) {
                controls.originalEdgeWidth->SetValue(static_cast<int>(controls.config.edges.originalEdge.width * 10.0));
                if (controls.originalEdgeWidthLabel) {
                    controls.originalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.originalEdge.width));
                }
            }
            
            if (controls.meshEdgeEnabled && controls.meshEdgeEnabled->IsShown()) {
                controls.meshEdgeEnabled->SetValue(controls.config.edges.meshEdge.enabled);
            }
            if (controls.meshEdgeColor && controls.meshEdgeColor->IsShown()) {
                updateColorButton(controls.meshEdgeColor, quantityColorToWxColour(controls.config.edges.meshEdge.color));
            }
            if (controls.meshEdgeWidth && controls.meshEdgeWidth->IsShown()) {
                controls.meshEdgeWidth->SetValue(static_cast<int>(controls.config.edges.meshEdge.width * 10.0));
                if (controls.meshEdgeWidthLabel) {
                    controls.meshEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.meshEdge.width));
                }
            }
            if (controls.meshEdgeUseEffectiveColor && controls.meshEdgeUseEffectiveColor->IsShown()) {
                controls.meshEdgeUseEffectiveColor->SetValue(controls.config.edges.meshEdge.useEffectiveColor);
            }
            
            if (controls.silhouetteEdgeEnabled && controls.silhouetteEdgeEnabled->IsShown()) {
                controls.silhouetteEdgeEnabled->SetValue(controls.config.edges.silhouetteEdge.enabled);
            }
            if (controls.silhouetteEdgeColor && controls.silhouetteEdgeColor->IsShown()) {
                updateColorButton(controls.silhouetteEdgeColor, quantityColorToWxColour(controls.config.edges.silhouetteEdge.color));
            }
            if (controls.silhouetteEdgeWidth && controls.silhouetteEdgeWidth->IsShown()) {
                controls.silhouetteEdgeWidth->SetValue(static_cast<int>(controls.config.edges.silhouetteEdge.width * 10.0));
                if (controls.silhouetteEdgeWidthLabel) {
                    controls.silhouetteEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.silhouetteEdge.width));
                }
            }
            
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
            
            // Update preview with the current mode's configuration
            // Use updatePreviewForMode to ensure correct mode is passed
            LOG_INF_S("DisplayModeConfigDialog: Updating preview for mode " + std::to_string(static_cast<int>(mode)));
            updatePreviewForMode(mode);
        }
    });
    
    for (auto& pair : m_modeControls) {
        RenderingConfig::DisplayMode mode = pair.first;
        ModeControls& controls = pair.second;
        
        controls.materialAmbientColor->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.materialDiffuseColor->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.materialSpecularColor->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.materialEmissiveColor->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.originalEdgeColor->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.meshEdgeColor->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        controls.silhouetteEdgeColor->Bind(wxEVT_FLAT_BUTTON_CLICKED, &DisplayModeConfigDialog::onColorButtonClicked, this);
        
        controls.requireSurface->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updateConfigFromControls(currentMode);
                updateDrawStyleFromCheckboxes(currentMode);
                syncOriginalEdgeEnabled(currentMode);
                updatePreviewForMode(currentMode);
            }
        });
        controls.requireOriginalEdges->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updateConfigFromControls(currentMode);
                syncOriginalEdgeEnabled(currentMode);
                updateDrawStyleFromCheckboxes(currentMode);
                updatePreviewForMode(currentMode);
            }
        });
        controls.requireMeshEdges->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updateConfigFromControls(currentMode);
                updateDrawStyleFromCheckboxes(currentMode);
                syncOriginalEdgeEnabled(currentMode);
                updatePreviewForMode(currentMode);
            }
        });
        controls.requirePoints->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updateConfigFromControls(currentMode);
                updateDrawStyleFromCheckboxes(currentMode);
                syncOriginalEdgeEnabled(currentMode);
                updatePreviewForMode(currentMode);
            }
        });
        controls.drawStyle->Bind(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updateConfigFromControls(currentMode);
                updateCheckboxesFromDrawStyle(currentMode);
                syncOriginalEdgeEnabled(currentMode);
                updatePreviewForMode(currentMode);
            }
        });
        controls.lightModel->Bind(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updatePreviewForMode(currentMode);
            }
        });
        controls.textureEnabled->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updatePreviewForMode(currentMode);
            }
        });
        controls.blendMode->Bind(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updatePreviewForMode(currentMode);
            }
        });
        controls.materialOverrideEnabled->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updatePreviewForMode(currentMode);
            }
        });
        controls.materialShininess->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.materialShininess->GetValue()) / 10.0;
                ctrl.materialShininessLabel->SetLabel(wxString::Format("%.1f", value));
                // Debounced preview update for slider
                schedulePreviewUpdate(currentMode, 150);
            }
        });
        controls.materialShininess->Bind(wxEVT_FLAT_SLIDER_THUMB_DRAGGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.materialShininess->GetValue()) / 10.0;
                ctrl.materialShininessLabel->SetLabel(wxString::Format("%.1f", value));
                // Only update UI label during drag, don't update preview to avoid performance issues
            }
        });
        controls.materialTransparency->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                if (ctrl.materialTransparency && ctrl.materialTransparencyLabel) {
                    int sliderValue = ctrl.materialTransparency->GetValue();
                    double value = static_cast<double>(sliderValue) / 100.0;
                    ctrl.materialTransparencyLabel->SetLabel(wxString::Format("%.2f", value));
                }
                // Debounced preview update for slider - only update when dragging stops
                schedulePreviewUpdate(currentMode, 150); // 150ms delay
            }
        });
        controls.materialTransparency->Bind(wxEVT_FLAT_SLIDER_THUMB_DRAGGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                if (ctrl.materialTransparency && ctrl.materialTransparencyLabel) {
                    int sliderValue = ctrl.materialTransparency->GetValue();
                    double value = static_cast<double>(sliderValue) / 100.0;
                    ctrl.materialTransparencyLabel->SetLabel(wxString::Format("%.2f", value));
                }
                // Only update UI label during drag, don't update preview to avoid performance issues
            }
        });

        controls.originalEdgeEnabled->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updateDrawStyleFromCheckboxes(currentMode);
                updatePreviewForMode(currentMode);
            }
        });
        controls.originalEdgeWidth->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.originalEdgeWidth->GetValue()) / 10.0;
                ctrl.originalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", value));
                // Debounced preview update for slider
                schedulePreviewUpdate(currentMode, 150);
            }
        });
        controls.originalEdgeWidth->Bind(wxEVT_FLAT_SLIDER_THUMB_DRAGGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.originalEdgeWidth->GetValue()) / 10.0;
                ctrl.originalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", value));
                // Only update UI label during drag
            }
        });
        controls.meshEdgeEnabled->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updateDrawStyleFromCheckboxes(currentMode);
                updatePreviewForMode(currentMode);
            }
        });
        controls.meshEdgeWidth->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.meshEdgeWidth->GetValue()) / 10.0;
                ctrl.meshEdgeWidthLabel->SetLabel(wxString::Format("%.1f", value));
                // Debounced preview update for slider
                schedulePreviewUpdate(currentMode, 150);
            }
        });
        controls.meshEdgeUseEffectiveColor->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updatePreviewForMode(currentMode);
            }
        });
        controls.silhouetteEdgeEnabled->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updatePreviewForMode(currentMode);
            }
        });
        controls.silhouetteEdgeWidth->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.silhouetteEdgeWidth->GetValue()) / 10.0;
                ctrl.silhouetteEdgeWidthLabel->SetLabel(wxString::Format("%.1f", value));
                schedulePreviewUpdate(currentMode, 150);
            }
        });
        controls.silhouetteEdgeWidth->Bind(wxEVT_FLAT_SLIDER_THUMB_DRAGGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.silhouetteEdgeWidth->GetValue()) / 10.0;
                ctrl.silhouetteEdgeWidthLabel->SetLabel(wxString::Format("%.1f", value));
            }
        });
        controls.polygonOffsetEnabled->Bind(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, [this](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                updatePreviewForMode(currentMode);
            }
        });
        controls.polygonOffsetFactor->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.polygonOffsetFactor->GetValue()) / 10.0;
                ctrl.polygonOffsetFactorLabel->SetLabel(wxString::Format("%.1f", value));
                // Debounced preview update for slider
                schedulePreviewUpdate(currentMode, 150);
            }
        });
        controls.polygonOffsetUnits->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, [this, mode](wxCommandEvent&) {
            int currentPage = m_notebook->GetSelection();
            if (currentPage >= 0) {
                RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
                ModeControls& ctrl = m_modeControls[currentMode];
                double value = static_cast<double>(ctrl.polygonOffsetUnits->GetValue()) / 10.0;
                ctrl.polygonOffsetUnitsLabel->SetLabel(wxString::Format("%.1f", value));
                // Debounced preview update for slider
                schedulePreviewUpdate(currentMode, 150);
            }
        });
    }
}

void DisplayModeConfigDialog::updateControls()
{
    for (auto& pair : m_modeControls) {
        RenderingConfig::DisplayMode mode = pair.first;
        ModeControls& controls = pair.second;
        
        // Ensure config matches mode requirements before updating controls
        enforceModeRequirements(mode);
        
        controls.requireSurface->SetValue(controls.config.nodes.requireSurface);
        controls.requireOriginalEdges->SetValue(controls.config.nodes.requireOriginalEdges);
        controls.requireMeshEdges->SetValue(controls.config.nodes.requireMeshEdges);
        controls.requirePoints->SetValue(controls.config.nodes.requirePoints);

        // Set DrawStyle based on rendering configuration
        updateDrawStyleFromCheckboxes(mode);
        
        // Sync originalEdgeEnabled for Solid and FlatLines modes
        syncOriginalEdgeEnabled(mode);
        
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
        syncOriginalEdgeEnabled(mode);
        if (mode != RenderingConfig::DisplayMode::FlatLines && mode != RenderingConfig::DisplayMode::Solid) {
            if (controls.originalEdgeEnabled) {
                controls.originalEdgeEnabled->SetValue(controls.config.edges.originalEdge.enabled);
            }
        }
        if (controls.originalEdgeColor) {
            updateColorButton(controls.originalEdgeColor, quantityColorToWxColour(controls.config.edges.originalEdge.color));
        }
        if (controls.originalEdgeWidth) {
            controls.originalEdgeWidth->SetValue(static_cast<int>(controls.config.edges.originalEdge.width * 10.0));
            if (controls.originalEdgeWidthLabel) {
                controls.originalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.originalEdge.width));
            }
        }
        
        if (controls.meshEdgeEnabled) {
            controls.meshEdgeEnabled->SetValue(controls.config.edges.meshEdge.enabled);
        }
        if (controls.meshEdgeColor) {
            updateColorButton(controls.meshEdgeColor, quantityColorToWxColour(controls.config.edges.meshEdge.color));
        }
        if (controls.meshEdgeWidth) {
            controls.meshEdgeWidth->SetValue(static_cast<int>(controls.config.edges.meshEdge.width * 10.0));
            if (controls.meshEdgeWidthLabel) {
                controls.meshEdgeWidthLabel->SetLabel(wxString::Format("%.1f", controls.config.edges.meshEdge.width));
            }
        }
        if (controls.meshEdgeUseEffectiveColor) {
            controls.meshEdgeUseEffectiveColor->SetValue(controls.config.edges.meshEdge.useEffectiveColor);
        }
        
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

void DisplayModeConfigDialog::enforceModeRequirements(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        // NoShading: surface + original edges, BASE_COLOR lighting
        controls.config.nodes.requireSurface = true;
        controls.config.nodes.requireOriginalEdges = true;
        controls.config.nodes.requireMeshEdges = false;
        controls.config.nodes.requirePoints = false;
        controls.config.edges.originalEdge.enabled = true;
        controls.config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR;
        controls.config.rendering.blendMode = RenderingConfig::BlendMode::None;
        break;
        
    case RenderingConfig::DisplayMode::Points:
        // Points: points only (or surface + points)
        controls.config.nodes.requirePoints = true;
        controls.config.nodes.requireOriginalEdges = false;
        controls.config.nodes.requireMeshEdges = false;
        // Keep surface setting from config (can be true for surface+points or false for points only)
        controls.config.edges.originalEdge.enabled = false;
        controls.config.edges.meshEdge.enabled = false;
        controls.config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR;
        controls.config.rendering.blendMode = RenderingConfig::BlendMode::None;
        break;
        
    case RenderingConfig::DisplayMode::Wireframe:
        // Wireframe: no surface, only original edges
        controls.config.nodes.requireSurface = false;
        controls.config.nodes.requireOriginalEdges = true;
        controls.config.nodes.requireMeshEdges = false;
        controls.config.nodes.requirePoints = false;
        controls.config.edges.originalEdge.enabled = true;
        controls.config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR;
        controls.config.rendering.blendMode = RenderingConfig::BlendMode::None;
        break;
        
    case RenderingConfig::DisplayMode::Solid:
        // Solid: surface, optional original edges (synced)
        controls.config.nodes.requireSurface = true;
        controls.config.nodes.requireOriginalEdges = false;  // Default: no edges in Solid mode
        controls.config.nodes.requireMeshEdges = false;
        controls.config.nodes.requirePoints = false;
        // Sync originalEdge.enabled with requireOriginalEdges
        controls.config.edges.originalEdge.enabled = controls.config.nodes.requireOriginalEdges;
        controls.config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::PHONG;
        break;
        
    case RenderingConfig::DisplayMode::FlatLines:
        // FlatLines: surface + original edges (synced), PHONG lighting
        controls.config.nodes.requireSurface = true;
        controls.config.nodes.requireOriginalEdges = true;
        controls.config.nodes.requireMeshEdges = false;
        controls.config.nodes.requirePoints = false;
        // Sync originalEdge.enabled with requireOriginalEdges
        controls.config.edges.originalEdge.enabled = true;
        controls.config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::PHONG;
        controls.config.rendering.blendMode = RenderingConfig::BlendMode::None;
        break;
        
    case RenderingConfig::DisplayMode::Transparent:
        // Transparent: surface only, PHONG lighting, Alpha blend
        controls.config.nodes.requireSurface = true;
        controls.config.nodes.requireOriginalEdges = false;
        controls.config.nodes.requireMeshEdges = false;
        controls.config.nodes.requirePoints = false;
        controls.config.edges.originalEdge.enabled = false;
        controls.config.edges.meshEdge.enabled = false;
        controls.config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::PHONG;
        controls.config.rendering.blendMode = RenderingConfig::BlendMode::Alpha;
        controls.config.postProcessing.polygonOffset.enabled = false;
        break;
        
    case RenderingConfig::DisplayMode::HiddenLine:
        // HiddenLine: surface + silhouette edges (FreeCAD style)
        controls.config.nodes.requireSurface = true;
        controls.config.nodes.requireOriginalEdges = false;
        controls.config.nodes.requireMeshEdges = false;
        controls.config.nodes.requireSilhouetteEdges = true;
        controls.config.nodes.requirePoints = false;
        controls.config.edges.originalEdge.enabled = false;
        controls.config.edges.meshEdge.enabled = false;
        controls.config.edges.silhouetteEdge.enabled = true;
        controls.config.rendering.lightModel = DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR;
        controls.config.rendering.blendMode = RenderingConfig::BlendMode::None;
        break;
        
    case RenderingConfig::DisplayMode::Custom:
        // Custom: no restrictions, user can configure freely
        break;
    }
}

void DisplayModeConfigDialog::saveConfigForMode(RenderingConfig::DisplayMode mode)
{
    updateConfigFromControls(mode);
}

void DisplayModeConfigDialog::updateConfigFromControls(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    // In the new single geometry architecture for testing, DrawStyle controls multi-pass rendering
    // This allows testing surface/edges/points rendering from a single SoIndexedFaceSet using DrawStyle
    // The actual rendering is done in multiple passes in DisplayModePreviewCanvas::onPaint()
    if (controls.drawStyle && controls.drawStyle->IsShown()) {
        int drawStyleIndex = controls.drawStyle->GetSelection();

        // Reset all requirements first
        controls.config.nodes.requireSurface = false;
        controls.config.nodes.requireOriginalEdges = false;
        controls.config.nodes.requireMeshEdges = false;
        controls.config.nodes.requirePoints = false;

        // Set requirements based on DrawStyle selection
        // Choose appropriate edge type based on display mode
        bool useMeshEdges = (mode == RenderingConfig::DisplayMode::HiddenLine && drawStyleIndex != 0); // Only use mesh edges for non-filled styles in HiddenLine
        bool useSilhouetteEdges = (mode == RenderingConfig::DisplayMode::HiddenLine && drawStyleIndex == 0); // Use silhouette for filled style in HiddenLine

        switch (drawStyleIndex) {
        case 0: // FILLED - Surface only
            controls.config.nodes.requireSurface = true;
            break;
        case 1: // LINES - Edges only
            if (useMeshEdges) {
                controls.config.nodes.requireMeshEdges = true;
                controls.config.edges.meshEdge.enabled = true;
            } else {
                controls.config.nodes.requireOriginalEdges = true;
                controls.config.edges.originalEdge.enabled = true;
            }
            break;
        case 2: // POINTS - Points only
            controls.config.nodes.requirePoints = true;
            break;
        case 3: // FILLED+LINES - Surface+Edges
            controls.config.nodes.requireSurface = true;
            if (useSilhouetteEdges) {
                controls.config.nodes.requireSilhouetteEdges = true;
                controls.config.edges.silhouetteEdge.enabled = true;
            } else if (useMeshEdges) {
                controls.config.nodes.requireMeshEdges = true;
                controls.config.edges.meshEdge.enabled = true;
            } else {
                controls.config.nodes.requireOriginalEdges = true;
                controls.config.edges.originalEdge.enabled = true;
            }
            break;
        case 4: // FILLED+POINTS - Surface+Points
            controls.config.nodes.requireSurface = true;
            controls.config.nodes.requirePoints = true;
            break;
        case 5: // LINES+POINTS - Edges+Points
            controls.config.nodes.requirePoints = true;
            if (useMeshEdges) {
                controls.config.nodes.requireMeshEdges = true;
                controls.config.edges.meshEdge.enabled = true;
            } else {
                controls.config.nodes.requireOriginalEdges = true;
                controls.config.edges.originalEdge.enabled = true;
            }
            break;
        case 6: // FILLED+LINES+POINTS - Surface+Edges+Points
            controls.config.nodes.requireSurface = true;
            controls.config.nodes.requirePoints = true;
            if (useMeshEdges) {
                controls.config.nodes.requireMeshEdges = true;
                controls.config.edges.meshEdge.enabled = true;
            } else {
                controls.config.nodes.requireOriginalEdges = true;
                controls.config.edges.originalEdge.enabled = true;
            }
            break;
        }

        // Update individual checkboxes to reflect DrawStyle selection
        updateCheckboxesFromDrawStyle(mode);
        
        // Sync originalEdgeEnabled for Solid and FlatLines modes when DrawStyle changes
        syncOriginalEdgeEnabled(mode);
    } else {
        // Fallback to individual checkbox control (legacy behavior)
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
    wxColour ambientColour = controls.materialAmbientColor->GetBackgroundColor();
    if (ambientColour.IsOk()) {
        controls.config.rendering.materialOverride.ambientColor = wxColourToQuantityColor(ambientColour);
    }
    wxColour diffuseColour = controls.materialDiffuseColor->GetBackgroundColor();
    if (diffuseColour.IsOk()) {
        controls.config.rendering.materialOverride.diffuseColor = wxColourToQuantityColor(diffuseColour);
    }
    wxColour specularColour = controls.materialSpecularColor->GetBackgroundColor();
    if (specularColour.IsOk()) {
        controls.config.rendering.materialOverride.specularColor = wxColourToQuantityColor(specularColour);
    }
    wxColour emissiveColour = controls.materialEmissiveColor->GetBackgroundColor();
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
    
    // Enforce mode-specific requirements
    enforceModeRequirements(mode);
    
    // Sync originalEdgeEnabled for Solid and FlatLines modes
    syncOriginalEdgeEnabled(mode);
    
    // For other modes, use the checkbox value if shown
    if (mode != RenderingConfig::DisplayMode::FlatLines && mode != RenderingConfig::DisplayMode::Solid) {
        if (controls.originalEdgeEnabled && controls.originalEdgeEnabled->IsShown()) {
            controls.config.edges.originalEdge.enabled = controls.originalEdgeEnabled->GetValue();
        } else {
            controls.config.edges.originalEdge.enabled = false;
        }
    }
    if (controls.originalEdgeColor && controls.originalEdgeColor->IsShown()) {
        wxColour originalEdgeColour = controls.originalEdgeColor->GetBackgroundColor();
        if (originalEdgeColour.IsOk()) {
            controls.config.edges.originalEdge.color = wxColourToQuantityColor(originalEdgeColour);
        }
    }
    if (controls.originalEdgeWidth && controls.originalEdgeWidth->IsShown()) {
        controls.config.edges.originalEdge.width = static_cast<double>(controls.originalEdgeWidth->GetValue()) / 10.0;
    }
    
    if (controls.meshEdgeEnabled && controls.meshEdgeEnabled->IsShown()) {
        controls.config.edges.meshEdge.enabled = controls.meshEdgeEnabled->GetValue();
    }
    if (controls.meshEdgeColor && controls.meshEdgeColor->IsShown()) {
        wxColour meshEdgeColour = controls.meshEdgeColor->GetBackgroundColor();
        if (meshEdgeColour.IsOk()) {
            controls.config.edges.meshEdge.color = wxColourToQuantityColor(meshEdgeColour);
        }
    }
    if (controls.meshEdgeWidth && controls.meshEdgeWidth->IsShown()) {
        controls.config.edges.meshEdge.width = static_cast<double>(controls.meshEdgeWidth->GetValue()) / 10.0;
    }
    if (controls.meshEdgeUseEffectiveColor && controls.meshEdgeUseEffectiveColor->IsShown()) {
        controls.config.edges.meshEdge.useEffectiveColor = controls.meshEdgeUseEffectiveColor->GetValue();
    }
    
    if (controls.silhouetteEdgeEnabled && controls.silhouetteEdgeEnabled->IsShown()) {
        controls.config.edges.silhouetteEdge.enabled = controls.silhouetteEdgeEnabled->GetValue();
    }
    if (controls.silhouetteEdgeColor && controls.silhouetteEdgeColor->IsShown()) {
        wxColour silhouetteEdgeColour = controls.silhouetteEdgeColor->GetBackgroundColor();
        if (silhouetteEdgeColour.IsOk()) {
            controls.config.edges.silhouetteEdge.color = wxColourToQuantityColor(silhouetteEdgeColour);
        }
    }
    if (controls.silhouetteEdgeWidth && controls.silhouetteEdgeWidth->IsShown()) {
        controls.config.edges.silhouetteEdge.width = static_cast<double>(controls.silhouetteEdgeWidth->GetValue()) / 10.0;
    }
    
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

void DisplayModeConfigDialog::updateColorButton(FlatButton* button, const wxColour& color)
{
    if (!button) return;
    button->SetBackgroundColor(color);
    // Set text color to contrast with background
    wxColour textColor = wxColour(255 - color.Red(), 255 - color.Green(), 255 - color.Blue());
    button->SetTextColor(textColor);
    button->Refresh();
}

void DisplayModeConfigDialog::onColorButtonClicked(wxCommandEvent& event)
{
    FlatButton* button = dynamic_cast<FlatButton*>(event.GetEventObject());
    if (!button) return;
    
    wxColour currentColor = button->GetBackgroundColor();
    wxColour newColor = getColorFromDialog(currentColor);
    
    if (newColor.IsOk()) {
        updateColorButton(button, newColor);
        int currentPage = m_notebook->GetSelection();
        if (currentPage >= 0) {
            RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
            updatePreviewForMode(currentMode);
        }
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
        int currentPage = m_notebook->GetSelection();
        if (currentPage >= 0) {
            RenderingConfig::DisplayMode currentMode = this->getModeFromPageIndex(currentPage);
            updatePreviewForMode(currentMode);
        }
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
    updatePreviewForMode(mode);
}

void DisplayModeConfigDialog::schedulePreviewUpdate(RenderingConfig::DisplayMode mode, int delayMs)
{
    m_pendingPreviewMode = mode;
    m_previewUpdatePending = true;

    if (m_previewUpdateTimer.IsRunning()) {
        m_previewUpdateTimer.Stop();
    }
    m_previewUpdateTimer.Start(delayMs, true); // One-shot timer
}

void DisplayModeConfigDialog::onPreviewUpdateTimer(wxTimerEvent& event)
{
    if (m_previewUpdatePending && m_previewCanvas) {
        m_previewUpdatePending = false;
        updatePreviewForMode(m_pendingPreviewMode);
    }
}

void DisplayModeConfigDialog::updatePreviewForMode(RenderingConfig::DisplayMode mode)
{
    if (!m_previewCanvas) return;

    // For tab switching, config should already be up-to-date from enforceModeRequirements
    // For control changes, config should be up-to-date from updateConfigFromControls calls
    // But to be safe, ensure config is current before updating preview
    updateConfigFromControls(mode);
    DisplayModeConfig config = m_modeControls[mode].config;

    m_previewCanvas->updateDisplayMode(mode, config);
}

void DisplayModeConfigDialog::updateDrawStyleFromCheckboxes(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    if (!controls.drawStyle || !controls.drawStyle->IsShown()) {
        return;
    }
    
    bool showSurface = controls.config.nodes.requireSurface;
    bool showEdges = controls.config.nodes.requireOriginalEdges || controls.config.nodes.requireMeshEdges;
    bool showPoints = controls.config.nodes.requirePoints;
    
    int drawStyleIndex = 0;
    if (showPoints && !showEdges && !showSurface) {
        drawStyleIndex = 2; // POINTS
    } else if (showEdges && !showPoints && !showSurface) {
        drawStyleIndex = 1; // LINES
    } else if (showSurface && !showEdges && !showPoints) {
        drawStyleIndex = 0; // FILLED
    } else if (showSurface && showEdges && !showPoints) {
        drawStyleIndex = 3; // FILLED+LINES
    } else if (showSurface && showPoints && !showEdges) {
        drawStyleIndex = 4; // FILLED+POINTS
    } else if (showEdges && showPoints && !showSurface) {
        drawStyleIndex = 5; // LINES+POINTS
    } else if (showSurface && showEdges && showPoints) {
        drawStyleIndex = 6; // FILLED+LINES+POINTS
    }
    
    controls.drawStyle->SetSelection(drawStyleIndex);
}

void DisplayModeConfigDialog::updateCheckboxesFromDrawStyle(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    if (!controls.drawStyle || !controls.drawStyle->IsShown()) {
        return;
    }
    
    if (controls.requireSurface && controls.requireSurface->IsShown()) {
        controls.requireSurface->SetValue(controls.config.nodes.requireSurface);
    }
    if (controls.requireOriginalEdges && controls.requireOriginalEdges->IsShown()) {
        controls.requireOriginalEdges->SetValue(controls.config.nodes.requireOriginalEdges);
    }
    if (controls.requireMeshEdges && controls.requireMeshEdges->IsShown()) {
        controls.requireMeshEdges->SetValue(controls.config.nodes.requireMeshEdges);
    }
    if (controls.requirePoints && controls.requirePoints->IsShown()) {
        controls.requirePoints->SetValue(controls.config.nodes.requirePoints);
    }
}

void DisplayModeConfigDialog::syncOriginalEdgeEnabled(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    if (mode == RenderingConfig::DisplayMode::Solid || mode == RenderingConfig::DisplayMode::FlatLines) {
        controls.config.edges.originalEdge.enabled = controls.config.nodes.requireOriginalEdges;
        if (controls.originalEdgeEnabled && controls.originalEdgeEnabled->IsShown()) {
            controls.originalEdgeEnabled->SetValue(controls.config.nodes.requireOriginalEdges);
        }
    }
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
    mainSizer->Add(scrolled, 1, wxEXPAND | wxALL, 3);
    customPage->SetSizer(mainSizer);
    
    m_notebook->AddPage(customPage, "Custom");
}

void DisplayModeConfigDialog::updateModeVisibility(RenderingConfig::DisplayMode mode)
{
    ModeControls& controls = m_modeControls[mode];
    
    bool showAll = (mode == RenderingConfig::DisplayMode::Solid);
    
    // In the new single geometry architecture, use DrawStyle instead of individual checkboxes
    // Hide individual checkboxes and show DrawStyle control
    if (controls.requireSurface) controls.requireSurface->Show(false);
    if (controls.requireOriginalEdges) controls.requireOriginalEdges->Show(false);
    if (controls.requireMeshEdges) controls.requireMeshEdges->Show(false);
    if (controls.requirePoints) controls.requirePoints->Show(false);
    if (controls.drawStyle) controls.drawStyle->Show(true);

    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
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
        
        if (controls.postProcessingBox) controls.postProcessingBox->Show(true);
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(true);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(true);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(true);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(true);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(true);
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
        
        if (controls.postProcessingBox) controls.postProcessingBox->Show(true);
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(true);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(true);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(true);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(true);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(true);
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
        
        if (controls.postProcessingBox) controls.postProcessingBox->Show(true);
        if (controls.polygonOffsetEnabled) controls.polygonOffsetEnabled->Show(true);
        if (controls.polygonOffsetFactor) controls.polygonOffsetFactor->Show(true);
        if (controls.polygonOffsetFactorLabel) controls.polygonOffsetFactorLabel->Show(true);
        if (controls.polygonOffsetUnits) controls.polygonOffsetUnits->Show(true);
        if (controls.polygonOffsetUnitsLabel) controls.polygonOffsetUnitsLabel->Show(true);
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
        if (controls.requireMeshEdges) controls.requireMeshEdges->Show(false);
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
        if (controls.materialTransparency) controls.materialTransparencyLabel->Show(false);
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

        // Show silhouette edge controls instead of mesh edge controls
        if (controls.silhouetteEdgeEnabled) controls.silhouetteEdgeEnabled->Show(true);
        if (controls.silhouetteEdgeColor) controls.silhouetteEdgeColor->Show(true);
        if (controls.silhouetteEdgeWidth) controls.silhouetteEdgeWidth->Show(true);
        if (controls.silhouetteEdgeWidthLabel) controls.silhouetteEdgeWidthLabel->Show(true);

        if (controls.postProcessingBox) controls.postProcessingBox->Show(true);
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
        
        if (controls.postProcessingBox) controls.postProcessingBox->Show(true);
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

// Layout helper functions implementation
FlatComboBox* DisplayModeConfigDialog::createComboBox(wxWindow* parent, const wxString& label, 
                                                       const std::vector<wxString>& items, int defaultSelection)
{
    FlatComboBox* combo = new FlatComboBox(parent, wxID_ANY);
    for (const auto& item : items) {
        combo->Append(item);
    }
    if (defaultSelection >= 0 && defaultSelection < static_cast<int>(items.size())) {
        combo->SetSelection(defaultSelection);
    }
    combo->SetMinSize(wxSize(COMBOBOX_WIDTH, -1));
    return combo;
}

FlatButton* DisplayModeConfigDialog::createColorButton(wxWindow* parent, const wxString& label)
{
    return new FlatButton(parent, wxID_ANY, "Choose Color", wxDefaultPosition, 
                         wxSize(BUTTON_WIDTH, CONTROL_HEIGHT), FlatButton::ButtonStyle::OUTLINE);
}

wxBoxSizer* DisplayModeConfigDialog::createSliderWithLabel(wxWindow* parent, FlatSlider*& slider, 
                                                           wxStaticText*& label, int value, int minValue, 
                                                           int maxValue, const wxString& format)
{
    slider = new FlatSlider(parent, wxID_ANY, value, minValue, maxValue);
    slider->SetMinSize(wxSize(SLIDER_WIDTH, -1));
    
    wxString labelText;
    if (format == "%.1f") {
        labelText = wxString::Format("%.1f", static_cast<double>(value));
    } else if (format == "%.2f") {
        labelText = wxString::Format("%.2f", static_cast<double>(value));
    } else {
        labelText = wxString::Format("%d", value);
    }
    label = new wxStaticText(parent, wxID_ANY, labelText);
    label->SetMinSize(wxSize(LABEL_WIDTH, -1));
    
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(slider, 0, wxRIGHT, 3);
    sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
    return sizer;
}

void DisplayModeConfigDialog::addGridRow(wxFlexGridSizer* grid, wxWindow* parent, 
                                         const wxString& label, wxWindow* control)
{
    if (!label.IsEmpty()) {
        grid->Add(new wxStaticText(parent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    } else {
        grid->Add(new wxStaticText(parent, wxID_ANY, ""), 0, wxALIGN_CENTER_VERTICAL);
    }
    grid->Add(control, 0);
}

void DisplayModeConfigDialog::addGridRow(wxFlexGridSizer* grid, wxWindow* parent, 
                                         const wxString& label, wxSizer* sizer)
{
    if (!label.IsEmpty()) {
        grid->Add(new wxStaticText(parent, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    } else {
        grid->Add(new wxStaticText(parent, wxID_ANY, ""), 0, wxALIGN_CENTER_VERTICAL);
    }
    grid->Add(sizer, 0);
}

void DisplayModeConfigDialog::addCheckBox(wxSizer* sizer, FlatCheckBox* checkbox, int flags, int border)
{
    sizer->Add(checkbox, 0, flags, border);
}

