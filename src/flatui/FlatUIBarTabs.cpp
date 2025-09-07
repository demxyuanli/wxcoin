#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIBarStateManager.h"
#include <wx/dcbuffer.h>
#include <string>
#include "config/ThemeManager.h"
#include "logger/Logger.h"

// Tab painting and click handling are now managed by FlatBarSpaceContainer
// These methods are kept for backward compatibility but are no longer used

void FlatUIBar::OnMouseDown(wxMouseEvent& evt)
{
	// Tab clicks are now handled by FlatBarSpaceContainer
	// Only process non-tab related mouse events here
	evt.Skip();
}

void FlatUIBar::SetTabStyle(TabStyle style)
{
	m_tabStyle = style;

	// Set default border widths based on style
	switch (style) {
	case TabStyle::DEFAULT:
		m_tabBorderTop = 2;
		m_tabBorderBottom = 0;
		m_tabBorderLeft = 1;
		m_tabBorderRight = 1;
		break;
	case TabStyle::UNDERLINE:
		m_tabBorderTop = 0;
		m_tabBorderBottom = 2;
		m_tabBorderLeft = 0;
		m_tabBorderRight = 0;
		break;
	case TabStyle::BUTTON:
		m_tabBorderTop = 1;
		m_tabBorderBottom = 1;
		m_tabBorderLeft = 1;
		m_tabBorderRight = 1;
		break;
	case TabStyle::FLAT:
		m_tabBorderTop = 1;
		m_tabBorderBottom = 0;
		m_tabBorderLeft = 0;
		m_tabBorderRight = 0;
		break;
	}

	Refresh();
}

void FlatUIBar::SetTabBorderWidths(int top, int bottom, int left, int right)
{
	m_tabBorderTop = top;
	m_tabBorderBottom = bottom;
	m_tabBorderLeft = left;
	m_tabBorderRight = right;
	Refresh();
}

void FlatUIBar::SetTabBorderColour(const wxColour& colour)
{
	m_tabBorderColour = colour;
	// Also update individual border colors for backward compatibility
	m_tabBorderTopColour = colour;
	m_tabBorderBottomColour = colour;
	m_tabBorderLeftColour = colour;
	m_tabBorderRightColour = colour;
	Refresh();
}

void FlatUIBar::SetActiveTabBackgroundColour(const wxColour& colour)
{
	m_activeTabBgColour = colour;
	Refresh();
}

void FlatUIBar::SetActiveTabTextColour(const wxColour& colour)
{
	m_activeTabTextColour = colour;
	Refresh();
}

void FlatUIBar::SetInactiveTabTextColour(const wxColour& colour)
{
	m_inactiveTabTextColour = colour;
	Refresh();
}

void FlatUIBar::SetTabBorderStyle(TabBorderStyle style)
{
	m_tabBorderStyle = style;
	Refresh();
}

void FlatUIBar::SetTabCornerRadius(int radius)
{
	m_tabCornerRadius = radius;
	Refresh();
}

void FlatUIBar::SetTabBorderTopColour(const wxColour& colour)
{
	m_tabBorderTopColour = colour;
	Refresh();
}

void FlatUIBar::SetTabBorderBottomColour(const wxColour& colour)
{
	m_tabBorderBottomColour = colour;
	Refresh();
}

void FlatUIBar::SetTabBorderLeftColour(const wxColour& colour)
{
	m_tabBorderLeftColour = colour;
	Refresh();
}

void FlatUIBar::SetTabBorderRightColour(const wxColour& colour)
{
	m_tabBorderRightColour = colour;
	Refresh();
}

void FlatUIBar::SetTabBorderTopWidth(int width)
{
	m_tabBorderTop = width;
	Refresh();
}

void FlatUIBar::SetTabBorderBottomWidth(int width)
{
	m_tabBorderBottom = width;
	Refresh();
}

void FlatUIBar::SetTabBorderLeftWidth(int width)
{
	m_tabBorderLeft = width;
	Refresh();
}

void FlatUIBar::SetTabBorderRightWidth(int width)
{
	m_tabBorderRight = width;
	Refresh();
}

void FlatUIBar::SetBarTopMargin(int margin)
{
	m_barTopMargin = margin;
	if (IsShown()) {
		m_layoutManager->UpdateLayout(GetClientSize());
		Refresh();
	}
}

void FlatUIBar::SetBarBottomMargin(int margin)
{
	m_barBottomMargin = margin;
	if (IsShown()) {
		m_layoutManager->UpdateLayout(GetClientSize());
		Refresh();
	}
}