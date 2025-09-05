#include "widgets/FlatWidgetsInputsPanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/scrolwin.h>
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(FlatWidgetsInputsPanel, wxPanel)
EVT_TEXT(wxID_ANY, FlatWidgetsInputsPanel::OnLineEditTextChanged)
EVT_COMBOBOX(wxID_ANY, FlatWidgetsInputsPanel::OnComboBoxSelectionChanged)
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
	m_normalComboBox = new FlatComboBox(m_scrolledWindow, wxID_ANY, "Select item",
		wxDefaultPosition, wxSize(200, -1),
		FlatComboBox::ComboBoxStyle::DEFAULT_STYLE);
	m_normalComboBox->Append("Item 1");
	m_normalComboBox->Append("Item 2");
	m_normalComboBox->Append("Item 3");
	m_normalComboBox->Append("Item 4");

	m_editableComboBox = new FlatComboBox(m_scrolledWindow, wxID_ANY, "Editable combo",
		wxDefaultPosition, wxSize(200, -1),
		FlatComboBox::ComboBoxStyle::EDITABLE);
	m_editableComboBox->Append("Option 1");
	m_editableComboBox->Append("Option 2");
	m_editableComboBox->Append("Option 3");

	m_searchComboBox = new FlatComboBox(m_scrolledWindow, wxID_ANY, "Search combo",
		wxDefaultPosition, wxSize(200, -1),
		FlatComboBox::ComboBoxStyle::SEARCH);
	m_searchComboBox->Append("Search Item 1");
	m_searchComboBox->Append("Search Item 2");
	m_searchComboBox->Append("Search Item 3");
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
	comboBoxRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 5);
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

	m_normalComboBox->Bind(wxEVT_COMBOBOX, &FlatWidgetsInputsPanel::OnComboBoxSelectionChanged, this);
	m_editableComboBox->Bind(wxEVT_COMBOBOX, &FlatWidgetsInputsPanel::OnComboBoxSelectionChanged, this);
	m_searchComboBox->Bind(wxEVT_COMBOBOX, &FlatWidgetsInputsPanel::OnComboBoxSelectionChanged, this);
}

void FlatWidgetsInputsPanel::OnLineEditTextChanged(wxCommandEvent& event)
{
	// Handle text change events
	wxString text = event.GetString();
	// You can add logging or other handling here
}

void FlatWidgetsInputsPanel::OnComboBoxSelectionChanged(wxCommandEvent& event)
{
	wxString selection = event.GetString();
	wxMessageBox("ComboBox selection changed to: " + selection, "FlatWidgetsInputsPanel", wxOK | wxICON_INFORMATION);
}
