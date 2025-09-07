#include "flatui/FlatUICustomControl.h"
#include "config/ThemeManager.h"
#include "flatui/FlatUIEventManager.h"
#include <wx/dcbuffer.h>

FlatUICustomControl::FlatUICustomControl(wxWindow* parent, wxWindowID id)
	: wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE),
	m_backgroundColor(CFG_COLOUR("CustomControlBgColour")),
	m_hover(false)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	// Use event manager to bind events
	auto& eventManager = FlatUIEventManager::getInstance();

	// Method 1: Use bindCustomEvents template method to bind multiple events
	eventManager.bindCustomEvents(
		this,                                   // Control instance
		&FlatUICustomControl::OnPaint,         // Paint event handler
		&FlatUICustomControl::OnMouseDown,     // Mouse down event handler
		&FlatUICustomControl::OnMouseMove,     // Mouse move event handler
		&FlatUICustomControl::OnMouseLeave     // Mouse leave event handler
	);

	// Method 2: Use bindEvent template method to bind events individually
	// eventManager.bindEvent(this, wxEVT_PAINT, &FlatUICustomControl::OnPaint);
	// eventManager.bindEvent(this, wxEVT_LEFT_DOWN, &FlatUICustomControl::OnMouseDown);

	// Method 3: Use bindSizeEvents method to bind size events
	eventManager.bindSizeEvents(this, [this](wxSizeEvent& event) {
		// Handle size change event
		Refresh();
		event.Skip();
		});

	SetMinSize(wxSize(100, 50));
}

FlatUICustomControl::~FlatUICustomControl()
{
}

void FlatUICustomControl::SetBackgroundColor(const wxColour& color)
{
	m_backgroundColor = color;
	Refresh();
}

void FlatUICustomControl::OnPaint(wxPaintEvent& evt)
{
	wxAutoBufferedPaintDC dc(this);
	dc.Clear();

	wxSize size = GetClientSize();

	// Draw background
	wxColour bgColor = m_backgroundColor;
	if (m_hover) {
		// Darken color when mouse hovers
		int red = bgColor.Red() * 0.8;
		int green = bgColor.Green() * 0.8;
		int blue = bgColor.Blue() * 0.8;
		bgColor.Set(red, green, blue);
	}

	dc.SetBrush(bgColor);
	dc.SetPen(wxPen(CFG_COLOUR("ThemeBlackPenColour")));
	dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

	// Draw text
	dc.SetTextForeground(CFG_COLOUR("DefaultTextColour"));
	wxString text = "Custom Control";
	wxSize textSize = dc.GetTextExtent(text);
	int x = (size.GetWidth() - textSize.GetWidth()) / 2;
	int y = (size.GetHeight() - textSize.GetHeight()) / 2;
	dc.DrawText(text, x, y);
}

void FlatUICustomControl::OnMouseDown(wxMouseEvent& evt)
{
	// Handle mouse click event
	wxCommandEvent event(wxEVT_BUTTON, GetId());
	event.SetEventObject(this);
	ProcessWindowEvent(event);

	evt.Skip();
}

void FlatUICustomControl::OnMouseMove(wxMouseEvent& evt)
{
	bool oldHover = m_hover;
	m_hover = true;

	if (m_hover != oldHover) {
		Refresh();
	}

	evt.Skip();
}

void FlatUICustomControl::OnMouseLeave(wxMouseEvent& evt)
{
	bool oldHover = m_hover;
	m_hover = false;

	if (m_hover != oldHover) {
		Refresh();
	}

	evt.Skip();
}