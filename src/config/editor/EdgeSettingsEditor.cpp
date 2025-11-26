#include "config/editor/EdgeSettingsEditor.h"
#include "config/EdgeSettingsConfig.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/colordlg.h>
#include <wx/statline.h>
#include <wx/settings.h>
#include "edges/ModularEdgeComponent.h"

EdgeSettingsEditor::EdgeSettingsEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId)
    : ConfigCategoryEditor(parent, configManager, categoryId)
    , m_viewer(nullptr)
    , m_notebook(nullptr)
    , m_globalPage(nullptr)
    , m_selectedPage(nullptr)
    , m_hoverPage(nullptr)
    , m_featureEdgePage(nullptr)
    , m_globalShowEdgesCheckbox(nullptr)
    , m_globalEdgeWidthSlider(nullptr)
    , m_globalEdgeWidthLabel(nullptr)
    , m_globalEdgeColorButton(nullptr)
    , m_globalEdgeColorEnabledCheckbox(nullptr)
    , m_globalEdgeStyleChoice(nullptr)
    , m_globalEdgeOpacitySlider(nullptr)
    , m_globalEdgeOpacityLabel(nullptr)
    , m_selectedShowEdgesCheckbox(nullptr)
    , m_selectedEdgeWidthSlider(nullptr)
    , m_selectedEdgeWidthLabel(nullptr)
    , m_selectedEdgeColorButton(nullptr)
    , m_selectedEdgeColorEnabledCheckbox(nullptr)
    , m_selectedEdgeStyleChoice(nullptr)
    , m_selectedEdgeOpacitySlider(nullptr)
    , m_selectedEdgeOpacityLabel(nullptr)
    , m_hoverShowEdgesCheckbox(nullptr)
    , m_hoverEdgeWidthSlider(nullptr)
    , m_hoverEdgeWidthLabel(nullptr)
    , m_hoverEdgeColorButton(nullptr)
    , m_hoverEdgeColorEnabledCheckbox(nullptr)
    , m_hoverEdgeStyleChoice(nullptr)
    , m_hoverEdgeOpacitySlider(nullptr)
    , m_hoverEdgeOpacityLabel(nullptr)
    , m_featureEdgeAngleSlider(nullptr)
    , m_featureEdgeAngleLabel(nullptr)
    , m_featureEdgeMinLengthSlider(nullptr)
    , m_featureEdgeMinLengthLabel(nullptr)
    , m_onlyConvexCheckbox(nullptr)
    , m_onlyConcaveCheckbox(nullptr)
    , m_showNormalLinesCheckbox(nullptr)
    , m_showFaceNormalLinesCheckbox(nullptr)
    , m_normalLengthSlider(nullptr)
    , m_normalLengthLabel(nullptr)
    , m_featureEdgeAngle(30)
    , m_featureEdgeMinLength(0.1)
    , m_onlyConvex(false)
    , m_onlyConcave(false)
    , m_normalLength(1.0)
{
    createUI();
}

EdgeSettingsEditor::~EdgeSettingsEditor() {
}

void EdgeSettingsEditor::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for different settings pages
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    // Create pages
    createGlobalPage();
    createSelectedPage();
    createHoverPage();
    createFeatureEdgePage();
    
    // Add pages to notebook
    m_notebook->AddPage(m_globalPage, "Global Objects");
    m_notebook->AddPage(m_selectedPage, "Selected Objects");
    m_notebook->AddPage(m_hoverPage, "Hover Objects");
    m_notebook->AddPage(m_featureEdgePage, "Feature Edges");
    
    mainSizer->Add(m_notebook, 1, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    FitInside();
    
    bindEvents();
    loadSettings();
    updateControls();
}

void EdgeSettingsEditor::createGlobalPage() {
    m_globalPage = new wxPanel(m_notebook);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Edge visibility
    m_globalShowEdgesCheckbox = new wxCheckBox(m_globalPage, wxID_ANY, "Show Edges");
    sizer->Add(m_globalShowEdgesCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge width
    wxStaticText* widthLabel = new wxStaticText(m_globalPage, wxID_ANY, "Edge Width:");
    sizer->Add(widthLabel, 0, wxALL, 5);
    
    wxBoxSizer* widthSizer = new wxBoxSizer(wxHORIZONTAL);
    m_globalEdgeWidthSlider = new wxSlider(m_globalPage, wxID_ANY, 10, 1, 50,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_globalEdgeWidthLabel = new wxStaticText(m_globalPage, wxID_ANY, "1.0");
    widthSizer->Add(m_globalEdgeWidthSlider, 1, wxEXPAND | wxRIGHT, 5);
    widthSizer->Add(m_globalEdgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(widthSizer, 0, wxALL | wxEXPAND, 5);
    
    // Edge color enabled
    m_globalEdgeColorEnabledCheckbox = new wxCheckBox(m_globalPage, wxID_ANY, "Enable Edge Color");
    sizer->Add(m_globalEdgeColorEnabledCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge color
    wxStaticText* colorLabel = new wxStaticText(m_globalPage, wxID_ANY, "Edge Color:");
    sizer->Add(colorLabel, 0, wxALL, 5);
    
    m_globalEdgeColorButton = new wxButton(m_globalPage, wxID_ANY, "Select Color");
    sizer->Add(m_globalEdgeColorButton, 0, wxALL | wxEXPAND, 5);
    
    // Edge style
    wxStaticText* styleLabel = new wxStaticText(m_globalPage, wxID_ANY, "Edge Style:");
    sizer->Add(styleLabel, 0, wxALL, 5);
    
    m_globalEdgeStyleChoice = new wxChoice(m_globalPage, wxID_ANY);
    m_globalEdgeStyleChoice->Append("Solid");
    m_globalEdgeStyleChoice->Append("Dashed");
    m_globalEdgeStyleChoice->Append("Dotted");
    m_globalEdgeStyleChoice->Append("Dash-Dot");
    sizer->Add(m_globalEdgeStyleChoice, 0, wxALL | wxEXPAND, 5);
    
    // Edge opacity
    wxStaticText* opacityLabel = new wxStaticText(m_globalPage, wxID_ANY, "Edge Opacity:");
    sizer->Add(opacityLabel, 0, wxALL, 5);
    
    wxBoxSizer* opacitySizer = new wxBoxSizer(wxHORIZONTAL);
    m_globalEdgeOpacitySlider = new wxSlider(m_globalPage, wxID_ANY, 100, 10, 100,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_globalEdgeOpacityLabel = new wxStaticText(m_globalPage, wxID_ANY, "100%");
    opacitySizer->Add(m_globalEdgeOpacitySlider, 1, wxEXPAND | wxRIGHT, 5);
    opacitySizer->Add(m_globalEdgeOpacityLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(opacitySizer, 0, wxALL | wxEXPAND, 5);
    
    m_globalPage->SetSizer(sizer);
}

void EdgeSettingsEditor::createSelectedPage() {
    m_selectedPage = new wxPanel(m_notebook);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Edge visibility
    m_selectedShowEdgesCheckbox = new wxCheckBox(m_selectedPage, wxID_ANY, "Show Edges");
    sizer->Add(m_selectedShowEdgesCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge width
    wxStaticText* widthLabel = new wxStaticText(m_selectedPage, wxID_ANY, "Edge Width:");
    sizer->Add(widthLabel, 0, wxALL, 5);
    
    wxBoxSizer* widthSizer = new wxBoxSizer(wxHORIZONTAL);
    m_selectedEdgeWidthSlider = new wxSlider(m_selectedPage, wxID_ANY, 10, 1, 50,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_selectedEdgeWidthLabel = new wxStaticText(m_selectedPage, wxID_ANY, "1.0");
    widthSizer->Add(m_selectedEdgeWidthSlider, 1, wxEXPAND | wxRIGHT, 5);
    widthSizer->Add(m_selectedEdgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(widthSizer, 0, wxALL | wxEXPAND, 5);
    
    // Edge color enabled
    m_selectedEdgeColorEnabledCheckbox = new wxCheckBox(m_selectedPage, wxID_ANY, "Enable Edge Color");
    sizer->Add(m_selectedEdgeColorEnabledCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge color
    wxStaticText* colorLabel = new wxStaticText(m_selectedPage, wxID_ANY, "Edge Color:");
    sizer->Add(colorLabel, 0, wxALL, 5);
    
    m_selectedEdgeColorButton = new wxButton(m_selectedPage, wxID_ANY, "Select Color");
    sizer->Add(m_selectedEdgeColorButton, 0, wxALL | wxEXPAND, 5);
    
    // Edge style
    wxStaticText* styleLabel = new wxStaticText(m_selectedPage, wxID_ANY, "Edge Style:");
    sizer->Add(styleLabel, 0, wxALL, 5);
    
    m_selectedEdgeStyleChoice = new wxChoice(m_selectedPage, wxID_ANY);
    m_selectedEdgeStyleChoice->Append("Solid");
    m_selectedEdgeStyleChoice->Append("Dashed");
    m_selectedEdgeStyleChoice->Append("Dotted");
    m_selectedEdgeStyleChoice->Append("Dash-Dot");
    sizer->Add(m_selectedEdgeStyleChoice, 0, wxALL | wxEXPAND, 5);
    
    // Edge opacity
    wxStaticText* opacityLabel = new wxStaticText(m_selectedPage, wxID_ANY, "Edge Opacity:");
    sizer->Add(opacityLabel, 0, wxALL, 5);
    
    wxBoxSizer* opacitySizer = new wxBoxSizer(wxHORIZONTAL);
    m_selectedEdgeOpacitySlider = new wxSlider(m_selectedPage, wxID_ANY, 100, 10, 100,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_selectedEdgeOpacityLabel = new wxStaticText(m_selectedPage, wxID_ANY, "100%");
    opacitySizer->Add(m_selectedEdgeOpacitySlider, 1, wxEXPAND | wxRIGHT, 5);
    opacitySizer->Add(m_selectedEdgeOpacityLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(opacitySizer, 0, wxALL | wxEXPAND, 5);
    
    m_selectedPage->SetSizer(sizer);
}

void EdgeSettingsEditor::createHoverPage() {
    m_hoverPage = new wxPanel(m_notebook);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Edge visibility
    m_hoverShowEdgesCheckbox = new wxCheckBox(m_hoverPage, wxID_ANY, "Show Edges");
    sizer->Add(m_hoverShowEdgesCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge width
    wxStaticText* widthLabel = new wxStaticText(m_hoverPage, wxID_ANY, "Edge Width:");
    sizer->Add(widthLabel, 0, wxALL, 5);
    
    wxBoxSizer* widthSizer = new wxBoxSizer(wxHORIZONTAL);
    m_hoverEdgeWidthSlider = new wxSlider(m_hoverPage, wxID_ANY, 10, 1, 50,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_hoverEdgeWidthLabel = new wxStaticText(m_hoverPage, wxID_ANY, "1.0");
    widthSizer->Add(m_hoverEdgeWidthSlider, 1, wxEXPAND | wxRIGHT, 5);
    widthSizer->Add(m_hoverEdgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(widthSizer, 0, wxALL | wxEXPAND, 5);
    
    // Edge color enabled
    m_hoverEdgeColorEnabledCheckbox = new wxCheckBox(m_hoverPage, wxID_ANY, "Enable Edge Color");
    sizer->Add(m_hoverEdgeColorEnabledCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge color
    wxStaticText* colorLabel = new wxStaticText(m_hoverPage, wxID_ANY, "Edge Color:");
    sizer->Add(colorLabel, 0, wxALL, 5);
    
    m_hoverEdgeColorButton = new wxButton(m_hoverPage, wxID_ANY, "Select Color");
    sizer->Add(m_hoverEdgeColorButton, 0, wxALL | wxEXPAND, 5);
    
    // Edge style
    wxStaticText* styleLabel = new wxStaticText(m_hoverPage, wxID_ANY, "Edge Style:");
    sizer->Add(styleLabel, 0, wxALL, 5);
    
    m_hoverEdgeStyleChoice = new wxChoice(m_hoverPage, wxID_ANY);
    m_hoverEdgeStyleChoice->Append("Solid");
    m_hoverEdgeStyleChoice->Append("Dashed");
    m_hoverEdgeStyleChoice->Append("Dotted");
    m_hoverEdgeStyleChoice->Append("Dash-Dot");
    sizer->Add(m_hoverEdgeStyleChoice, 0, wxALL | wxEXPAND, 5);
    
    // Edge opacity
    wxStaticText* opacityLabel = new wxStaticText(m_hoverPage, wxID_ANY, "Edge Opacity:");
    sizer->Add(opacityLabel, 0, wxALL, 5);
    
    wxBoxSizer* opacitySizer = new wxBoxSizer(wxHORIZONTAL);
    m_hoverEdgeOpacitySlider = new wxSlider(m_hoverPage, wxID_ANY, 100, 10, 100,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_hoverEdgeOpacityLabel = new wxStaticText(m_hoverPage, wxID_ANY, "100%");
    opacitySizer->Add(m_hoverEdgeOpacitySlider, 1, wxEXPAND | wxRIGHT, 5);
    opacitySizer->Add(m_hoverEdgeOpacityLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(opacitySizer, 0, wxALL | wxEXPAND, 5);
    
    m_hoverPage->SetSizer(sizer);
}

void EdgeSettingsEditor::createFeatureEdgePage() {
    m_featureEdgePage = new wxPanel(m_notebook);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Feature edge settings section
    wxStaticText* featureSectionLabel = new wxStaticText(m_featureEdgePage, wxID_ANY, "Feature Edge Settings");
    wxFont boldFont = featureSectionLabel->GetFont();
    boldFont.MakeBold();
    featureSectionLabel->SetFont(boldFont);
    sizer->Add(featureSectionLabel, 0, wxALL, 5);
    
    wxStaticText* angleLabelStatic = new wxStaticText(m_featureEdgePage, wxID_ANY, "Feature Angle Threshold (degrees):");
    sizer->Add(angleLabelStatic, 0, wxALL, 5);
    m_featureEdgeAngleSlider = new wxSlider(m_featureEdgePage, wxID_ANY, 30, 1, 90, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_featureEdgeAngleLabel = new wxStaticText(m_featureEdgePage, wxID_ANY, "30");
    wxBoxSizer* angleSizer = new wxBoxSizer(wxHORIZONTAL);
    angleSizer->Add(m_featureEdgeAngleSlider, 1, wxEXPAND | wxRIGHT, 5);
    angleSizer->Add(m_featureEdgeAngleLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(angleSizer, 0, wxALL | wxEXPAND, 5);
    
    wxStaticText* minLenLabelStatic = new wxStaticText(m_featureEdgePage, wxID_ANY, "Min Feature Edge Length:");
    sizer->Add(minLenLabelStatic, 0, wxALL, 5);
    m_featureEdgeMinLengthSlider = new wxSlider(m_featureEdgePage, wxID_ANY, 1, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_featureEdgeMinLengthLabel = new wxStaticText(m_featureEdgePage, wxID_ANY, "0.1");
    wxBoxSizer* minLenSizer = new wxBoxSizer(wxHORIZONTAL);
    minLenSizer->Add(m_featureEdgeMinLengthSlider, 1, wxEXPAND | wxRIGHT, 5);
    minLenSizer->Add(m_featureEdgeMinLengthLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(minLenSizer, 0, wxALL | wxEXPAND, 5);
    
    m_onlyConvexCheckbox = new wxCheckBox(m_featureEdgePage, wxID_ANY, "Only Convex Edges");
    m_onlyConcaveCheckbox = new wxCheckBox(m_featureEdgePage, wxID_ANY, "Only Concave Edges");
    sizer->Add(m_onlyConvexCheckbox, 0, wxALL, 5);
    sizer->Add(m_onlyConcaveCheckbox, 0, wxALL, 5);
    
    // Add separator
    sizer->AddSpacer(10);
    wxStaticLine* separator = new wxStaticLine(m_featureEdgePage, wxID_ANY);
    sizer->Add(separator, 0, wxEXPAND | wxALL, 5);
    sizer->AddSpacer(10);
    
    // Normal display settings section
    wxStaticText* normalSectionLabel = new wxStaticText(m_featureEdgePage, wxID_ANY, "Normal Display Settings");
    normalSectionLabel->SetFont(boldFont);
    sizer->Add(normalSectionLabel, 0, wxALL, 5);
    
    m_showNormalLinesCheckbox = new wxCheckBox(m_featureEdgePage, wxID_ANY, "Show Normal Lines");
    m_showNormalLinesCheckbox->SetValue(false);
    sizer->Add(m_showNormalLinesCheckbox, 0, wxALL, 5);
    
    m_showFaceNormalLinesCheckbox = new wxCheckBox(m_featureEdgePage, wxID_ANY, "Show Face Normal Lines");
    m_showFaceNormalLinesCheckbox->SetValue(false);
    sizer->Add(m_showFaceNormalLinesCheckbox, 0, wxALL, 5);
    
    wxStaticText* normalLengthLabel = new wxStaticText(m_featureEdgePage, wxID_ANY, "Normal Line Length:");
    sizer->Add(normalLengthLabel, 0, wxALL, 5);
    m_normalLengthSlider = new wxSlider(m_featureEdgePage, wxID_ANY, 50, 10, 200, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_normalLengthLabel = new wxStaticText(m_featureEdgePage, wxID_ANY, "1.0");
    wxBoxSizer* normalLengthSizer = new wxBoxSizer(wxHORIZONTAL);
    normalLengthSizer->Add(m_normalLengthSlider, 1, wxEXPAND | wxRIGHT, 5);
    normalLengthSizer->Add(m_normalLengthLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(normalLengthSizer, 0, wxALL | wxEXPAND, 5);
    
    m_featureEdgePage->SetSizer(sizer);
}

void EdgeSettingsEditor::bindEvents() {
    // Global page events
    if (m_globalShowEdgesCheckbox) m_globalShowEdgesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onGlobalShowEdgesCheckbox, this);
    if (m_globalEdgeWidthSlider) m_globalEdgeWidthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onGlobalEdgeWidthSlider, this);
    if (m_globalEdgeColorButton) m_globalEdgeColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsEditor::onGlobalEdgeColorButton, this);
    if (m_globalEdgeColorEnabledCheckbox) m_globalEdgeColorEnabledCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onGlobalEdgeColorEnabledCheckbox, this);
    if (m_globalEdgeStyleChoice) m_globalEdgeStyleChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &EdgeSettingsEditor::onGlobalEdgeStyleChoice, this);
    if (m_globalEdgeOpacitySlider) m_globalEdgeOpacitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onGlobalEdgeOpacitySlider, this);
    
    // Selected page events
    if (m_selectedShowEdgesCheckbox) m_selectedShowEdgesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onSelectedShowEdgesCheckbox, this);
    if (m_selectedEdgeWidthSlider) m_selectedEdgeWidthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onSelectedEdgeWidthSlider, this);
    if (m_selectedEdgeColorButton) m_selectedEdgeColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsEditor::onSelectedEdgeColorButton, this);
    if (m_selectedEdgeColorEnabledCheckbox) m_selectedEdgeColorEnabledCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onSelectedEdgeColorEnabledCheckbox, this);
    if (m_selectedEdgeStyleChoice) m_selectedEdgeStyleChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &EdgeSettingsEditor::onSelectedEdgeStyleChoice, this);
    if (m_selectedEdgeOpacitySlider) m_selectedEdgeOpacitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onSelectedEdgeOpacitySlider, this);
    
    // Hover page events
    if (m_hoverShowEdgesCheckbox) m_hoverShowEdgesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onHoverShowEdgesCheckbox, this);
    if (m_hoverEdgeWidthSlider) m_hoverEdgeWidthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onHoverEdgeWidthSlider, this);
    if (m_hoverEdgeColorButton) m_hoverEdgeColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsEditor::onHoverEdgeColorButton, this);
    if (m_hoverEdgeColorEnabledCheckbox) m_hoverEdgeColorEnabledCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onHoverEdgeColorEnabledCheckbox, this);
    if (m_hoverEdgeStyleChoice) m_hoverEdgeStyleChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &EdgeSettingsEditor::onHoverEdgeStyleChoice, this);
    if (m_hoverEdgeOpacitySlider) m_hoverEdgeOpacitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onHoverEdgeOpacitySlider, this);
    
    // Feature edge page events
    if (m_featureEdgeAngleSlider) m_featureEdgeAngleSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onFeatureEdgeAngleSlider, this);
    if (m_featureEdgeMinLengthSlider) m_featureEdgeMinLengthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onFeatureEdgeMinLengthSlider, this);
    if (m_onlyConvexCheckbox) m_onlyConvexCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onFeatureEdgeConvexCheckbox, this);
    if (m_onlyConcaveCheckbox) m_onlyConcaveCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onFeatureEdgeConcaveCheckbox, this);
    
    // Normal display events
    if (m_showNormalLinesCheckbox) m_showNormalLinesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onShowNormalLinesCheckbox, this);
    if (m_showFaceNormalLinesCheckbox) m_showFaceNormalLinesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsEditor::onShowFaceNormalLinesCheckbox, this);
    if (m_normalLengthSlider) m_normalLengthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsEditor::onNormalLengthSlider, this);
}

void EdgeSettingsEditor::loadConfig() {
    // UI is already created in createUI()
    // Just refresh the values
    loadSettings();
    updateControls();
}

void EdgeSettingsEditor::saveConfig() {
    applySettings();
    saveSettings();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void EdgeSettingsEditor::resetConfig() {
    EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
    config.resetToDefaults();
    loadSettings();
    updateControls();
}

void EdgeSettingsEditor::loadSettings() {
    EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
    m_globalSettings = config.getGlobalSettings();
    m_selectedSettings = config.getSelectedSettings();
    m_hoverSettings = config.getHoverSettings();
    m_featureEdgeAngle = config.getFeatureEdgeAngle();
    m_featureEdgeMinLength = config.getFeatureEdgeMinLength();
    m_onlyConvex = config.getFeatureEdgeOnlyConvex();
    m_onlyConcave = config.getFeatureEdgeOnlyConcave();
}

void EdgeSettingsEditor::saveSettings() {
    EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
    config.setGlobalSettings(m_globalSettings);
    config.setSelectedSettings(m_selectedSettings);
    config.setHoverSettings(m_hoverSettings);
    config.setFeatureEdgeAngle(m_featureEdgeAngle);
    config.setFeatureEdgeMinLength(m_featureEdgeMinLength);
    config.setFeatureEdgeOnlyConvex(m_onlyConvex);
    config.setFeatureEdgeOnlyConcave(m_onlyConcave);
    config.saveToFile();
}

void EdgeSettingsEditor::updateControls() {
    // Update global controls
    if (m_globalShowEdgesCheckbox) m_globalShowEdgesCheckbox->SetValue(m_globalSettings.showEdges);
    if (m_globalEdgeWidthSlider) m_globalEdgeWidthSlider->SetValue(static_cast<int>(m_globalSettings.edgeWidth * 10));
    if (m_globalEdgeWidthLabel) m_globalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_globalSettings.edgeWidth));
    if (m_globalEdgeColorEnabledCheckbox) m_globalEdgeColorEnabledCheckbox->SetValue(m_globalSettings.edgeColorEnabled);
    if (m_globalEdgeStyleChoice) m_globalEdgeStyleChoice->SetSelection(m_globalSettings.edgeStyle);
    if (m_globalEdgeOpacitySlider) m_globalEdgeOpacitySlider->SetValue(static_cast<int>(m_globalSettings.edgeOpacity * 100));
    if (m_globalEdgeOpacityLabel) m_globalEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_globalSettings.edgeOpacity * 100));
    
    // Update selected controls
    if (m_selectedShowEdgesCheckbox) m_selectedShowEdgesCheckbox->SetValue(m_selectedSettings.showEdges);
    if (m_selectedEdgeWidthSlider) m_selectedEdgeWidthSlider->SetValue(static_cast<int>(m_selectedSettings.edgeWidth * 10));
    if (m_selectedEdgeWidthLabel) m_selectedEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_selectedSettings.edgeWidth));
    if (m_selectedEdgeColorEnabledCheckbox) m_selectedEdgeColorEnabledCheckbox->SetValue(m_selectedSettings.edgeColorEnabled);
    if (m_selectedEdgeStyleChoice) m_selectedEdgeStyleChoice->SetSelection(m_selectedSettings.edgeStyle);
    if (m_selectedEdgeOpacitySlider) m_selectedEdgeOpacitySlider->SetValue(static_cast<int>(m_selectedSettings.edgeOpacity * 100));
    if (m_selectedEdgeOpacityLabel) m_selectedEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_selectedSettings.edgeOpacity * 100));
    
    // Update hover controls
    if (m_hoverShowEdgesCheckbox) m_hoverShowEdgesCheckbox->SetValue(m_hoverSettings.showEdges);
    if (m_hoverEdgeWidthSlider) m_hoverEdgeWidthSlider->SetValue(static_cast<int>(m_hoverSettings.edgeWidth * 10));
    if (m_hoverEdgeWidthLabel) m_hoverEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_hoverSettings.edgeWidth));
    if (m_hoverEdgeColorEnabledCheckbox) m_hoverEdgeColorEnabledCheckbox->SetValue(m_hoverSettings.edgeColorEnabled);
    if (m_hoverEdgeStyleChoice) m_hoverEdgeStyleChoice->SetSelection(m_hoverSettings.edgeStyle);
    if (m_hoverEdgeOpacitySlider) m_hoverEdgeOpacitySlider->SetValue(static_cast<int>(m_hoverSettings.edgeOpacity * 100));
    if (m_hoverEdgeOpacityLabel) m_hoverEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_hoverSettings.edgeOpacity * 100));
    
    // Update feature edge controls
    if (m_featureEdgeAngleSlider) m_featureEdgeAngleSlider->SetValue(m_featureEdgeAngle);
    if (m_featureEdgeAngleLabel) m_featureEdgeAngleLabel->SetLabel(wxString::Format("%d", m_featureEdgeAngle));
    if (m_featureEdgeMinLengthSlider) m_featureEdgeMinLengthSlider->SetValue(static_cast<int>(m_featureEdgeMinLength * 10));
    if (m_featureEdgeMinLengthLabel) m_featureEdgeMinLengthLabel->SetLabel(wxString::Format("%.1f", m_featureEdgeMinLength));
    if (m_onlyConvexCheckbox) m_onlyConvexCheckbox->SetValue(m_onlyConvex);
    if (m_onlyConcaveCheckbox) m_onlyConcaveCheckbox->SetValue(m_onlyConcave);
    
    // Update normal display controls
    if (m_normalLengthSlider) {
        m_normalLengthSlider->SetValue(static_cast<int>(m_normalLength * 50));
        if (m_normalLengthLabel) m_normalLengthLabel->SetLabel(wxString::Format("%.2f", m_normalLength));
    }
    
    // Update color buttons
    updateColorButtons();
}

void EdgeSettingsEditor::applySettings() {
    if (!m_viewer) {
        LOG_WRN("Cannot apply edge settings: OCCViewer not available", "EdgeSettingsEditor");
        return;
    }
    
    // Update config with current dialog settings
    EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
    config.setGlobalSettings(m_globalSettings);
    config.setSelectedSettings(m_selectedSettings);
    config.setHoverSettings(m_hoverSettings);
    config.setFeatureEdgeAngle(m_featureEdgeAngle);
    config.setFeatureEdgeMinLength(m_featureEdgeMinLength);
    config.setFeatureEdgeOnlyConvex(m_onlyConvex);
    config.setFeatureEdgeOnlyConcave(m_onlyConcave);
    
    // Apply settings to geometries
    config.applySettingsToGeometries();
    
    for (auto& geometry : m_viewer->getAllGeometry()) {
        if (geometry && geometry->modularEdgeComponent) {
            geometry->modularEdgeComponent->extractFeatureEdges(geometry->getShape(), m_featureEdgeAngle, m_featureEdgeMinLength, m_onlyConvex, m_onlyConcave, m_globalSettings.edgeColor, m_globalSettings.edgeWidth);
            geometry->modularEdgeComponent->applyAppearanceToEdgeNode(EdgeType::Feature, m_globalSettings.edgeColor, m_globalSettings.edgeWidth);
            geometry->modularEdgeComponent->updateEdgeDisplay(geometry->getCoinNode());
        }
    }
    
    LOG_INF("Edge settings applied successfully", "EdgeSettingsEditor");
}

// Event handlers - Global page
void EdgeSettingsEditor::onGlobalShowEdgesCheckbox(wxCommandEvent& event) {
    m_globalSettings.showEdges = m_globalShowEdgesCheckbox->GetValue();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onGlobalEdgeWidthSlider(wxCommandEvent& event) {
    m_globalSettings.edgeWidth = static_cast<double>(m_globalEdgeWidthSlider->GetValue()) / 10.0;
    if (m_globalEdgeWidthLabel) m_globalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_globalSettings.edgeWidth));
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onGlobalEdgeColorButton(wxCommandEvent& event) {
    wxColour currentColor = quantityColorToWxColour(m_globalSettings.edgeColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    
    wxColourDialog colorDialog(this, &colorData);
    colorDialog.SetTitle("Select Global Edge Color");
    
    if (colorDialog.ShowModal() == wxID_OK) {
        m_globalSettings.edgeColor = wxColourToQuantityColor(colorDialog.GetColourData().GetColour());
        updateColorButtons();
        if (m_changeCallback) m_changeCallback();
    }
}

void EdgeSettingsEditor::onGlobalEdgeColorEnabledCheckbox(wxCommandEvent& event) {
    m_globalSettings.edgeColorEnabled = m_globalEdgeColorEnabledCheckbox->GetValue();
    updateColorButtons();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onGlobalEdgeStyleChoice(wxCommandEvent& event) {
    m_globalSettings.edgeStyle = m_globalEdgeStyleChoice->GetSelection();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onGlobalEdgeOpacitySlider(wxCommandEvent& event) {
    m_globalSettings.edgeOpacity = static_cast<double>(m_globalEdgeOpacitySlider->GetValue()) / 100.0;
    if (m_globalEdgeOpacityLabel) m_globalEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_globalSettings.edgeOpacity * 100));
    if (m_changeCallback) m_changeCallback();
}

// Event handlers - Selected page
void EdgeSettingsEditor::onSelectedShowEdgesCheckbox(wxCommandEvent& event) {
    m_selectedSettings.showEdges = m_selectedShowEdgesCheckbox->GetValue();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onSelectedEdgeWidthSlider(wxCommandEvent& event) {
    m_selectedSettings.edgeWidth = static_cast<double>(m_selectedEdgeWidthSlider->GetValue()) / 10.0;
    if (m_selectedEdgeWidthLabel) m_selectedEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_selectedSettings.edgeWidth));
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onSelectedEdgeColorButton(wxCommandEvent& event) {
    wxColour currentColor = quantityColorToWxColour(m_selectedSettings.edgeColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    
    wxColourDialog colorDialog(this, &colorData);
    colorDialog.SetTitle("Select Selected Edge Color");
    
    if (colorDialog.ShowModal() == wxID_OK) {
        m_selectedSettings.edgeColor = wxColourToQuantityColor(colorDialog.GetColourData().GetColour());
        updateColorButtons();
        if (m_changeCallback) m_changeCallback();
    }
}

void EdgeSettingsEditor::onSelectedEdgeColorEnabledCheckbox(wxCommandEvent& event) {
    m_selectedSettings.edgeColorEnabled = m_selectedEdgeColorEnabledCheckbox->GetValue();
    updateColorButtons();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onSelectedEdgeStyleChoice(wxCommandEvent& event) {
    m_selectedSettings.edgeStyle = m_selectedEdgeStyleChoice->GetSelection();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onSelectedEdgeOpacitySlider(wxCommandEvent& event) {
    m_selectedSettings.edgeOpacity = static_cast<double>(m_selectedEdgeOpacitySlider->GetValue()) / 100.0;
    if (m_selectedEdgeOpacityLabel) m_selectedEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_selectedSettings.edgeOpacity * 100));
    if (m_changeCallback) m_changeCallback();
}

// Event handlers - Hover page
void EdgeSettingsEditor::onHoverShowEdgesCheckbox(wxCommandEvent& event) {
    m_hoverSettings.showEdges = m_hoverShowEdgesCheckbox->GetValue();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onHoverEdgeWidthSlider(wxCommandEvent& event) {
    m_hoverSettings.edgeWidth = static_cast<double>(m_hoverEdgeWidthSlider->GetValue()) / 10.0;
    if (m_hoverEdgeWidthLabel) m_hoverEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_hoverSettings.edgeWidth));
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onHoverEdgeColorButton(wxCommandEvent& event) {
    wxColour currentColor = quantityColorToWxColour(m_hoverSettings.edgeColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    
    wxColourDialog colorDialog(this, &colorData);
    colorDialog.SetTitle("Select Hover Edge Color");
    
    if (colorDialog.ShowModal() == wxID_OK) {
        m_hoverSettings.edgeColor = wxColourToQuantityColor(colorDialog.GetColourData().GetColour());
        updateColorButtons();
        if (m_changeCallback) m_changeCallback();
    }
}

void EdgeSettingsEditor::onHoverEdgeColorEnabledCheckbox(wxCommandEvent& event) {
    m_hoverSettings.edgeColorEnabled = m_hoverEdgeColorEnabledCheckbox->GetValue();
    updateColorButtons();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onHoverEdgeStyleChoice(wxCommandEvent& event) {
    m_hoverSettings.edgeStyle = m_hoverEdgeStyleChoice->GetSelection();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onHoverEdgeOpacitySlider(wxCommandEvent& event) {
    m_hoverSettings.edgeOpacity = static_cast<double>(m_hoverEdgeOpacitySlider->GetValue()) / 100.0;
    if (m_hoverEdgeOpacityLabel) m_hoverEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_hoverSettings.edgeOpacity * 100));
    if (m_changeCallback) m_changeCallback();
}

// Feature edge event handlers
void EdgeSettingsEditor::onFeatureEdgeAngleSlider(wxCommandEvent& event) {
    m_featureEdgeAngle = m_featureEdgeAngleSlider->GetValue();
    if (m_featureEdgeAngleLabel) m_featureEdgeAngleLabel->SetLabel(wxString::Format("%d", m_featureEdgeAngle));
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onFeatureEdgeMinLengthSlider(wxCommandEvent& event) {
    m_featureEdgeMinLength = m_featureEdgeMinLengthSlider->GetValue() / 10.0;
    if (m_featureEdgeMinLengthLabel) m_featureEdgeMinLengthLabel->SetLabel(wxString::Format("%.1f", m_featureEdgeMinLength));
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onFeatureEdgeConvexCheckbox(wxCommandEvent& event) {
    m_onlyConvex = m_onlyConvexCheckbox->GetValue();
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onFeatureEdgeConcaveCheckbox(wxCommandEvent& event) {
    m_onlyConcave = m_onlyConcaveCheckbox->GetValue();
    if (m_changeCallback) m_changeCallback();
}

// Normal display event handlers
void EdgeSettingsEditor::onShowNormalLinesCheckbox(wxCommandEvent& event) {
    bool showNormals = m_showNormalLinesCheckbox->GetValue();
    if (m_viewer) {
        m_viewer->setShowNormalLines(showNormals);
    }
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onShowFaceNormalLinesCheckbox(wxCommandEvent& event) {
    bool showFaceNormals = m_showFaceNormalLinesCheckbox->GetValue();
    if (m_viewer) {
        m_viewer->setShowFaceNormalLines(showFaceNormals);
    }
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::onNormalLengthSlider(wxCommandEvent& event) {
    m_normalLength = static_cast<double>(m_normalLengthSlider->GetValue()) / 50.0;
    if (m_normalLengthLabel) m_normalLengthLabel->SetLabel(wxString::Format("%.2f", m_normalLength));
    if (m_viewer) {
        m_viewer->setNormalLength(m_normalLength);
    }
    if (m_changeCallback) m_changeCallback();
}

void EdgeSettingsEditor::updateColorButtons() {
    // Update global color button
    if (m_globalEdgeColorButton) {
        if (m_globalSettings.edgeColorEnabled) {
            wxColour color = quantityColorToWxColour(m_globalSettings.edgeColor);
            m_globalEdgeColorButton->SetBackgroundColour(color);
            m_globalEdgeColorButton->SetForegroundColour(wxColour(255 - color.Red(), 255 - color.Green(), 255 - color.Blue()));
            m_globalEdgeColorButton->SetLabel("Global Edge Color");
        } else {
            m_globalEdgeColorButton->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
            m_globalEdgeColorButton->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
            m_globalEdgeColorButton->SetLabel("Global Edge Color (Disabled)");
        }
    }
    
    // Update selected color button
    if (m_selectedEdgeColorButton) {
        if (m_selectedSettings.edgeColorEnabled) {
            wxColour color = quantityColorToWxColour(m_selectedSettings.edgeColor);
            m_selectedEdgeColorButton->SetBackgroundColour(color);
            m_selectedEdgeColorButton->SetForegroundColour(wxColour(255 - color.Red(), 255 - color.Green(), 255 - color.Blue()));
            m_selectedEdgeColorButton->SetLabel("Selected Edge Color");
        } else {
            m_selectedEdgeColorButton->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
            m_selectedEdgeColorButton->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
            m_selectedEdgeColorButton->SetLabel("Selected Edge Color (Disabled)");
        }
    }
    
    // Update hover color button
    if (m_hoverEdgeColorButton) {
        if (m_hoverSettings.edgeColorEnabled) {
            wxColour color = quantityColorToWxColour(m_hoverSettings.edgeColor);
            m_hoverEdgeColorButton->SetBackgroundColour(color);
            m_hoverEdgeColorButton->SetForegroundColour(wxColour(255 - color.Red(), 255 - color.Green(), 255 - color.Blue()));
            m_hoverEdgeColorButton->SetLabel("Hover Edge Color");
        } else {
            m_hoverEdgeColorButton->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
            m_hoverEdgeColorButton->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
            m_hoverEdgeColorButton->SetLabel("Hover Edge Color (Disabled)");
        }
    }
}

wxColour EdgeSettingsEditor::quantityColorToWxColour(const Quantity_Color& color) {
    return wxColour(
        static_cast<unsigned char>(color.Red() * 255),
        static_cast<unsigned char>(color.Green() * 255),
        static_cast<unsigned char>(color.Blue() * 255)
    );
}

Quantity_Color EdgeSettingsEditor::wxColourToQuantityColor(const wxColour& color) {
    return Quantity_Color(
        static_cast<double>(color.Red()) / 255.0,
        static_cast<double>(color.Green()) / 255.0,
        static_cast<double>(color.Blue()) / 255.0,
        Quantity_TOC_RGB
    );
}

