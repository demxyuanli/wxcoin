#include "widgets/GhostWindow.h"
#include "widgets/ModernDockPanel.h"
#include "DPIManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/dcmemory.h>
#include <wx/bitmap.h>

wxBEGIN_EVENT_TABLE(GhostWindow, wxFrame)
EVT_PAINT(GhostWindow::OnPaint)
EVT_TIMER(wxID_ANY, GhostWindow::OnTimer)
EVT_CLOSE(GhostWindow::OnClose)
wxEND_EVENT_TABLE()

GhostWindow::GhostWindow()
	: wxFrame(nullptr, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP | wxBORDER_NONE),
	m_sourcePanel(nullptr),
	m_visible(false),
	m_transparency(DEFAULT_TRANSPARENCY),
	m_targetTransparency(DEFAULT_TRANSPARENCY),
	m_animating(false),
	m_animationTimer(this),
	m_animationStep(0),
	m_totalAnimationSteps(FADE_STEPS)
{
	// Initialize DPI-aware sizes
	double dpiScale = DPIManager::getInstance().getDPIScale();
	m_minSize = wxSize(static_cast<int>(MIN_GHOST_WIDTH * dpiScale),
		static_cast<int>(MIN_GHOST_HEIGHT * dpiScale));
	m_maxSize = wxSize(static_cast<int>(MAX_GHOST_WIDTH * dpiScale),
		static_cast<int>(MAX_GHOST_HEIGHT * dpiScale));
	m_titleBarHeight = static_cast<int>(TITLE_BAR_HEIGHT * dpiScale);
	m_borderWidth = static_cast<int>(BORDER_WIDTH * dpiScale);

	// Set background style for custom painting
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);

	// Initially hidden
	Hide();
}

GhostWindow::~GhostWindow()
{
	HideGhost();
}

void GhostWindow::ShowGhost(ModernDockPanel* panel, const wxPoint& startPos)
{
	if (!panel) return;

	m_sourcePanel = panel;
	m_visible = true;

	// Create ghost content from panel
	CreateGhostContent(panel);

	// Calculate appropriate size
	wxSize panelSize = panel->GetSize();
	wxSize ghostSize = wxSize(
		std::max(m_minSize.x, std::min(m_maxSize.x, panelSize.x)),
		std::max(m_minSize.y, std::min(m_maxSize.y, panelSize.y))
	);

	SetSize(ghostSize);
	Move(startPos - wxPoint(ghostSize.x / 2, 20)); // Offset from cursor

	// Set initial transparency
	SetTransparency(m_transparency);

	// Show and start fade-in animation
	Show();
	StartFadeIn();
	Raise();
}

void GhostWindow::HideGhost()
{
	if (m_visible) {
		m_visible = false;
		m_sourcePanel = nullptr;

		if (m_animationTimer.IsRunning()) {
			m_animationTimer.Stop();
		}

		Hide();
	}
}

void GhostWindow::UpdatePosition(const wxPoint& pos)
{
	if (!m_visible) return;

	wxSize size = GetSize();
	Move(pos - wxPoint(size.x / 2, 20)); // Keep offset from cursor
}

void GhostWindow::SetTransparency(int alpha)
{
	m_transparency = std::max(0, std::min(255, alpha));

#ifdef __WXMSW__
	// On Windows, use SetTransparent
	SetTransparent(m_transparency);
#else
	// On other platforms, might need different approach
	SetTransparent(m_transparency);
#endif
}

void GhostWindow::SetContent(const wxBitmap& bitmap)
{
	m_contentBitmap = bitmap;
	if (m_visible) {
		Refresh();
	}
}

void GhostWindow::SetTitle(const wxString& title)
{
	m_title = title;
	if (m_visible) {
		Refresh();
	}
}

void GhostWindow::StartFadeIn()
{
	m_targetTransparency = DEFAULT_TRANSPARENCY;
	m_transparency = 50; // Start more transparent
	m_animationStep = 0;
	m_animating = true;
	m_animationTimer.Start(FADE_TIMER_INTERVAL);
}

void GhostWindow::StartFadeOut()
{
	m_targetTransparency = 50;
	m_animationStep = 0;
	m_animating = true;
	m_animationTimer.Start(FADE_TIMER_INTERVAL);
}

void GhostWindow::CreateGhostContent(ModernDockPanel* panel)
{
	if (!panel) return;

	// Get panel size
	wxSize panelSize = panel->GetSize();
	if (panelSize.x <= 0 || panelSize.y <= 0) {
		panelSize = wxSize(200, 150); // Fallback size
	}

	// Create bitmap to capture panel content
	wxBitmap bitmap(panelSize.x, panelSize.y);
	wxMemoryDC memDC(bitmap);

	// Clear background
	memDC.SetBackground(wxBrush(wxColour(45, 45, 48))); // Dark background
	memDC.Clear();

	// Draw simplified panel representation
	memDC.SetPen(wxPen(wxColour(0, 122, 204), 2));
	memDC.SetBrush(*wxTRANSPARENT_BRUSH);
	memDC.DrawRectangle(0, 0, panelSize.x, panelSize.y);

	// Draw tab bar representation
	if (panel->GetContentCount() > 0) {
		int tabHeight = 35; // Increased height for better text readability
		memDC.SetBrush(wxBrush(wxColour(63, 63, 70)));
		memDC.DrawRectangle(0, 0, panelSize.x, tabHeight);

		// Calculate tab width based on content count and panel size
		int tabCount = panel->GetContentCount();
		int tabWidth;
		if (tabCount == 1) {
			tabWidth = 150; // Fixed width for single tab
		}
		else {
			// For multiple tabs, calculate width based on panel size and tab count
			// Ensure minimum width for readability and maximum width for aesthetics
			int calculatedWidth = std::max(120, panelSize.x / std::max(1, tabCount));
			tabWidth = std::min(300, calculatedWidth); // Cap at 300px max

			// Ensure minimum tab width for readability
			if (tabWidth < 100) {
				tabWidth = 100;
			}
		}

		// Draw active tab
		memDC.SetBrush(wxBrush(wxColour(0, 122, 204)));
		memDC.DrawRectangle(0, 0, tabWidth, tabHeight);

		// Draw tab text with better positioning and font
		memDC.SetTextForeground(wxColour(241, 241, 241));

		// Use a larger font for better readability
		wxFont tabFont = wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
		memDC.SetFont(tabFont);

		wxString title = panel->GetContentTitle(panel->GetSelectedIndex());
		if (!title.IsEmpty()) {
			// Calculate text rectangle with proper margins
			int textMargin = 8; // Adequate margin for text
			wxRect textRect(textMargin, 0, tabWidth - 2 * textMargin, tabHeight);

			// Use both vertical and horizontal centering for better text positioning
			memDC.DrawLabel(title, textRect, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
		}
	}

	// Set title from panel
	m_title = panel->GetTitle();
	if (m_title.IsEmpty() && panel->GetContentCount() > 0) {
		m_title = panel->GetContentTitle(panel->GetSelectedIndex());
	}

	memDC.SelectObject(wxNullBitmap);
	m_contentBitmap = bitmap;
}

void GhostWindow::UpdateTransparency()
{
	if (!m_animating) return;

	// Calculate interpolated transparency
	double progress = static_cast<double>(m_animationStep) / m_totalAnimationSteps;
	int newTransparency = m_transparency +
		static_cast<int>((m_targetTransparency - m_transparency) * progress);

	SetTransparency(newTransparency);
}

void GhostWindow::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	// Create graphics context for smooth rendering
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (!gc) {
		event.Skip();
		return;
	}

	// Clear background
	gc->SetBrush(wxBrush(wxColour(45, 45, 48, 200)));
	gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);

	RenderGhostFrame(gc);

	delete gc;
}

void GhostWindow::RenderGhostFrame(wxGraphicsContext* gc)
{
	if (!gc) return;

	wxSize size = GetSize();

	// Draw border
	wxColour borderColor(0, 122, 204, 180);
	gc->SetPen(wxPen(borderColor, m_borderWidth));
	gc->SetBrush(*wxTRANSPARENT_BRUSH);
	gc->DrawRectangle(0, 0, size.x, size.y);

	// Draw title bar if we have a title
	if (!m_title.IsEmpty()) {
		wxColour titleBgColor(63, 63, 70, 180);
		gc->SetBrush(wxBrush(titleBgColor));
		gc->DrawRectangle(m_borderWidth, m_borderWidth,
			size.x - 2 * m_borderWidth, m_titleBarHeight);

		// Draw title text
		gc->SetFont(GetFont(), wxColour(241, 241, 241));
		double textWidth, textHeight;
		gc->GetTextExtent(m_title, &textWidth, &textHeight);

		int textX = m_borderWidth + 8;
		int textY = m_borderWidth + (m_titleBarHeight - textHeight) / 2;
		gc->DrawText(m_title, textX, textY);
	}

	// Draw content bitmap if available
	if (m_contentBitmap.IsOk()) {
		int contentY = m_titleBarHeight + m_borderWidth;
		int contentHeight = size.y - contentY - m_borderWidth;

		if (contentHeight > 0) {
			// Scale bitmap to fit content area
			wxRect contentRect(m_borderWidth, contentY,
				size.x - 2 * m_borderWidth, contentHeight);

			// Draw with some transparency
			gc->SetInterpolationQuality(wxINTERPOLATION_GOOD);
			gc->DrawBitmap(m_contentBitmap,
				contentRect.x, contentRect.y,
				contentRect.width, contentRect.height);
		}
	}

	// Draw drag indicator (small dots or lines)
	DrawDragIndicator(gc);
}

void GhostWindow::DrawDragIndicator(wxGraphicsContext* gc)
{
	if (!gc) return;

	// Draw subtle grip lines in title bar
	if (m_titleBarHeight > 0) {
		wxColour gripColor(120, 120, 120, 100);
		gc->SetPen(wxPen(gripColor, 1));

		int centerX = GetSize().x / 2;
		int gripY = m_borderWidth + m_titleBarHeight / 2;
		int gripSpacing = 2;
		int gripLength = 8;

		// Draw three grip lines
		for (int i = -1; i <= 1; ++i) {
			int x = centerX + i * gripSpacing;
			gc->StrokeLine(x, gripY - gripLength / 2, x, gripY + gripLength / 2);
		}
	}
}

void GhostWindow::OnTimer(wxTimerEvent& event)
{
	if (event.GetTimer().GetId() == m_animationTimer.GetId()) {
		if (m_animating) {
			m_animationStep++;

			if (m_animationStep >= m_totalAnimationSteps) {
				// Animation complete
				m_animating = false;
				m_transparency = m_targetTransparency;
				m_animationTimer.Stop();
				SetTransparency(m_transparency);
			}
			else {
				UpdateTransparency();
			}
		}
	}
}

void GhostWindow::OnClose(wxCloseEvent& event)
{
	HideGhost();
	event.Veto(); // Don't actually close, just hide
}