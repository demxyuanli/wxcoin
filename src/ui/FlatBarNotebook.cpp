#include "ui/FlatBarNotebook.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/sizer.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(FlatBarNotebook, wxPanel)
EVT_PAINT(FlatBarNotebook::OnPaint)
EVT_LEFT_DOWN(FlatBarNotebook::OnLeftDown)
EVT_MOTION(FlatBarNotebook::OnMouseMove)
EVT_LEAVE_WINDOW(FlatBarNotebook::OnMouseLeave)
EVT_SIZE(FlatBarNotebook::OnSize)
wxEND_EVENT_TABLE()

FlatBarNotebook::FlatBarNotebook(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
	: wxPanel(parent, id, pos, size)
	, m_selectedPage(-1)
	, m_hoveredTab(-1)
{
	// Set background style for custom painting
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);

	// Initialize FlatBar-style tab properties
	m_tabHeight = 24;
	m_tabPadding = 8;
	m_tabSpacing = 2;
	m_tabBorderTop = 2;
	m_tabBorderLeft = 1;
	m_tabBorderRight = 1;

	// Initialize colors from theme
	UpdateThemeColors();

	// Set minimum size
	SetMinSize(wxSize(200, m_tabHeight + 100));
}

void FlatBarNotebook::UpdateThemeColors()
{
	// Get colors from theme manager (same as FlatBar)
	m_activeTabBgColour = CFG_COLOUR("BarActiveTabBgColour");
	m_activeTabTextColour = CFG_COLOUR("BarActiveTextColour");
	m_inactiveTabTextColour = CFG_COLOUR("BarInactiveTextColour");
	m_tabBorderTopColour = CFG_COLOUR("BarTabBorderTopColour");
	m_tabBorderColour = CFG_COLOUR("BarTabBorderColour");

	Refresh();
}

int FlatBarNotebook::AddPage(wxWindow* page, const wxString& text, bool select, int imageId)
{
	wxUnusedVar(imageId);

	if (!page) return -1;

	// Create page info
	auto pageInfo = std::make_unique<PageInfo>(page, text);

	// Reparent page to this notebook
	page->Reparent(this);
	page->Hide(); // Initially hidden

	// Add to pages collection
	m_pages.push_back(std::move(pageInfo));
	int pageIndex = static_cast<int>(m_pages.size() - 1);

	// Select if requested or if this is the first page
	if (select || m_selectedPage == -1) {
		SetSelection(pageIndex);
	}

	// Update layout
	UpdateTabLayout();
	Refresh();

	return pageIndex;
}

bool FlatBarNotebook::DeletePage(size_t page)
{
	if (page >= m_pages.size()) return false;

	// Hide and unparent the page
	m_pages[page]->page->Hide();
	m_pages[page]->page->Reparent(GetParent());

	// Remove from collection
	m_pages.erase(m_pages.begin() + page);

	// Adjust selected page
	if (m_selectedPage == static_cast<int>(page)) {
		// Select adjacent page
		if (m_selectedPage >= static_cast<int>(m_pages.size())) {
			m_selectedPage = static_cast<int>(m_pages.size()) - 1;
		}
		if (m_selectedPage >= 0) {
			SelectPage(m_selectedPage);
		}
		else {
			m_selectedPage = -1;
		}
	}
	else if (m_selectedPage > static_cast<int>(page)) {
		m_selectedPage--;
	}

	// Update layout
	UpdateTabLayout();
	Refresh();

	return true;
}

bool FlatBarNotebook::DeleteAllPages()
{
	// Hide and unparent all pages
	for (auto& pageInfo : m_pages) {
		pageInfo->page->Hide();
		pageInfo->page->Reparent(GetParent());
	}

	m_pages.clear();
	m_selectedPage = -1;
	m_hoveredTab = -1;

	Refresh();
	return true;
}

int FlatBarNotebook::GetSelection() const
{
	return m_selectedPage;
}

bool FlatBarNotebook::SetSelection(size_t page)
{
	if (page >= m_pages.size()) return false;

	SelectPage(static_cast<int>(page));
	return true;
}

wxWindow* FlatBarNotebook::GetPage(size_t page) const
{
	if (page >= m_pages.size()) return nullptr;
	return m_pages[page]->page;
}

wxString FlatBarNotebook::GetPageText(size_t page) const
{
	if (page >= m_pages.size()) return wxEmptyString;
	return m_pages[page]->text;
}

bool FlatBarNotebook::SetPageText(size_t page, const wxString& text)
{
	if (page >= m_pages.size()) return false;

	m_pages[page]->text = text;
	UpdateTabLayout();
	Refresh();
	return true;
}

size_t FlatBarNotebook::GetPageCount() const
{
	return m_pages.size();
}

void FlatBarNotebook::OnPageChanged(int page)
{
	// This can be overridden by derived classes
	wxUnusedVar(page);
}

void FlatBarNotebook::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	// Clear background
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();

	// Paint tabs
	PaintTabs(dc);

	// Paint content area border
	if (m_selectedPage >= 0) {
		wxSize clientSize = GetClientSize();
		dc.SetPen(wxPen(CFG_COLOUR("BarBorderColour"), 1));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(0, m_tabHeight, clientSize.GetWidth(), clientSize.GetHeight() - m_tabHeight);
	}
}

void FlatBarNotebook::PaintTabs(wxDC& dc)
{
	if (m_pages.empty()) return;

	wxSize clientSize = GetClientSize();

	// Set up drawing context
	wxFont tabFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	dc.SetFont(tabFont);

	int currentX = 0;
	int tabY = 4; // Add some top margin like FlatBar

	// Paint each tab
	for (size_t i = 0; i < m_pages.size(); ++i) {
		bool isActive = (static_cast<int>(i) == m_selectedPage);
		bool isHovered = (static_cast<int>(i) == m_hoveredTab);

		// Calculate tab width
		int tabWidth = CalculateTabWidth(dc, m_pages[i]->text);

		// Create tab rectangle
		wxRect tabRect(currentX, tabY, tabWidth, m_tabHeight);
		m_pages[i]->tabRect = tabRect;

		// Render tab using FlatBar style
		RenderTab(dc, tabRect, m_pages[i]->text, isActive, isHovered);

		currentX += tabWidth + m_tabSpacing;
	}
}

void FlatBarNotebook::RenderTab(wxDC& dc, const wxRect& rect, const wxString& text, bool isActive, bool isHovered)
{
	if (isActive) {
		// Active tab - FlatBar DEFAULT style
		wxColour activeTabBgColour = m_activeTabBgColour;
		wxColour activeTabTextColour = m_activeTabTextColour;
		wxColour tabBorderTopColour = m_tabBorderTopColour;
		wxColour tabBorderColour = m_tabBorderColour;

		dc.SetBrush(wxBrush(activeTabBgColour));
		dc.SetTextForeground(activeTabTextColour);

		// Fill background of active tab (excluding the top border)
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(rect.x, rect.y + m_tabBorderTop, rect.width, rect.height - m_tabBorderTop);

		// Draw borders like FlatBar
		if (m_tabBorderTop > 0) {
			dc.SetPen(wxPen(tabBorderTopColour, m_tabBorderTop));
			dc.DrawLine(rect.GetLeft(), rect.GetTop() + m_tabBorderTop / 2,
				rect.GetRight() + 1, rect.GetTop() + m_tabBorderTop / 2);
		}
		if (m_tabBorderLeft > 0) {
			dc.SetPen(wxPen(tabBorderColour, m_tabBorderLeft));
			dc.DrawLine(rect.GetLeft(), rect.GetTop() + m_tabBorderTop,
				rect.GetLeft(), rect.GetBottom());
		}
		if (m_tabBorderRight > 0) {
			dc.SetPen(wxPen(tabBorderColour, m_tabBorderRight));
			dc.DrawLine(rect.GetRight() + 1, rect.GetTop() + m_tabBorderTop,
				rect.GetRight() + 1, rect.GetBottom());
		}
	}
	else {
		// Inactive tab - no background, no borders (FlatBar style)
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetTextForeground(m_inactiveTabTextColour);
	}

	// Draw tab text
	wxSize textSize = dc.GetTextExtent(text);
	int textX = rect.x + m_tabPadding;
	int textY = rect.y + (rect.height - textSize.GetHeight()) / 2;

	dc.DrawText(text, textX, textY);
}

int FlatBarNotebook::CalculateTabWidth(wxDC& dc, const wxString& text) const
{
	wxSize textSize = dc.GetTextExtent(text);
	return textSize.GetWidth() + m_tabPadding * 2;
}

void FlatBarNotebook::UpdateTabLayout()
{
	if (m_pages.empty()) return;

	// Update tab rectangles
	wxClientDC dc(this);
	dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	int currentX = 0;
	int tabY = 4;

	for (auto& pageInfo : m_pages) {
		int tabWidth = CalculateTabWidth(dc, pageInfo->text);
		pageInfo->tabRect = wxRect(currentX, tabY, tabWidth, m_tabHeight);
		currentX += tabWidth + m_tabSpacing;
	}

	// Update content area for selected page
	if (m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
		wxSize clientSize = GetClientSize();
		wxRect contentRect(0, m_tabHeight, clientSize.GetWidth(), clientSize.GetHeight() - m_tabHeight);

		// Show selected page and hide others
		for (size_t i = 0; i < m_pages.size(); ++i) {
			if (static_cast<int>(i) == m_selectedPage) {
				m_pages[i]->page->Show();
				m_pages[i]->page->SetSize(contentRect);
				m_pages[i]->isActive = true;
			}
			else {
				m_pages[i]->page->Hide();
				m_pages[i]->isActive = false;
			}
		}
	}
}

int FlatBarNotebook::HitTestTab(const wxPoint& pos) const
{
	for (size_t i = 0; i < m_pages.size(); ++i) {
		if (m_pages[i]->tabRect.Contains(pos)) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

void FlatBarNotebook::SelectPage(int page)
{
	if (page < 0 || page >= static_cast<int>(m_pages.size())) return;

	// Hide currently selected page
	if (m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
		m_pages[m_selectedPage]->page->Hide();
		m_pages[m_selectedPage]->isActive = false;
	}

	// Update selection
	m_selectedPage = page;

	// Show new selected page
	if (m_selectedPage >= 0) {
		wxSize clientSize = GetClientSize();
		wxRect contentRect(0, m_tabHeight, clientSize.GetWidth(), clientSize.GetHeight() - m_tabHeight);

		m_pages[m_selectedPage]->page->Show();
		m_pages[m_selectedPage]->page->SetSize(contentRect);
		m_pages[m_selectedPage]->isActive = true;

		// Notify page change
		OnPageChanged(m_selectedPage);
	}

	Refresh();
}

void FlatBarNotebook::OnLeftDown(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();
	int tabIndex = HitTestTab(pos);

	if (tabIndex >= 0 && tabIndex != m_selectedPage) {
		SelectPage(tabIndex);
	}

	event.Skip();
}

void FlatBarNotebook::OnMouseMove(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();
	int newHoveredTab = HitTestTab(pos);

	if (newHoveredTab != m_hoveredTab) {
		m_hoveredTab = newHoveredTab;
		Refresh();
	}

	event.Skip();
}

void FlatBarNotebook::OnMouseLeave(wxMouseEvent& event)
{
	if (m_hoveredTab != -1) {
		m_hoveredTab = -1;
		Refresh();
	}

	event.Skip();
}

void FlatBarNotebook::OnSize(wxSizeEvent& event)
{
	UpdateTabLayout();
	event.Skip();
}