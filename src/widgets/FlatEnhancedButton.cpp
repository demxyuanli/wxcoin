#include "widgets/FlatEnhancedButton.h"
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>

// Event table
BEGIN_EVENT_TABLE(FlatEnhancedButton, wxButton)
EVT_ENTER_WINDOW(FlatEnhancedButton::OnMouseEnter)
EVT_LEAVE_WINDOW(FlatEnhancedButton::OnMouseLeave)
EVT_LEFT_DOWN(FlatEnhancedButton::OnMouseDown)
EVT_LEFT_UP(FlatEnhancedButton::OnMouseUp)
EVT_PAINT(FlatEnhancedButton::OnPaint)
END_EVENT_TABLE()

FlatEnhancedButton::FlatEnhancedButton(wxWindow* parent, wxWindowID id, const wxString& label,
	const wxPoint& pos, const wxSize& size, long style,
	const wxValidator& validator, const wxString& name)
	: wxButton(parent, id, label, pos, size, style, validator, name)
	, m_isHovered(false)
	, m_isPressed(false)
	, m_borderRadius(4.0)
{
	// Initialize colors with flat UI theme
	m_normalColor = wxColour(240, 240, 240);
	m_hoverColor = wxColour(220, 220, 220);
	m_pressedColor = wxColour(200, 200, 200);
	m_borderColor = wxColour(180, 180, 180);

	// Set initial background color
	SetBackgroundColour(m_normalColor);

	// Enable double buffering for smooth rendering
	SetDoubleBuffered(true);

	// Set background style for wxAutoBufferedPaintDC
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

FlatEnhancedButton::~FlatEnhancedButton()
{
}

void FlatEnhancedButton::OnMouseEnter(wxMouseEvent& event)
{
	m_isHovered = true;
	SetBackgroundColour(m_hoverColor);
	Refresh();
	event.Skip();
}

void FlatEnhancedButton::OnMouseLeave(wxMouseEvent& event)
{
	m_isHovered = false;
	m_isPressed = false;
	SetBackgroundColour(m_normalColor);
	Refresh();
	event.Skip();
}

void FlatEnhancedButton::OnMouseDown(wxMouseEvent& event)
{
	m_isPressed = true;
	SetBackgroundColour(m_pressedColor);
	Refresh();
	event.Skip();
}

void FlatEnhancedButton::OnMouseUp(wxMouseEvent& event)
{
	m_isPressed = false;
	if (m_isHovered) {
		SetBackgroundColour(m_hoverColor);
	}
	else {
		SetBackgroundColour(m_normalColor);
	}
	Refresh();
	event.Skip();
}

void FlatEnhancedButton::OnPaint(wxPaintEvent& event)
{
	wxUnusedVar(event);
	wxAutoBufferedPaintDC dc(this);

	// Draw button background with rounded corners effect
	wxRect rect = GetClientRect();

	// Create rounded rectangle path using wxGraphicsContext
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
	if (gc) {
		// Set background color based on button state
		wxColour bgColor = GetBackgroundColour();
		gc->SetBrush(wxBrush(bgColor));

		// Draw rounded rectangle background
		gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, m_borderRadius);

		// Draw border
		gc->SetPen(wxPen(m_borderColor, 1));
		gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, m_borderRadius);

		delete gc;
	}
	else {
		// Fallback to standard DC if graphics context is not available
		dc.SetBrush(wxBrush(GetBackgroundColour()));
		dc.SetPen(wxPen(m_borderColor, 1));
		dc.DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, m_borderRadius);
	}

	// Draw button content (icon or text)
	if (GetBitmap().IsOk()) {
		// Draw icon centered
		wxBitmap bmp = GetBitmap();
		int x = (rect.width - bmp.GetWidth()) / 2;
		int y = (rect.height - bmp.GetHeight()) / 2;
		dc.DrawBitmap(bmp, x, y, true);
	}
	else if (!GetLabel().IsEmpty()) {
		// Draw text centered
		dc.SetFont(GetFont());
		dc.SetTextForeground(GetForegroundColour());
		wxSize textSize = dc.GetTextExtent(GetLabel());
		int x = (rect.width - textSize.GetWidth()) / 2;
		int y = (rect.height - textSize.GetHeight()) / 2;
		dc.DrawText(GetLabel(), x, y);
	}
}