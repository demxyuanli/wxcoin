#include "flatui/FlatBarSpaceContainer.h"
#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIHomeSpace.h"
#include "flatui/FlatUISystemButtons.h"
#include "flatui/FlatUIFunctionSpace.h"
#include "flatui/FlatUIProfileSpace.h"
#include "flatui/FlatUITabDropdown.h"
#include "flatui/FlatUIBarStateManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>

wxBEGIN_EVENT_TABLE(FlatBarSpaceContainer, wxControl)
EVT_PAINT(FlatBarSpaceContainer::OnPaint)
EVT_SIZE(FlatBarSpaceContainer::OnSize)
EVT_LEFT_DOWN(FlatBarSpaceContainer::OnMouseDown)
EVT_LEFT_UP(FlatBarSpaceContainer::OnMouseUp)
EVT_MOTION(FlatBarSpaceContainer::OnMouseMove)
EVT_MOUSE_CAPTURE_LOST(FlatBarSpaceContainer::OnMouseCaptureLost)
wxEND_EVENT_TABLE()

FlatBarSpaceContainer::FlatBarSpaceContainer(wxWindow* parent, wxWindowID id,
	const wxPoint& pos, const wxSize& size, long style)
	: wxControl(parent, id, pos, size, style | wxBORDER_NONE),
	m_homeSpace(nullptr),
	m_systemButtons(nullptr),
	m_functionSpace(nullptr),
	m_profileSpace(nullptr),
	m_tabDropdown(nullptr),
	m_functionSpaceCenterAlign(false),
	m_profileSpaceRightAlign(true),
	m_isDragging(false),
	m_hasTabOverflow(false)
{
	SetName("FlatBarSpaceContainer");
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);

	// Create tab dropdown component
	m_tabDropdown = new FlatUITabDropdown(this);
	m_tabDropdown->Hide(); // Initially hidden

	// Register theme change listener
	auto& themeManager = ThemeManager::getInstance();
	themeManager.addThemeChangeListener(this, [this]() {
		RefreshTheme();
		});

	LOG_INF("FlatBarSpaceContainer created", "BarSpaceContainer");
}

FlatBarSpaceContainer::~FlatBarSpaceContainer()
{
	if (HasCapture()) {
		ReleaseMouse();
	}
	LOG_INF("FlatBarSpaceContainer destroyed", "BarSpaceContainer");
}

void FlatBarSpaceContainer::SetHomeSpace(FlatUIHomeSpace* homeSpace)
{
	m_homeSpace = homeSpace;
	if (m_homeSpace && m_homeSpace->GetParent() != this) {
		m_homeSpace->Reparent(this);
	}
	UpdateLayout();
}

void FlatBarSpaceContainer::SetSystemButtons(FlatUISystemButtons* systemButtons)
{
	m_systemButtons = systemButtons;
	if (m_systemButtons && m_systemButtons->GetParent() != this) {
		m_systemButtons->Reparent(this);
	}
	UpdateLayout();
}

void FlatBarSpaceContainer::SetFunctionSpace(FlatUIFunctionSpace* functionSpace)
{
	m_functionSpace = functionSpace;
	if (m_functionSpace && m_functionSpace->GetParent() != this) {
		m_functionSpace->Reparent(this);
	}
	UpdateLayout();
}

void FlatBarSpaceContainer::SetProfileSpace(FlatUIProfileSpace* profileSpace)
{
	m_profileSpace = profileSpace;
	if (m_profileSpace && m_profileSpace->GetParent() != this) {
		m_profileSpace->Reparent(this);
	}
	UpdateLayout();
}

void FlatBarSpaceContainer::SetTabAreaRect(const wxRect& rect)
{
	m_tabAreaRect = rect;
	Refresh();
}

wxSize FlatBarSpaceContainer::DoGetBestSize() const
{
	int height = CFG_INT("BarRenderHeight");
	int width = 0;

	// Calculate minimum width based on components
	if (m_homeSpace && m_homeSpace->IsShown()) {
		width += m_homeSpace->GetButtonWidth() + ELEMENT_SPACING;
	}

	if (m_systemButtons && m_systemButtons->IsShown()) {
		width += m_systemButtons->GetRequiredWidth() + ELEMENT_SPACING;
	}

	if (m_functionSpace && m_functionSpace->IsShown()) {
		width += m_functionSpace->GetSpaceWidth() + ELEMENT_SPACING;
	}

	if (m_profileSpace && m_profileSpace->IsShown()) {
		width += m_profileSpace->GetSpaceWidth() + ELEMENT_SPACING;
	}

	width += 0;

	return wxSize(wxMax(width, 200), height);
}

void FlatBarSpaceContainer::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);
	DrawBackground(dc);

	// Paint tabs in the tab area
	PaintTabs(dc);

	event.Skip();
}

void FlatBarSpaceContainer::OnSize(wxSizeEvent& event)
{
	UpdateLayout();
	event.Skip();
}

void FlatBarSpaceContainer::UpdateLayout()
{
	wxSize clientSize = GetClientSize();
	if (clientSize.GetWidth() <= 0 || clientSize.GetHeight() <= 0) return;

	PositionComponents(); // This handles smart hiding/showing of components
	UpdateTabOverflow();  // Update tab overflow after positioning components
	if (IsShown()) {
		Refresh();
	}
}

void FlatBarSpaceContainer::PositionComponents()
{
	wxSize clientSize = GetClientSize();
	int currentX = 0;
	int elementY = 0;
	int innerHeight = clientSize.GetHeight() - 1; // Reserve 1 pixel for bottom border

	// 1. Position HomeSpace (leftmost) - always visible
	int homeWidth = 0;
	if (m_homeSpace) {
		homeWidth = m_homeSpace->GetButtonWidth();
		m_homeSpace->SetPosition(wxPoint(currentX, elementY));
		m_homeSpace->SetSize(homeWidth, innerHeight);
		currentX += homeWidth + ELEMENT_SPACING;
	}

	// 2. Calculate SystemButtons position (rightmost) - always visible
	int sysButtonsWidth = 0;
	int sysButtonsX = clientSize.GetWidth();
	if (m_systemButtons) {
		sysButtonsWidth = m_systemButtons->GetRequiredWidth();
		sysButtonsX -= sysButtonsWidth;
		m_systemButtons->SetPosition(wxPoint(sysButtonsX, elementY));
		m_systemButtons->SetSize(sysButtonsWidth, innerHeight);
	}

	// 3. Calculate available space for middle components
	int rightBoundary = sysButtonsX - ELEMENT_SPACING;
	int availableWidth = rightBoundary - currentX;
	if (availableWidth < 0) availableWidth = 0;

	// 4. Get required widths
	int funcWidth = m_functionSpace ? m_functionSpace->GetSpaceWidth() : 0;
	int profileWidth = m_profileSpace ? m_profileSpace->GetSpaceWidth() : 0;

	// Calculate actual tabs width needed
	int actualTabsWidth = 0;
	wxWindow* parentBar = GetParent();
	if (parentBar) {
		FlatUIBar* flatUIBar = dynamic_cast<FlatUIBar*>(parentBar);
		if (flatUIBar && flatUIBar->GetPageCount() > 0) {
			wxClientDC dc(this);
			actualTabsWidth = flatUIBar->CalculateTabsWidth(dc);
		}
	}

	// 5. Implement smart hiding strategy
	bool showFunction = true;
	bool showProfile = true;
	int tabAreaWidth = actualTabsWidth;

	// Calculate minimum required width for essential components (Home + minimal tabs + SystemButtons)
	const int MIN_TAB_WIDTH = 100; // Minimum tab area width
	int essentialWidth = homeWidth + ELEMENT_SPACING + MIN_TAB_WIDTH + ELEMENT_SPACING + sysButtonsWidth;

	// Strategy 1: Try to fit everything
	int totalNeededWidth = actualTabsWidth + ELEMENT_SPACING;
	if (funcWidth > 0) totalNeededWidth += funcWidth + ELEMENT_SPACING;
	if (profileWidth > 0) totalNeededWidth += profileWidth + ELEMENT_SPACING;

	if (totalNeededWidth > availableWidth) {
		// Strategy 2: Hide FunctionSpace first
		showFunction = false;
		totalNeededWidth = actualTabsWidth + ELEMENT_SPACING;
		if (profileWidth > 0) totalNeededWidth += profileWidth + ELEMENT_SPACING;

		if (totalNeededWidth > availableWidth) {
			// Strategy 3: Hide ProfileSpace as well
			showProfile = false;
			totalNeededWidth = actualTabsWidth + ELEMENT_SPACING;

			if (totalNeededWidth > availableWidth) {
				// Strategy 4: Truncate tabs if necessary
				tabAreaWidth = availableWidth - ELEMENT_SPACING;
				if (tabAreaWidth < MIN_TAB_WIDTH) {
					tabAreaWidth = MIN_TAB_WIDTH;
				}
			}
		}
	}

	// 6. Apply visibility decisions
	if (m_functionSpace) {
		if (showFunction && m_functionSpace->IsShown()) {
			// Keep visible
		}
		else if (!showFunction && m_functionSpace->IsShown()) {
			m_functionSpace->Show(false);
		}
		else if (showFunction && !m_functionSpace->IsShown()) {
			m_functionSpace->Show(true);
		}
	}

	if (m_profileSpace) {
		if (showProfile && m_profileSpace->IsShown()) {
			// Keep visible
		}
		else if (!showProfile && m_profileSpace->IsShown()) {
			m_profileSpace->Show(false);
		}
		else if (showProfile && !m_profileSpace->IsShown()) {
			m_profileSpace->Show(true);
		}
	}

	// 7. Position tab area
	if (tabAreaWidth > 0) {
		m_tabAreaRect = wxRect(currentX, elementY, tabAreaWidth, innerHeight);
		currentX += tabAreaWidth + ELEMENT_SPACING;
	}
	else {
		m_tabAreaRect = wxRect(0, 0, 0, 0);
	}

	// 8. Position FunctionSpace if visible
	if (showFunction && m_functionSpace) {
		// Calculate the middle area between tab area and profile/system buttons
		int middleAreaStart = currentX;
		int middleAreaEnd = rightBoundary;
		if (showProfile && profileWidth > 0) {
			middleAreaEnd -= (profileWidth + ELEMENT_SPACING);
		}

		int middleAreaWidth = middleAreaEnd - middleAreaStart;
		int funcX = middleAreaStart;

		// Center FunctionSpace in the middle area
		if (middleAreaWidth > funcWidth) {
			funcX = middleAreaStart + (middleAreaWidth - funcWidth) / 2;
		}

		m_functionSpace->SetPosition(wxPoint(funcX, elementY));
		m_functionSpace->SetSize(funcWidth, innerHeight);
	}

	// 9. Position ProfileSpace if visible
	if (showProfile && m_profileSpace) {
		int profileX = rightBoundary - profileWidth;
		m_profileSpace->SetPosition(wxPoint(profileX, elementY));
		m_profileSpace->SetSize(profileWidth, innerHeight);
	}
}

void FlatBarSpaceContainer::DrawBackground(wxDC& dc)
{
	wxSize clientSize = GetClientSize();
	dc.SetBrush(wxBrush(CFG_COLOUR("BarBackgroundColour")));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0, 0, clientSize.GetWidth(), clientSize.GetHeight());

	// Draw 1-pixel bottom border
	dc.SetPen(wxPen(CFG_COLOUR("BarBorderColour"), 1));
	dc.DrawLine(0, clientSize.GetHeight() - 1, clientSize.GetWidth(), clientSize.GetHeight() - 1);
}

void FlatBarSpaceContainer::PaintTabs(wxDC& dc)
{
	// Get parent FlatUIBar to access tab information
	FlatUIBar* parentBar = dynamic_cast<FlatUIBar*>(GetParent());
	if (!parentBar || parentBar->GetPageCount() == 0) {
		LOG_DBG("No parent bar or no pages to paint", "BarSpaceContainer");
		return;
	}

	if (m_tabAreaRect.GetWidth() <= 0) {
		LOG_DBG("Tab area width <= 0, not painting tabs", "BarSpaceContainer");
		return;
	}

	// Update tab overflow state before painting
	UpdateTabOverflow();

	wxSize clientSize = GetClientSize();

	// LOG_DBG("PaintTabs: Drawing tabs in area (" + std::to_string(m_tabAreaRect.x) +
	//        "," + std::to_string(m_tabAreaRect.y) + "," + std::to_string(m_tabAreaRect.width) +
	//        "," + std::to_string(m_tabAreaRect.height) + ")", "BarSpaceContainer");

	// Set up drawing context
	dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	int currentX = m_tabAreaRect.x;
	int tabY = m_tabAreaRect.y + 4; // Add some top margin
	int tabHeight = clientSize.GetHeight() - 4; // Use full container height minus top margin for tabs
	int tabPadding = CFG_INT("BarTabPadding");
	int tabSpacing = CFG_INT("BarTabSpacing");

	// Reserve space for dropdown button if there are hidden tabs
	int availableTabWidth = m_tabAreaRect.width;
	if (m_hasTabOverflow) {
		const int DROPDOWN_SPACING = 2; // Smaller spacing between last tab and dropdown button
		availableTabWidth -= (DROPDOWN_BUTTON_WIDTH + DROPDOWN_SPACING);
		// Dropdown button is now drawn by the FlatUITabDropdown component
	}

	// Only draw visible tabs
	for (size_t tabIndex : m_visibleTabIndices) {
		FlatUIPage* page = parentBar->GetPage(tabIndex);
		if (!page) continue;

		wxString label = page->GetLabel();
		wxSize labelSize = dc.GetTextExtent(label);
		int tabWidth = CalculateTabWidth(dc, label);

		// Double check if tab fits in available area
		if (currentX + tabWidth > m_tabAreaRect.x + availableTabWidth) {
			LOG_DBG("Tab " + std::to_string(tabIndex) + " doesn't fit in available width, stopping", "BarSpaceContainer");
			break;
		}

		// Determine if this tab is active (use same logic as original)
		bool isActive = false;
		if (parentBar->IsBarPinned()) {
			isActive = (tabIndex == parentBar->GetActivePage());
		}
		else {
			isActive = (tabIndex == parentBar->GetStateManager()->GetActiveFloatingPage());
		}

		// Use original FlatUIBar drawing logic
		wxRect tabRect(currentX, tabY, tabWidth, tabHeight);

		if (isActive) {
			// Get colors from parent bar
			wxColour activeTabBgColour = CFG_COLOUR("BarActiveTabBgColour");
			wxColour activeTabTextColour = CFG_COLOUR("BarActiveTextColour");
			wxColour tabBorderTopColour = CFG_COLOUR("BarTabBorderTopColour");
			wxColour tabBorderColour = CFG_COLOUR("BarTabBorderColour");

			dc.SetBrush(wxBrush(activeTabBgColour));
			dc.SetTextForeground(activeTabTextColour);

			// Draw tab using DEFAULT style (most common)
			int tabBorderTop = 2;
			int tabBorderLeft = 1;
			int tabBorderRight = 1;

			// Fill background of active tab (excluding the top border)
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(tabRect.x, tabRect.y + tabBorderTop, tabRect.width, tabRect.height - tabBorderTop);

			// Draw borders
			if (tabBorderTop > 0) {
				dc.SetPen(wxPen(tabBorderTopColour, tabBorderTop));
				dc.DrawLine(tabRect.GetLeft(), tabRect.GetTop() + tabBorderTop / 2,
					tabRect.GetRight() + 1, tabRect.GetTop() + tabBorderTop / 2);
			}
			if (tabBorderLeft > 0) {
				dc.SetPen(wxPen(tabBorderColour, tabBorderLeft));
				dc.DrawLine(tabRect.GetLeft(), tabRect.GetTop() + tabBorderTop,
					tabRect.GetLeft(), tabRect.GetBottom());
			}
			if (tabBorderRight > 0) {
				dc.SetPen(wxPen(tabBorderColour, tabBorderRight));
				dc.DrawLine(tabRect.GetRight() + 1, tabRect.GetTop() + tabBorderTop,
					tabRect.GetRight() + 1, tabRect.GetBottom());
			}
		}
		else {
			// Inactive tab - no background, no borders
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetTextForeground(CFG_COLOUR("BarInactiveTextColour"));
		}

		// Draw tab text
		int textX = currentX + tabPadding;
		int textY = tabY + (tabHeight - labelSize.GetHeight()) / 2;

		// LOG_DBG("Drawing tab " + std::to_string(tabIndex) + " '" + label.ToStdString() +
		//        "' at (" + std::to_string(textX) + "," + std::to_string(textY) +
		//        "), active=" + (isActive ? "true" : "false"), "BarSpaceContainer");

		dc.DrawText(label, textX, textY);

		currentX += tabWidth + tabSpacing;
	}
}

void FlatBarSpaceContainer::OnMouseDown(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();

	// Check if click is in tab area first
	if (HandleTabClick(pos)) {
		return; // Tab click handled, don't process as drag
	}

	// Check if click is in a drag area (not on child controls)
	if (IsInDragArea(pos)) {
		StartDrag(pos);
	}

	event.Skip();
}

void FlatBarSpaceContainer::OnMouseMove(wxMouseEvent& event)
{
	if (m_isDragging && event.Dragging() && event.LeftIsDown()) {
		UpdateDrag(event.GetPosition());
	}
	else if (!m_isDragging && IsInDragArea(event.GetPosition())) {
		// Change cursor to indicate draggable area
		SetCursor(wxCursor(wxCURSOR_ARROW));
	}
	else {
		SetCursor(wxNullCursor);
	}

	event.Skip();
}

void FlatBarSpaceContainer::OnMouseUp(wxMouseEvent& event)
{
	if (m_isDragging) {
		EndDrag();
	}
	event.Skip();
}

void FlatBarSpaceContainer::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	if (m_isDragging) {
		EndDrag();
	}
}

void FlatBarSpaceContainer::StartDrag(const wxPoint& startPos)
{
	m_isDragging = true;
	m_dragStartPos = startPos;

	// Use rubber band drag method like FlatUISpacerControl
	wxWindow* topWindow = wxGetTopLevelParent(this);
	if (topWindow) {
		wxPoint screenPos = ClientToScreen(m_dragStartPos);
		wxPoint clientPos = topWindow->ScreenToClient(screenPos);

		wxMouseEvent downEvt(wxEVT_LEFT_DOWN);
		downEvt.SetPosition(clientPos);
		downEvt.SetEventObject(topWindow);
		topWindow->ProcessWindowEvent(downEvt);

		LOG_INF("Started rubber band drag from position (" + std::to_string(startPos.x) + "," +
			std::to_string(startPos.y) + ")", "BarSpaceContainer");
	}
}

void FlatBarSpaceContainer::UpdateDrag(const wxPoint& currentPos)
{
	if (!m_isDragging) return;

	// Forward motion event to top window (rubber band style)
	wxWindow* topWindow = wxGetTopLevelParent(this);
	if (topWindow) {
		wxPoint screenPos = ClientToScreen(currentPos);
		wxPoint clientPos = topWindow->ScreenToClient(screenPos);

		wxMouseEvent motionEvt(wxEVT_MOTION);
		motionEvt.SetPosition(clientPos);
		motionEvt.SetEventObject(topWindow);
		motionEvt.SetLeftDown(true);
		motionEvt.SetEventType(wxEVT_MOTION);
		topWindow->ProcessWindowEvent(motionEvt);

		LOG_DBG("Rubber band dragging to position (" + std::to_string(currentPos.x) + "," +
			std::to_string(currentPos.y) + ")", "BarSpaceContainer");
	}
}

void FlatBarSpaceContainer::EndDrag()
{
	if (!m_isDragging) return;

	// Forward mouse up event to top window
	wxWindow* topWindow = wxGetTopLevelParent(this);
	if (topWindow) {
		wxPoint currentPos = ScreenToClient(wxGetMousePosition());
		wxPoint screenPos = ClientToScreen(currentPos);
		wxPoint clientPos = topWindow->ScreenToClient(screenPos);

		wxMouseEvent releaseEvt(wxEVT_LEFT_UP);
		releaseEvt.SetPosition(clientPos);
		releaseEvt.SetEventObject(topWindow);
		topWindow->ProcessWindowEvent(releaseEvt);
	}

	m_isDragging = false;

	LOG_INF("Ended rubber band drag", "BarSpaceContainer");
}

bool FlatBarSpaceContainer::IsInDragArea(const wxPoint& pos) const
{
	// Check if position is in tab area or empty space (not on child controls)
	if (m_tabAreaRect.Contains(pos)) {
		return true;
	}

	// Check if position is in empty space between components
	wxSize clientSize = GetClientSize();
	wxRect fullRect(0, 0, clientSize.GetWidth(), clientSize.GetHeight());

	// If not in any child control, consider it a drag area
	wxWindow* childAtPos = GetChildren().GetFirst() ?
		dynamic_cast<wxWindow*>(GetChildren().GetFirst()->GetData()) : nullptr;

	for (wxWindowList::Node* node = GetChildren().GetFirst(); node; node = node->GetNext()) {
		wxWindow* child = node->GetData();
		if (child && child->IsShown() && child->GetRect().Contains(pos)) {
			return false; // Click is on a child control
		}
	}

	return fullRect.Contains(pos); // Click is in empty space
}

bool FlatBarSpaceContainer::HandleTabClick(const wxPoint& pos)
{
	// Check if click is in tab area
	if (!m_tabAreaRect.Contains(pos)) {
		return false;
	}

	FlatUIBar* parentBar = dynamic_cast<FlatUIBar*>(GetParent());
	if (!parentBar || parentBar->GetPageCount() == 0) {
		return false;
	}

	// Check if click is on dropdown button
	if (m_hasTabOverflow && m_tabDropdown && m_tabDropdown->IsDropdownShown()) {
		wxRect dropdownRect = m_tabDropdown->GetDropdownRect();
		if (dropdownRect.Contains(pos)) {
			LOG_INF("Dropdown button clicked", "BarSpaceContainer");
			// The custom dropdown will handle the click automatically
			return true;
		}
	}

	// Calculate which visible tab was clicked
	int currentX = m_tabAreaRect.x;
	int tabPadding = CFG_INT("BarTabPadding");
	int tabSpacing = CFG_INT("BarTabSpacing");

	wxClientDC dc(this);
	dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	// Only check visible tabs
	for (size_t tabIndex : m_visibleTabIndices) {
		FlatUIPage* page = parentBar->GetPage(tabIndex);
		if (!page) continue;

		wxString label = page->GetLabel();
		wxSize labelSize = dc.GetTextExtent(label);
		int tabWidth = CalculateTabWidth(dc, label);

		wxRect tabRect(currentX, m_tabAreaRect.y, tabWidth, m_tabAreaRect.height);

		if (tabRect.Contains(pos)) {
			LOG_INF("Tab " + std::to_string(tabIndex) + " (" + label.ToStdString() + ") clicked", "BarSpaceContainer");

			// Use EventDispatcher to handle tab click properly for both pinned and unpinned states
			if (parentBar) {
				if (parentBar->IsBarPinned()) {
					// Pinned state: use SetActivePage directly
					parentBar->SetActivePage(tabIndex);
				}
				else {
					// Unpinned state: manually handle like EventDispatcher does
					FlatUIBarStateManager* stateManager = parentBar->GetStateManager();
					if (stateManager) {
						stateManager->SetActiveFloatingPage(tabIndex);
						stateManager->SetActivePage(tabIndex);

						// Show the page in float panel
						FlatUIPage* clickedPage = parentBar->GetPage(tabIndex);
						if (clickedPage) {
							parentBar->ShowPageInFloatPanel(clickedPage);
							LOG_INF("Showed page '" + clickedPage->GetLabel().ToStdString() + "' in float panel for unpinned tab click", "BarSpaceContainer");
						}
					}
				}
			}

			Refresh(); // Refresh to show new active state
			return true;
		}

		currentX += tabWidth + tabSpacing;
	}

	return false;
}

int FlatBarSpaceContainer::CalculateTabWidth(wxDC& dc, const wxString& label) const
{
	wxSize labelSize = dc.GetTextExtent(label);
	int tabPadding = CFG_INT("BarTabPadding");
	return labelSize.GetWidth() + tabPadding * 2;
}

void FlatBarSpaceContainer::UpdateTabOverflow()
{
	FlatUIBar* parentBar = dynamic_cast<FlatUIBar*>(GetParent());
	if (!parentBar || parentBar->GetPageCount() == 0) {
		m_visibleTabIndices.clear();
		m_hiddenTabIndices.clear();
		m_hasTabOverflow = false;
		return;
	}

	m_visibleTabIndices.clear();
	m_hiddenTabIndices.clear();

	wxClientDC dc(this);
	dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));

	int tabSpacing = CFG_INT("BarTabSpacing");
	int availableWidth = m_tabAreaRect.width;
	int currentWidth = 0;
	bool needsDropdown = false;

	// First pass: determine if we need a dropdown button
	for (size_t i = 0; i < parentBar->GetPageCount(); ++i) {
		FlatUIPage* page = parentBar->GetPage(i);
		if (!page) continue;

		int tabWidth = CalculateTabWidth(dc, page->GetLabel());
		if (i > 0) currentWidth += tabSpacing;

		if (currentWidth + tabWidth > availableWidth) {
			needsDropdown = true;
			break;
		}

		currentWidth += tabWidth;
	}

	m_hasTabOverflow = needsDropdown;

	// Second pass: determine visible tabs (accounting for dropdown button space)
	if (needsDropdown) {
		const int DROPDOWN_SPACING = 2; // Smaller spacing between last tab and dropdown button
		availableWidth -= (DROPDOWN_BUTTON_WIDTH + DROPDOWN_SPACING);
	}

	currentWidth = 0;
	for (size_t i = 0; i < parentBar->GetPageCount(); ++i) {
		FlatUIPage* page = parentBar->GetPage(i);
		if (!page) continue;

		int tabWidth = CalculateTabWidth(dc, page->GetLabel());
		if (i > 0) currentWidth += tabSpacing;

		if (currentWidth + tabWidth <= availableWidth) {
			m_visibleTabIndices.push_back(i);
			currentWidth += tabWidth;
		}
		else {
			m_hiddenTabIndices.push_back(i);
		}
	}

	// Update tab dropdown component
	if (m_tabDropdown) {
		m_tabDropdown->SetParentBar(parentBar);
		m_tabDropdown->UpdateHiddenTabs(m_hiddenTabIndices);

		if (!m_hiddenTabIndices.empty()) {
			// Calculate the actual end position of the last visible tab
			int lastTabEnd = m_tabAreaRect.x;
			int tabSpacing = CFG_INT("BarTabSpacing");

			for (size_t i = 0; i < m_visibleTabIndices.size(); ++i) {
				size_t tabIndex = m_visibleTabIndices[i];
				FlatUIPage* page = parentBar->GetPage(tabIndex);
				if (page) {
					wxClientDC dc(this);
					dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
					int tabWidth = CalculateTabWidth(dc, page->GetLabel());

					if (i > 0) lastTabEnd += tabSpacing;
					lastTabEnd += tabWidth;
				}
			}

			// Position the dropdown button right after the last visible tab with minimal spacing
			// Use same height as tab area to align with tabs
			const int DROPDOWN_SPACING = 2;
			wxRect dropdownRect(lastTabEnd + DROPDOWN_SPACING,
				m_tabAreaRect.y, DROPDOWN_BUTTON_WIDTH, m_tabAreaRect.height);
			m_tabDropdown->SetDropdownRect(dropdownRect);
		}
	}
	else {
		LOG_ERR("Tab dropdown component is not initialized", "BarSpaceContainer");
	}
}

std::vector<size_t> FlatBarSpaceContainer::GetVisibleTabIndices() const
{
	return m_visibleTabIndices;
}

std::vector<size_t> FlatBarSpaceContainer::GetHiddenTabIndices() const
{
	return m_hiddenTabIndices;
}

void FlatBarSpaceContainer::RefreshTheme() {
	// Update control properties
	SetFont(CFG_DEFAULTFONT());
	SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));

	// Update child components
	if (m_homeSpace) {
		m_homeSpace->RefreshTheme();
	}

	if (m_systemButtons) {
		m_systemButtons->RefreshTheme();
	}

	if (m_functionSpace) {
		m_functionSpace->RefreshTheme();
	}

	if (m_profileSpace) {
		m_profileSpace->RefreshTheme();
	}

	if (m_tabDropdown) {
		m_tabDropdown->SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));
	}

	// Note: Refresh is handled by parent frame for performance
}