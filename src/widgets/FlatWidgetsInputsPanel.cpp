#include "widgets/FlatWidgetsInputsPanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/scrolwin.h>
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(FlatWidgetsInputsPanel, wxPanel)
EVT_TEXT(wxID_ANY, FlatWidgetsInputsPanel::OnLineEditTextChanged)
wxEND_EVENT_TABLE()

FlatWidgetsInputsPanel::FlatWidgetsInputsPanel(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
{
	SetBackgroundColour(wxColour(250, 250, 250));
	
	CreateControls();
	LayoutPanel();
	BindEvents();
}

void FlatWidgetsInputsPanel::CreateControls()
{
	// Create scrolled window first
	m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
	m_scrolledWindow->SetScrollRate(10, 10);
	m_scrolledWindow->SetBackgroundColour(wxColour(250, 250, 250));
	
	// Create FlatLineEdit examples on the scrolled window
	m_normalLineEdit = new FlatLineEdit(m_scrolledWindow, wxID_ANY, "Normal input",
		wxDefaultPosition, wxSize(200, -1),
		FlatLineEdit::LineEditStyle::DEFAULT_STYLE);
	m_normalLineEdit->SetPlaceholderText("Enter text here...");

	m_searchLineEdit = new FlatLineEdit(m_scrolledWindow, wxID_ANY, "",
		wxDefaultPosition, wxSize(200, -1),
		FlatLineEdit::LineEditStyle::SEARCH);
	m_searchLineEdit->SetPlaceholderText("Search...");

	m_passwordLineEdit = new FlatLineEdit(m_scrolledWindow, wxID_ANY, "",
		wxDefaultPosition, wxSize(200, -1),
		FlatLineEdit::LineEditStyle::PASSWORD);
	m_passwordLineEdit->SetPlaceholderText("Enter password...");

	m_clearableLineEdit = new FlatLineEdit(m_scrolledWindow, wxID_ANY, "Clearable text",
		wxDefaultPosition, wxSize(200, -1),
		FlatLineEdit::LineEditStyle::CLEARABLE);
	m_clearableLineEdit->SetPlaceholderText("Type and clear...");

	// Create FlatComboBox examples on the scrolled window
	m_normalComboBox = new FlatComboBox(m_scrolledWindow, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxSize(200, -1),
		FlatComboBox::ComboBoxStyle::DEFAULT_STYLE);
	m_normalComboBox->Append("Apple");
	m_normalComboBox->Append("Banana");
	m_normalComboBox->Append("Cherry");
	m_normalComboBox->Append("Date");
	m_normalComboBox->Append("Elderberry");
	m_normalComboBox->Append("Fig");
	m_normalComboBox->SetSelection(0);

	m_editableComboBox = new FlatComboBox(m_scrolledWindow, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxSize(200, -1),
		FlatComboBox::ComboBoxStyle::EDITABLE);
	m_editableComboBox->Append("Red");
	m_editableComboBox->Append("Green");
	m_editableComboBox->Append("Blue");
	m_editableComboBox->Append("Yellow");
	m_editableComboBox->Append("Purple");
	m_editableComboBox->SetSelection(0);

	m_searchComboBox = new FlatComboBox(m_scrolledWindow, wxID_ANY, wxEmptyString,
		wxDefaultPosition, wxSize(200, -1),
		FlatComboBox::ComboBoxStyle::SEARCH);

	// Add normal items
	m_searchComboBox->Append("Monday");
	m_searchComboBox->Append("Tuesday");
	m_searchComboBox->Append("Wednesday");

	// Add separator
	m_searchComboBox->AppendSeparator();

	// Add color picker items
	m_searchComboBox->AppendColorPicker("Red", wxColour(255, 0, 0));
	m_searchComboBox->AppendColorPicker("Green", wxColour(0, 255, 0));
	m_searchComboBox->AppendColorPicker("Blue", wxColour(0, 0, 255));

	// Add separator
	m_searchComboBox->AppendSeparator();

	// Add checkbox items
	m_searchComboBox->AppendCheckbox("Bold", true);
	m_searchComboBox->AppendCheckbox("Italic", false);
	m_searchComboBox->AppendCheckbox("Underline", false);

	// Add separator
	m_searchComboBox->AppendSeparator();

	// Add radio button items (grouped)
	m_searchComboBox->AppendRadioButton("Small", "size", false);
	m_searchComboBox->AppendRadioButton("Medium", "size", true);
	m_searchComboBox->AppendRadioButton("Large", "size", false);

	m_searchComboBox->SetSelection(0);
}

void FlatWidgetsInputsPanel::LayoutPanel()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);

	// LineEdit section
	wxStaticBoxSizer* lineEditSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "FlatLineEdit Examples");
	wxBoxSizer* lineEditRow1 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* lineEditRow2 = new wxBoxSizer(wxHORIZONTAL);

	lineEditRow1->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Normal:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	lineEditRow1->Add(m_normalLineEdit, 0, wxALL, 5);
	lineEditRow1->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	lineEditRow1->Add(m_searchLineEdit, 0, wxALL, 5);

	lineEditRow2->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Password:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	lineEditRow2->Add(m_passwordLineEdit, 0, wxALL, 5);
	lineEditRow2->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Clearable:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	lineEditRow2->Add(m_clearableLineEdit, 0, wxALL, 5);

	lineEditSizer->Add(lineEditRow1, 0, wxEXPAND);
	lineEditSizer->Add(lineEditRow2, 0, wxEXPAND);

	// ComboBox section
	wxStaticBoxSizer* comboBoxSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "FlatComboBox Examples");
	wxBoxSizer* comboBoxRow = new wxBoxSizer(wxHORIZONTAL);

	comboBoxRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Normal:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	comboBoxRow->Add(m_normalComboBox, 0, wxALL, 5);
	comboBoxRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Editable:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	comboBoxRow->Add(m_editableComboBox, 0, wxALL, 5);
	comboBoxRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Advanced:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
	comboBoxRow->Add(m_searchComboBox, 0, wxALL, 5);

	comboBoxSizer->Add(comboBoxRow, 0, wxEXPAND);

	scrolledSizer->Add(lineEditSizer, 0, wxEXPAND | wxALL, 10);
	scrolledSizer->Add(comboBoxSizer, 0, wxEXPAND | wxALL, 10);

	m_scrolledWindow->SetSizer(scrolledSizer);
	mainSizer->Add(m_scrolledWindow, 1, wxEXPAND);

	SetSizer(mainSizer);
}

void FlatWidgetsInputsPanel::BindEvents()
{
	m_normalLineEdit->Bind(wxEVT_TEXT, &FlatWidgetsInputsPanel::OnLineEditTextChanged, this);
	m_searchLineEdit->Bind(wxEVT_TEXT, &FlatWidgetsInputsPanel::OnLineEditTextChanged, this);
	m_passwordLineEdit->Bind(wxEVT_TEXT, &FlatWidgetsInputsPanel::OnLineEditTextChanged, this);
	m_clearableLineEdit->Bind(wxEVT_TEXT, &FlatWidgetsInputsPanel::OnLineEditTextChanged, this);

	m_normalComboBox->Bind(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, &FlatWidgetsInputsPanel::OnComboBoxSelectionChanged, this);
	m_editableComboBox->Bind(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, &FlatWidgetsInputsPanel::OnComboBoxSelectionChanged, this);
	m_searchComboBox->Bind(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, &FlatWidgetsInputsPanel::OnComboBoxSelectionChanged, this);
	
	m_normalComboBox->Bind(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, [this](wxCommandEvent&) {
	});
	m_editableComboBox->Bind(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, [this](wxCommandEvent&) {
	});
	m_searchComboBox->Bind(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, [this](wxCommandEvent&) {
	});
}

void FlatWidgetsInputsPanel::OnLineEditTextChanged(wxCommandEvent& event)
{
	// Handle text change events
	wxString text = event.GetString();
	// You can add logging or other handling here
}

void FlatWidgetsInputsPanel::OnComboBoxSelectionChanged(wxCommandEvent& event)
{
	wxString senderName = "Unknown";
	FlatComboBox* comboBox = nullptr;

	if (event.GetEventObject() == m_normalComboBox) {
		senderName = "Normal ComboBox";
		comboBox = m_normalComboBox;
	}
	else if (event.GetEventObject() == m_editableComboBox) {
		senderName = "Editable ComboBox";
		comboBox = m_editableComboBox;
	}
	else if (event.GetEventObject() == m_searchComboBox) {
		senderName = "Advanced ComboBox";
		comboBox = m_searchComboBox;
	}

	if (comboBox) {
		int selection = comboBox->GetSelection();
		if (selection >= 0) {
			wxString itemText = comboBox->GetString(selection);
			FlatComboBox::ItemType itemType = comboBox->GetItemType(selection);

			wxString typeStr;
			switch (itemType) {
			case FlatComboBox::ItemType::NORMAL:
				typeStr = "Normal";
				break;
			case FlatComboBox::ItemType::COLOR_PICKER:
				typeStr = "Color Picker";
				break;
			case FlatComboBox::ItemType::CHECKBOX:
				typeStr = comboBox->IsItemChecked(selection) ? "Checkbox (Checked)" : "Checkbox (Unchecked)";
				break;
			case FlatComboBox::ItemType::RADIO_BUTTON:
				typeStr = comboBox->IsItemChecked(selection) ? "Radio Button (Selected)" : "Radio Button";
				break;
			case FlatComboBox::ItemType::SEPARATOR:
				typeStr = "Separator";
				break;
			}

			wxString message = wxString::Format("%s\nItem: %s\nType: %s",
				senderName, itemText, typeStr);
		}
	}
}
