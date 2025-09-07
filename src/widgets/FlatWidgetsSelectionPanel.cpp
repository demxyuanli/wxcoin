#include "widgets/FlatWidgetsSelectionPanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/scrolwin.h>
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(FlatWidgetsSelectionPanel, wxPanel)
EVT_CHECKBOX(wxID_ANY, FlatWidgetsSelectionPanel::OnCheckBoxClicked)
EVT_COMMAND(wxID_ANY, wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, FlatWidgetsSelectionPanel::OnRadioButtonClicked)
EVT_COMMAND(wxID_ANY, wxEVT_FLAT_SWITCH_TOGGLED, FlatWidgetsSelectionPanel::OnSwitchToggled)
EVT_SLIDER(wxID_ANY, FlatWidgetsSelectionPanel::OnSliderValueChanged)
wxEND_EVENT_TABLE()

FlatWidgetsSelectionPanel::FlatWidgetsSelectionPanel(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
{
	SetBackgroundColour(wxColour(250, 250, 250));
	
	CreateControls();
	LayoutPanel();
	BindEvents();
}

void FlatWidgetsSelectionPanel::CreateControls()
{
	// Create scrolled window first
	m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
	m_scrolledWindow->SetScrollRate(10, 10);
	m_scrolledWindow->SetBackgroundColour(wxColour(250, 250, 250));
	
	// Create FlatCheckBox examples on the scrolled window
	m_normalCheckBox = new FlatCheckBox(m_scrolledWindow, wxID_ANY, "Normal CheckBox",
		wxDefaultPosition, wxDefaultSize,
		FlatCheckBox::CheckBoxStyle::DEFAULT_STYLE);

	m_switchCheckBox = new FlatCheckBox(m_scrolledWindow, wxID_ANY, "Switch CheckBox",
		wxDefaultPosition, wxDefaultSize,
		FlatCheckBox::CheckBoxStyle::SWITCH);

	m_radioCheckBox = new FlatCheckBox(m_scrolledWindow, wxID_ANY, "Radio CheckBox",
		wxDefaultPosition, wxDefaultSize,
		FlatCheckBox::CheckBoxStyle::RADIO);

	// Create FlatRadioButton examples on the scrolled window
	m_radioButton1 = new FlatRadioButton(m_scrolledWindow, wxID_ANY, "Option 1");
	m_radioButton2 = new FlatRadioButton(m_scrolledWindow, wxID_ANY, "Option 2");
	m_radioButton3 = new FlatRadioButton(m_scrolledWindow, wxID_ANY, "Option 3");
	m_radioButton1->SetValue(true);

	// Create FlatSwitch examples on the scrolled window
	m_normalSwitch = new FlatSwitch(m_scrolledWindow, wxID_ANY, false,
		wxDefaultPosition, wxDefaultSize,
		FlatSwitch::SwitchStyle::DEFAULT_STYLE);
	m_normalSwitch->SetLabel("Normal Switch");

	m_roundSwitch = new FlatSwitch(m_scrolledWindow, wxID_ANY, true,
		wxDefaultPosition, wxDefaultSize,
		FlatSwitch::SwitchStyle::ROUND);
	m_roundSwitch->SetLabel("Round Switch");

	m_squareSwitch = new FlatSwitch(m_scrolledWindow, wxID_ANY, false,
		wxDefaultPosition, wxDefaultSize,
		FlatSwitch::SwitchStyle::SQUARE);
	m_squareSwitch->SetLabel("Square Switch");

	// Create FlatSlider examples on the scrolled window
	m_normalSlider = new FlatSlider(m_scrolledWindow, wxID_ANY, 50, 0, 100,
		wxDefaultPosition, wxSize(200, -1),
		FlatSlider::SliderStyle::NORMAL);
	m_normalSlider->SetShowValue(true);
	m_normalSlider->SetShowTicks(true);

	m_progressSlider = new FlatSlider(m_scrolledWindow, wxID_ANY, 75, 0, 100,
		wxDefaultPosition, wxSize(200, -1),
		FlatSlider::SliderStyle::PROGRESS);
	m_progressSlider->SetShowValue(true);

	m_verticalSlider = new FlatSlider(m_scrolledWindow, wxID_ANY, 30, 0, 100,
		wxDefaultPosition, wxSize(-1, 150),
		FlatSlider::SliderStyle::VERTICAL);
	m_verticalSlider->SetShowValue(true);
}

void FlatWidgetsSelectionPanel::LayoutPanel()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);

	// CheckBox section
	wxStaticBoxSizer* checkBoxSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "FlatCheckBox Examples");
	wxBoxSizer* checkBoxRow = new wxBoxSizer(wxHORIZONTAL);

	checkBoxRow->Add(m_normalCheckBox, 0, wxALL, 5);
	checkBoxRow->Add(m_switchCheckBox, 0, wxALL, 5);
	checkBoxRow->Add(m_radioCheckBox, 0, wxALL, 5);

	checkBoxSizer->Add(checkBoxRow, 0, wxEXPAND);

	// RadioButton section
	wxStaticBoxSizer* radioButtonSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "FlatRadioButton Examples");
	wxBoxSizer* radioButtonRow = new wxBoxSizer(wxHORIZONTAL);

	radioButtonRow->Add(m_radioButton1, 0, wxALL, 5);
	radioButtonRow->Add(m_radioButton2, 0, wxALL, 5);
	radioButtonRow->Add(m_radioButton3, 0, wxALL, 5);

	radioButtonSizer->Add(radioButtonRow, 0, wxEXPAND);

	// Switch section
	wxStaticBoxSizer* switchSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "FlatSwitch Examples");
	wxBoxSizer* switchRow = new wxBoxSizer(wxHORIZONTAL);

	switchRow->Add(m_normalSwitch, 0, wxALL, 5);
	switchRow->Add(m_roundSwitch, 0, wxALL, 5);
	switchRow->Add(m_squareSwitch, 0, wxALL, 5);

	switchSizer->Add(switchRow, 0, wxEXPAND);

	// Slider section
	wxStaticBoxSizer* sliderSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "FlatSlider Examples");
	wxBoxSizer* sliderRow = new wxBoxSizer(wxHORIZONTAL);

	wxBoxSizer* horizontalSliders = new wxBoxSizer(wxVERTICAL);
	horizontalSliders->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Normal Slider:"), 0, wxBOTTOM, 2);
	horizontalSliders->Add(m_normalSlider, 0, wxBOTTOM, 10);
	horizontalSliders->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Progress Slider:"), 0, wxBOTTOM, 2);
	horizontalSliders->Add(m_progressSlider, 0, wxBOTTOM, 10);

	wxBoxSizer* verticalSliderContainer = new wxBoxSizer(wxVERTICAL);
	verticalSliderContainer->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Vertical Slider:"), 0, wxBOTTOM, 2);
	verticalSliderContainer->Add(m_verticalSlider, 0, wxALIGN_CENTER_HORIZONTAL);

	sliderRow->Add(horizontalSliders, 1, wxEXPAND);
	sliderRow->Add(verticalSliderContainer, 0, wxLEFT, 20);

	sliderSizer->Add(sliderRow, 0, wxEXPAND);

	scrolledSizer->Add(checkBoxSizer, 0, wxEXPAND | wxALL, 10);
	scrolledSizer->Add(radioButtonSizer, 0, wxEXPAND | wxALL, 10);
	scrolledSizer->Add(switchSizer, 0, wxEXPAND | wxALL, 10);
	scrolledSizer->Add(sliderSizer, 0, wxEXPAND | wxALL, 10);

	m_scrolledWindow->SetSizer(scrolledSizer);
	mainSizer->Add(m_scrolledWindow, 1, wxEXPAND);

	SetSizer(mainSizer);
}

void FlatWidgetsSelectionPanel::BindEvents()
{
	m_normalCheckBox->Bind(wxEVT_CHECKBOX, &FlatWidgetsSelectionPanel::OnCheckBoxClicked, this);
	m_switchCheckBox->Bind(wxEVT_CHECKBOX, &FlatWidgetsSelectionPanel::OnCheckBoxClicked, this);
	m_radioCheckBox->Bind(wxEVT_CHECKBOX, &FlatWidgetsSelectionPanel::OnCheckBoxClicked, this);

	m_radioButton1->Bind(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, &FlatWidgetsSelectionPanel::OnRadioButtonClicked, this);
	m_radioButton2->Bind(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, &FlatWidgetsSelectionPanel::OnRadioButtonClicked, this);
	m_radioButton3->Bind(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, &FlatWidgetsSelectionPanel::OnRadioButtonClicked, this);

	m_normalSwitch->Bind(wxEVT_FLAT_SWITCH_TOGGLED, &FlatWidgetsSelectionPanel::OnSwitchToggled, this);
	m_roundSwitch->Bind(wxEVT_FLAT_SWITCH_TOGGLED, &FlatWidgetsSelectionPanel::OnSwitchToggled, this);
	m_squareSwitch->Bind(wxEVT_FLAT_SWITCH_TOGGLED, &FlatWidgetsSelectionPanel::OnSwitchToggled, this);

	m_normalSlider->Bind(wxEVT_SLIDER, &FlatWidgetsSelectionPanel::OnSliderValueChanged, this);
	m_progressSlider->Bind(wxEVT_SLIDER, &FlatWidgetsSelectionPanel::OnSliderValueChanged, this);
	m_verticalSlider->Bind(wxEVT_SLIDER, &FlatWidgetsSelectionPanel::OnSliderValueChanged, this);
}

void FlatWidgetsSelectionPanel::OnCheckBoxClicked(wxCommandEvent& event)
{
	wxString label = "";
	if (event.GetEventObject() == m_normalCheckBox) {
		label = "Normal CheckBox";
	}
	else if (event.GetEventObject() == m_switchCheckBox) {
		label = "Switch CheckBox";
	}
	else if (event.GetEventObject() == m_radioCheckBox) {
		label = "Radio CheckBox";
	}

	bool checked = event.IsChecked();
	wxMessageBox(label + " " + (checked ? "checked" : "unchecked"), "FlatWidgetsSelectionPanel", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsSelectionPanel::OnRadioButtonClicked(wxCommandEvent& event)
{
	wxString label = "";
	if (event.GetEventObject() == m_radioButton1) {
		label = "Radio Button 1";
	}
	else if (event.GetEventObject() == m_radioButton2) {
		label = "Radio Button 2";
	}
	else if (event.GetEventObject() == m_radioButton3) {
		label = "Radio Button 3";
	}

	bool checked = event.GetInt() != 0;
	if (checked)
		wxMessageBox(label + " selected", "FlatWidgetsSelectionPanel", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsSelectionPanel::OnSwitchToggled(wxCommandEvent& event)
{
	bool toggled = event.GetInt() != 0;
	wxString label = "";
	if (event.GetEventObject() == m_normalSwitch) {
		label = "Normal Switch";
	}
	else if (event.GetEventObject() == m_roundSwitch) {
		label = "Round Switch";
	}
	else if (event.GetEventObject() == m_squareSwitch) {
		label = "Square Switch";
	}

	wxMessageBox(label + " " + (toggled ? "toggled ON" : "toggled OFF"), "FlatWidgetsSelectionPanel", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsSelectionPanel::OnSliderValueChanged(wxCommandEvent& event)
{
	int value = event.GetInt();
	wxString label = "";
	if (event.GetEventObject() == m_normalSlider) {
		label = "Normal Slider";
	}
	else if (event.GetEventObject() == m_progressSlider) {
		label = "Progress Slider";
	}
	else if (event.GetEventObject() == m_verticalSlider) {
		label = "Vertical Slider";
	}

	// You can add logging or other handling here
}
