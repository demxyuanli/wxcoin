#ifndef FLAT_WIDGETS_EXAMPLE_H
#define FLAT_WIDGETS_EXAMPLE_H

#include <wx/wx.h>
#include <wx/panel.h>
#include "widgets/FlatButton.h"
#include "widgets/FlatLineEdit.h"
#include "widgets/FlatComboBox.h"
#include "widgets/FlatCheckBox.h"
#include "widgets/FlatSlider.h"
#include "widgets/FlatProgressBar.h"
#include "widgets/FlatSwitch.h"
#include "widgets/FlatRadioButton.h"

class FlatWidgetsExample : public wxPanel
{
public:
	FlatWidgetsExample(wxWindow* parent);
	virtual ~FlatWidgetsExample() = default;

private:
	// Flat widgets
	FlatButton* m_primaryButton;
	FlatButton* m_secondaryButton;
	FlatButton* m_outlineButton;
	FlatButton* m_textButton;
	FlatButton* m_iconButton;

	FlatLineEdit* m_normalLineEdit;
	FlatLineEdit* m_searchLineEdit;
	FlatLineEdit* m_passwordLineEdit;
	FlatLineEdit* m_clearableLineEdit;

	FlatComboBox* m_normalComboBox;
	FlatComboBox* m_editableComboBox;
	FlatComboBox* m_searchComboBox;

	FlatCheckBox* m_normalCheckBox;
	FlatCheckBox* m_switchCheckBox;
	FlatCheckBox* m_radioCheckBox;

	FlatRadioButton* m_radioButton1;
	FlatRadioButton* m_radioButton2;
	FlatRadioButton* m_radioButton3;

	FlatSlider* m_normalSlider;
	FlatSlider* m_progressSlider;
	FlatSlider* m_verticalSlider;

	FlatProgressBar* m_normalProgressBar;
	FlatProgressBar* m_indeterminateProgressBar;
	FlatProgressBar* m_stripedProgressBar;

	FlatSwitch* m_normalSwitch;
	FlatSwitch* m_roundSwitch;
	FlatSwitch* m_squareSwitch;

	// Event handlers
	void OnPrimaryButtonClicked(wxCommandEvent& event);
	void OnSecondaryButtonClicked(wxCommandEvent& event);
	void OnOutlineButtonClicked(wxCommandEvent& event);
	void OnTextButtonClicked(wxCommandEvent& event);
	void OnIconButtonClicked(wxCommandEvent& event);

	void OnLineEditTextChanged(wxCommandEvent& event);
	void OnLineEditFocusGained(wxCommandEvent& event);
	void OnLineEditFocusLost(wxCommandEvent& event);

	void OnComboBoxSelectionChanged(wxCommandEvent& event);
	void OnComboBoxDropdownOpened(wxCommandEvent& event);
	void OnComboBoxDropdownClosed(wxCommandEvent& event);

	void OnCheckBoxClicked(wxCommandEvent& event);
	void OnCheckBoxStateChanged(wxCommandEvent& event);

	void OnRadioButtonClicked(wxCommandEvent& event);

	void OnSliderValueChanged(wxCommandEvent& event);
	void OnSliderThumbDragged(wxCommandEvent& event);

	void OnProgressBarValueChanged(wxCommandEvent& event);
	void OnProgressBarCompleted(wxCommandEvent& event);

	void OnSwitchToggled(wxCommandEvent& event);
	void OnSwitchStateChanged(wxCommandEvent& event);

	// Helper methods
	void CreateWidgets();
	void LayoutWidgets();
	void BindEvents();
	void ApplyTheme();

	DECLARE_EVENT_TABLE()
};

// Usage example:
/*
	// Create the example panel
	auto* examplePanel = new FlatWidgetsExample(this);

	// Example of using FlatButton
	auto* button = new FlatButton(this, wxID_ANY, "Click Me",
								 wxDefaultPosition, wxDefaultSize,
								 FlatButton::ButtonStyle::PRIMARY);
	button->Bind(wxEVT_FLAT_BUTTON_CLICKED, &MyFrame::OnButtonClicked, this);

	// Example of using FlatLineEdit
	auto* lineEdit = new FlatLineEdit(this, wxID_ANY, "Enter text...",
									 wxDefaultPosition, wxDefaultSize,
									 FlatLineEdit::LineEditStyle::NORMAL);
	lineEdit->SetPlaceholderText("Enter your text here");
	lineEdit->Bind(wxEVT_FLAT_LINE_EDIT_TEXT_CHANGED, &MyFrame::OnTextChanged, this);

	// Example of using FlatComboBox
	auto* comboBox = new FlatComboBox(this, wxID_ANY, "Select item",
									 wxDefaultPosition, wxDefaultSize,
									 FlatComboBox::ComboBoxStyle::NORMAL);
	comboBox->Append("Item 1");
	comboBox->Append("Item 2");
	comboBox->Append("Item 3");
	comboBox->Bind(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, &MyFrame::OnSelectionChanged, this);

	// Example of using FlatCheckBox
	auto* checkBox = new FlatCheckBox(this, wxID_ANY, "Check me",
									 wxDefaultPosition, wxDefaultSize,
									 FlatCheckBox::CheckBoxStyle::NORMAL);
	checkBox->Bind(wxEVT_FLAT_CHECK_BOX_CLICKED, &MyFrame::OnCheckBoxClicked, this);

	// Example of using FlatSlider
	auto* slider = new FlatSlider(this, wxID_ANY, 50, 0, 100,
								 wxDefaultPosition, wxDefaultSize,
								 FlatSlider::SliderStyle::NORMAL);
	slider->SetShowValue(true);
	slider->SetShowTicks(true);
	slider->Bind(wxEVT_FLAT_SLIDER_VALUE_CHANGED, &MyFrame::OnSliderChanged, this);

	// Example of using FlatProgressBar
	auto* progressBar = new FlatProgressBar(this, wxID_ANY, 75, 0, 100,
										   wxDefaultPosition, wxDefaultSize,
										   FlatProgressBar::ProgressBarStyle::NORMAL);
	progressBar->SetShowPercentage(true);
	progressBar->SetShowValue(true);
	progressBar->Bind(wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, &MyFrame::OnProgressChanged, this);

	// Example of using FlatSwitch
	auto* switchControl = new FlatSwitch(this, wxID_ANY, false,
										wxDefaultPosition, wxDefaultSize,
										FlatSwitch::SwitchStyle::NORMAL);
	switchControl->SetLabel("Enable feature");
	switchControl->Bind(wxEVT_FLAT_SWITCH_TOGGLED, &MyFrame::OnSwitchToggled, this);
*/

#endif // FLAT_WIDGETS_EXAMPLE_H
