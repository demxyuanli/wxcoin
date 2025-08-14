#ifndef FLAT_DOCK_CAPTION_BAR_H
#define FLAT_DOCK_CAPTION_BAR_H

#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/button.h>

class FlatDockContainer;

class FlatDockCaptionBar : public wxPanel {
public:
	FlatDockCaptionBar(FlatDockContainer* owner, wxWindow* parent);
	void SetTitle(const wxString& title);

private:
	void OnFloat(wxCommandEvent&);
	void OnClose(wxCommandEvent&);
	void BuildUi();

private:
	FlatDockContainer* m_owner;
	wxStaticText* m_title;
	wxButton* m_btnFloat;
	wxButton* m_btnClose;
};

#endif


