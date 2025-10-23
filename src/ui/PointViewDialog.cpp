#include "PointViewDialog.h"
#include <wx/colordlg.h>
#include <wx/statline.h>
#include "OCCViewer.h"
#include "RenderingEngine.h"
#include "config/RenderingConfig.h"

// Constructor
PointViewDialog::PointViewDialog(wxWindow* parent, OCCViewer* occViewer, RenderingEngine* renderingEngine)
    : FramelessModalPopup(parent, "Point View Settings", wxSize(400, 300))
    , m_occViewer(occViewer)
    , m_renderingEngine(renderingEngine)
    , m_showPointView(false)
    , m_showSolid(true)
    , m_pointSize(3.0)
    , m_pointColor(1.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_pointShape(0)
{
    // Load current settings
    if (m_occViewer) {
        const auto& displaySettings = m_occViewer->getDisplaySettings();
        m_showPointView = displaySettings.showPointView;
        m_showSolid = displaySettings.showSolidWithPointView;
        m_pointSize = displaySettings.pointSize;
        m_pointColor = displaySettings.pointColor;
        m_pointShape = displaySettings.pointShape;
    }

    // Set up title bar with icon
    SetTitleIcon("pointview", wxSize(20, 20));
    ShowTitleIcon(true);

    createControls();
    layoutControls();
    bindEvents();
    updateControls();
}

// Destructor
PointViewDialog::~PointViewDialog()
{
}

// Create dialog controls
void PointViewDialog::createControls()
{
    // Use the content panel provided by FramelessModalPopup
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Point view controls
    wxStaticBoxSizer* pointViewSizer = new wxStaticBoxSizer(wxVERTICAL, m_contentPanel, "Point View");

    // Enable point view checkbox
    m_showPointViewCheckbox = new wxCheckBox(m_contentPanel, wxID_ANY, "Enable Point View");
    m_showPointViewCheckbox->SetValue(m_showPointView);

    // Show solid checkbox
    m_showSolidCheckbox = new wxCheckBox(m_contentPanel, wxID_ANY, "Show Solid Geometry");
    m_showSolidCheckbox->SetValue(m_showSolid);

    // Point size slider
    wxBoxSizer* pointSizeSizer = new wxBoxSizer(wxHORIZONTAL);
    pointSizeSizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Point Size:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    m_pointSizeSlider = new wxSlider(m_contentPanel, wxID_ANY, static_cast<int>(m_pointSize * 10), 5, 100);
    m_pointSizeLabel = new wxStaticText(m_contentPanel, wxID_ANY, wxString::Format("%.1f", m_pointSize));
    pointSizeSizer->Add(m_pointSizeSlider, 1, wxEXPAND | wxRIGHT, 5);
    pointSizeSizer->Add(m_pointSizeLabel, 0, wxALIGN_CENTER_VERTICAL);

    // Point color button
    wxBoxSizer* pointColorSizer = new wxBoxSizer(wxHORIZONTAL);
    pointColorSizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Point Color:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_pointColorButton = new wxButton(m_contentPanel, wxID_ANY, "Choose Color");
    updateColorButton(m_pointColorButton, quantityColorToWxColour(m_pointColor));
    pointColorSizer->Add(m_pointColorButton, 0);

    // Point shape choice
    wxBoxSizer* pointShapeSizer = new wxBoxSizer(wxHORIZONTAL);
    pointShapeSizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Point Shape:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_pointShapeChoice = new wxChoice(m_contentPanel, wxID_ANY);
    m_pointShapeChoice->Append("Square");
    m_pointShapeChoice->Append("Circle");
    m_pointShapeChoice->Append("Triangle");
    m_pointShapeChoice->SetSelection(m_pointShape);
    pointShapeSizer->Add(m_pointShapeChoice, 0);

    // Add controls to point view sizer
    pointViewSizer->Add(m_showPointViewCheckbox, 0, wxALL, 5);
    pointViewSizer->Add(m_showSolidCheckbox, 0, wxALL, 5);
    pointViewSizer->Add(pointSizeSizer, 0, wxEXPAND | wxALL, 5);
    pointViewSizer->Add(pointColorSizer, 0, wxALL, 5);
    pointViewSizer->Add(pointShapeSizer, 0, wxALL, 5);

    mainSizer->Add(pointViewSizer, 1, wxEXPAND | wxALL, 10);

    // Dialog buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_applyButton = new wxButton(m_contentPanel, wxID_ANY, "Apply");
    m_resetButton = new wxButton(m_contentPanel, wxID_ANY, "Reset");
    m_okButton = new wxButton(m_contentPanel, wxID_ANY, "OK");
    m_cancelButton = new wxButton(m_contentPanel, wxID_ANY, "Cancel");

    buttonSizer->Add(m_applyButton, 0, wxRIGHT, 5);
    buttonSizer->Add(m_resetButton, 0, wxRIGHT, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_cancelButton, 0, wxRIGHT, 5);
    buttonSizer->Add(m_okButton, 0);

    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

    m_contentPanel->SetSizer(mainSizer);
}

// Layout controls
void PointViewDialog::layoutControls()
{
    // Controls are laid out in createControls()
}

// Bind events
void PointViewDialog::bindEvents()
{
    m_showPointViewCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PointViewDialog::onShowPointViewCheckbox, this);
    m_showSolidCheckbox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &PointViewDialog::onShowSolidCheckbox, this);
    m_pointSizeSlider->Bind(wxEVT_COMMAND_SLIDER_UPDATED, &PointViewDialog::onPointSizeSlider, this);
    m_pointColorButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PointViewDialog::onPointColorButton, this);
    m_pointShapeChoice->Bind(wxEVT_COMMAND_CHOICE_SELECTED, &PointViewDialog::onPointShapeChoice, this);

    m_applyButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PointViewDialog::onApply, this);
    m_resetButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PointViewDialog::onReset, this);
    m_okButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PointViewDialog::onOK, this);
    m_cancelButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PointViewDialog::onCancel, this);
}

// Update controls state
void PointViewDialog::updateControls()
{
    bool enabled = m_showPointViewCheckbox->GetValue();
    m_showSolidCheckbox->Enable(enabled);
    m_pointSizeSlider->Enable(enabled);
    m_pointColorButton->Enable(enabled);
    m_pointShapeChoice->Enable(enabled);
}

// Event handlers
void PointViewDialog::onShowPointViewCheckbox(wxCommandEvent& event)
{
    m_showPointView = event.IsChecked();
    updateControls();
}

void PointViewDialog::onShowSolidCheckbox(wxCommandEvent& event)
{
    m_showSolid = event.IsChecked();
}

void PointViewDialog::onPointSizeSlider(wxCommandEvent& event)
{
    m_pointSize = static_cast<double>(m_pointSizeSlider->GetValue()) / 10.0;
    m_pointSizeLabel->SetLabel(wxString::Format("%.1f", m_pointSize));
}

void PointViewDialog::onPointColorButton(wxCommandEvent& event)
{
    wxColour currentColor = quantityColorToWxColour(m_pointColor);
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(currentColor);

    if (colorDialog.ShowModal() == wxID_OK) {
        wxColour newColor = colorDialog.GetColourData().GetColour();
        m_pointColor = wxColourToQuantityColor(newColor);
        updateColorButton(m_pointColorButton, newColor);
    }
}

void PointViewDialog::onPointShapeChoice(wxCommandEvent& event)
{
    m_pointShape = event.GetSelection();
}

void PointViewDialog::onApply(wxCommandEvent& event)
{
    applySettings();
}

void PointViewDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void PointViewDialog::onOK(wxCommandEvent& event)
{
    applySettings();
    EndModal(wxID_OK);
}

void PointViewDialog::onReset(wxCommandEvent& event)
{
    resetToDefaults();
}

// Apply settings to viewer
void PointViewDialog::applySettings()
{
	if (m_occViewer) {
		auto displaySettings = m_occViewer->getDisplaySettings();
		displaySettings.showPointView = m_showPointView;
		displaySettings.showSolidWithPointView = m_showSolid;
		displaySettings.pointSize = m_pointSize;
		displaySettings.pointColor = m_pointColor;
		displaySettings.pointShape = m_pointShape;

		// Apply display mode based on point view settings
		if (m_showPointView) {
			if (m_showSolid) {
				displaySettings.displayMode = RenderingConfig::DisplayMode::Solid;
			} else {
				displaySettings.displayMode = RenderingConfig::DisplayMode::Points;
			}
		}

		m_occViewer->setDisplaySettings(displaySettings);
		
		// Force view refresh - use OCCViewer's method instead
		if (m_occViewer) {
			// Trigger geometry regeneration for all geometries
			auto geometries = m_occViewer->getAllGeometry();
			for (auto& geometry : geometries) {
				if (geometry) {
					// Use default mesh parameters
					MeshParameters defaultParams;
					geometry->forceCoinRepresentationRebuild(defaultParams);
				}
			}
		}
	}
}

// Reset to defaults
void PointViewDialog::resetToDefaults()
{
    m_showPointView = false;
    m_showSolid = true;
    m_pointSize = 3.0;
    m_pointColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB);
    m_pointShape = 0;

    // Update UI
    m_showPointViewCheckbox->SetValue(m_showPointView);
    m_showSolidCheckbox->SetValue(m_showSolid);
    m_pointSizeSlider->SetValue(static_cast<int>(m_pointSize * 10));
    m_pointSizeLabel->SetLabel(wxString::Format("%.1f", m_pointSize));
    updateColorButton(m_pointColorButton, quantityColorToWxColour(m_pointColor));
    m_pointShapeChoice->SetSelection(m_pointShape);

    updateControls();
}

// Helper methods
wxColour PointViewDialog::quantityColorToWxColour(const Quantity_Color& color)
{
    return wxColour(
        static_cast<unsigned char>(color.Red() * 255),
        static_cast<unsigned char>(color.Green() * 255),
        static_cast<unsigned char>(color.Blue() * 255)
    );
}

Quantity_Color PointViewDialog::wxColourToQuantityColor(const wxColour& color)
{
    return Quantity_Color(
        static_cast<double>(color.Red()) / 255.0,
        static_cast<double>(color.Green()) / 255.0,
        static_cast<double>(color.Blue()) / 255.0,
        Quantity_TOC_RGB
    );
}

void PointViewDialog::updateColorButton(wxButton* button, const wxColour& color)
{
    button->SetBackgroundColour(color);
    button->Refresh();
}
