#include "EdgeSettingsDialog.h"
#include "config/RenderingConfig.h"
#include "config/EdgeSettingsConfig.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/colordlg.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <string>

EdgeSettingsDialog::EdgeSettingsDialog(wxWindow* parent, OCCViewer* viewer)
    : wxDialog(parent, wxID_ANY, "Edge Settings", wxDefaultPosition, wxSize(600, 700))
    , m_viewer(viewer)
    , m_currentPage("global")
{
    loadSettings();
    createControls();
    bindEvents();
    updateControls();
    
    LOG_INF_S("EdgeSettingsDialog created");
}

EdgeSettingsDialog::~EdgeSettingsDialog()
{
    LOG_INF_S("EdgeSettingsDialog destroyed");
}

void EdgeSettingsDialog::createControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for different settings pages
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    // Create pages
    createGlobalPage();
    createSelectedPage();
    createHoverPage();
    
    // Add pages to notebook
    m_notebook->AddPage(m_globalPage, "Global Objects");
    m_notebook->AddPage(m_selectedPage, "Selected Objects");
    m_notebook->AddPage(m_hoverPage, "Hover Objects");
    
    mainSizer->Add(m_notebook, 1, wxALL | wxEXPAND, 10);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_applyButton = new wxButton(this, wxID_APPLY, "Apply");
    m_resetButton = new wxButton(this, wxID_RESET, "Reset");
    m_saveButton = new wxButton(this, wxID_SAVE, "Save Config");
    m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    m_okButton = new wxButton(this, wxID_OK, "OK");
    
    buttonSizer->Add(m_applyButton, 0, wxALL, 5);
    buttonSizer->Add(m_resetButton, 0, wxALL, 5);
    buttonSizer->Add(m_saveButton, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_cancelButton, 0, wxALL, 5);
    buttonSizer->Add(m_okButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    Layout();
}

void EdgeSettingsDialog::createGlobalPage()
{
    m_globalPage = new wxPanel(m_notebook);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Edge visibility
    m_globalShowEdgesCheckbox = new wxCheckBox(m_globalPage, wxID_ANY, "Show Edges");
    m_globalShowEdgesCheckbox->SetValue(m_globalSettings.showEdges);
    sizer->Add(m_globalShowEdgesCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge width
    wxStaticText* widthLabel = new wxStaticText(m_globalPage, wxID_ANY, "Edge Width:");
    sizer->Add(widthLabel, 0, wxALL, 5);
    
    wxBoxSizer* widthSizer = new wxBoxSizer(wxHORIZONTAL);
    m_globalEdgeWidthSlider = new wxSlider(m_globalPage, wxID_ANY, 
        static_cast<int>(m_globalSettings.edgeWidth * 10), 1, 50,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_globalEdgeWidthLabel = new wxStaticText(m_globalPage, wxID_ANY, 
        wxString::Format("%.1f", m_globalSettings.edgeWidth));
    widthSizer->Add(m_globalEdgeWidthSlider, 1, wxEXPAND | wxRIGHT, 5);
    widthSizer->Add(m_globalEdgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(widthSizer, 0, wxALL | wxEXPAND, 5);
    
    // Edge color enabled
    m_globalEdgeColorEnabledCheckbox = new wxCheckBox(m_globalPage, wxID_ANY, "Enable Edge Color");
    m_globalEdgeColorEnabledCheckbox->SetValue(m_globalSettings.edgeColorEnabled);
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
    m_globalEdgeStyleChoice->SetSelection(m_globalSettings.edgeStyle);
    sizer->Add(m_globalEdgeStyleChoice, 0, wxALL | wxEXPAND, 5);
    
    // Edge opacity
    wxStaticText* opacityLabel = new wxStaticText(m_globalPage, wxID_ANY, "Edge Opacity:");
    sizer->Add(opacityLabel, 0, wxALL, 5);
    
    wxBoxSizer* opacitySizer = new wxBoxSizer(wxHORIZONTAL);
    m_globalEdgeOpacitySlider = new wxSlider(m_globalPage, wxID_ANY, 
        static_cast<int>(m_globalSettings.edgeOpacity * 100), 10, 100,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_globalEdgeOpacityLabel = new wxStaticText(m_globalPage, wxID_ANY, 
        wxString::Format("%.0f%%", m_globalSettings.edgeOpacity * 100));
    opacitySizer->Add(m_globalEdgeOpacitySlider, 1, wxEXPAND | wxRIGHT, 5);
    opacitySizer->Add(m_globalEdgeOpacityLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(opacitySizer, 0, wxALL | wxEXPAND, 5);
    
    m_globalPage->SetSizer(sizer);
}

void EdgeSettingsDialog::createSelectedPage()
{
    m_selectedPage = new wxPanel(m_notebook);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Edge visibility
    m_selectedShowEdgesCheckbox = new wxCheckBox(m_selectedPage, wxID_ANY, "Show Edges");
    m_selectedShowEdgesCheckbox->SetValue(m_selectedSettings.showEdges);
    sizer->Add(m_selectedShowEdgesCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge width
    wxStaticText* widthLabel = new wxStaticText(m_selectedPage, wxID_ANY, "Edge Width:");
    sizer->Add(widthLabel, 0, wxALL, 5);
    
    wxBoxSizer* widthSizer = new wxBoxSizer(wxHORIZONTAL);
    m_selectedEdgeWidthSlider = new wxSlider(m_selectedPage, wxID_ANY, 
        static_cast<int>(m_selectedSettings.edgeWidth * 10), 1, 50,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_selectedEdgeWidthLabel = new wxStaticText(m_selectedPage, wxID_ANY, 
        wxString::Format("%.1f", m_selectedSettings.edgeWidth));
    widthSizer->Add(m_selectedEdgeWidthSlider, 1, wxEXPAND | wxRIGHT, 5);
    widthSizer->Add(m_selectedEdgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(widthSizer, 0, wxALL | wxEXPAND, 5);
    
    // Edge color enabled
    m_selectedEdgeColorEnabledCheckbox = new wxCheckBox(m_selectedPage, wxID_ANY, "Enable Edge Color");
    m_selectedEdgeColorEnabledCheckbox->SetValue(m_selectedSettings.edgeColorEnabled);
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
    m_selectedEdgeStyleChoice->SetSelection(m_selectedSettings.edgeStyle);
    sizer->Add(m_selectedEdgeStyleChoice, 0, wxALL | wxEXPAND, 5);
    
    // Edge opacity
    wxStaticText* opacityLabel = new wxStaticText(m_selectedPage, wxID_ANY, "Edge Opacity:");
    sizer->Add(opacityLabel, 0, wxALL, 5);
    
    wxBoxSizer* opacitySizer = new wxBoxSizer(wxHORIZONTAL);
    m_selectedEdgeOpacitySlider = new wxSlider(m_selectedPage, wxID_ANY, 
        static_cast<int>(m_selectedSettings.edgeOpacity * 100), 10, 100,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_selectedEdgeOpacityLabel = new wxStaticText(m_selectedPage, wxID_ANY, 
        wxString::Format("%.0f%%", m_selectedSettings.edgeOpacity * 100));
    opacitySizer->Add(m_selectedEdgeOpacitySlider, 1, wxEXPAND | wxRIGHT, 5);
    opacitySizer->Add(m_selectedEdgeOpacityLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(opacitySizer, 0, wxALL | wxEXPAND, 5);
    
    m_selectedPage->SetSizer(sizer);
}

void EdgeSettingsDialog::createHoverPage()
{
    m_hoverPage = new wxPanel(m_notebook);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Edge visibility
    m_hoverShowEdgesCheckbox = new wxCheckBox(m_hoverPage, wxID_ANY, "Show Edges");
    m_hoverShowEdgesCheckbox->SetValue(m_hoverSettings.showEdges);
    sizer->Add(m_hoverShowEdgesCheckbox, 0, wxALL | wxEXPAND, 5);
    
    // Edge width
    wxStaticText* widthLabel = new wxStaticText(m_hoverPage, wxID_ANY, "Edge Width:");
    sizer->Add(widthLabel, 0, wxALL, 5);
    
    wxBoxSizer* widthSizer = new wxBoxSizer(wxHORIZONTAL);
    m_hoverEdgeWidthSlider = new wxSlider(m_hoverPage, wxID_ANY, 
        static_cast<int>(m_hoverSettings.edgeWidth * 10), 1, 50,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_hoverEdgeWidthLabel = new wxStaticText(m_hoverPage, wxID_ANY, 
        wxString::Format("%.1f", m_hoverSettings.edgeWidth));
    widthSizer->Add(m_hoverEdgeWidthSlider, 1, wxEXPAND | wxRIGHT, 5);
    widthSizer->Add(m_hoverEdgeWidthLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(widthSizer, 0, wxALL | wxEXPAND, 5);
    
    // Edge color enabled
    m_hoverEdgeColorEnabledCheckbox = new wxCheckBox(m_hoverPage, wxID_ANY, "Enable Edge Color");
    m_hoverEdgeColorEnabledCheckbox->SetValue(m_hoverSettings.edgeColorEnabled);
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
    m_hoverEdgeStyleChoice->SetSelection(m_hoverSettings.edgeStyle);
    sizer->Add(m_hoverEdgeStyleChoice, 0, wxALL | wxEXPAND, 5);
    
    // Edge opacity
    wxStaticText* opacityLabel = new wxStaticText(m_hoverPage, wxID_ANY, "Edge Opacity:");
    sizer->Add(opacityLabel, 0, wxALL, 5);
    
    wxBoxSizer* opacitySizer = new wxBoxSizer(wxHORIZONTAL);
    m_hoverEdgeOpacitySlider = new wxSlider(m_hoverPage, wxID_ANY, 
        static_cast<int>(m_hoverSettings.edgeOpacity * 100), 10, 100,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_hoverEdgeOpacityLabel = new wxStaticText(m_hoverPage, wxID_ANY, 
        wxString::Format("%.0f%%", m_hoverSettings.edgeOpacity * 100));
    opacitySizer->Add(m_hoverEdgeOpacitySlider, 1, wxEXPAND | wxRIGHT, 5);
    opacitySizer->Add(m_hoverEdgeOpacityLabel, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(opacitySizer, 0, wxALL | wxEXPAND, 5);
    
    m_hoverPage->SetSizer(sizer);
}

void EdgeSettingsDialog::bindEvents()
{
    // Global page events
    m_globalShowEdgesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsDialog::onGlobalShowEdgesCheckbox, this);
    m_globalEdgeWidthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsDialog::onGlobalEdgeWidthSlider, this);
    m_globalEdgeColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsDialog::onGlobalEdgeColorButton, this);
    m_globalEdgeColorEnabledCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsDialog::onGlobalEdgeColorEnabledCheckbox, this);
    m_globalEdgeStyleChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &EdgeSettingsDialog::onGlobalEdgeStyleChoice, this);
    m_globalEdgeOpacitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsDialog::onGlobalEdgeOpacitySlider, this);
    
    // Selected page events
    m_selectedShowEdgesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsDialog::onSelectedShowEdgesCheckbox, this);
    m_selectedEdgeWidthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsDialog::onSelectedEdgeWidthSlider, this);
    m_selectedEdgeColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsDialog::onSelectedEdgeColorButton, this);
    m_selectedEdgeColorEnabledCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsDialog::onSelectedEdgeColorEnabledCheckbox, this);
    m_selectedEdgeStyleChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &EdgeSettingsDialog::onSelectedEdgeStyleChoice, this);
    m_selectedEdgeOpacitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsDialog::onSelectedEdgeOpacitySlider, this);
    
    // Hover page events
    m_hoverShowEdgesCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsDialog::onHoverShowEdgesCheckbox, this);
    m_hoverEdgeWidthSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsDialog::onHoverEdgeWidthSlider, this);
    m_hoverEdgeColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsDialog::onHoverEdgeColorButton, this);
    m_hoverEdgeColorEnabledCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &EdgeSettingsDialog::onHoverEdgeColorEnabledCheckbox, this);
    m_hoverEdgeStyleChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &EdgeSettingsDialog::onHoverEdgeStyleChoice, this);
    m_hoverEdgeOpacitySlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &EdgeSettingsDialog::onHoverEdgeOpacitySlider, this);
    
    // Button events
    m_applyButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsDialog::onApply, this);
    m_resetButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsDialog::onReset, this);
    m_saveButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsDialog::onSave, this);
    m_cancelButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsDialog::onCancel, this);
    m_okButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EdgeSettingsDialog::onOK, this);
}

void EdgeSettingsDialog::updateControls()
{
    // Update global controls
    m_globalShowEdgesCheckbox->SetValue(m_globalSettings.showEdges);
    m_globalEdgeWidthSlider->SetValue(static_cast<int>(m_globalSettings.edgeWidth * 10));
    m_globalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_globalSettings.edgeWidth));
    m_globalEdgeColorEnabledCheckbox->SetValue(m_globalSettings.edgeColorEnabled);
    m_globalEdgeStyleChoice->SetSelection(m_globalSettings.edgeStyle);
    m_globalEdgeOpacitySlider->SetValue(static_cast<int>(m_globalSettings.edgeOpacity * 100));
    m_globalEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_globalSettings.edgeOpacity * 100));
    
    // Update selected controls
    m_selectedShowEdgesCheckbox->SetValue(m_selectedSettings.showEdges);
    m_selectedEdgeWidthSlider->SetValue(static_cast<int>(m_selectedSettings.edgeWidth * 10));
    m_selectedEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_selectedSettings.edgeWidth));
    m_selectedEdgeColorEnabledCheckbox->SetValue(m_selectedSettings.edgeColorEnabled);
    m_selectedEdgeStyleChoice->SetSelection(m_selectedSettings.edgeStyle);
    m_selectedEdgeOpacitySlider->SetValue(static_cast<int>(m_selectedSettings.edgeOpacity * 100));
    m_selectedEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_selectedSettings.edgeOpacity * 100));
    
    // Update hover controls
    m_hoverShowEdgesCheckbox->SetValue(m_hoverSettings.showEdges);
    m_hoverEdgeWidthSlider->SetValue(static_cast<int>(m_hoverSettings.edgeWidth * 10));
    m_hoverEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_hoverSettings.edgeWidth));
    m_hoverEdgeColorEnabledCheckbox->SetValue(m_hoverSettings.edgeColorEnabled);
    m_hoverEdgeStyleChoice->SetSelection(m_hoverSettings.edgeStyle);
    m_hoverEdgeOpacitySlider->SetValue(static_cast<int>(m_hoverSettings.edgeOpacity * 100));
    m_hoverEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_hoverSettings.edgeOpacity * 100));
    
    // Update color buttons
    updateColorButtons();
}

void EdgeSettingsDialog::loadSettings()
{
    EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
    m_globalSettings = config.getGlobalSettings();
    m_selectedSettings = config.getSelectedSettings();
    m_hoverSettings = config.getHoverSettings();
    m_currentSettings = m_globalSettings;
}

void EdgeSettingsDialog::saveSettings()
{
    EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
    config.setGlobalSettings(m_globalSettings);
    config.setSelectedSettings(m_selectedSettings);
    config.setHoverSettings(m_hoverSettings);
    config.saveToFile();
}

void EdgeSettingsDialog::applySettings()
{
    if (!m_viewer) {
        LOG_ERR_S("Cannot apply edge settings: OCCViewer not available");
        return;
    }
    
    // Update config with current dialog settings
    EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
    config.setGlobalSettings(m_globalSettings);
    config.setSelectedSettings(m_selectedSettings);
    config.setHoverSettings(m_hoverSettings);
    
    // Apply settings to geometries (this will trigger notifications)
    config.applySettingsToGeometries();
    
    // Force refresh
    if (GetParent()) {
        GetParent()->Refresh(true);
        GetParent()->Update();
    }
    
    LOG_INF_S("Edge settings applied successfully");
}

// Global page event handlers
void EdgeSettingsDialog::onGlobalShowEdgesCheckbox(wxCommandEvent& event)
{
    m_globalSettings.showEdges = m_globalShowEdgesCheckbox->GetValue();
    LOG_INF_S("Global show edges changed to: " + std::string(m_globalSettings.showEdges ? "enabled" : "disabled"));
}

void EdgeSettingsDialog::onGlobalEdgeWidthSlider(wxCommandEvent& event)
{
    m_globalSettings.edgeWidth = static_cast<double>(m_globalEdgeWidthSlider->GetValue()) / 10.0;
    m_globalEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_globalSettings.edgeWidth));
    LOG_INF_S("Global edge width changed to: " + std::to_string(m_globalSettings.edgeWidth));
}

void EdgeSettingsDialog::onGlobalEdgeColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_globalSettings.edgeColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    
    wxColourDialog colorDialog(this, &colorData);
    colorDialog.SetTitle("Select Global Edge Color");
    
    if (colorDialog.ShowModal() == wxID_OK) {
        m_globalSettings.edgeColor = wxColourToQuantityColor(colorDialog.GetColourData().GetColour());
        updateColorButtons();
        LOG_INF_S("Global edge color changed");
    }
}

void EdgeSettingsDialog::onGlobalEdgeColorEnabledCheckbox(wxCommandEvent& event)
{
    m_globalSettings.edgeColorEnabled = m_globalEdgeColorEnabledCheckbox->GetValue();
    updateColorButtons();
    LOG_INF_S("Global edge color enabled changed to: " + std::string(m_globalSettings.edgeColorEnabled ? "enabled" : "disabled"));
}

void EdgeSettingsDialog::onGlobalEdgeStyleChoice(wxCommandEvent& event)
{
    m_globalSettings.edgeStyle = m_globalEdgeStyleChoice->GetSelection();
    LOG_INF_S("Global edge style changed to: " + std::to_string(m_globalSettings.edgeStyle));
}

void EdgeSettingsDialog::onGlobalEdgeOpacitySlider(wxCommandEvent& event)
{
    m_globalSettings.edgeOpacity = static_cast<double>(m_globalEdgeOpacitySlider->GetValue()) / 100.0;
    m_globalEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_globalSettings.edgeOpacity * 100));
    LOG_INF_S("Global edge opacity changed to: " + std::to_string(m_globalSettings.edgeOpacity));
}

// Selected page event handlers
void EdgeSettingsDialog::onSelectedShowEdgesCheckbox(wxCommandEvent& event)
{
    m_selectedSettings.showEdges = m_selectedShowEdgesCheckbox->GetValue();
    LOG_INF_S("Selected show edges changed to: " + std::string(m_selectedSettings.showEdges ? "enabled" : "disabled"));
}

void EdgeSettingsDialog::onSelectedEdgeWidthSlider(wxCommandEvent& event)
{
    m_selectedSettings.edgeWidth = static_cast<double>(m_selectedEdgeWidthSlider->GetValue()) / 10.0;
    m_selectedEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_selectedSettings.edgeWidth));
    LOG_INF_S("Selected edge width changed to: " + std::to_string(m_selectedSettings.edgeWidth));
}

void EdgeSettingsDialog::onSelectedEdgeColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_selectedSettings.edgeColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    
    wxColourDialog colorDialog(this, &colorData);
    colorDialog.SetTitle("Select Selected Edge Color");
    
    if (colorDialog.ShowModal() == wxID_OK) {
        m_selectedSettings.edgeColor = wxColourToQuantityColor(colorDialog.GetColourData().GetColour());
        updateColorButtons();
        LOG_INF_S("Selected edge color changed");
    }
}

void EdgeSettingsDialog::onSelectedEdgeColorEnabledCheckbox(wxCommandEvent& event)
{
    m_selectedSettings.edgeColorEnabled = m_selectedEdgeColorEnabledCheckbox->GetValue();
    updateColorButtons();
    LOG_INF_S("Selected edge color enabled changed to: " + std::string(m_selectedSettings.edgeColorEnabled ? "enabled" : "disabled"));
}

void EdgeSettingsDialog::onSelectedEdgeStyleChoice(wxCommandEvent& event)
{
    m_selectedSettings.edgeStyle = m_selectedEdgeStyleChoice->GetSelection();
    LOG_INF_S("Selected edge style changed to: " + std::to_string(m_selectedSettings.edgeStyle));
}

void EdgeSettingsDialog::onSelectedEdgeOpacitySlider(wxCommandEvent& event)
{
    m_selectedSettings.edgeOpacity = static_cast<double>(m_selectedEdgeOpacitySlider->GetValue()) / 100.0;
    m_selectedEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_selectedSettings.edgeOpacity * 100));
    LOG_INF_S("Selected edge opacity changed to: " + std::to_string(m_selectedSettings.edgeOpacity));
}

// Hover page event handlers
void EdgeSettingsDialog::onHoverShowEdgesCheckbox(wxCommandEvent& event)
{
    m_hoverSettings.showEdges = m_hoverShowEdgesCheckbox->GetValue();
    LOG_INF_S("Hover show edges changed to: " + std::string(m_hoverSettings.showEdges ? "enabled" : "disabled"));
}

void EdgeSettingsDialog::onHoverEdgeWidthSlider(wxCommandEvent& event)
{
    m_hoverSettings.edgeWidth = static_cast<double>(m_hoverEdgeWidthSlider->GetValue()) / 10.0;
    m_hoverEdgeWidthLabel->SetLabel(wxString::Format("%.1f", m_hoverSettings.edgeWidth));
    LOG_INF_S("Hover edge width changed to: " + std::to_string(m_hoverSettings.edgeWidth));
}

void EdgeSettingsDialog::onHoverEdgeColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_hoverSettings.edgeColor);
    wxColourData colorData;
    colorData.SetColour(currentColor);
    
    wxColourDialog colorDialog(this, &colorData);
    colorDialog.SetTitle("Select Hover Edge Color");
    
    if (colorDialog.ShowModal() == wxID_OK) {
        m_hoverSettings.edgeColor = wxColourToQuantityColor(colorDialog.GetColourData().GetColour());
        updateColorButtons();
        LOG_INF_S("Hover edge color changed");
    }
}

void EdgeSettingsDialog::onHoverEdgeColorEnabledCheckbox(wxCommandEvent& event)
{
    m_hoverSettings.edgeColorEnabled = m_hoverEdgeColorEnabledCheckbox->GetValue();
    updateColorButtons();
    LOG_INF_S("Hover edge color enabled changed to: " + std::string(m_hoverSettings.edgeColorEnabled ? "enabled" : "disabled"));
}

void EdgeSettingsDialog::onHoverEdgeStyleChoice(wxCommandEvent& event)
{
    m_hoverSettings.edgeStyle = m_hoverEdgeStyleChoice->GetSelection();
    LOG_INF_S("Hover edge style changed to: " + std::to_string(m_hoverSettings.edgeStyle));
}

void EdgeSettingsDialog::onHoverEdgeOpacitySlider(wxCommandEvent& event)
{
    m_hoverSettings.edgeOpacity = static_cast<double>(m_hoverEdgeOpacitySlider->GetValue()) / 100.0;
    m_hoverEdgeOpacityLabel->SetLabel(wxString::Format("%.0f%%", m_hoverSettings.edgeOpacity * 100));
    LOG_INF_S("Hover edge opacity changed to: " + std::to_string(m_hoverSettings.edgeOpacity));
}

void EdgeSettingsDialog::onApply(wxCommandEvent& event)
{
    applySettings();
    
    wxMessageBox("Edge settings applied to all objects", "Edge Settings Applied", wxOK | wxICON_INFORMATION);
}

void EdgeSettingsDialog::onSave(wxCommandEvent& event)
{
    saveSettings();
    wxMessageBox("Edge settings saved to configuration file", "Settings Saved", wxOK | wxICON_INFORMATION);
}

void EdgeSettingsDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void EdgeSettingsDialog::onOK(wxCommandEvent& event)
{
    // Apply settings first
    applySettings();
    
    // Then save to file
    saveSettings();
    
    EndModal(wxID_OK);
}

void EdgeSettingsDialog::onReset(wxCommandEvent& event)
{
    EdgeSettingsConfig& config = EdgeSettingsConfig::getInstance();
    config.resetToDefaults();
    loadSettings();
    updateControls();
    LOG_INF_S("Edge settings reset to defaults");
}

void EdgeSettingsDialog::updateColorButtons()
{
    // Update global color button
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
    
    // Update selected color button
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
    
    // Update hover color button
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

wxColour EdgeSettingsDialog::quantityColorToWxColour(const Quantity_Color& color)
{
    return wxColour(
        static_cast<unsigned char>(color.Red() * 255),
        static_cast<unsigned char>(color.Green() * 255),
        static_cast<unsigned char>(color.Blue() * 255)
    );
}

Quantity_Color EdgeSettingsDialog::wxColourToQuantityColor(const wxColour& color)
{
    return Quantity_Color(
        static_cast<double>(color.Red()) / 255.0,
        static_cast<double>(color.Green()) / 255.0,
        static_cast<double>(color.Blue()) / 255.0,
        Quantity_TOC_RGB
    );
} 