#include "flatui/FlatUISystemButtons.h"
#include "flatui/FlatUIFrame.h"
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"
#include <wx/dcbuffer.h> // For wxAutoBufferedPaintDC

FlatUISystemButtons::FlatUISystemButtons(wxWindow* parent, wxWindowID id)
	: wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE),
	m_minimizeButtonHover(false),
	m_maximizeButtonHover(false),
	m_closeButtonHover(false),
	m_buttonWidth(CFG_INT("SystemButtonWidth")),
	m_buttonSpacing(CFG_INT("SystemButtonSpacing"))
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	SetDoubleBuffered(true);

	// Register theme change listener
	ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
		RefreshTheme();
		});

	Bind(wxEVT_PAINT, &FlatUISystemButtons::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &FlatUISystemButtons::OnMouseDown, this);
	Bind(wxEVT_MOTION, &FlatUISystemButtons::OnMouseMove, this);
	Bind(wxEVT_LEAVE_WINDOW, &FlatUISystemButtons::OnMouseLeave, this);
}

FlatUISystemButtons::~FlatUISystemButtons()
{
	// Remove theme change listener
	ThemeManager::getInstance().removeThemeChangeListener(this);
}

int FlatUISystemButtons::GetRequiredWidth() const {
	return GetAllRequiredWidth(m_buttonWidth, m_buttonSpacing);
}

int FlatUISystemButtons::GetAllRequiredWidth(int buttonWidth, int buttonSpacing) {
	return (buttonWidth * 3) + (buttonSpacing * 2);
}

void FlatUISystemButtons::SetButtonRects(const wxRect& minimizeRect, const wxRect& maximizeRect, const wxRect& closeRect)
{
	// These rects are relative to this FlatUISystemButtons control's own client area.
	// When FlatUIBar calculates their positions, it will be in FlatUIBar's coordinate space.
	// FlatUIBar will then position FlatUISystemButtons control, and then call this method
	// with rects adjusted to be relative to FlatUISystemButtons's origin (0,0).

	// For now, assume the rects passed are already relative to this control's (0,0)
	// This means FlatUIBar would calculate absolute positions, then create these rects
	// for us by subtracting our FlatUISystemButtons::GetRect().GetTopLeft().
	// Simpler: FlatUIBar positions us, and we just draw within our GetClientSize().
	// FlatUIBar will tell us our total width/height. We then derive the button rects from that.

	wxSize clientSize = GetClientSize();

	int buttonWidth = CFG_INT("SystemButtonHeight");;
	int buttonHeight = CFG_INT("SystemButtonWidth");

	int sysButtonY = (clientSize.GetHeight() - buttonHeight) / 2;
	if (sysButtonY < 0) sysButtonY = 0;

	// Calculate rects based on our client size, anchored to the right
	int currentX = clientSize.GetWidth();

	currentX -= buttonWidth;
	m_closeButtonRect = wxRect(currentX, sysButtonY, buttonWidth, buttonHeight);

	currentX -= m_buttonSpacing;
	currentX -= buttonWidth;
	m_maximizeButtonRect = wxRect(currentX, sysButtonY, buttonWidth, buttonHeight);

	currentX -= m_buttonSpacing;
	currentX -= buttonWidth;
	m_minimizeButtonRect = wxRect(currentX, sysButtonY, buttonWidth, buttonHeight);

	// Note: The SetButtonRects in the header was perhaps misleading.
	// This control will draw its buttons within its own bounds, right-aligned.
	// FlatUIBar just needs to give this control the correct overall size and position.
	// So, this method might not be strictly needed if we always calculate from client size in OnPaint
	// Or, it can be used to confirm/override if FlatUIBar wants to dictate sub-rects.
	// For robust encapsulation, this control should manage its internal button layout.
	Refresh(); // In case rects changed affecting hover state display
}

wxFrame* FlatUISystemButtons::GetTopLevelFrame() const
{
	wxWindow* topLevelWindow = wxGetTopLevelParent(const_cast<FlatUISystemButtons*>(this));
	return wxDynamicCast(topLevelWindow, wxFrame);
}

void FlatUISystemButtons::PaintButton(wxDC& dc, const wxRect& rect, const wxString& symbol, bool hover, bool isClose, bool isMaximizedOrPseudo)
{
	wxColour btnTextColour = CFG_COLOUR("SystemButtonTextColour");
	wxColour hoverBgColour = CFG_COLOUR("DropdownHoverColour");
	wxColour hoverTextColour = CFG_COLOUR("SystemButtonHoverTextColour");
	// Use theme background color for normal state
	wxColour normalBgColour = CFG_COLOUR("BarBackgroundColour");

	if (isClose) {
		dc.SetBrush(hover ? CFG_COLOUR("SystemButtonCloseHoverColour") : normalBgColour);
		dc.SetTextForeground(hover ? CFG_COLOUR("SystemButtonHoverTextColour") : btnTextColour);
	}
	else {
		dc.SetBrush(hover ? hoverBgColour : normalBgColour);
		dc.SetTextForeground(hover ? hoverTextColour : btnTextColour);
	}
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(rect);

	wxString actualSymbol = symbol;
	if (!isClose && isMaximizedOrPseudo && symbol == wxString(wchar_t(0x2610))) { // Maximize symbol
		actualSymbol = wxString(wchar_t(0x2750)); // Restore symbol (two vertical bars)
	}
	else if (!isClose && !isMaximizedOrPseudo && symbol == wxString(wchar_t(0x2750))) { // Restore symbol, but frame is not maximized
		actualSymbol = wxString(wchar_t(0x2610)); // Maximize symbol (empty square)
	}

	wxFont currentFont = dc.GetFont();
	// Ensure a specific font for symbols if default font doesn't render them well
	// For instance, Arial Unicode MS or Segoe UI Symbol
	// currentFont.SetFaceName("Segoe UI Symbol"); // Example
	dc.SetFont(currentFont);

	wxSize textSize = dc.GetTextExtent(actualSymbol);
	int textY = rect.GetY() + (rect.GetHeight() - textSize.GetHeight()) / 2;
	// Specific adjustment for minimize button if needed
	if (symbol == "_") textY = rect.GetY() + (rect.GetHeight() - textSize.GetHeight()) / 4;

	dc.DrawText(actualSymbol, rect.GetX() + (rect.GetWidth() - textSize.GetWidth()) / 2, textY);
}

void FlatUISystemButtons::OnPaint(wxPaintEvent& evt)
{
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(CFG_COLOUR("BarBackgroundColour"));
	dc.Clear();

	wxSize size = GetClientSize();
	int contentX = 0;
	int contentY = 0;
	int contentW = size.GetWidth();
	int contentH = size.GetHeight();
	if (contentW < 0) contentW = 0;
	if (contentH < 0) contentH = 0;

	int currentButtonWidth = CFG_INT("SystemButtonWidth");
	int currentButtonHeight = CFG_INT("SystemButtonHeight");
	int currentButtonY = contentY + (contentH - currentButtonHeight) / 2;
	if (currentButtonY < contentY) currentButtonY = contentY;

	int xPos = contentX + contentW;

	xPos -= currentButtonWidth;
	m_closeButtonRect = wxRect(xPos, currentButtonY, currentButtonWidth, currentButtonHeight);

	xPos -= m_buttonSpacing;
	xPos -= currentButtonWidth;
	m_maximizeButtonRect = wxRect(xPos, currentButtonY, currentButtonWidth, currentButtonHeight);

	xPos -= m_buttonSpacing;
	xPos -= currentButtonWidth;
	m_minimizeButtonRect = wxRect(xPos, currentButtonY, currentButtonWidth, currentButtonHeight);

	FlatUIFrame* topFrame = wxDynamicCast(GetTopLevelFrame(), FlatUIFrame);
	bool isEffectivelyMaximized = topFrame ? topFrame->IsPseudoMaximized() : false;

	//PaintButton(dc, m_minimizeButtonRect, "_", m_minimizeButtonHover);
	//PaintButton(dc, m_maximizeButtonRect, isEffectivelyMaximized ? wxString(wchar_t(0x2750)) : wxString(wchar_t(0x2610)), m_maximizeButtonHover, false, isEffectivelyMaximized);
	//PaintButton(dc, m_closeButtonRect, "X", m_closeButtonHover, true);

	PaintSvgButton(dc, m_minimizeButtonRect, "minimize", m_minimizeButtonHover);
	PaintSvgButton(dc, m_maximizeButtonRect, isEffectivelyMaximized ? "restore" : "maximize", m_maximizeButtonHover);
	PaintSvgButton(dc, m_closeButtonRect, "close", m_closeButtonHover, true);
}

void FlatUISystemButtons::PaintSvgButton(wxDC& dc, const wxRect& rect, const wxString& iconName, bool hover, bool isClose)
{
	wxColour hoverBgColour = CFG_COLOUR("DropdownHoverColour");
	wxColour normalBgColour = CFG_COLOUR("BarBackgroundColour");

	if (isClose) {
		dc.SetBrush(hover ? CFG_COLOUR("SystemButtonCloseHoverColour") : normalBgColour);
	}
	else {
		dc.SetBrush(hover ? hoverBgColour : normalBgColour);
	}
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(rect);

	wxSize iconSize(16, 16);
	wxBitmap iconBitmap = SvgIconManager::GetInstance().GetIconBitmap(iconName, iconSize);

	if (iconBitmap.IsOk()) {
		int iconX = rect.GetX() + (rect.GetWidth() - iconSize.GetWidth()) / 2;
		int iconY = rect.GetY() + (rect.GetHeight() - iconSize.GetHeight()) / 2;

		dc.DrawBitmap(iconBitmap, iconX, iconY, true);
	}
}

void FlatUISystemButtons::OnMouseDown(wxMouseEvent& evt)
{
	HandleSystemButtonAction(evt.GetPosition(), evt);
	// evt.Skip() is handled in HandleSystemButtonAction
}

void FlatUISystemButtons::HandleSystemButtonAction(const wxPoint& pos, wxMouseEvent& evt)
{
	FlatUIFrame* frame = wxDynamicCast(GetTopLevelFrame(), FlatUIFrame); // Cast to FlatFrame
	if (!frame) { evt.Skip(); return; }

	bool handled = false;
	if (m_closeButtonRect.Contains(pos)) {
		frame->Close(true);
		handled = true;
	}
	else if (m_maximizeButtonRect.Contains(pos)) {
		if (frame->IsPseudoMaximized()) {
			frame->RestoreFromPseudoMaximize();
		}
		else {
			frame->PseudoMaximize();
		}
		Refresh(); // Refresh to update button symbol
		handled = true;
	}
	else if (m_minimizeButtonRect.Contains(pos)) {
		frame->Iconize(true);
		handled = true;
	}

	if (handled) {
		// SetFocus(); // Usually not needed, and can steal focus undesirably
		evt.Skip(false);
	}
	else {
		evt.Skip();
	}
}

void FlatUISystemButtons::OnMouseMove(wxMouseEvent& evt)
{
	wxPoint pos = evt.GetPosition();
	bool needsRefresh = false;

	bool oldHoverMin = m_minimizeButtonHover;
	m_minimizeButtonHover = m_minimizeButtonRect.Contains(pos);
	if (oldHoverMin != m_minimizeButtonHover) needsRefresh = true;

	bool oldHoverMax = m_maximizeButtonHover;
	m_maximizeButtonHover = m_maximizeButtonRect.Contains(pos);
	if (oldHoverMax != m_maximizeButtonHover) needsRefresh = true;

	bool oldHoverClose = m_closeButtonHover;
	m_closeButtonHover = m_closeButtonRect.Contains(pos);
	if (oldHoverClose != m_closeButtonHover) needsRefresh = true;

	if (needsRefresh) {
		Refresh();
		Update();
	}
	evt.Skip();
}

void FlatUISystemButtons::OnMouseLeave(wxMouseEvent& evt)
{
	bool needsRefresh = false;
	if (m_minimizeButtonHover) { m_minimizeButtonHover = false; needsRefresh = true; }
	if (m_maximizeButtonHover) { m_maximizeButtonHover = false; needsRefresh = true; }
	if (m_closeButtonHover) { m_closeButtonHover = false; needsRefresh = true; }

	if (needsRefresh) {
		Refresh();
		Update();
	}
	evt.Skip();
}

void FlatUISystemButtons::RefreshTheme()
{
	// Update theme-based settings
	m_buttonWidth = CFG_INT("SystemButtonWidth");
	m_buttonSpacing = CFG_INT("SystemButtonSpacing");

	// Update control properties
	SetFont(CFG_DEFAULTFONT());

	// Recalculate button positions
	SetButtonRects(m_minimizeButtonRect, m_maximizeButtonRect, m_closeButtonRect);

	// Force refresh to redraw with new theme colors
	Refresh(true);
	Update();
}