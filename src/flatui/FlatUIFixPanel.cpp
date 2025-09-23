#include "flatui/FlatUIFixPanel.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIUnpinButton.h"
#include "logger/Logger.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>

wxBEGIN_EVENT_TABLE(FlatUIFixPanel, wxPanel)
EVT_SIZE(FlatUIFixPanel::OnSize)
EVT_PAINT(FlatUIFixPanel::OnPaint)
EVT_BUTTON(wxID_BACKWARD, FlatUIFixPanel::OnScrollLeft)
EVT_BUTTON(wxID_FORWARD, FlatUIFixPanel::OnScrollRight)
wxEND_EVENT_TABLE()

FlatUIFixPanel::FlatUIFixPanel(wxWindow* parent, wxWindowID id)
	: wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
	m_activePageIndex(wxNOT_FOUND),
	m_unpinButton(nullptr),
	m_scrollingEnabled(false),
	m_scrollContainer(nullptr),
	m_leftScrollButton(nullptr),
	m_rightScrollButton(nullptr),
	m_scrollOffset(0),
	m_scrollStep(50),
	m_mainSizer(nullptr),
	m_scrollSizer(nullptr)
{
	SetName("FlatUIFixPanel");
	SetDoubleBuffered(true);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

#ifdef __WXMSW__
	HWND hwnd = (HWND)GetHandle();
	if (hwnd) {
		long exStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
		::SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED);
	}
#endif

	// Create main sizer for scroll layout
	m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(m_mainSizer);

	// Create scroll container with clipping enabled
	m_scrollContainer = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN | wxBORDER_NONE);
	m_scrollContainer->SetName("FixPanelScrollContainer");
	m_scrollContainer->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
	m_scrollContainer->SetCanFocus(false);

	// Create scroll sizer for content
	m_scrollSizer = new wxBoxSizer(wxHORIZONTAL);
	m_scrollContainer->SetSizer(m_scrollSizer);

	// Add scroll container to main sizer (leave 1 pixel at bottom for red border)
	m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND | wxBOTTOM, 1);

	// Create unpin button
	m_unpinButton = new FlatUIUnpinButton(this, wxID_ANY);
	m_unpinButton->SetName("FixPanelUnpinButton");
	m_unpinButton->SetDoubleBuffered(true);
	m_unpinButton->Show(true); // Initially shown when fix panel is visible

	// Ensure unpin button is on top by default
	m_unpinButton->Raise();

	// Create scroll controls (initially hidden)
	CreateScrollControls();

	LOG_INF("Created FlatUIFixPanel with scroll functionality", "FlatUIFixPanel");
}

FlatUIFixPanel::~FlatUIFixPanel()
{
	// Pages are owned by their creator, don't delete them here
	// Just clear the vector
	m_pages.clear();

	if (m_unpinButton) {
		m_unpinButton->Destroy();
		m_unpinButton = nullptr;
	}

	LOG_INF("Destroyed FlatUIFixPanel", "FlatUIFixPanel");
}

void FlatUIFixPanel::AddPage(FlatUIPage* page)
{
	if (!page) {
		return;
	}

	// Check if page already exists to avoid duplicates
	for (auto* existingPage : m_pages) {
		if (existingPage == page) {
			LOG_DBG("Page '" + page->GetLabel().ToStdString() + "' already exists in FixPanel, skipping", "FlatUIFixPanel");
			return;
		}
	}

	// Reparent the page to this fix panel
	page->Reparent(this);
	m_pages.push_back(page);

	// Hide the page initially
	page->Hide();

	// If this is the first page, make it active
	if (m_pages.size() == 1) {
		SetActivePage(static_cast<size_t>(0));
	}

	LOG_INF("Added page '" + page->GetLabel().ToStdString() + "' to FixPanel", "FlatUIFixPanel");
	RecalculateSize();
}

void FlatUIFixPanel::SetActivePage(size_t pageIndex)
{
	if (pageIndex >= m_pages.size()) {
		return;
	}

	// Quick exit if already active
	if (m_activePageIndex == pageIndex && m_activePageIndex < m_pages.size() &&
		m_pages[m_activePageIndex] && m_pages[m_activePageIndex]->IsShown()) {
		return;
	}

	// Batch all page changes to minimize visual updates
	Freeze();

	try {
		// Hide current active page
		if (m_activePageIndex < m_pages.size() && m_pages[m_activePageIndex]) {
			m_pages[m_activePageIndex]->SetActive(false);
			m_pages[m_activePageIndex]->Hide();
		}

		// Show new active page
		m_activePageIndex = pageIndex;
		FlatUIPage* newActivePage = m_pages[m_activePageIndex];
		if (newActivePage) {
			newActivePage->SetActive(true);
			newActivePage->Show();

			// Reset scroll position when changing pages
			m_scrollOffset = 0;

			PositionActivePage();

			LOG_INF("Set active page to '" + newActivePage->GetLabel().ToStdString() + "'", "FlatUIFixPanel");
		}
	}
	catch (...) {
		Thaw();
		throw;
	}

	Thaw();

	// Defer layout update to avoid multiple calls
	CallAfter([this]() {
		UpdateLayout();
		});
}

void FlatUIFixPanel::SetActivePage(FlatUIPage* page)
{
	if (!page) {
		return;
	}

	for (size_t i = 0; i < m_pages.size(); ++i) {
		if (m_pages[i] == page) {
			SetActivePage(static_cast<size_t>(i));
			return;
		}
	}
}

FlatUIPage* FlatUIFixPanel::GetActivePage() const
{
	if (m_activePageIndex < m_pages.size()) {
		return m_pages[m_activePageIndex];
	}
	return nullptr;
}

FlatUIPage* FlatUIFixPanel::GetPage(size_t index) const
{
	if (index < m_pages.size()) {
		return m_pages[index];
	}
	return nullptr;
}

void FlatUIFixPanel::UpdateLayout()
{
	if (!IsShown()) {
		return;
	}

	// Only update if there are actual changes needed
	FlatUIPage* activePage = GetActivePage();
	if (!activePage) {
		return;
	}

	Freeze();

	PositionActivePage();
	PositionUnpinButton();
	UpdateScrollButtons();

	Thaw();

	// Use deferred refresh to batch multiple layout updates
	CallAfter([this]() {
		if (IsShown()) {
			Refresh(false);
		}
		});
}

void FlatUIFixPanel::RecalculateSize()
{
	if (m_pages.empty()) {
		SetMinSize(wxSize(100, 60));
		return;
	}

	// Calculate the maximum size needed by any page
	wxSize maxSize(0, 0);
	for (auto* page : m_pages) {
		if (page) {
			wxSize pageSize = page->GetBestSize();
			maxSize.SetWidth(wxMax(maxSize.GetWidth(), pageSize.GetWidth()));
			maxSize.SetHeight(wxMax(maxSize.GetHeight(), pageSize.GetHeight()));
		}
	}

	SetMinSize(maxSize);
	LOG_INF("RecalculateSize: FixPanel size set to (" +
		std::to_string(maxSize.GetWidth()) + "," +
		std::to_string(maxSize.GetHeight()) + ")", "FlatUIFixPanel");
}

void FlatUIFixPanel::ShowUnpinButton(bool show)
{
	if (m_unpinButton) {
		m_unpinButton->Show(show);
		if (show) {
			PositionUnpinButton();
			m_unpinButton->Raise(); // Ensure it's on top when shown
		}
		LOG_INF("Unpin button " + std::string(show ? "shown" : "hidden"), "FlatUIFixPanel");
	}
}

wxSize FlatUIFixPanel::DoGetBestSize() const
{
	if (m_pages.empty()) {
		return wxSize(100, 60);
	}

	// Return the size of the largest page
	wxSize maxSize(0, 0);
	for (const auto* page : m_pages) {
		if (page) {
			wxSize pageSize = page->GetBestSize();
			maxSize.SetWidth(wxMax(maxSize.GetWidth(), pageSize.GetWidth()));
			maxSize.SetHeight(wxMax(maxSize.GetHeight(), pageSize.GetHeight()));
		}
	}

	return maxSize;
}

void FlatUIFixPanel::OnSize(wxSizeEvent& event)
{
	// Use debounced layout update to prevent excessive refreshes during resize
	static wxTimer* resizeTimer = nullptr;
	if (!resizeTimer) {
		resizeTimer = new wxTimer(this, wxID_ANY);
		resizeTimer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
			UpdateLayout();
		});
	}
	
	// Stop previous timer and start new one with 50ms delay
	resizeTimer->Stop();
	resizeTimer->StartOnce(50);

	event.Skip();
}

void FlatUIFixPanel::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);
	wxSize size = GetSize();
	wxPoint position = GetPosition();

	// Debug logging to track paint events
	static int paintCount = 0;
	paintCount++;
	LOG_INF("FlatUIFixPanel::OnPaint #" + std::to_string(paintCount) + 
		": position=(" + std::to_string(position.x) + "," + std::to_string(position.y) + 
		"), size=(" + std::to_string(size.GetWidth()) + "," + std::to_string(size.GetHeight()) + ")", "FlatUIFixPanel");

	// Fill background with white
	dc.SetBackground(wxBrush(CFG_COLOUR("ScrolledWindowBgColour")));
	dc.Clear();

	// Draw border around the entire panel
	dc.SetPen(wxPen(CFG_COLOUR("BarBorderColour"), 1));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawLine(0, size.GetHeight() - 1, size.GetWidth(), size.GetHeight() - 1);

	// CRITICAL FIX: Do NOT call event.Skip() here to prevent paint event propagation
	// that causes infinite loop between FlatUIBar and FlatUIFixPanel
	// event.Skip(); // REMOVED to prevent dead loop
}

void FlatUIFixPanel::PositionUnpinButton()
{
	if (!m_unpinButton || !m_unpinButton->IsShown()) {
		return;
	}

	wxSize panelSize = GetSize();
	wxSize buttonSize = m_unpinButton->GetBestSize();

	// Position at bottom-right corner with small margin
	int margin = 2;
	wxPoint buttonPos(
		panelSize.GetWidth() - buttonSize.GetWidth() - margin,
		panelSize.GetHeight() - buttonSize.GetHeight() - margin
	);

	m_unpinButton->SetPosition(buttonPos);
	m_unpinButton->SetSize(buttonSize);

	// Ensure button is on top of other controls
	m_unpinButton->Raise();

	LOG_DBG("Positioned unpin button at (" +
		std::to_string(buttonPos.x) + "," +
		std::to_string(buttonPos.y) + ") and raised to top", "FlatUIFixPanel");
}

void FlatUIFixPanel::PositionActivePage()
{
	FlatUIPage* activePage = GetActivePage();
	if (!activePage || !m_scrollContainer) {
		return;
	}

	// Reparent page to scroll container if needed
	if (activePage->GetParent() != m_scrollContainer) {
		activePage->Reparent(m_scrollContainer);
	}

	// Get page size before any positioning
	wxSize pageSize = activePage->GetBestSize();
	wxSize currentContainerSize = m_scrollContainer->GetSize();

	// Determine if we need scrolling based on current container size
	bool needsScrolling = pageSize.GetWidth() > currentContainerSize.GetWidth();

	// Update scroll layout first
	if (needsScrolling) {
		EnableScrolling(true);

		// Adjust main sizer to make room for scroll buttons
		m_mainSizer->Clear();
		m_mainSizer->Add(m_leftScrollButton, 0, wxEXPAND | wxBOTTOM, 1);
		m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND | wxBOTTOM, 1);
		// Right scroll button needs both right margin (6px) and bottom margin (1px)
		// We need to use a sub-sizer to handle different margin values
		wxBoxSizer* rightButtonSizer = new wxBoxSizer(wxHORIZONTAL);
		rightButtonSizer->Add(m_rightScrollButton, 1, wxEXPAND | wxBOTTOM, 1);
		m_mainSizer->Add(rightButtonSizer, 0, wxEXPAND | wxRIGHT, 16);

		// Force layout update to get correct container size
		Layout();
	}
	else {
		EnableScrolling(false);

		// Use full space for scroll container (leave 1 pixel at bottom for red border)
		m_mainSizer->Clear();
		m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND | wxBOTTOM, 1);

		// Force layout update
		Layout();
	}

	// Get updated container size after layout
	wxSize containerSize = m_scrollContainer->GetSize();

	// Position page in scroll container with scroll offset
	// Ensure page position is never negative to prevent blank areas in top-left corner
	wxPoint pagePos(wxMax(0, -m_scrollOffset), 0);
	
	// Debug logging to track page positioning
	static int positionCount = 0;
	positionCount++;
	LOG_INF("FlatUIFixPanel::PositionActivePage #" + std::to_string(positionCount) + 
		": m_scrollOffset=" + std::to_string(m_scrollOffset) + 
		", pagePos=(" + std::to_string(pagePos.x) + "," + std::to_string(pagePos.y) + 
		"), containerSize=(" + std::to_string(containerSize.GetWidth()) + "," + std::to_string(containerSize.GetHeight()) + ")", "FlatUIFixPanel");
	
	activePage->SetPosition(pagePos);
	// Reduce page height by 1 pixel to create visual separation
	int adjustedHeight = containerSize.GetHeight() - 1;
	activePage->SetSize(wxSize(pageSize.GetWidth(), adjustedHeight));
	activePage->Layout();

	// Debug logging for heights
	wxSize fixPanelSize = GetSize();
	LOG_DBG("Height debug - FixPanel: " + std::to_string(fixPanelSize.GetHeight()) +
		", ScrollContainer: " + std::to_string(containerSize.GetHeight()) +
		", PageBestSize: " + std::to_string(pageSize.GetHeight()) +
		", PageSetSize: " + std::to_string(adjustedHeight), "FlatUIFixPanel");

	// Ensure unpin button is on top by raising it after positioning the page
	if (m_unpinButton && m_unpinButton->IsShown()) {
		m_unpinButton->Raise();
	}

	UpdateScrollButtons();

	LOG_DBG("Positioned active page '" + activePage->GetLabel().ToStdString() +
		"' in scroll container with offset " + std::to_string(m_scrollOffset), "FlatUIFixPanel");
}

void FlatUIFixPanel::HideAllPages()
{
	for (auto* page : m_pages) {
		if (page && page->IsShown()) {
			page->Hide();
		}
	}
}

void FlatUIFixPanel::ClearContent()
{
	LOG_INF("Clearing FixPanel content", "FlatUIFixPanel");

	Freeze();

	// Hide and deactivate all pages
	for (auto* page : m_pages) {
		if (page) {
			page->SetActive(false);
			page->Hide();
			// Don't destroy the page, just reparent it back to its original parent if needed
		}
	}

	// Clear the pages vector (pages are owned by their original creators)
	m_pages.clear();

	// Reset active page index
	m_activePageIndex = wxNOT_FOUND;

	// Hide unpin button
	if (m_unpinButton) {
		m_unpinButton->Hide();
	}

	Thaw();

	LOG_INF("FixPanel content cleared", "FlatUIFixPanel");
}

void FlatUIFixPanel::ResetState()
{
	LOG_INF("Resetting FixPanel state", "FlatUIFixPanel");

	Freeze();

	// Hide all pages and deactivate them
	for (auto* page : m_pages) {
		if (page) {
			page->SetActive(false);
			page->Hide();
		}
	}

	// Reset active page index but keep pages
	m_activePageIndex = wxNOT_FOUND;

	// Hide unpin button
	if (m_unpinButton) {
		m_unpinButton->Hide();
	}

	// Reset scroll position
	m_scrollOffset = 0;
	UpdateScrollButtons();

	Thaw();

	LOG_INF("FixPanel state reset", "FlatUIFixPanel");
}

void FlatUIFixPanel::EnableScrolling(bool enable)
{
	if (m_scrollingEnabled == enable) {
		return;
	}

	m_scrollingEnabled = enable;
	UpdateScrollButtons();
	UpdateLayout();

	LOG_INF("Scrolling " + std::string(enable ? "enabled" : "disabled"), "FlatUIFixPanel");
}

void FlatUIFixPanel::ScrollLeft()
{
	if (!m_scrollingEnabled || !NeedsScrolling()) {
		return;
	}

	m_scrollOffset = wxMax(0, m_scrollOffset - m_scrollStep);
	UpdateScrollPosition();
	UpdateScrollButtons();

	LOG_DBG("Scrolled left, offset: " + std::to_string(m_scrollOffset), "FlatUIFixPanel");
}

void FlatUIFixPanel::ScrollRight()
{
	if (!m_scrollingEnabled || !NeedsScrolling()) {
		return;
	}

	FlatUIPage* activePage = GetActivePage();
	if (!activePage) {
		return;
	}

	wxSize pageSize = activePage->GetBestSize();
	wxSize containerSize = m_scrollContainer->GetSize();
	int maxOffset = wxMax(0, pageSize.GetWidth() - containerSize.GetWidth());

	m_scrollOffset = wxMin(maxOffset, m_scrollOffset + m_scrollStep);
	UpdateScrollPosition();
	UpdateScrollButtons();

	LOG_DBG("Scrolled right, offset: " + std::to_string(m_scrollOffset), "FlatUIFixPanel");
}

void FlatUIFixPanel::UpdateScrollButtons()
{
	if (!m_leftScrollButton || !m_rightScrollButton) {
		return;
	}

	bool needsScrolling = NeedsScrolling();

	if (needsScrolling && m_scrollingEnabled) {
		m_leftScrollButton->Show();
		m_rightScrollButton->Show();

		// Enable/disable based on scroll position
		m_leftScrollButton->Enable(m_scrollOffset > 0);

		FlatUIPage* activePage = GetActivePage();
		if (activePage) {
			wxSize pageSize = activePage->GetBestSize();
			wxSize containerSize = m_scrollContainer->GetSize();
			int maxOffset = wxMax(0, pageSize.GetWidth() - containerSize.GetWidth());
			m_rightScrollButton->Enable(m_scrollOffset < maxOffset);
		}
	}
	else {
		m_leftScrollButton->Hide();
		m_rightScrollButton->Hide();
	}

	Layout();
}

void FlatUIFixPanel::CreateScrollControls()
{
	// Create left scroll button with custom border
	m_leftScrollButton = new wxButton(this, wxID_BACKWARD, "<",
		wxDefaultPosition, wxSize(16, -1), wxBORDER_NONE);
	m_leftScrollButton->SetName("LeftScrollButton");
	m_leftScrollButton->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
	m_leftScrollButton->Hide();

	// Create right scroll button with custom border
	m_rightScrollButton = new wxButton(this, wxID_FORWARD, ">",
		wxDefaultPosition, wxSize(16, -1), wxBORDER_NONE);
	m_rightScrollButton->SetName("RightScrollButton");
	m_rightScrollButton->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
	m_rightScrollButton->Hide();

	// Bind paint events for custom border drawing
	m_leftScrollButton->Bind(wxEVT_PAINT, [this](wxPaintEvent& event) {
		wxPaintDC dc(m_leftScrollButton);
		wxSize size = m_leftScrollButton->GetSize();

		// Clear background
		dc.SetBackground(wxBrush(m_leftScrollButton->GetBackgroundColour()));
		dc.Clear();

		// Draw button text
		dc.SetTextForeground(CFG_COLOUR("DefaultTextColour"));
		wxString text = "<";
		wxSize textSize = dc.GetTextExtent(text);
		int x = (size.GetWidth() - textSize.GetWidth()) / 2;
		int y = (size.GetHeight() - textSize.GetHeight()) / 2;
		dc.DrawText(text, x, y);

		// Draw right border
		dc.SetPen(wxPen(CFG_COLOUR("PanelBorderColour"), 1));
		dc.DrawLine(size.GetWidth() - 1, 0, size.GetWidth() - 1, size.GetHeight());
		});

	m_rightScrollButton->Bind(wxEVT_PAINT, [this](wxPaintEvent& event) {
		wxPaintDC dc(m_rightScrollButton);
		wxSize size = m_rightScrollButton->GetSize();

		// Clear background
		dc.SetBackground(wxBrush(m_rightScrollButton->GetBackgroundColour()));
		dc.Clear();

		// Draw button text
		dc.SetTextForeground(CFG_COLOUR("DefaultTextColour"));
		wxString text = ">";
		wxSize textSize = dc.GetTextExtent(text);
		int x = (size.GetWidth() - textSize.GetWidth()) / 2;
		int y = (size.GetHeight() - textSize.GetHeight()) / 2;
		dc.DrawText(text, x, y);

		// Draw left border
		dc.SetPen(wxPen(CFG_COLOUR("PanelBorderColour"), 1));
		dc.DrawLine(0, 0, 0, size.GetHeight());
		});

	LOG_INF("Created scroll control buttons with custom borders", "FlatUIFixPanel");
}

void FlatUIFixPanel::UpdateScrollPosition()
{
	FlatUIPage* activePage = GetActivePage();
	if (!activePage || !m_scrollContainer) {
		return;
	}

	// Move the page within the scroll container based on scroll offset
	// Ensure page position is never negative to prevent blank areas in top-left corner
	wxPoint newPos(wxMax(0, -m_scrollOffset), 0);
	
	// Debug logging to track scroll position updates
	static int scrollUpdateCount = 0;
	scrollUpdateCount++;
	LOG_INF("FlatUIFixPanel::UpdateScrollPosition #" + std::to_string(scrollUpdateCount) + 
		": m_scrollOffset=" + std::to_string(m_scrollOffset) + 
		", newPos=(" + std::to_string(newPos.x) + "," + std::to_string(newPos.y) + ")", "FlatUIFixPanel");
	
	activePage->SetPosition(newPos);

	// Optimized layout and refresh to prevent excessive redraws
	activePage->Layout();
	
	// Use deferred refresh instead of immediate Update() to prevent loops
	m_scrollContainer->Layout();
	
	// Only refresh the specific areas that need updating
	activePage->Refresh(false); // Use false to avoid erasing background
	m_scrollContainer->Refresh(false);
	
	// Defer panel refresh to prevent immediate redraw loops
	CallAfter([this]() {
		Refresh(false);
	});
}

bool FlatUIFixPanel::NeedsScrolling() const
{
	FlatUIPage* activePage = GetActivePage();
	if (!activePage || !m_scrollContainer) {
		return false;
	}

	wxSize pageSize = activePage->GetBestSize();
	wxSize containerSize = m_scrollContainer->GetSize();

	return pageSize.GetWidth() > containerSize.GetWidth();
}

void FlatUIFixPanel::OnScrollLeft(wxCommandEvent& event)
{
	ScrollLeft();
	event.Skip();
}

void FlatUIFixPanel::OnScrollRight(wxCommandEvent& event)
{
	ScrollRight();
	event.Skip();
}