#ifndef FLAT_MESSAGE_PANEL_H
#define FLAT_MESSAGE_PANEL_H

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/frame.h>
#include <wx/splitter.h>
#include <wx/scrolwin.h>
#include "widgets/FlatEnhancedButton.h"

class FlatMessagePanel : public wxPanel {
public:
	FlatMessagePanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = "Message Output",
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL);
	~FlatMessagePanel() override;

	// Access to internal text control
	wxTextCtrl* GetTextCtrl() const { return m_textCtrl; }

	// Helpers
	void SetTitle(const wxString& title);
	void SetMessage(const wxString& text);
	void AppendMessage(const wxString& text);
	void Clear();

	// Attach a right-side widget (e.g., PerformancePanel) into the splitter
	void AttachRightPanel(wxWindow* rightWidget);

	// Splitter helpers
	void SetInitialSashPosition(int px);
	void SetSashGravity(double gravity);

private:
	// UI header
	wxStaticText* m_titleText;
	FlatEnhancedButton* m_btnFloat;
	FlatEnhancedButton* m_btnMinimize;
	FlatEnhancedButton* m_btnClose;

	// Body: splitter with left text and right arbitrary widget
	wxSplitterWindow* m_splitter;
	wxPanel* m_leftPanel;
	wxScrolledWindow* m_rightScroll;
	wxTextCtrl* m_textCtrl;
	wxPanel* m_rightContainer;  // Kept for compatibility but set to nullptr

	wxFrame* m_floatingFrame;
	wxWindow* m_rightWidget;

	// Layout helpers
	void BuildHeader(wxSizer* parentSizer, const wxString& title);
	void BuildBody(wxSizer* parentSizer);

	// Painting to avoid black gaps/flicker
	void OnPaint(wxPaintEvent& e);
	void OnSize(wxSizeEvent& e);

	// Button handlers
	void OnFloat(wxCommandEvent& e);
	void OnMinimize(wxCommandEvent& e);
	void OnClose(wxCommandEvent& e);

	wxDECLARE_EVENT_TABLE();
};

#endif // FLAT_MESSAGE_PANEL_H
