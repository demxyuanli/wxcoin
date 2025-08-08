#include "widgets/FlatWidgetsExample.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/artprov.h>

BEGIN_EVENT_TABLE(FlatWidgetsExample, wxPanel)
EVT_BUTTON(wxID_ANY, FlatWidgetsExample::OnPrimaryButtonClicked)
EVT_TEXT(wxID_ANY, FlatWidgetsExample::OnLineEditTextChanged)
EVT_COMBOBOX(wxID_ANY, FlatWidgetsExample::OnComboBoxSelectionChanged)
EVT_CHECKBOX(wxID_ANY, FlatWidgetsExample::OnCheckBoxClicked)
EVT_SLIDER(wxID_ANY, FlatWidgetsExample::OnSliderValueChanged)
EVT_COMMAND(wxID_ANY, wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, FlatWidgetsExample::OnProgressBarValueChanged)
EVT_COMMAND(wxID_ANY, wxEVT_FLAT_SWITCH_TOGGLED, FlatWidgetsExample::OnSwitchToggled)
END_EVENT_TABLE()

FlatWidgetsExample::FlatWidgetsExample(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    wxLogMessage("FlatWidgetsExample constructor started");

    SetBackgroundColour(wxColour(240, 240, 240));

    LayoutWidgets();
    wxLogMessage("FlatWidgetsExample layout completed");

    BindEvents();
    wxLogMessage("FlatWidgetsExample events bound");

    ApplyTheme();
    wxLogMessage("FlatWidgetsExample theme applied");

    wxLogMessage("FlatWidgetsExample constructor completed");
}

void FlatWidgetsExample::CreateWidgets()
{
    // 这个方法将在LayoutWidgets中调用，传入正确的父窗口
}

void FlatWidgetsExample::LayoutWidgets()
{
    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    scrolledWindow->SetScrollRate(10, 10);
    scrolledWindow->SetBackgroundColour(wxColour(250, 250, 250));

    // Create FlatButton examples
    m_primaryButton = new FlatButton(scrolledWindow, wxID_ANY, "Primary Button",
        wxDefaultPosition, wxDefaultSize,
        FlatButton::ButtonStyle::PRIMARY);
    m_secondaryButton = new FlatButton(scrolledWindow, wxID_ANY, "Secondary Button",
        wxDefaultPosition, wxDefaultSize,
        FlatButton::ButtonStyle::SECONDARY);
    m_outlineButton = new FlatButton(scrolledWindow, wxID_ANY, "Outline Button",
        wxDefaultPosition, wxDefaultSize,
        FlatButton::ButtonStyle::OUTLINE);
    m_textButton = new FlatButton(scrolledWindow, wxID_ANY, "Text Button",
        wxDefaultPosition, wxDefaultSize,
        FlatButton::ButtonStyle::TEXT);
    m_iconButton = new FlatButton(scrolledWindow, wxID_ANY, "",
        wxDefaultPosition, wxSize(40, 40),
        FlatButton::ButtonStyle::ICON_ONLY);
    m_iconButton->SetIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_BUTTON, wxSize(16, 16)));

    // Create FlatLineEdit examples
    m_normalLineEdit = new FlatLineEdit(scrolledWindow, wxID_ANY, "Normal input",
        wxDefaultPosition, wxSize(200, -1),
        FlatLineEdit::LineEditStyle::DEFAULT_STYLE);
    m_normalLineEdit->SetPlaceholderText("Enter text here...");

    m_searchLineEdit = new FlatLineEdit(scrolledWindow, wxID_ANY, "",
        wxDefaultPosition, wxSize(200, -1),
        FlatLineEdit::LineEditStyle::SEARCH);
    m_searchLineEdit->SetPlaceholderText("Search...");

    m_passwordLineEdit = new FlatLineEdit(scrolledWindow, wxID_ANY, "",
        wxDefaultPosition, wxSize(200, -1),
        FlatLineEdit::LineEditStyle::PASSWORD);
    m_passwordLineEdit->SetPlaceholderText("Enter password...");

    m_clearableLineEdit = new FlatLineEdit(scrolledWindow, wxID_ANY, "Clearable text",
        wxDefaultPosition, wxSize(200, -1),
        FlatLineEdit::LineEditStyle::CLEARABLE);
    m_clearableLineEdit->SetPlaceholderText("Type and clear...");

    // Create FlatComboBox examples
    m_normalComboBox = new FlatComboBox(scrolledWindow, wxID_ANY, "Select item",
        wxDefaultPosition, wxSize(200, -1),
        FlatComboBox::ComboBoxStyle::DEFAULT_STYLE);
    m_normalComboBox->Append("Item 1");
    m_normalComboBox->Append("Item 2");
    m_normalComboBox->Append("Item 3");
    m_normalComboBox->Append("Item 4");

    m_editableComboBox = new FlatComboBox(scrolledWindow, wxID_ANY, "Editable combo",
        wxDefaultPosition, wxSize(200, -1),
        FlatComboBox::ComboBoxStyle::EDITABLE);
    m_editableComboBox->Append("Option 1");
    m_editableComboBox->Append("Option 2");
    m_editableComboBox->Append("Option 3");

    m_searchComboBox = new FlatComboBox(scrolledWindow, wxID_ANY, "Search combo",
        wxDefaultPosition, wxSize(200, -1),
        FlatComboBox::ComboBoxStyle::SEARCH);
    m_searchComboBox->Append("Search Item 1");
    m_searchComboBox->Append("Search Item 2");
    m_searchComboBox->Append("Search Item 3");

    // Create FlatCheckBox examples
    m_normalCheckBox = new FlatCheckBox(scrolledWindow, wxID_ANY, "Normal CheckBox",
        wxDefaultPosition, wxDefaultSize,
        FlatCheckBox::CheckBoxStyle::DEFAULT_STYLE);

    m_switchCheckBox = new FlatCheckBox(scrolledWindow, wxID_ANY, "Switch CheckBox",
        wxDefaultPosition, wxDefaultSize,
        FlatCheckBox::CheckBoxStyle::SWITCH);

    m_radioCheckBox = new FlatCheckBox(scrolledWindow, wxID_ANY, "Radio CheckBox",
        wxDefaultPosition, wxDefaultSize,
        FlatCheckBox::CheckBoxStyle::RADIO);

    // Create FlatRadioButton examples
    m_radioButton1 = new FlatRadioButton(scrolledWindow, wxID_ANY, "Option 1");
    m_radioButton2 = new FlatRadioButton(scrolledWindow, wxID_ANY, "Option 2");
    m_radioButton3 = new FlatRadioButton(scrolledWindow, wxID_ANY, "Option 3");
    m_radioButton1->SetValue(true);

    // Create FlatSlider examples
    m_normalSlider = new FlatSlider(scrolledWindow, wxID_ANY, 50, 0, 100,
        wxDefaultPosition, wxSize(200, -1),
        FlatSlider::SliderStyle::NORMAL);
    m_normalSlider->SetShowValue(true);
    m_normalSlider->SetShowTicks(true);

    m_progressSlider = new FlatSlider(scrolledWindow, wxID_ANY, 75, 0, 100,
        wxDefaultPosition, wxSize(200, -1),
        FlatSlider::SliderStyle::PROGRESS);
    m_progressSlider->SetShowValue(true);

    m_verticalSlider = new FlatSlider(scrolledWindow, wxID_ANY, 30, 0, 100,
        wxDefaultPosition, wxSize(-1, 150),
        FlatSlider::SliderStyle::VERTICAL);
    m_verticalSlider->SetShowValue(true);

    // Create FlatProgressBar examples
    m_normalProgressBar = new FlatProgressBar(scrolledWindow, wxID_ANY, 60, 0, 100,
        wxDefaultPosition, wxSize(200, 20),
        FlatProgressBar::ProgressBarStyle::DEFAULT_STYLE);
    m_normalProgressBar->SetShowPercentage(true);
    m_normalProgressBar->SetShowValue(true);

    m_indeterminateProgressBar = new FlatProgressBar(scrolledWindow, wxID_ANY, 0, 0, 100,
        wxDefaultPosition, wxSize(200, 20),
        FlatProgressBar::ProgressBarStyle::INDETERMINATE);
    m_indeterminateProgressBar->SetShowLabel(true);
    m_indeterminateProgressBar->SetLabel("Loading...");

    m_stripedProgressBar = new FlatProgressBar(scrolledWindow, wxID_ANY, 80, 0, 100,
        wxDefaultPosition, wxSize(200, 20),
        FlatProgressBar::ProgressBarStyle::STRIPED);
    m_stripedProgressBar->SetShowPercentage(true);

    // Create FlatSwitch examples
    m_normalSwitch = new FlatSwitch(scrolledWindow, wxID_ANY, false,
        wxDefaultPosition, wxDefaultSize,
        FlatSwitch::SwitchStyle::DEFAULT_STYLE);
    m_normalSwitch->SetLabel("Normal Switch");

    m_roundSwitch = new FlatSwitch(scrolledWindow, wxID_ANY, true,
        wxDefaultPosition, wxDefaultSize,
        FlatSwitch::SwitchStyle::ROUND);
    m_roundSwitch->SetLabel("Round Switch");

    m_squareSwitch = new FlatSwitch(scrolledWindow, wxID_ANY, false,
        wxDefaultPosition, wxDefaultSize,
        FlatSwitch::SwitchStyle::SQUARE);
    m_squareSwitch->SetLabel("Square Switch");

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);

    // Buttons section
    wxStaticBoxSizer* buttonSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "FlatButton Examples");
    wxBoxSizer* buttonRow1 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* buttonRow2 = new wxBoxSizer(wxHORIZONTAL);

    buttonRow1->Add(m_primaryButton, 0, wxALL, 5);
    buttonRow1->Add(m_secondaryButton, 0, wxALL, 5);
    buttonRow1->Add(m_outlineButton, 0, wxALL, 5);
    buttonRow2->Add(m_textButton, 0, wxALL, 5);
    buttonRow2->Add(m_iconButton, 0, wxALL, 5);

    buttonSizer->Add(buttonRow1, 0, wxEXPAND);
    buttonSizer->Add(buttonRow2, 0, wxEXPAND);

    // LineEdit section
    wxStaticBoxSizer* lineEditSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "FlatLineEdit Examples");
    wxBoxSizer* lineEditRow1 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* lineEditRow2 = new wxBoxSizer(wxHORIZONTAL);

    lineEditRow1->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Normal:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    lineEditRow1->Add(m_normalLineEdit, 0, wxALL, 5);
    lineEditRow1->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
    lineEditRow1->Add(m_searchLineEdit, 0, wxALL, 5);

    lineEditRow2->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Password:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    lineEditRow2->Add(m_passwordLineEdit, 0, wxALL, 5);
    lineEditRow2->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Clearable:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
    lineEditRow2->Add(m_clearableLineEdit, 0, wxALL, 5);

    lineEditSizer->Add(lineEditRow1, 0, wxEXPAND);
    lineEditSizer->Add(lineEditRow2, 0, wxEXPAND);

    // ComboBox section
    wxStaticBoxSizer* comboBoxSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "FlatComboBox Examples");
    wxBoxSizer* comboBoxRow = new wxBoxSizer(wxHORIZONTAL);

    comboBoxRow->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Normal:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    comboBoxRow->Add(m_normalComboBox, 0, wxALL, 5);
    comboBoxRow->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Editable:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
    comboBoxRow->Add(m_editableComboBox, 0, wxALL, 5);
    comboBoxRow->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
    comboBoxRow->Add(m_searchComboBox, 0, wxALL, 5);

    comboBoxSizer->Add(comboBoxRow, 0, wxEXPAND);

    // CheckBox section
    wxStaticBoxSizer* checkBoxSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "FlatCheckBox Examples");
    wxBoxSizer* checkBoxRow = new wxBoxSizer(wxHORIZONTAL);

    checkBoxRow->Add(m_normalCheckBox, 0, wxALL, 5);
    checkBoxRow->Add(m_switchCheckBox, 0, wxALL, 5);
    checkBoxRow->Add(m_radioCheckBox, 0, wxALL, 5);

    checkBoxSizer->Add(checkBoxRow, 0, wxEXPAND);

    // RadioButton section
    wxStaticBoxSizer* radioButtonSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "FlatRadioButton Examples");
    wxBoxSizer* radioButtonRow = new wxBoxSizer(wxHORIZONTAL);

    radioButtonRow->Add(m_radioButton1, 0, wxALL, 5);
    radioButtonRow->Add(m_radioButton2, 0, wxALL, 5);
    radioButtonRow->Add(m_radioButton3, 0, wxALL, 5);

    radioButtonSizer->Add(radioButtonRow, 0, wxEXPAND);

    // Slider section
    wxStaticBoxSizer* sliderSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "FlatSlider Examples");
    wxBoxSizer* sliderRow = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* horizontalSliders = new wxBoxSizer(wxVERTICAL);
    horizontalSliders->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Normal Slider:"), 0, wxBOTTOM, 2);
    horizontalSliders->Add(m_normalSlider, 0, wxBOTTOM, 10);
    horizontalSliders->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Progress Slider:"), 0, wxBOTTOM, 2);
    horizontalSliders->Add(m_progressSlider, 0, wxBOTTOM, 10);

    wxBoxSizer* verticalSliderContainer = new wxBoxSizer(wxVERTICAL);
    verticalSliderContainer->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Vertical Slider:"), 0, wxBOTTOM, 2);
    verticalSliderContainer->Add(m_verticalSlider, 0, wxALIGN_CENTER_HORIZONTAL);

    sliderRow->Add(horizontalSliders, 1, wxEXPAND);
    sliderRow->Add(verticalSliderContainer, 0, wxLEFT, 20);

    sliderSizer->Add(sliderRow, 0, wxEXPAND);

    // ProgressBar section
    wxStaticBoxSizer* progressBarSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "FlatProgressBar Examples");
    wxBoxSizer* progressBarRow = new wxBoxSizer(wxVERTICAL);

    progressBarRow->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Normal Progress Bar:"), 0, wxBOTTOM, 2);
    progressBarRow->Add(m_normalProgressBar, 0, wxEXPAND | wxBOTTOM, 10);
    progressBarRow->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Indeterminate Progress Bar:"), 0, wxBOTTOM, 2);
    progressBarRow->Add(m_indeterminateProgressBar, 0, wxEXPAND | wxBOTTOM, 10);
    progressBarRow->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Striped Progress Bar:"), 0, wxBOTTOM, 2);
    progressBarRow->Add(m_stripedProgressBar, 0, wxEXPAND | wxBOTTOM, 10);

    progressBarSizer->Add(progressBarRow, 0, wxEXPAND);

    // Switch section
    wxStaticBoxSizer* switchSizer = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "FlatSwitch Examples");
    wxBoxSizer* switchRow = new wxBoxSizer(wxHORIZONTAL);

    switchRow->Add(m_normalSwitch, 0, wxALL, 5);
    switchRow->Add(m_roundSwitch, 0, wxALL, 5);
    switchRow->Add(m_squareSwitch, 0, wxALL, 5);

    switchSizer->Add(switchRow, 0, wxEXPAND);

    // Add all sections to scrolled sizer
    scrolledSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
    scrolledSizer->Add(lineEditSizer, 0, wxEXPAND | wxALL, 10);
    scrolledSizer->Add(comboBoxSizer, 0, wxEXPAND | wxALL, 10);
    scrolledSizer->Add(checkBoxSizer, 0, wxEXPAND | wxALL, 10);
    scrolledSizer->Add(radioButtonSizer, 0, wxEXPAND | wxALL, 10);
    scrolledSizer->Add(sliderSizer, 0, wxEXPAND | wxALL, 10);
    scrolledSizer->Add(progressBarSizer, 0, wxEXPAND | wxALL, 10);
    scrolledSizer->Add(switchSizer, 0, wxEXPAND | wxALL, 10);

    scrolledWindow->SetSizer(scrolledSizer);
    mainSizer->Add(scrolledWindow, 1, wxEXPAND);

    SetSizer(mainSizer);
}

void FlatWidgetsExample::BindEvents()
{
    // Bind button events
    m_primaryButton->Bind(wxEVT_BUTTON, &FlatWidgetsExample::OnPrimaryButtonClicked, this);
    m_secondaryButton->Bind(wxEVT_BUTTON, &FlatWidgetsExample::OnSecondaryButtonClicked, this);
    m_outlineButton->Bind(wxEVT_BUTTON, &FlatWidgetsExample::OnOutlineButtonClicked, this);
    m_textButton->Bind(wxEVT_BUTTON, &FlatWidgetsExample::OnTextButtonClicked, this);
    m_iconButton->Bind(wxEVT_BUTTON, &FlatWidgetsExample::OnIconButtonClicked, this);

    // Bind line edit events
    m_normalLineEdit->Bind(wxEVT_TEXT, &FlatWidgetsExample::OnLineEditTextChanged, this);
    m_searchLineEdit->Bind(wxEVT_TEXT, &FlatWidgetsExample::OnLineEditTextChanged, this);
    m_passwordLineEdit->Bind(wxEVT_TEXT, &FlatWidgetsExample::OnLineEditTextChanged, this);
    m_clearableLineEdit->Bind(wxEVT_TEXT, &FlatWidgetsExample::OnLineEditTextChanged, this);

    // Bind combo box events
    m_normalComboBox->Bind(wxEVT_COMBOBOX, &FlatWidgetsExample::OnComboBoxSelectionChanged, this);
    m_editableComboBox->Bind(wxEVT_COMBOBOX, &FlatWidgetsExample::OnComboBoxSelectionChanged, this);
    m_searchComboBox->Bind(wxEVT_COMBOBOX, &FlatWidgetsExample::OnComboBoxSelectionChanged, this);

    // Bind check box events
    m_normalCheckBox->Bind(wxEVT_CHECKBOX, &FlatWidgetsExample::OnCheckBoxClicked, this);
    m_switchCheckBox->Bind(wxEVT_CHECKBOX, &FlatWidgetsExample::OnCheckBoxClicked, this);
    m_radioCheckBox->Bind(wxEVT_CHECKBOX, &FlatWidgetsExample::OnCheckBoxClicked, this);

    // Bind radio button events
    m_radioButton1->Bind(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, &FlatWidgetsExample::OnRadioButtonClicked, this);
    m_radioButton2->Bind(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, &FlatWidgetsExample::OnRadioButtonClicked, this);
    m_radioButton3->Bind(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, &FlatWidgetsExample::OnRadioButtonClicked, this);

    // Bind slider events
    m_normalSlider->Bind(wxEVT_SLIDER, &FlatWidgetsExample::OnSliderValueChanged, this);
    m_progressSlider->Bind(wxEVT_SLIDER, &FlatWidgetsExample::OnSliderValueChanged, this);
    m_verticalSlider->Bind(wxEVT_SLIDER, &FlatWidgetsExample::OnSliderValueChanged, this);

    // Bind progress bar events
    m_normalProgressBar->Bind(wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, &FlatWidgetsExample::OnProgressBarValueChanged, this);
    m_indeterminateProgressBar->Bind(wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, &FlatWidgetsExample::OnProgressBarValueChanged, this);
    m_stripedProgressBar->Bind(wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, &FlatWidgetsExample::OnProgressBarValueChanged, this);

    // Bind switch events
    m_normalSwitch->Bind(wxEVT_FLAT_SWITCH_TOGGLED, &FlatWidgetsExample::OnSwitchToggled, this);
    m_roundSwitch->Bind(wxEVT_FLAT_SWITCH_TOGGLED, &FlatWidgetsExample::OnSwitchToggled, this);
    m_squareSwitch->Bind(wxEVT_FLAT_SWITCH_TOGGLED, &FlatWidgetsExample::OnSwitchToggled, this);
}

void FlatWidgetsExample::ApplyTheme()
{
    // Apply theme colors and fonts to all widgets
    wxColour bgColor = wxColour(250, 250, 250);
    wxColour textColor = wxColour(50, 50, 50);
    wxFont defaultFont = wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    SetBackgroundColour(bgColor);
    SetForegroundColour(textColor);
    SetFont(defaultFont);
}

// Event handlers
void FlatWidgetsExample::OnPrimaryButtonClicked(wxCommandEvent& event)
{
    wxMessageBox("Primary button clicked!", "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsExample::OnSecondaryButtonClicked(wxCommandEvent& event)
{
    wxMessageBox("Secondary button clicked!", "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsExample::OnOutlineButtonClicked(wxCommandEvent& event)
{
    wxMessageBox("Outline button clicked!", "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsExample::OnTextButtonClicked(wxCommandEvent& event)
{
    wxMessageBox("Text button clicked!", "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsExample::OnIconButtonClicked(wxCommandEvent& event)
{
    wxMessageBox("Icon button clicked!", "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsExample::OnLineEditTextChanged(wxCommandEvent& event)
{
    // Handle text change events
    wxString text = event.GetString();
    // You can add logging or other handling here
}

void FlatWidgetsExample::OnComboBoxSelectionChanged(wxCommandEvent& event)
{
    wxString selection = event.GetString();
    wxMessageBox("ComboBox selection changed to: " + selection, "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsExample::OnCheckBoxClicked(wxCommandEvent& event)
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
    wxMessageBox(label + " " + (checked ? "checked" : "unchecked"), "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsExample::OnRadioButtonClicked(wxCommandEvent& event)
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
        wxMessageBox(label + " selected", "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsExample::OnSliderValueChanged(wxCommandEvent& event)
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

    // Update progress bar based on slider value
    if (event.GetEventObject() == m_normalSlider) {
        m_normalProgressBar->SetValue(value);
    }
    else if (event.GetEventObject() == m_progressSlider) {
        m_stripedProgressBar->SetValue(value);
    }
}

void FlatWidgetsExample::OnProgressBarValueChanged(wxCommandEvent& event)
{
    int value = event.GetInt();
    // Handle progress bar value changes
}

void FlatWidgetsExample::OnSwitchToggled(wxCommandEvent& event)
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

    wxMessageBox(label + " " + (toggled ? "toggled ON" : "toggled OFF"), "FlatWidgetsExample", wxOK | wxICON_INFORMATION);
}