#include "flatui/FlatUIFloatPanel.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIPinButton.h"
#include "logger/Logger.h"
#include "config/ThemeManager.h"
// Define the custom event
wxDEFINE_EVENT(wxEVT_FLOAT_PANEL_DISMISSED, wxCommandEvent);

wxBEGIN_EVENT_TABLE(FlatUIFloatPanel, wxFrame)
EVT_PAINT(FlatUIFloatPanel::OnPaint)
EVT_SIZE(FlatUIFloatPanel::OnSize)
EVT_ENTER_WINDOW(FlatUIFloatPanel::OnMouseEnter)
EVT_LEAVE_WINDOW(FlatUIFloatPanel::OnMouseLeave)
EVT_ACTIVATE(FlatUIFloatPanel::OnActivate)
EVT_KILL_FOCUS(FlatUIFloatPanel::OnKillFocus)
EVT_TIMER(wxID_ANY, FlatUIFloatPanel::OnAutoHideTimer)
EVT_COMMAND(wxID_ANY, wxEVT_PIN_BUTTON_CLICKED, FlatUIFloatPanel::OnPinButtonClicked)
EVT_BUTTON(wxID_BACKWARD, FlatUIFloatPanel::OnScrollLeft)
EVT_BUTTON(wxID_FORWARD, FlatUIFloatPanel::OnScrollRight)
wxEND_EVENT_TABLE()

FlatUIFloatPanel::FlatUIFloatPanel(wxWindow* parent)
	: wxFrame(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE),
	m_currentPage(nullptr),
	m_contentPanel(nullptr),
	m_parentWindow(parent),
	m_pinButton(nullptr),
	m_autoHideTimer(this),
	m_borderWidth(0),
	m_shadowOffset(0),
	m_scrollingEnabled(false),
	m_scrollContainer(nullptr),
	m_leftScrollButton(nullptr),
	m_rightScrollButton(nullptr),
	m_scrollOffset(0),
	m_scrollStep(50),
	m_mainSizer(nullptr),
	m_scrollSizer(nullptr)
{
	SetName("FlatUIFloatPanel");

	// Create content panel
	m_contentPanel = new wxPanel(this, wxID_ANY);
	m_contentPanel->SetName("FloatPanelContent");

	// Create main sizer for content panel
	m_mainSizer = new wxBoxSizer(wxHORIZONTAL);
	m_contentPanel->SetSizer(m_mainSizer);

	// Create scroll container
	m_scrollContainer = new wxPanel(m_contentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxCLIP_CHILDREN);
	m_scrollContainer->SetName("FloatPanelScrollContainer");
	m_scrollContainer->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
	// Enable clipping to ensure content outside the container is not visible
	m_scrollContainer->SetCanFocus(false);

	// Create scroll sizer for page content (horizontal for proper scrolling)
	m_scrollSizer = new wxBoxSizer(wxHORIZONTAL);
	m_scrollContainer->SetSizer(m_scrollSizer);

	// Add scroll container to main sizer (initially takes full space)
	m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND);

	// Create sizer for layout (this will be used for the page content)
	m_sizer = m_scrollSizer;

	// Create pin button for the float panel - make it a child of the content panel to avoid overlap
	m_pinButton = new FlatUIPinButton(m_contentPanel, wxID_ANY);
	m_pinButton->SetName("FloatPanelPinButton");
	m_pinButton->Show(false); // Initially hidden, will be shown when float panel is displayed
	// Ensure pin button has proper layering and visibility settings
	m_pinButton->SetCanFocus(false); // Prevent focus issues
	// Ensure pin button is always on top within its parent
	m_pinButton->SetWindowStyleFlag(m_pinButton->GetWindowStyleFlag() | wxSTAY_ON_TOP);
	LOG_INF("Created pin button for float panel as child of content panel, initially hidden", "FlatUIFloatPanel");

	// Create scroll controls (initially hidden)
	CreateScrollControls();

	// Setup appearance and event handlers
	SetupAppearance();
	SetupEventHandlers();

	// Main frame sizer
	wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
	frameSizer->Add(m_contentPanel, 1, wxEXPAND | wxALL, m_borderWidth);
	SetSizer(frameSizer);
	SetDoubleBuffered(true);
	LOG_INF("Created FlatUIFloatPanel with scroll functionality", "FlatUIFloatPanel");
}

FlatUIFloatPanel::~FlatUIFloatPanel()
{
	StopAutoHideTimer();

	if (m_currentPage) {
		m_sizer->Detach(m_currentPage);
		m_currentPage = nullptr;
	}

	if (m_pinButton) {
		m_pinButton->Destroy();
		m_pinButton = nullptr;
	}

	LOG_INF("Destroyed FlatUIFloatPanel", "FlatUIFloatPanel");
}

void FlatUIFloatPanel::SetupAppearance()
{
	// Set colors based on configuration or defaults
	m_borderColour = CFG_COLOUR("BarBorderColour");
	m_backgroundColour = CFG_COLOUR("ScrolledWindowBgColour");
	m_shadowColour = CFG_COLOUR("FloatPanelShadowColour");
	m_borderWidth = 1; // Ensure border width is set

	// Always use white background
	SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
	m_contentPanel->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
}

void FlatUIFloatPanel::SetupEventHandlers()
{
	// Setup global mouse tracking for auto-hide
	if (wxWindow* topLevel = wxGetTopLevelParent(m_parentWindow)) {
		topLevel->Bind(wxEVT_MOTION, &FlatUIFloatPanel::OnGlobalMouseMove, this);
	}
}

void FlatUIFloatPanel::SetPageContent(FlatUIPage* page)
{
	if (m_currentPage == page) {
		return; // Same page, no change needed
	}

	// Remove current page if any
	if (m_currentPage) {
		m_sizer->Detach(m_currentPage);
		m_currentPage->Reparent(m_parentWindow);
		m_currentPage->Hide();
	}

	// Set new page
	m_currentPage = page;
	if (m_currentPage) {
		m_currentPage->Reparent(m_scrollContainer);
		// Don't use sizer for page positioning - we need direct control for scrolling
		m_currentPage->Show();

		// Get page size for scroll calculation
		wxSize pageSize = m_currentPage->GetBestSize();
		wxSize currentContainerSize = m_scrollContainer->GetSize();

		// Determine if we need scrolling first
		bool needsScrolling = pageSize.GetWidth() > currentContainerSize.GetWidth();

		// Update layout first
		if (needsScrolling) {
			EnableScrolling(true);

			// Adjust main sizer to make room for scroll buttons
			m_mainSizer->Clear();
			m_mainSizer->Add(m_leftScrollButton, 0, wxEXPAND);
			m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND);
			m_mainSizer->Add(m_rightScrollButton, 0, wxEXPAND | wxRIGHT, 12);

			// Force layout update
			m_contentPanel->Layout();
		}
		else {
			EnableScrolling(false);

			// Use full space for scroll container
			m_mainSizer->Clear();
			m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND);

			// Force layout update
			m_contentPanel->Layout();
		}

		// Set position and size for the page after layout
		wxSize containerSize = m_scrollContainer->GetSize();
		int pageWidth = pageSize.GetWidth();
		int pageHeight = wxMax(pageSize.GetHeight(), containerSize.GetHeight());
		m_currentPage->SetPosition(wxPoint(0, 0));
		m_currentPage->SetSize(pageWidth, pageHeight);

		// Update layout
		m_contentPanel->Layout();
		Layout();

		UpdateScrollButtons();

		// Force refresh to ensure proper redraw
		m_contentPanel->Refresh();
		m_contentPanel->Update();
		Refresh();
		Update();

		LOG_INF("Set page content: " + m_currentPage->GetLabel().ToStdString(), "FlatUIFloatPanel");

		// Ensure pin button is visible and on top after all layout operations
		if (m_pinButton) {
			// Position pin button correctly
			wxSize contentSize = m_contentPanel->GetSize();
			wxSize pinSize = m_pinButton->GetBestSize();
			int margin = 2;

			int pinX = wxMax(0, contentSize.GetWidth() - pinSize.GetWidth() - margin);
			int pinY = wxMax(0, contentSize.GetHeight() - pinSize.GetHeight() - margin);

			m_pinButton->SetPosition(wxPoint(pinX, pinY));
			m_pinButton->SetSize(pinSize);
			m_pinButton->Show(true);
			m_pinButton->Enable(true);
			m_pinButton->Raise();

			// Force immediate update of pin button
			m_pinButton->Update();
			m_pinButton->Refresh();

			LOG_INF("Pin button repositioned and raised after page content change", "FlatUIFloatPanel");
		}
	}
}

void FlatUIFloatPanel::ShowAt(const wxPoint& position, const wxSize& size)
{
	if (!m_currentPage) {
		LOG_INF("ShowAt: No current page, cannot show float panel", "FlatUIFloatPanel");
		return;
	}

	wxSize panelSize = size;
	if (panelSize == wxDefaultSize) {
		// Calculate size based on page content if no explicit size is given
		wxSize pageSize = m_currentPage->GetBestSize();
		// Add border and shadow space
		pageSize.SetWidth(pageSize.GetWidth() + m_borderWidth * 2 + m_shadowOffset);
		pageSize.SetHeight(pageSize.GetHeight() + m_borderWidth * 2 + m_shadowOffset);
		// Ensure minimum size
		pageSize.SetWidth(wxMax(pageSize.GetWidth(), 300));
		pageSize.SetHeight(wxMax(pageSize.GetHeight(), 100));
		panelSize = pageSize;
	}

	// Check screen boundaries and adjust position if needed
	wxSize screenSize = wxGetDisplaySize();
	wxPoint adjustedPos = position;

	if (adjustedPos.x + panelSize.GetWidth() > screenSize.GetWidth()) {
		adjustedPos.x = screenSize.GetWidth() - panelSize.GetWidth() - 10;
	}
	if (adjustedPos.y + panelSize.GetHeight() > screenSize.GetHeight()) {
		adjustedPos.y = screenSize.GetHeight() - panelSize.GetHeight() - 10;
	}

	// Ensure position is not negative
	adjustedPos.x = wxMax(adjustedPos.x, 0);
	adjustedPos.y = wxMax(adjustedPos.y, 0);

	// Set size and position for the panel
	SetSize(panelSize);
	SetPosition(adjustedPos);

	// Show the panel itself before its children
	Show(true);

	// Update page layout first
	if (m_currentPage) {
		m_currentPage->UpdateLayout();
	}

	// Force a complete layout and refresh cycle first
	Layout();
	Refresh();
	Update();

	// Position and show pin button AFTER all layout operations are complete
	if (m_pinButton) {
		wxSize pinSize = m_pinButton->GetBestSize();
		int margin = 2; // Small margin from edges

		// Since pin button is now a child of content panel, use content panel size
		wxSize contentSize = m_contentPanel->GetSize();
		int pinX = wxMax(0, contentSize.GetWidth() - pinSize.GetWidth() - margin);
		int pinY = wxMax(0, contentSize.GetHeight() - pinSize.GetHeight() - margin);

		m_pinButton->SetPosition(wxPoint(pinX, pinY));
		m_pinButton->SetSize(pinSize);

		// Force pin button to be always visible and on top - AFTER layout
		m_pinButton->Show(true);
		m_pinButton->Enable(true);
		m_pinButton->Raise();

		// Force refresh to ensure pin button is rendered
		m_pinButton->Update();
		m_pinButton->Refresh();

		LOG_INF("Pin button positioned and shown at (" +
			std::to_string(pinX) + ", " + std::to_string(pinY) + ") relative to content panel AFTER layout", "FlatUIFloatPanel");
	}

	// Start auto-hide monitoring
	StartAutoHideTimer();
}

void FlatUIFloatPanel::HidePanel()
{
	if (IsShown()) {
		LOG_INF("HidePanel: Starting to hide float panel", "FlatUIFloatPanel");

		StopAutoHideTimer();

		// Hide pin button first - it should not be visible when panel is hidden
		if (m_pinButton) {
			m_pinButton->Hide();
			LOG_INF("HidePanel: Hidden pin button", "FlatUIFloatPanel");
		}

		// Hide scroll buttons
		if (m_leftScrollButton) {
			m_leftScrollButton->Hide();
		}
		if (m_rightScrollButton) {
			m_rightScrollButton->Hide();
		}

		// Hide the panel
		Hide();

		// Return page to original parent
		if (m_currentPage) {
			m_currentPage->Reparent(m_parentWindow);
			m_currentPage->Hide();
			m_currentPage = nullptr;
			LOG_INF("HidePanel: Returned page to original parent", "FlatUIFloatPanel");
		}

		// Reset scroll state
		m_scrollOffset = 0;
		m_scrollingEnabled = false;

		// Notify parent that float panel was hidden
		if (m_parentWindow) {
			wxCommandEvent event(wxEVT_FLOAT_PANEL_DISMISSED, GetId());
			event.SetEventObject(this);
			wxPostEvent(m_parentWindow, event);
		}

		LOG_INF("Hidden float panel", "FlatUIFloatPanel");
	}
}

void FlatUIFloatPanel::ForceHide()
{
	HidePanel();
}

bool FlatUIFloatPanel::ShouldAutoHide(const wxPoint& globalMousePos) const
{
	if (!IsShown()) {
		return false;
	}

	// Get panel bounds in screen coordinates
	wxRect panelRect = GetScreenRect();

	// Add a small margin around the panel to prevent immediate hiding
	panelRect.Inflate(5, 5);

	// Check if mouse is outside the panel area
	bool mouseOutside = !panelRect.Contains(globalMousePos);

	// Also check if mouse is over the parent bar area
	bool mouseOverParent = false;
	if (m_parentWindow) {
		wxRect parentRect = m_parentWindow->GetScreenRect();
		mouseOverParent = parentRect.Contains(globalMousePos);
	}

	// Should auto-hide if mouse is outside panel and not over parent
	return mouseOutside && !mouseOverParent;
}

void FlatUIFloatPanel::StartAutoHideTimer()
{
	if (!m_autoHideTimer.IsRunning()) {
		m_autoHideTimer.Start(AUTO_HIDE_DELAY_MS);
	}
}

void FlatUIFloatPanel::StopAutoHideTimer()
{
	if (m_autoHideTimer.IsRunning()) {
		m_autoHideTimer.Stop();
	}
}

void FlatUIFloatPanel::CheckAutoHide()
{
	wxPoint mousePos = wxGetMousePosition();
	if (ShouldAutoHide(mousePos)) {
		HidePanel();
	}
}

void FlatUIFloatPanel::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxSize size = GetSize();

	// Fill background with white
	dc.SetBackground(wxBrush(CFG_COLOUR("ScrolledWindowBgColour")));
	dc.Clear();

	// Draw shadow first (behind the panel)
	DrawShadow(dc);

	// Draw custom border (bottom border only)
	DrawCustomBorder(dc);

	event.Skip();
}

void FlatUIFloatPanel::DrawShadow(wxDC& dc)
{
	if (m_shadowOffset <= 0) return;

	wxSize size = GetSize();
	wxRect shadowRect(m_shadowOffset, m_shadowOffset,
		size.GetWidth() - m_shadowOffset,
		size.GetHeight() - m_shadowOffset);

	// Create a simple shadow effect
	dc.SetBrush(wxBrush(m_shadowColour));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(shadowRect);
}

void FlatUIFloatPanel::DrawCustomBorder(wxDC& dc)
{
	if (m_borderWidth <= 0) return;

	wxSize size = GetSize();

	// Calculate bottom border position - subtract shadow offset to keep within bounds
	int borderBottom = size.GetHeight() - m_shadowOffset - 1;
	int borderRight = size.GetWidth() - m_shadowOffset;

	dc.SetPen(wxPen(m_borderColour, m_borderWidth));

	// Draw only bottom border
	dc.DrawLine(0, borderBottom, borderRight, borderBottom);
}

void FlatUIFloatPanel::OnSize(wxSizeEvent& event)
{
	Layout();

	// Update scroll functionality when size changes
	if (m_currentPage) {
		// Get page and current container size
		wxSize pageSize = m_currentPage->GetBestSize();
		wxSize currentContainerSize = m_scrollContainer->GetSize();

		// Determine if we need scrolling
		bool needsScrolling = pageSize.GetWidth() > currentContainerSize.GetWidth();

		if (needsScrolling) {
			EnableScrolling(true);

			// Adjust main sizer to make room for scroll buttons
			m_mainSizer->Clear();
			m_mainSizer->Add(m_leftScrollButton, 0, wxEXPAND);
			m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND);
			m_mainSizer->Add(m_rightScrollButton, 0, wxEXPAND | wxRIGHT, 12);
		}
		else {
			EnableScrolling(false);

			// Use full space for scroll container
			m_mainSizer->Clear();
			m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND);
		}

		UpdateScrollButtons();
		UpdateScrollPosition();

		// Force layout and refresh after scroll changes
		m_contentPanel->Layout();
		m_contentPanel->Refresh();
		m_contentPanel->Update();
	}

	// Reposition and ensure pin button is visible when panel size changes
	if (m_pinButton) {
		wxSize contentSize = m_contentPanel->GetSize();
		wxSize pinSize = m_pinButton->GetBestSize();
		int margin = 4;

		// Ensure pin button is within the content panel bounds
		int maxPinX = contentSize.GetWidth() - pinSize.GetWidth() - margin;
		int maxPinY = contentSize.GetHeight() - pinSize.GetHeight() - margin;

		// Make sure position is not negative
		int pinX = wxMax(0, maxPinX);
		int pinY = wxMax(0, maxPinY);

		m_pinButton->SetPosition(wxPoint(pinX, pinY));

		// Ensure pin button is always visible after repositioning
		m_pinButton->Show(true);
		m_pinButton->Raise();
		m_pinButton->Update();

		LOG_INF("OnSize: Pin button repositioned to (" +
			std::to_string(pinX) + ", " + std::to_string(pinY) + ") relative to content panel", "FlatUIFloatPanel");
	}

	// Force a deferred scroll check for edge cases
	CallAfter([this]() {
		if (m_currentPage && m_scrollContainer) {
			wxSize pageSize = m_currentPage->GetBestSize();
			wxSize containerSize = m_scrollContainer->GetSize();
			bool shouldNeedScrolling = pageSize.GetWidth() > containerSize.GetWidth();

			if (shouldNeedScrolling != m_scrollingEnabled) {
				LOG_INF("OnSize: Correcting scroll state - should be " +
					std::string(shouldNeedScrolling ? "enabled" : "disabled"), "FlatUIFloatPanel");

				// Re-apply correct layout
				if (shouldNeedScrolling) {
					EnableScrolling(true);
					m_mainSizer->Clear();
					m_mainSizer->Add(m_leftScrollButton, 0, wxEXPAND);
					m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND);
					m_mainSizer->Add(m_rightScrollButton, 0, wxEXPAND | wxRIGHT, 12);
				}
				else {
					EnableScrolling(false);
					m_mainSizer->Clear();
					m_mainSizer->Add(m_scrollContainer, 1, wxEXPAND);
				}

				UpdateScrollButtons();
				m_contentPanel->Layout();
				Layout();

				// Ensure pin button is still visible and on top after layout changes
				if (m_pinButton) {
					m_pinButton->Show(true);
					m_pinButton->Raise();
					m_pinButton->Update();
					LOG_INF("OnSize CallAfter: Ensured pin button visibility after scroll state correction", "FlatUIFloatPanel");
				}
			}
		}
		});

	Refresh(); // Redraw custom border and shadow
	event.Skip();
}

void FlatUIFloatPanel::OnMouseEnter(wxMouseEvent& event)
{
	StopAutoHideTimer();
	event.Skip();
}

void FlatUIFloatPanel::OnMouseLeave(wxMouseEvent& event)
{
	StartAutoHideTimer();
	event.Skip();
}

void FlatUIFloatPanel::OnActivate(wxActivateEvent& event)
{
	if (!event.GetActive()) {
		// Panel is being deactivated, start auto-hide timer
		StartAutoHideTimer();
	}
	else {
		// Panel is being activated, stop auto-hide timer
		StopAutoHideTimer();
	}
	event.Skip();
}

void FlatUIFloatPanel::OnKillFocus(wxFocusEvent& event)
{
	StartAutoHideTimer();
	event.Skip();
}

void FlatUIFloatPanel::OnAutoHideTimer(wxTimerEvent& event)
{
	CheckAutoHide();
}

void FlatUIFloatPanel::OnGlobalMouseMove(wxMouseEvent& event)
{
	if (IsShown()) {
		wxPoint globalPos = wxGetMousePosition();
		if (ShouldAutoHide(globalPos)) {
			StartAutoHideTimer();
		}
		else {
			StopAutoHideTimer();
		}
	}
	event.Skip();
}

void FlatUIFloatPanel::OnPinButtonClicked(wxCommandEvent& event)
{
	// Forward the pin button click to the parent window (FlatUIBar)
	if (m_parentWindow) {
		wxCommandEvent pinEvent(wxEVT_PIN_BUTTON_CLICKED, GetId());
		pinEvent.SetEventObject(this);
		wxPostEvent(m_parentWindow, pinEvent);

		LOG_INF("Pin button clicked in float panel, forwarding to parent", "FlatUIFloatPanel");
	}

	event.Skip();
}

void FlatUIFloatPanel::EnableScrolling(bool enable)
{
	if (m_scrollingEnabled == enable) {
		return;
	}

	m_scrollingEnabled = enable;
	UpdateScrollButtons();

	LOG_INF("Scrolling " + std::string(enable ? "enabled" : "disabled"), "FlatUIFloatPanel");
}

void FlatUIFloatPanel::ScrollLeft()
{
	if (!m_scrollingEnabled || !NeedsScrolling()) {
		return;
	}

	m_scrollOffset = wxMax(0, m_scrollOffset - m_scrollStep);
	UpdateScrollPosition();
	UpdateScrollButtons();

	// Force immediate redraw of the entire panel
	Refresh();
	Update();

	LOG_DBG("Scrolled left, offset: " + std::to_string(m_scrollOffset), "FlatUIFloatPanel");
}

void FlatUIFloatPanel::ScrollRight()
{
	if (!m_scrollingEnabled || !NeedsScrolling()) {
		return;
	}

	if (!m_currentPage) {
		return;
	}

	wxSize pageSize = m_currentPage->GetBestSize();
	wxSize containerSize = m_scrollContainer->GetSize();
	int maxOffset = wxMax(0, pageSize.GetWidth() - containerSize.GetWidth());

	m_scrollOffset = wxMin(maxOffset, m_scrollOffset + m_scrollStep);
	UpdateScrollPosition();
	UpdateScrollButtons();

	// Force immediate redraw of the entire panel
	Refresh();
	Update();

	LOG_DBG("Scrolled right, offset: " + std::to_string(m_scrollOffset), "FlatUIFloatPanel");
}

void FlatUIFloatPanel::UpdateScrollButtons()
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

		if (m_currentPage) {
			wxSize pageSize = m_currentPage->GetBestSize();
			wxSize containerSize = m_scrollContainer->GetSize();
			int maxOffset = wxMax(0, pageSize.GetWidth() - containerSize.GetWidth());
			m_rightScrollButton->Enable(m_scrollOffset < maxOffset);
		}
	}
	else {
		m_leftScrollButton->Hide();
		m_rightScrollButton->Hide();
	}
}

void FlatUIFloatPanel::CreateScrollControls()
{
	// Create left scroll button with custom border
	m_leftScrollButton = new wxButton(m_contentPanel, wxID_BACKWARD, "<",
		wxDefaultPosition, wxSize(16, -1), wxBORDER_NONE);
	m_leftScrollButton->SetName("LeftScrollButton");
	m_leftScrollButton->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
	m_leftScrollButton->Hide();

	// Create right scroll button with custom border
	m_rightScrollButton = new wxButton(m_contentPanel, wxID_FORWARD, ">",
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
		dc.SetPen(wxPen(m_borderColour, 1));
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
		dc.SetPen(wxPen(m_borderColour, 1));
		dc.DrawLine(0, 0, 0, size.GetHeight());
		});

	LOG_INF("Created scroll control buttons with custom borders", "FlatUIFloatPanel");
}

void FlatUIFloatPanel::UpdateScrollPosition()
{
	if (!m_currentPage || !m_scrollContainer) {
		return;
	}

	wxSize containerSize = m_scrollContainer->GetSize();
	wxSize pageSize = m_currentPage->GetBestSize();

	// Set page size to its best size, but at least as tall as the container
	int pageWidth = pageSize.GetWidth();
	int pageHeight = wxMax(pageSize.GetHeight(), containerSize.GetHeight());

	// Position the page with scroll offset
	wxPoint newPos(-m_scrollOffset, 0);
	m_currentPage->SetPosition(newPos);
	m_currentPage->SetSize(pageWidth, pageHeight);

	// Force immediate layout and refresh of both the page and container
	m_currentPage->Layout();
	m_currentPage->Refresh();
	m_currentPage->Update();

	m_scrollContainer->Refresh();
	m_scrollContainer->Update();

	// Also refresh the content panel to ensure proper redraw
	m_contentPanel->Refresh();
	m_contentPanel->Update();
}

bool FlatUIFloatPanel::NeedsScrolling() const
{
	if (!m_currentPage || !m_scrollContainer) {
		return false;
	}

	wxSize pageSize = m_currentPage->GetBestSize();
	wxSize containerSize = m_scrollContainer->GetSize();

	return pageSize.GetWidth() > containerSize.GetWidth();
}

void FlatUIFloatPanel::OnScrollLeft(wxCommandEvent& event)
{
	ScrollLeft();
	event.Skip();
}

void FlatUIFloatPanel::OnScrollRight(wxCommandEvent& event)
{
	ScrollRight();
	event.Skip();
}