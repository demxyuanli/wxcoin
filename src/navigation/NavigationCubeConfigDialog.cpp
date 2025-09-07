#include "NavigationCubeConfigDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

BEGIN_EVENT_TABLE(NavigationCubeConfigDialog, wxDialog)
EVT_BUTTON(wxID_OK, NavigationCubeConfigDialog::OnOK)
EVT_BUTTON(wxID_CANCEL, NavigationCubeConfigDialog::OnCancel)
EVT_BUTTON(wxID_ANY, NavigationCubeConfigDialog::OnChooseColor)
END_EVENT_TABLE()

NavigationCubeConfigDialog::NavigationCubeConfigDialog(wxWindow* parent, int x, int y, int size, int viewportSize, const wxColour& color, int maxX, int maxY)
	: wxDialog(parent, wxID_ANY, "Nav Cube Configuration", wxDefaultPosition, wxSize(300, 280))
	, m_color(color)
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxGridSizer* gridSizer = new wxGridSizer(5, 2, 5, 5);

	gridSizer->Add(new wxStaticText(this, wxID_ANY, "XPos:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
	m_xCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, maxX, x);
	gridSizer->Add(m_xCtrl, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(this, wxID_ANY, "YPos:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
	m_yCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, maxY, y);
	gridSizer->Add(m_yCtrl, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(this, wxID_ANY, "Size:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
	m_sizeCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 50, std::min(maxX, maxY) / 2, size);
	gridSizer->Add(m_sizeCtrl, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(this, wxID_ANY, "ViewPort Size:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
	m_viewportSizeCtrl = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 50, std::min(maxX, maxY), viewportSize);
	gridSizer->Add(m_viewportSizeCtrl, 0, wxEXPAND);

	gridSizer->Add(new wxStaticText(this, wxID_ANY, "Colour:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
	m_colorButton = new wxButton(this, wxID_ANY, "Choice Color");
	m_colorButton->SetBackgroundColour(m_color);
	gridSizer->Add(m_colorButton, 0, wxEXPAND);

	mainSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this, wxID_OK, "Ok"), 0, wxRIGHT, 5);
	buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"));
	mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);

	SetSizer(mainSizer);
	Layout();
	Centre();
}

void NavigationCubeConfigDialog::OnOK(wxCommandEvent& event) {
	EndModal(wxID_OK);
}

void NavigationCubeConfigDialog::OnCancel(wxCommandEvent& event) {
	EndModal(wxID_CANCEL);
}

void NavigationCubeConfigDialog::OnChooseColor(wxCommandEvent& event) {
	wxColourDialog colorDialog(this);
	if (colorDialog.ShowModal() == wxID_OK) {
		m_color = colorDialog.GetColourData().GetColour();
		m_colorButton->SetBackgroundColour(m_color);
		m_colorButton->Refresh();
	}
}