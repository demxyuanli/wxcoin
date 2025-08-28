#include "flatui/FlatUIBar.h"
#include <wx/dcbuffer.h>
#include "config/ThemeManager.h"
#include "logger/Logger.h"

void FlatUIBar::SetHomeButtonMenu(wxMenu* menu)
{
	// if (m_homeSpace) m_homeSpace->SetMenu(menu); // Removed as FlatUIHomeSpace now uses FlatUIHomeMenu internally
}

void FlatUIBar::SetHomeButtonIcon(const wxBitmap& icon)
{
	if (m_homeSpace) m_homeSpace->SetIcon(icon);
}

void FlatUIBar::SetHomeButtonWidth(int width)
{
	if (m_homeSpace && width > 0) {
		m_homeSpace->SetButtonWidth(width);
		if (IsShown()) Layout(); // Trigger re-layout of FlatUIBar if visible
	}
}

void FlatUIBar::SetFunctionSpaceControl(wxWindow* funcControl, int width)
{
	if (m_functionSpace) {
		m_functionSpace->SetChildControl(funcControl);
		if (width > 0) m_functionSpace->SetSpaceWidth(width);
		// Only show if control exists and user toggle state is visible
		bool shouldShow = (funcControl != nullptr) && m_functionSpaceUserVisible;
		m_functionSpace->Show(shouldShow);
		if (IsShown()) Layout(); // Trigger re-layout of FlatUIBar
	}
}

void FlatUIBar::SetProfileSpaceControl(wxWindow* profControl, int width)
{
	if (m_profileSpace) {
		m_profileSpace->SetChildControl(profControl);
		if (width > 0) m_profileSpace->SetSpaceWidth(width);
		// Only show if control exists and user toggle state is visible
		bool shouldShow = (profControl != nullptr) && m_profileSpaceUserVisible;
		m_profileSpace->Show(shouldShow);
		if (IsShown()) Layout(); // Trigger re-layout of FlatUIBar
	}
}

void FlatUIBar::SetTabFunctionSpacerAutoExpand(bool autoExpand)
{
	if (m_tabFunctionSpacer) {
		m_tabFunctionSpacer->SetAutoExpand(autoExpand);

		if (IsShown()) {
			m_layoutManager->UpdateLayout(GetClientSize());
			Refresh();
		}
	}
}

void FlatUIBar::SetFunctionProfileSpacerAutoExpand(bool autoExpand)
{
	if (m_functionProfileSpacer) {
		m_functionProfileSpacer->SetAutoExpand(autoExpand);

		if (IsShown()) {
			m_layoutManager->UpdateLayout(GetClientSize());
			Refresh();
		}
	}
}

void FlatUIBar::SetFunctionSpaceCenterAlign(bool center)
{
	m_functionSpaceCenterAlign = center;
	if (IsShown()) {
		m_layoutManager->UpdateLayout(GetClientSize());
		Refresh();
	}
}

void FlatUIBar::SetProfileSpaceRightAlign(bool rightAlign)
{
	m_profileSpaceRightAlign = rightAlign;
	if (IsShown()) {
		m_layoutManager->UpdateLayout(GetClientSize());
		Refresh();
	}
}

void FlatUIBar::AddSpaceSeparator(SpacerLocation location, int width, bool drawSeparator, bool canDrag, bool autoExpand)
{
	FlatUISpacerControl** targetSpacer = nullptr;
	wxString logLocation;
	wxString spacerName;

	switch (location) {
	case SPACER_TAB_FUNCTION:
		targetSpacer = &m_tabFunctionSpacer;
		logLocation = "TabFunction";
		spacerName = "TabFunctionSpacer";
		break;
	case SPACER_FUNCTION_PROFILE:
		targetSpacer = &m_functionProfileSpacer;
		logLocation = "FunctionProfile";
		spacerName = "FunctionProfileSpacer";
		break;
	default:
		LOG_ERR("FlatUIBar::AddSpaceSeparator - Invalid location specified", "FlatUIBar");
		return;
	}

	if (!*targetSpacer) {
		*targetSpacer = new FlatUISpacerControl(this, width);
		(*targetSpacer)->SetName(spacerName);
		(*targetSpacer)->SetCanDragWindow(canDrag);
		(*targetSpacer)->SetDoubleBuffered(true);
	}

	if (width > 0) {
		(*targetSpacer)->SetSpacerWidth(width);
		(*targetSpacer)->SetDrawSeparator(drawSeparator);
		(*targetSpacer)->SetShowDragFlag(canDrag);
		(*targetSpacer)->SetAutoExpand(autoExpand);
		(*targetSpacer)->Show();
	}
	else {
		(*targetSpacer)->Hide();
	}

	if (IsShown()) {
		m_layoutManager->UpdateLayout(GetClientSize());
		Refresh();
	}
}

// UpdateElementPositionsAndSizes function has been moved to FlatUIBarLayoutManager

int FlatUIBar::CalculateTabsWidth(wxDC& dc) const
{
	int tabPadding = CFG_INT("BarTabPadding");
	int tabSpacing = CFG_INT("BarTabSpacing");
	int totalWidth = 0;
	if (GetPageCount() == 0) return 0;

	for (size_t i = 0; i < GetPageCount(); ++i)
	{
		FlatUIPage* page = GetPage(i);
		if (!page) continue;
		wxString label = page->GetLabel();
		wxSize labelSize = dc.GetTextExtent(label);
		totalWidth += labelSize.GetWidth() + tabPadding * 2;
		if (i < GetPageCount() - 1)
		{
			totalWidth += tabSpacing;
		}
	}
	// Add extra space for the right border of the last tab
	if (GetPageCount() > 0 && GetTabBorderRightWidth() > 0) {
		totalWidth += 1;  // Reserve space for right border
	}

	return totalWidth;
}

void FlatUIBar::ToggleFunctionSpaceVisibility()
{
	if (m_functionSpace) {
		bool visible = m_functionSpace->IsShown();
		bool newVisible = !visible;
		m_functionSpaceUserVisible = newVisible;  // Update user toggle state
		m_functionSpace->Show(newVisible);
		if (m_tabFunctionSpacer) {
			m_tabFunctionSpacer->Show(newVisible);
		}
		if (IsShown()) {
			m_layoutManager->UpdateLayout(GetClientSize());
			Refresh();
		}
	}
}

void FlatUIBar::ToggleProfileSpaceVisibility()
{
	if (m_profileSpace) {
		bool visible = m_profileSpace->IsShown();
		bool newVisible = !visible;
		m_profileSpaceUserVisible = newVisible;  // Update user toggle state
		m_profileSpace->Show(newVisible);
		if (m_functionProfileSpacer) {
			m_functionProfileSpacer->Show(newVisible);
		}
		if (IsShown()) {
			m_layoutManager->UpdateLayout(GetClientSize());
			Refresh();
		}
	}
}