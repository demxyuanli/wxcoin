#include "SliceParamDialog.h"
#include "OCCViewer.h"
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <Inventor/SbLinear.h>

wxBEGIN_EVENT_TABLE(SliceParamDialog, wxFrame)
	EVT_SLIDER(wxID_ANY, SliceParamDialog::OnOpacitySliderChanged)
	EVT_CHOICE(wxID_ANY, SliceParamDialog::OnDirectionChanged)
	EVT_SPINCTRLDOUBLE(wxID_ANY, SliceParamDialog::OnOffsetChanged)
	EVT_COLOURPICKER_CHANGED(wxID_ANY, SliceParamDialog::OnColorChanged)
	EVT_CHECKBOX(wxID_ANY, SliceParamDialog::OnContoursChanged)
	EVT_BUTTON(wxID_DOWN, SliceParamDialog::OnToggleSizeButton)
	EVT_BUTTON(wxID_FORWARD, SliceParamDialog::OnMouseModeToggle)
wxEND_EVENT_TABLE()

SliceParamDialog::SliceParamDialog(wxWindow* parent, OCCViewer* viewer)
	: wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxSize(240, 360),
		wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR | wxNO_BORDER)
	, m_viewer(viewer)
{
	// Set 30% transparency for the entire dialog
	SetTransparent(static_cast<wxByte>(255 * 0.7)); // 70% opacity = 30% transparency

	// Create content panel with background
	m_contentPanel = new wxPanel(this);
	m_contentPanel->SetBackgroundColour(wxColour(45, 45, 48)); // Dark background
	
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Add title bar manually
	wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* titleText = new wxStaticText(m_contentPanel, wxID_ANY, "Slice Parameters");
	wxFont titleFont = titleText->GetFont();
	titleFont.SetPointSize(9);
	titleFont.SetWeight(wxFONTWEIGHT_BOLD);
	titleText->SetFont(titleFont);
	titleText->SetForegroundColour(wxColour(220, 220, 220));
	
	titleSizer->Add(titleText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
	
	// Toggle size button (minimize/maximize)
	m_toggleSizeBtn = new wxButton(m_contentPanel, wxID_DOWN, "-", wxDefaultPosition, wxSize(24, 24));
	m_toggleSizeBtn->SetBackgroundColour(wxColour(60, 60, 65));
	m_toggleSizeBtn->SetForegroundColour(wxColour(220, 220, 220));
	m_toggleSizeBtn->SetToolTip("Minimize/Maximize");
	titleSizer->Add(m_toggleSizeBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	
	mainSizer->Add(titleSizer, 0, wxEXPAND | wxALL, 4);
	
	// Separator line
	wxPanel* separator = new wxPanel(m_contentPanel, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
	separator->SetBackgroundColour(wxColour(80, 80, 85));
	mainSizer->Add(separator, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
	
	// Main content panel (can be hidden when minimized)
	m_mainContent = new wxPanel(m_contentPanel);
	m_mainContent->SetBackgroundColour(wxColour(45, 45, 48));
	wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

	// Appearance section
	wxStaticBox* appearanceBox = new wxStaticBox(m_mainContent, wxID_ANY, "Appearance");
	appearanceBox->SetForegroundColour(wxColour(200, 200, 200));
	wxStaticBoxSizer* appearanceSizer = new wxStaticBoxSizer(appearanceBox, wxVERTICAL);

	// Color row
	wxBoxSizer* colorSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* colorLabel = new wxStaticText(m_mainContent, wxID_ANY, "Color:");
	colorLabel->SetForegroundColour(wxColour(200, 200, 200));
	colorSizer->Add(colorLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	m_colorPicker = new wxColourPickerCtrl(m_mainContent, wxID_ANY, wxColour(178, 242, 178)); // Light green
	colorSizer->Add(m_colorPicker, 1, wxEXPAND);
	appearanceSizer->Add(colorSizer, 0, wxEXPAND | wxALL, 5);

	// Opacity row
	wxBoxSizer* opacitySizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* opacityLabel = new wxStaticText(m_mainContent, wxID_ANY, "Opacity:");
	opacityLabel->SetForegroundColour(wxColour(200, 200, 200));
	opacitySizer->Add(opacityLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	
	m_opacitySlider = new wxSlider(m_mainContent, wxID_ANY, 15, 0, 100, wxDefaultPosition, wxSize(-1, -1), wxSL_HORIZONTAL);
	opacitySizer->Add(m_opacitySlider, 1, wxEXPAND | wxRIGHT, 4);
	
	m_opacityValue = new wxStaticText(m_mainContent, wxID_ANY, "15%", wxDefaultPosition, wxSize(32, -1), wxALIGN_RIGHT);
	m_opacityValue->SetForegroundColour(wxColour(200, 200, 200));
	opacitySizer->Add(m_opacityValue, 0, wxALIGN_CENTER_VERTICAL);

	appearanceSizer->Add(opacitySizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

	// Show contours checkbox
	m_showContours = new wxCheckBox(m_mainContent, wxID_ANY, "Show contours");
	m_showContours->SetForegroundColour(wxColour(200, 200, 200));
	appearanceSizer->Add(m_showContours, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

	contentSizer->Add(appearanceSizer, 0, wxEXPAND | wxALL, 6);

	// Preset section
	wxStaticBox* presetBox = new wxStaticBox(m_mainContent, wxID_ANY, "Slice Preset");
	presetBox->SetForegroundColour(wxColour(200, 200, 200));
	wxStaticBoxSizer* presetSizer = new wxStaticBoxSizer(presetBox, wxVERTICAL);

	// Preset direction
	wxStaticText* presetLabel = new wxStaticText(m_mainContent, wxID_ANY, "Preset:");
	presetLabel->SetForegroundColour(wxColour(200, 200, 200));
	presetSizer->Add(presetLabel, 0, wxALL, 5);

	m_direction = new wxChoice(m_mainContent, wxID_ANY);
	m_direction->Append("XY Plane (Top View)");
	m_direction->Append("XZ Plane (Front View)");
	m_direction->Append("YZ Plane (Right View)");
	m_direction->Append("Diagonal XY");
	m_direction->Append("Diagonal XZ");
	m_direction->Append("Diagonal YZ");
	m_direction->Append("Isometric (1,1,1)");
	m_direction->Append("Reverse Isometric (-1,-1,1)");
	m_direction->Append("Custom X Axis");
	m_direction->Append("Custom Y Axis");
	m_direction->Append("Custom Z Axis");
	m_direction->SetSelection(0); // Default to XY plane
	presetSizer->Add(m_direction, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);

	contentSizer->Add(presetSizer, 0, wxEXPAND | wxALL, 6);

	// Position section
	wxStaticBox* positionBox = new wxStaticBox(m_mainContent, wxID_ANY, "Position");
	positionBox->SetForegroundColour(wxColour(200, 200, 200));
	wxStaticBoxSizer* positionSizer = new wxStaticBoxSizer(positionBox, wxVERTICAL);

	// Offset row
	wxBoxSizer* offsetSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* offsetLabel = new wxStaticText(m_mainContent, wxID_ANY, "Offset:");
	offsetLabel->SetForegroundColour(wxColour(200, 200, 200));
	offsetSizer->Add(offsetLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	m_offsetCtrl = new wxSpinCtrlDouble(m_mainContent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize);
	m_offsetCtrl->SetRange(-1000.0, 1000.0);
	m_offsetCtrl->SetIncrement(1.0);
	m_offsetCtrl->SetValue(0.0);
	m_offsetCtrl->SetDigits(2);
	offsetSizer->Add(m_offsetCtrl, 1, wxEXPAND);

	positionSizer->Add(offsetSizer, 0, wxEXPAND | wxALL, 5);

	contentSizer->Add(positionSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	// Mouse mode control
	wxStaticBox* modeBox = new wxStaticBox(m_mainContent, wxID_ANY, "Mouse Mode");
	modeBox->SetForegroundColour(wxColour(200, 200, 200));
	wxStaticBoxSizer* modeSizer = new wxStaticBoxSizer(modeBox, wxVERTICAL);

	m_mouseModeBtn = new wxButton(m_mainContent, wxID_FORWARD, "Mode: Drag Slice");
	m_mouseModeBtn->SetBackgroundColour(wxColour(120, 60, 60));  // Red tone for drag mode
	m_mouseModeBtn->SetForegroundColour(wxColour(220, 220, 220));
	m_mouseModeBtn->SetToolTip("Click to toggle between drag slice and rotate camera modes");
	modeSizer->Add(m_mouseModeBtn, 0, wxEXPAND | wxALL, 5);

	contentSizer->Add(modeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);

	m_mainContent->SetSizer(contentSizer);
	mainSizer->Add(m_mainContent, 1, wxEXPAND);

	m_contentPanel->SetSizer(mainSizer);
	
	// Set frame sizer
	wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
	frameSizer->Add(m_contentPanel, 1, wxEXPAND);
	SetSizer(frameSizer);
	
	Layout();

	// Set size (increased for mouse mode section)
	SetClientSize(wxSize(240, 480));
	SetMinSize(wxSize(240, 480));
	SetMaxSize(wxSize(240, 480));

	// Bind to parent resize event
	if (GetParent()) {
		GetParent()->Bind(wxEVT_SIZE, &SliceParamDialog::OnParentResize, this);
	}

	// Set initial drag mode to enabled
	if (m_viewer) {
		m_viewer->setSliceDragEnabled(true);
	}
}

void SliceParamDialog::ShowAtCanvasTopLeft() {
	UpdatePosition();
	Show(true);
}

void SliceParamDialog::UpdatePosition() {
	// Position the dialog at the top-left of the parent canvas with 4px margin
	if (GetParent()) {
		wxPoint canvasScreenPos = GetParent()->GetScreenPosition();
		wxPoint dialogPos(canvasScreenPos.x + 4, canvasScreenPos.y + 4);
		SetPosition(dialogPos);
	}
}

void SliceParamDialog::OnParentResize(wxSizeEvent& event) {
	// Reposition the dialog when parent canvas resizes
	UpdatePosition();
	event.Skip(); // Let the parent handle the event too
}

void SliceParamDialog::OnOpacitySliderChanged(wxCommandEvent& event) {
	if (m_opacitySlider && m_opacityValue) {
		int value = m_opacitySlider->GetValue();
		m_opacityValue->SetLabel(wxString::Format("%d%%", value));
		ApplyChanges();
	}
}

void SliceParamDialog::OnDirectionChanged(wxCommandEvent& event) {
	ApplyChanges();
}

void SliceParamDialog::OnOffsetChanged(wxSpinDoubleEvent& event) {
	ApplyChanges();
}

void SliceParamDialog::OnColorChanged(wxColourPickerEvent& event) {
	ApplyChanges();
}

void SliceParamDialog::OnContoursChanged(wxCommandEvent& event) {
	ApplyChanges();
}

void SliceParamDialog::OnToggleSizeButton(wxCommandEvent& event) {
	if (m_isMinimized) {
		// Restore to full size
		SetClientSize(wxSize(240, 480));
		m_mainContent->Show(true);
		m_toggleSizeBtn->SetLabel("-");
		m_isMinimized = false;
	} else {
		// Minimize to title bar only
		m_mainContent->Show(false);
		
		// Get title bar height
		wxSize titleBarSize = GetSize();
		titleBarSize.SetHeight(32); // Only title bar height
		SetClientSize(titleBarSize);
		
		m_toggleSizeBtn->SetLabel("+");
		m_isMinimized = true;
	}
	
	Layout();
	Refresh();
}

void SliceParamDialog::OnMouseModeToggle(wxCommandEvent& event) {
	m_isDragMode = !m_isDragMode;
	
	// Apply mode change to viewer
	if (m_viewer) {
		m_viewer->setSliceDragEnabled(m_isDragMode);
	}
	
	if (m_mouseModeBtn) {
		if (m_isDragMode) {
			m_mouseModeBtn->SetLabel("Mode: Drag Slice");
			m_mouseModeBtn->SetBackgroundColour(wxColour(120, 60, 60)); // Red tone
		} else {
			m_mouseModeBtn->SetLabel("Mode: Rotate Camera");
			m_mouseModeBtn->SetBackgroundColour(wxColour(60, 120, 60)); // Green tone
		}
		m_mouseModeBtn->Refresh();
	}
}

bool SliceParamDialog::isDragMode() const {
	return m_isDragMode;
}

void SliceParamDialog::ApplyChanges() {
	if (!m_viewer) return;

	// Auto-apply parameters to viewer
	wxColour color = getPlaneColor();
	SbVec3f newColor(
		color.Red() / 255.0f,
		color.Green() / 255.0f,
		color.Blue() / 255.0f
	);
	m_viewer->setSlicePlaneColor(newColor);
	
	// Apply opacity (transparency)
	double opacity = getPlaneOpacity();
	m_viewer->setSlicePlaneOpacity(static_cast<float>(opacity));
	
	m_viewer->setShowSectionContours(getShowSectionContours());
	
	// Update slice plane based on preset direction
	int preset = getSliceDirection();
	float offset = static_cast<float>(getSliceOffset());
	SbVec3f normal(0, 0, 1);
	
	switch (preset) {
		case 0: normal = SbVec3f(0, 0, 1); break;  // XY Plane (Top View) - Z normal
		case 1: normal = SbVec3f(0, 1, 0); break;  // XZ Plane (Front View) - Y normal
		case 2: normal = SbVec3f(1, 0, 0); break;  // YZ Plane (Right View) - X normal
		case 3: {  // Diagonal XY
			SbVec3f diag(1, 1, 0);
			diag.normalize();
			normal = diag;
			break;
		}
		case 4: {  // Diagonal XZ
			SbVec3f diag(1, 0, 1);
			diag.normalize();
			normal = diag;
			break;
		}
		case 5: {  // Diagonal YZ
			SbVec3f diag(0, 1, 1);
			diag.normalize();
			normal = diag;
			break;
		}
		case 6: {  // Isometric (1,1,1)
			SbVec3f iso(1, 1, 1);
			iso.normalize();
			normal = iso;
			break;
		}
		case 7: {  // Reverse Isometric (-1,-1,1)
			SbVec3f iso(-1, -1, 1);
			iso.normalize();
			normal = iso;
			break;
		}
		case 8: normal = SbVec3f(1, 0, 0); break;  // Custom X Axis
		case 9: normal = SbVec3f(0, 1, 0); break;  // Custom Y Axis
		case 10: normal = SbVec3f(0, 0, 1); break; // Custom Z Axis
		default: break;
	}
	
	m_viewer->setSlicePlane(normal, offset);
}

wxColour SliceParamDialog::getPlaneColor() const {
	return m_colorPicker ? m_colorPicker->GetColour() : wxColour(178, 242, 178); // Light green
}

double SliceParamDialog::getPlaneOpacity() const {
	return m_opacitySlider ? (m_opacitySlider->GetValue() / 100.0) : 0.15; // Default 15% opacity = 85% transparency
}

bool SliceParamDialog::getShowSectionContours() const {
	return m_showContours && m_showContours->GetValue();
}

int SliceParamDialog::getSliceDirection() const {
	return m_direction ? m_direction->GetSelection() : 2;
}

double SliceParamDialog::getSliceOffset() const {
	return m_offsetCtrl ? m_offsetCtrl->GetValue() : 0.0;
}

void SliceParamDialog::setPlaneColor(const wxColour& color) {
	if (m_colorPicker) {
		m_colorPicker->SetColour(color);
	}
}

void SliceParamDialog::setPlaneOpacity(double opacity) {
	if (m_opacitySlider && m_opacityValue) {
		int value = static_cast<int>(opacity * 100);
		m_opacitySlider->SetValue(value);
		m_opacityValue->SetLabel(wxString::Format("%d%%", value));
	}
}

void SliceParamDialog::setShowSectionContours(bool show) {
	if (m_showContours) {
		m_showContours->SetValue(show);
	}
}

void SliceParamDialog::setSliceDirection(int direction) {
	if (m_direction && direction >= 0 && direction < static_cast<int>(m_direction->GetCount())) {
		m_direction->SetSelection(direction);
	}
}

void SliceParamDialog::setSliceOffset(double offset) {
	if (m_offsetCtrl) {
		m_offsetCtrl->SetValue(offset);
	}
}

