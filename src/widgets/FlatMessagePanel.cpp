#include "widgets/FlatMessagePanel.h"
#include "config/SvgIconManager.h"
#include "config/FontManager.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>

wxBEGIN_EVENT_TABLE(FlatMessagePanel, wxPanel)
// Buttons are bound via Bind in ctor
EVT_PAINT(FlatMessagePanel::OnPaint)
EVT_SIZE(FlatMessagePanel::OnSize)
wxEND_EVENT_TABLE()

FlatMessagePanel::FlatMessagePanel(wxWindow* parent, wxWindowID id, const wxString& title,
	const wxPoint& pos, const wxSize& size, long style)
	: wxPanel(parent, id, pos, size, style)
	, m_titleText(nullptr)
	, m_btnFloat(nullptr)
	, m_btnMinimize(nullptr)
	, m_btnClose(nullptr)
	, m_splitter(nullptr)
	, m_leftPanel(nullptr)
	, m_rightScroll(nullptr)
	, m_textCtrl(nullptr)
	, m_rightContainer(nullptr)
	, m_floatingFrame(nullptr)
	, m_rightWidget(nullptr)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
	// Prevent default background erase to avoid flicker/black gaps
	Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});

	wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
	BuildHeader(root, title);
	BuildBody(root);
	SetSizer(root);
	Layout();

	// Register theme change listener
	ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
		RefreshTheme();
		});

	// Apply configured font to all sub-controls
	try {
		FontManager& fm = FontManager::getInstance();
		wxFont f = fm.getLabelFont();
		if (f.IsOk()) {
			SetFont(f);
			if (m_titleText) m_titleText->SetFont(f);
			if (m_textCtrl) m_textCtrl->SetFont(f);
			if (m_btnFloat) m_btnFloat->SetFont(f);
			if (m_btnMinimize) m_btnMinimize->SetFont(f);
			if (m_btnClose) m_btnClose->SetFont(f);
		}
	}
	catch (...) {
		// Fallback silently if font manager unavailable
	}
}

FlatMessagePanel::~FlatMessagePanel()
{
	// Remove theme change listener
	ThemeManager::getInstance().removeThemeChangeListener(this);
	
	if (m_floatingFrame) {
		m_floatingFrame->Destroy();
		m_floatingFrame = nullptr;
	}
}

void FlatMessagePanel::BuildHeader(wxSizer* parentSizer, const wxString& title)
{
	wxPanel* header = new wxPanel(this);
	header->SetBackgroundStyle(wxBG_STYLE_PAINT);
	header->SetDoubleBuffered(true);
	header->SetBackgroundColour(GetBackgroundColour());
	wxBoxSizer* hs = new wxBoxSizer(wxHORIZONTAL);

	m_titleText = new wxStaticText(header, wxID_ANY, title);
	hs->Add(m_titleText, 1, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

	m_btnFloat = new FlatEnhancedButton(header, wxID_ANY, "", wxDefaultPosition, wxSize(30, 20));
	m_btnMinimize = new FlatEnhancedButton(header, wxID_ANY, "", wxDefaultPosition, wxSize(30, 20));
	m_btnClose = new FlatEnhancedButton(header, wxID_ANY, "", wxDefaultPosition, wxSize(30, 20));

	m_btnFloat->SetToolTip("Float (Ctrl+Shift+F)");
	m_btnMinimize->SetToolTip("Minimize (Ctrl+Shift+M)");
	m_btnClose->SetToolTip("Close (Ctrl+Shift+C)");

	// Load 12x12 SVG icons
	try {
		auto& iconMgr = SvgIconManager::GetInstance();
		wxBitmap bFloat = iconMgr.GetIconBitmap("float", wxSize(12, 12));
		wxBitmap bMin = iconMgr.GetIconBitmap("minimize", wxSize(12, 12));
		wxBitmap bClose = iconMgr.GetIconBitmap("close", wxSize(12, 12));
		if (bFloat.IsOk()) m_btnFloat->SetBitmap(bFloat);
		if (bMin.IsOk()) m_btnMinimize->SetBitmap(bMin);
		if (bClose.IsOk()) m_btnClose->SetBitmap(bClose);
	}
	catch (...) {
		// Fallback: use simple labels
		m_btnFloat->SetLabel("F");
		m_btnMinimize->SetLabel("-");
		m_btnClose->SetLabel("x");
	}

	// Push buttons to the right
	hs->AddStretchSpacer();
	hs->Add(m_btnFloat, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	hs->Add(m_btnMinimize, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	hs->Add(m_btnClose, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

	header->SetSizer(hs);
	parentSizer->Add(header, 0, wxEXPAND, 0);

	// Bind
	m_btnFloat->Bind(wxEVT_BUTTON, &FlatMessagePanel::OnFloat, this);
	m_btnMinimize->Bind(wxEVT_BUTTON, &FlatMessagePanel::OnMinimize, this);
	m_btnClose->Bind(wxEVT_BUTTON, &FlatMessagePanel::OnClose, this);
}

void FlatMessagePanel::BuildBody(wxSizer* parentSizer)
{
	m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxSP_3DBORDER | wxSP_LIVE_UPDATE | wxSP_NO_XP_THEME);
	m_splitter->SetSashGravity(0.5); // centered split by default
	m_splitter->SetMinimumPaneSize(100);
	m_splitter->SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_splitter->SetDoubleBuffered(true);

	// Left panel for messages
	m_leftPanel = new wxPanel(m_splitter);
	m_leftPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_leftPanel->SetDoubleBuffered(true);
	m_leftPanel->SetBackgroundColour(GetBackgroundColour());
	wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
	m_textCtrl = new wxTextCtrl(m_leftPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
		wxTE_READONLY | wxTE_MULTILINE | wxBORDER_NONE);
	m_textCtrl->SetBackgroundStyle(wxBG_STYLE_PAINT);
	leftSizer->Add(m_textCtrl, 1, wxEXPAND | wxALL, 2);
	m_leftPanel->SetSizer(leftSizer);

	// Right side: scrolled window for performance panel
	m_rightScroll = new wxScrolledWindow(m_splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxVSCROLL | wxHSCROLL | wxTAB_TRAVERSAL);
	m_rightScroll->SetScrollRate(10, 10);  // Enable both horizontal and vertical scrolling
	m_rightScroll->SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_rightScroll->SetDoubleBuffered(true);
	m_rightScroll->SetBackgroundColour(GetBackgroundColour());

	// Container is not needed anymore - performance panel will be direct child of scroll window
	m_rightContainer = nullptr;

	// Split with left taking more space initially (70% for messages, 30% for performance)
	m_splitter->SplitVertically(m_leftPanel, m_rightScroll, -300);  // negative value means 300px from right
	parentSizer->Add(m_splitter, 1, wxEXPAND, 0);
}

void FlatMessagePanel::AttachRightPanel(wxWindow* rightWidget)
{
	if (!m_rightScroll || !rightWidget) return;
	m_rightWidget = rightWidget;

	// Reparent performance panel directly into scrolled window
	m_rightWidget->Reparent(m_rightScroll);

	// Set virtual size to match the performance panel's best size
	wxSize bestSize = m_rightWidget->GetBestSize();
	m_rightScroll->SetVirtualSize(bestSize);

	// Position the panel at the top-left of scroll window
	m_rightWidget->SetPosition(wxPoint(0, 0));
	m_rightWidget->SetSize(bestSize);

	// Force refresh
	m_rightScroll->Refresh();
	Layout();
}

void FlatMessagePanel::SetInitialSashPosition(int px)
{
	if (m_splitter) m_splitter->SetSashPosition(px);
}

void FlatMessagePanel::SetSashGravity(double gravity)
{
	if (m_splitter) m_splitter->SetSashGravity(gravity);
}

void FlatMessagePanel::SetTitle(const wxString& title)
{
	if (m_titleText) m_titleText->SetLabel(title);
}

void FlatMessagePanel::SetMessage(const wxString& text)
{
	if (m_textCtrl) m_textCtrl->SetValue(text);
}

void FlatMessagePanel::AppendMessage(const wxString& text)
{
	if (m_textCtrl) {
		wxString current = m_textCtrl->GetValue();
		if (!current.IsEmpty()) current += "\n";
		m_textCtrl->SetValue(current + text);
		m_textCtrl->ShowPosition(m_textCtrl->GetLastPosition());
	}
}

void FlatMessagePanel::Clear()
{
	if (m_textCtrl) m_textCtrl->Clear();
}

void FlatMessagePanel::OnPaint(wxPaintEvent& e)
{
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	// Let children paint over
	e.Skip();
}

void FlatMessagePanel::OnFloat(wxCommandEvent& e)
{
	wxUnusedVar(e);
	if (m_floatingFrame && m_floatingFrame->IsShown()) {
		wxWindow* oldFrame = m_floatingFrame;
		Reparent(GetParent());
		oldFrame->Destroy();
		m_floatingFrame = nullptr;
		Show();
		GetParent()->Layout();
		return;
	}
	m_floatingFrame = new wxFrame(nullptr, wxID_ANY, "Message Output", wxDefaultPosition, wxSize(600, 400));
	m_floatingFrame->SetBackgroundStyle(wxBG_STYLE_PAINT);
	m_floatingFrame->SetDoubleBuffered(true);
	Reparent(m_floatingFrame);
	wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
	s->Add(this, 1, wxEXPAND | wxALL, 0);
	m_floatingFrame->SetSizer(s);
	m_floatingFrame->Show();
}

void FlatMessagePanel::OnMinimize(wxCommandEvent& e)
{
	wxUnusedVar(e);
	if (IsShown()) {
		Hide();
	}
	else {
		Show();
	}
	if (GetParent()) GetParent()->Layout();
}

void FlatMessagePanel::OnClose(wxCommandEvent& e)
{
	wxUnusedVar(e);
	Hide();
	if (GetParent()) GetParent()->Layout();
}

void FlatMessagePanel::OnSize(wxSizeEvent& e)
{
	// Refresh on size to avoid black borders
	Refresh();
	e.Skip();
}

void FlatMessagePanel::RefreshTheme()
{
	// Refresh SVG icons with new theme colors
	if (m_btnFloat && m_btnMinimize && m_btnClose) {
		try {
			auto& iconMgr = SvgIconManager::GetInstance();
			wxBitmap bFloat = iconMgr.GetIconBitmap("float", wxSize(12, 12));
			wxBitmap bMin = iconMgr.GetIconBitmap("minimize", wxSize(12, 12));
			wxBitmap bClose = iconMgr.GetIconBitmap("close", wxSize(12, 12));
			if (bFloat.IsOk()) m_btnFloat->SetBitmap(bFloat);
			if (bMin.IsOk()) m_btnMinimize->SetBitmap(bMin);
			if (bClose.IsOk()) m_btnClose->SetBitmap(bClose);
			
			// Refresh the buttons
			m_btnFloat->Refresh();
			m_btnMinimize->Refresh();
			m_btnClose->Refresh();
		}
		catch (...) {
			// Fallback: use simple labels
			m_btnFloat->SetLabel("F");
			m_btnMinimize->SetLabel("-");
			m_btnClose->SetLabel("x");
		}
	}
}