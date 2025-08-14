#include "widgets/FlatDockCaptionBar.h"
#include "widgets/FlatDockContainer.h"
#include "config/SvgIconManager.h"
#include <wx/sizer.h>
#include <wx/bmpbuttn.h>

FlatDockCaptionBar::FlatDockCaptionBar(FlatDockContainer* owner, wxWindow* parent)
	: wxPanel(parent, wxID_ANY), m_owner(owner)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
	BuildUi();
}

void FlatDockCaptionBar::BuildUi()
{
	wxBoxSizer* s = new wxBoxSizer(wxHORIZONTAL);
	m_title = new wxStaticText(this, wxID_ANY, "");
	m_btnFloat = new wxButton(this, wxID_ANY, "", wxDefaultPosition, wxSize(24,18));
	m_btnClose = new wxButton(this, wxID_ANY, "", wxDefaultPosition, wxSize(24,18));

	// Load SVG icons (fallback to text)
	try {
		auto& iconMgr = SvgIconManager::GetInstance();
		wxBitmap bFloat = iconMgr.GetIconBitmap("expand", wxSize(12,12));
		wxBitmap bClose = iconMgr.GetIconBitmap("close", wxSize(12,12));
		if (bFloat.IsOk()) m_btnFloat->SetBitmap(bFloat);
		if (bClose.IsOk()) m_btnClose->SetBitmap(bClose);
	} catch (...) {
		m_btnFloat->SetLabel("F");
		m_btnClose->SetLabel("X");
	}

	s->Add(m_title, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
	s->Add(m_btnFloat, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	s->Add(m_btnClose, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	SetSizer(s);

	Bind(wxEVT_BUTTON, &FlatDockCaptionBar::OnFloat, this, m_btnFloat->GetId());
	Bind(wxEVT_BUTTON, &FlatDockCaptionBar::OnClose, this, m_btnClose->GetId());
}

void FlatDockCaptionBar::SetTitle(const wxString& title)
{
	if (m_title) m_title->SetLabel(title);
}

void FlatDockCaptionBar::OnFloat(wxCommandEvent&)
{
	if (m_owner) m_owner->FloatSelectedTab();
}

void FlatDockCaptionBar::OnClose(wxCommandEvent&)
{
	if (!m_owner) return;
	auto* nb = m_owner->GetNotebook();
	int sel = nb ? nb->GetSelection() : -1;
	if (sel >= 0) nb->DeletePage(sel);
}


