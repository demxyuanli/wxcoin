#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIHomeSpace.h"
#include "flatui/FlatUIFunctionSpace.h"
#include "flatui/FlatUIProfileSpace.h"
#include "flatui/FlatUISystemButtons.h"
#include "flatui/FlatUIEventManager.h"
#include "flatui/FlatUISpacerControl.h"
#include "flatui/FlatUIFloatPanel.h"
#include "flatui/FlatUIFixPanel.h"
#include <string>
#include <numeric>
#include <wx/dcbuffer.h>
#include <wx/timer.h>
#include <wx/graphics.h>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include <logger/Logger.h>
#include "config/ThemeManager.h"
#include "flatui/FlatUIUnpinButton.h"
// FlatUIPinButton is now handled by FlatUIFloatPanel
#include <memory> // Required for std::unique_ptr and std::move
#include <wx/button.h>
#include <wx/menu.h>
// Note: wxEVT_PIN_STATE_CHANGED is now defined in FlatUIFrame for global event handling




int FlatUIBar::GetBarHeight()
{
    return CFG_INT("BarRenderHeight");
}

FlatUIBar::FlatUIBar(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxControl(parent, id, pos, size, style | wxBORDER_NONE),
    m_homeSpace(nullptr),
    m_functionSpace(nullptr),
    m_profileSpace(nullptr),
    m_systemButtons(nullptr),
    m_tabFunctionSpacer(nullptr),
    m_functionProfileSpacer(nullptr),
    m_unpinButton(nullptr),
    m_fixPanel(nullptr),
    m_tabStyle(TabStyle::DEFAULT),
    m_tabBorderStyle(TabBorderStyle::SOLID),
    m_tabBorderTop(0),
    m_tabBorderBottom(0),
    m_tabBorderLeft(0),
    m_tabBorderRight(0),
    m_tabCornerRadius(0),
    m_functionSpaceCenterAlign(false),
    m_profileSpaceRightAlign(false),
    m_temporarilyShownPage(nullptr),
    m_barUnpinnedHeight(CFG_INT("BarUnpinnedHeight")),
    m_floatPanel(nullptr),
    m_tabsDropdownButton(nullptr),
    m_hiddenTabsMenu(nullptr),
    m_visibleTabsCount(0),
    m_functionSpaceUserVisible(true),  // Default to visible
    m_profileSpaceUserVisible(true)    // Default to visible
{
    SetName("FlatUIBar");
    SetFont(CFG_DEFAULTFONT());
    
    // Initialize managers
    m_stateManager = std::make_unique<FlatUIBarStateManager>();
    m_pageManager = std::make_unique<FlatUIPageManager>();
    m_layoutManager = std::make_unique<FlatUIBarLayoutManager>(this);
    m_eventDispatcher = std::make_unique<FlatUIBarEventDispatcher>(this);
    m_performanceManager = std::make_unique<FlatUIBarPerformanceManager>(this);
    
    // Initialize event dispatcher with managers
    m_eventDispatcher->Initialize(m_stateManager.get(), m_pageManager.get(), m_layoutManager.get());
    
    // Initialize visual configuration
    // Initialize theme-based configuration values
    m_tabBorderColour = CFG_COLOUR("BarTabBorderColour");
    m_tabBorderTopColour = CFG_COLOUR("BarActiveTabTopBorderColour");
    m_tabBorderBottomColour = CFG_COLOUR("BarTabBorderColour");
    m_tabBorderLeftColour = CFG_COLOUR("BarTabBorderColour");
    m_tabBorderRightColour = CFG_COLOUR("BarTabBorderColour");
    m_activeTabBgColour = CFG_COLOUR("ActBarBackgroundColour");
    m_activeTabTextColour = CFG_COLOUR("BarActiveTextColour");
    m_inactiveTabTextColour = CFG_COLOUR("BarInactiveTextColour");
    m_barTopMargin = CFG_INT("BarTopMargin");
    m_barBottomMargin = CFG_INT("BarBottomMargin");
    m_tabTopSpacing = CFG_INT("TabTopSpacing");

#ifdef __WXMSW__
    HWND hwnd = (HWND)GetHandle();
    if (hwnd) {
        long exStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        ::SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED);
    }
#endif

    SetDoubleBuffered(true);
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    // Register theme change listener
    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        RefreshTheme();
    });

    SetBarTopMargin(0);
    SetBarBottomMargin(0);

    // Create the main container first
    m_barContainer = new FlatBarSpaceContainer(this, wxID_ANY);
    m_barContainer->SetName("BarSpaceContainer");
    m_barContainer->SetDoubleBuffered(true);
    
    // Create child component controls
    m_homeSpace = new FlatUIHomeSpace(m_barContainer, wxID_ANY);
    m_functionSpace = new FlatUIFunctionSpace(m_barContainer, wxID_ANY);
    m_profileSpace = new FlatUIProfileSpace(m_barContainer, wxID_ANY);
    m_systemButtons = new FlatUISystemButtons(m_barContainer, wxID_ANY);
    
    // Set up the container with components
    m_barContainer->SetHomeSpace(m_homeSpace);
    m_barContainer->SetFunctionSpace(m_functionSpace);
    m_barContainer->SetProfileSpace(m_profileSpace);
    m_barContainer->SetSystemButtons(m_systemButtons);
    m_barContainer->SetFunctionSpaceCenterAlign(m_functionSpaceCenterAlign);
    m_barContainer->SetProfileSpaceRightAlign(m_profileSpaceRightAlign);

    // Set names for all controls
    m_homeSpace->SetName("HomeSpace");
    m_functionSpace->SetName("FunctionSpace");
    m_profileSpace->SetName("ProfileSpace");
    m_systemButtons->SetName("SystemButtons");

    m_homeSpace->SetDoubleBuffered(true);
    m_functionSpace->SetDoubleBuffered(true);
    m_profileSpace->SetDoubleBuffered(true);
    m_systemButtons->SetDoubleBuffered(true);

    // Create the fixed panel for containing pages (pin button is now handled by float panel)
    m_fixPanel = new FlatUIFixPanel(this, wxID_ANY);
    m_fixPanel->SetName("FixPanel");
    m_fixPanel->SetDoubleBuffered(true);
    m_fixPanel->Show(m_stateManager->IsPinned()); // Show only when pinned initially
    
    // Get reference to unpin button from fix panel
    m_unpinButton = m_fixPanel->GetUnpinButton();
    LOG_INF("Created fix panel with unpin button, initially " + std::string(m_stateManager->IsPinned() ? "shown" : "hidden"), "FlatUIBar");

    m_tabFunctionSpacer = new FlatUISpacerControl(this, 10);
    m_functionProfileSpacer = new FlatUISpacerControl(this, 10);
    m_tabFunctionSpacer->SetName("TabFunctionSpacer");
    m_functionProfileSpacer->SetName("FunctionProfileSpacer");

    m_tabFunctionSpacer->SetDoubleBuffered(true);
    m_functionProfileSpacer->SetDoubleBuffered(true);

    m_tabFunctionSpacer->SetCanDragWindow(true);
    m_functionProfileSpacer->SetCanDragWindow(true);

    //m_tabFunctionSpacer->Hide();
    //m_functionProfileSpacer->Hide();

    // Create the dropdown button for truncated tabs
    m_tabsDropdownButton = new wxButton(this, wxID_ANY, "...", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxBU_EXACTFIT);
    m_tabsDropdownButton->Hide(); // Initially hidden
    m_hiddenTabsMenu = new wxMenu();
    Bind(wxEVT_BUTTON, &FlatUIBar::OnTabsDropdown, this, m_tabsDropdownButton->GetId());
    Bind(wxEVT_MENU, &FlatUIBar::OnHiddenTabMenuItem, this);

    // Temporarily disable dropdown functionality
    m_tabsDropdownButton->Show(false);

    // m_pages is default constructed (empty wxVector)

    FlatUIEventManager::getInstance().bindBarEvents(this);

    FlatUIEventManager::getInstance().bindHomeSpaceEvents(m_homeSpace);
    FlatUIEventManager::getInstance().bindSystemButtonsEvents(m_systemButtons);
    FlatUIEventManager::getInstance().bindFunctionSpaceEvents(m_functionSpace);
    FlatUIEventManager::getInstance().bindProfileSpaceEvents(m_profileSpace);

    int barHeight = GetBarHeight() + 2; // Add 2 for border
    SetMinSize(wxSize(-1, barHeight));

    // Ensure child controls are initially hidden if they don't have content
    // or based on some initial state. FlatUIBar will Show() them as needed during layout.
    // Don't force hide here - let the layout manager handle visibility based on content and user state
    // m_functionSpace->Show(false);
    // m_profileSpace->Show(false);

    // Bind unpin button event (fix panel is already created above)
    if (m_stateManager->IsPinned() && m_unpinButton) {
        Bind(wxEVT_UNPIN_BUTTON_CLICKED, &FlatUIBar::OnUnpinButtonClicked, this, m_unpinButton->GetId());
        LOG_INF("Constructor: Bound unpin button event for pinned state", "FlatUIBar");
    }
    
    // Create float panel for future unpinned state usage
    m_floatPanel = new FlatUIFloatPanel(this);
    
    // Bind pin button events from float panel
    Bind(wxEVT_PIN_BUTTON_CLICKED, &FlatUIBar::OnPinButtonClicked, this);
    
    // Bind float panel dismiss event
    Bind(wxEVT_FLOAT_PANEL_DISMISSED, &FlatUIBar::OnFloatPanelDismissed, this);
    
    LOG_INF("Constructor: Created FloatPanel for future unpinned state usage", "FlatUIBar");

    // Setup global mouse capture
    SetupGlobalMouseCapture();

    // Always initialize layout, regardless of visibility state
    CallAfter([this]() {
        m_layoutManager->UpdateLayout(GetClientSize());
        if (IsShown()) {
            Refresh();
        }
        LOG_INF("Constructor: Completed initial layout setup", "FlatUIBar");
    });

    Bind(wxEVT_SHOW, &FlatUIBar::OnShow, this);
}

FlatUIBar::~FlatUIBar() {
    // Unbind the show event
    Unbind(wxEVT_SHOW, &FlatUIBar::OnShow, this);

    // Unbind float panel dismiss event
    Unbind(wxEVT_FLOAT_PANEL_DISMISSED, &FlatUIBar::OnFloatPanelDismissed, this);

    // Release global mouse capture
    ReleaseGlobalMouseCapture();

    // Destroy the menu associated with the dropdown
    if (m_hiddenTabsMenu) {
        delete m_hiddenTabsMenu;
        m_hiddenTabsMenu = nullptr;
    }

    FlatUIEventManager::getInstance().unbindBarEvents(this);
    FlatUIEventManager::getInstance().unbindHomeSpaceEvents(m_homeSpace);
    FlatUIEventManager::getInstance().unbindSystemButtonsEvents(m_systemButtons);
    FlatUIEventManager::getInstance().unbindFunctionSpaceEvents(m_functionSpace);
    FlatUIEventManager::getInstance().unbindProfileSpaceEvents(m_profileSpace);
    
    if (m_floatPanel) {
        m_floatPanel->Destroy();
        m_floatPanel = nullptr;
    }

    if (m_fixPanel) {
        m_fixPanel->Destroy();
        m_fixPanel = nullptr;
    }

    // Pages are now managed by PageManager - will be cleaned up automatically
    ThemeManager::getInstance().removeThemeChangeListener(this);
}

void FlatUIBar::OnShow(wxShowEvent& event)
{
    if (event.IsShown()) {
        CallAfter([this]() {
            LOG_INF("OnShow: Processing show event for FlatUIBar", "FlatUIBar");
            
            // Always call layout manager to ensure proper layout
            m_layoutManager->UpdateLayout(GetClientSize());
            
            // Show logic considering pinned and temporarily shown states
            if (m_stateManager->IsPinned() && m_fixPanel) {
                // Show fix panel and set active page
                if (!m_fixPanel->IsShown()) {
                    m_fixPanel->Show();
                    LOG_INF("OnShow: Showed FixPanel in pinned state", "FlatUIBar");
                }
                
                // Set active page in fix panel if there are pages
                size_t activePage = m_stateManager->GetActivePage();
                FlatUIPage* page = m_pageManager->GetPage(activePage);
                if (page) {
                    m_fixPanel->SetActivePage(activePage);
                    LOG_INF("OnShow: Set active page in FixPanel", "FlatUIBar");
                }
            }
            else if (!m_stateManager->IsPinned() && m_fixPanel) {
                // If not pinned but fix panel exists, ensure it's hidden
                if (m_fixPanel->IsShown()) {
                    m_fixPanel->Hide();
                    LOG_INF("OnShow: Hidden FixPanel in unpinned state", "FlatUIBar");
                }
            }

            // Update button visibility after showing/hiding panels
            UpdateButtonVisibility();

            Refresh();
            });
    }
    event.Skip();
}

wxSize FlatUIBar::DoGetBestSize() const
{
    wxSize bestSize(0, 0);

    // Determine if we need to show pages, which dictates the total height.
    if (ShouldShowPages()) {
        // Pinned, or unpinned with a temporary page: calculate full size.
        bestSize.SetHeight(GetBarHeight() + m_barTopMargin); 

        if (m_fixPanel && m_fixPanel->IsShown()) {
            // Add the height of the fix panel.
            wxSize fixPanelSize = m_fixPanel->GetBestSize();
            bestSize.SetHeight(bestSize.GetHeight() + fixPanelSize.GetHeight());
            bestSize.SetWidth(wxMax(bestSize.GetWidth(), fixPanelSize.GetWidth()));
        }
    } else {
        // Unpinned and no temporary page: use the collapsed height.
        bestSize.SetHeight(m_barUnpinnedHeight);
    }
    
    // Ensure the width is at least the parent's width.
    if (GetParent()) {
        bestSize.SetWidth(wxMax(bestSize.GetWidth(), GetParent()->GetClientSize().GetWidth()));
    }

    return bestSize;
}

void FlatUIBar::AddPage(FlatUIPage* page)
{
    if (!page) {
        LOG_ERR("Cannot add null page", "FlatUIBar");
        return;
    }

    // Add page through page manager
    m_pageManager->AddPage(page);

    // Handle container setup based on current state
    if (m_stateManager->IsPinned() && m_fixPanel) {
        m_pageManager->ShowPageInFixPanel(page, m_fixPanel);
        if (m_pageManager->GetPageCount() == 1) {
            m_stateManager->SetActivePage(0);
            m_pageManager->SetPageActive(0, true);
        }
        LOG_INF_S("Added page '" + page->GetLabel().ToStdString() + "' to FixPanel");
    } else {
        // Unpinned state - keep page ready for float panel usage
        page->Hide();
        if (m_pageManager->GetPageCount() == 1) {
            m_stateManager->SetActivePage(0);
            m_pageManager->SetPageActive(0, true);
        }
        LOG_INF_S("Added page '" + page->GetLabel().ToStdString() + "' for FloatPanel usage");
    }

    // Update layout if visible
    if (IsShown()) {
        m_layoutManager->UpdateLayout(GetClientSize());
        Refresh();
    }
}

void FlatUIBar::SetActivePage(size_t pageIndex)
{
    FlatUIPage* page = m_pageManager->GetPage(pageIndex);
    if (!page) {
        LOG_ERR_S("Cannot set active page - invalid index: " + std::to_string(pageIndex));
        return;
    }

    // Enhanced check to avoid unnecessary work - compare both state and visual state
    if (m_stateManager->GetActivePage() == pageIndex) {
        if (m_stateManager->IsPinned() && m_fixPanel && m_fixPanel->GetActivePage() == page) {
            LOG_INF_S("Page " + std::to_string(pageIndex) + " already active and displayed, skipping update");
            return; // Already active and properly displayed
        }
        if (!m_stateManager->IsPinned() && m_stateManager->GetActiveFloatingPage() == pageIndex) {
            LOG_INF_S("Floating page " + std::to_string(pageIndex) + " already active, skipping update");
            return; // Already active floating page
        }
    }

    // Batch all updates to minimize redraws
    bool needsLayout = false;
    size_t oldPageIndex = m_stateManager->GetActivePage();
    
    // Use state manager to handle the page change
    m_stateManager->SetActivePage(pageIndex);

    // Update page active states efficiently
    m_pageManager->SetAllPagesInactive();
    m_pageManager->SetPageActive(pageIndex, true);

    // Handle container updates based on current state
    if (m_stateManager->IsPinned() && m_fixPanel) {
        m_fixPanel->SetActivePage(pageIndex);
        m_temporarilyShownPage = nullptr;
        needsLayout = true;
    }
    
    // Defer layout and refresh to avoid multiple updates
    if (needsLayout) {
        CallAfter([this]() {
            m_layoutManager->UpdateLayout(GetClientSize());
            Refresh();
        });
    } else {
        // For unpinned state, just refresh tabs
        CallAfter([this]() {
            Refresh();
        });
    }
    
    // Notify event dispatcher about the change
    m_eventDispatcher->BroadcastPageChange(oldPageIndex, pageIndex);
    
    LOG_INF_S("Set active page to '" + page->GetLabel().ToStdString() + "' (index " + std::to_string(pageIndex) + ")");
}

size_t FlatUIBar::GetPageCount() const noexcept { return m_pageManager->GetPageCount(); }
size_t FlatUIBar::GetActivePage() const noexcept { return m_stateManager->GetActivePage(); }
FlatUIPage* FlatUIBar::GetPage(size_t index) const { return m_pageManager->GetPage(index); }

void FlatUIBar::OnSize(wxSizeEvent& evt)
{
    wxSize newSize = GetClientSize();
    
    // Notify performance manager about size changes for invalidation
    if (m_performanceManager) {
        m_performanceManager->InvalidateAll();
    }
    
    // Position the container to fill the bar area
    if (m_barContainer) {
        int barHeight = GetBarHeight();
        // In unpinned state, barContainer should be at the top (no margin)
        // In pinned state, barContainer should have top margin
        int topMargin = m_stateManager->IsPinned() ? m_barTopMargin : 0;
        m_barContainer->SetPosition(wxPoint(0, topMargin));
        m_barContainer->SetSize(newSize.GetWidth(), barHeight);
        
        // Force container layout update
        m_barContainer->UpdateLayout();
    }
    
    // Use layout manager for positioning other components (like fixPanel)
    m_layoutManager->UpdateLayout(newSize);

    // Update child controls through container
    if (m_barContainer) {
        m_barContainer->Update();
    }
    if (m_tabFunctionSpacer) m_tabFunctionSpacer->Update();
    if (m_functionProfileSpacer) m_functionProfileSpacer->Update();

    // Update active page
    FlatUIPage* activePage = m_pageManager->GetActivePage();
    if (activePage) {
        activePage->Update();
    }

    // Notify event dispatcher
    m_eventDispatcher->HandleSizeEvent(newSize);

    Refresh(true);
    Update();
    evt.Skip();
}

void FlatUIBar::OnPinButtonClicked(wxCommandEvent& event)
{
    LOG_INF("OnPinButtonClicked: Pin button clicked", "FlatUIBar");
    
    // Use event dispatcher for centralized handling
    if (m_eventDispatcher) {
        m_eventDispatcher->HandlePinButtonClick();
    } else {
        // Fallback to direct state change
        LOG_INF("OnPinButtonClicked: Using fallback handling", "FlatUIBar");
        SetGlobalPinned(true);
    }
    
    event.Skip();
}

void FlatUIBar::OnUnpinButtonClicked(wxCommandEvent& event)
{
    LOG_INF("OnUnpinButtonClicked: Unpin button clicked", "FlatUIBar");
    
    // Use event dispatcher for centralized handling
    if (m_eventDispatcher) {
        m_eventDispatcher->HandleUnpinButtonClick();
    } else {
        // Fallback to direct state change
        LOG_INF("OnUnpinButtonClicked: Using fallback handling", "FlatUIBar");
        SetGlobalPinned(false);
    }
    
    event.Skip();
}

bool FlatUIBar::IsBarPinned() const
{
    return m_stateManager->IsPinned();
}

bool FlatUIBar::IsGlobalPinned() const
{
    return m_stateManager->IsPinned();
}

void FlatUIBar::OnGlobalMouseDown(wxMouseEvent& event)
{
    // With wxPopupTransientWindow, we only need to handle clicks within the bar area
    // The floating window will automatically dismiss itself on outside clicks
    
    if (!this || !IsShown()) {
        event.Skip();
        return;
    }

    // Only handle clicks when in unpinned state
    if (m_stateManager->IsPinned()) {
        event.Skip();
        return;
    }

    // Check if this is a click on the bar itself (for tab area empty space handling)
    wxPoint clickPos = event.GetPosition();
    wxWindow* clickedWindow = wxFindWindowAtPoint(clickPos);
    
    if (clickedWindow == this) {
        // This is handled by HandleTabAreaClick, so we don't need to do anything special here
        LOG_INF("Click on FlatUIBar detected", "FlatUIBar");
    }

    event.Skip();
}

bool FlatUIBar::IsPointInBarArea(const wxPoint& globalPoint) const
{
    wxPoint localPoint = ScreenToClient(globalPoint);
    wxRect barRect(0, 0, GetSize().GetWidth(), GetBarHeight());
    return barRect.Contains(localPoint);
}

void FlatUIBar::SetupGlobalMouseCapture()
{
    // Bind to multiple possible parent windows to ensure we catch global clicks
    wxWindow* topLevel = wxGetTopLevelParent(this);
    if (topLevel) {
        topLevel->Bind(wxEVT_LEFT_DOWN, &FlatUIBar::OnGlobalMouseDown, this);
        LOG_INF("Bound global mouse capture to top-level window: " + topLevel->GetName().ToStdString(), "FlatUIBar");
    }
    
    // Also bind to immediate parent if different from top-level
    wxWindow* parent = GetParent();
    if (parent && parent != topLevel) {
        parent->Bind(wxEVT_LEFT_DOWN, &FlatUIBar::OnGlobalMouseDown, this);
        LOG_INF("Bound global mouse capture to parent window: " + parent->GetName().ToStdString(), "FlatUIBar");
    }
    
    // Bind to self as well for additional coverage
    this->Bind(wxEVT_LEFT_DOWN, &FlatUIBar::OnGlobalMouseDown, this);
    LOG_INF("Bound global mouse capture to self", "FlatUIBar");
}

void FlatUIBar::ReleaseGlobalMouseCapture()
{
    wxWindow* topLevel = wxGetTopLevelParent(this);
    if (topLevel) {
        topLevel->Unbind(wxEVT_LEFT_DOWN, &FlatUIBar::OnGlobalMouseDown, this);
    }
    
    // Also release immediate parent if different from top-level
    wxWindow* parent = GetParent();
    if (parent && parent != topLevel) {
        parent->Unbind(wxEVT_LEFT_DOWN, &FlatUIBar::OnGlobalMouseDown, this);
    }
    
    // Unbind self
    this->Unbind(wxEVT_LEFT_DOWN, &FlatUIBar::OnGlobalMouseDown, this);
}

void FlatUIBar::SetGlobalPinned(bool pinned)
{
    if (!m_stateManager || m_stateManager->IsPinned() == pinned) {
        return; // No change needed or state manager not available
    }
    
    LOG_INF("SetGlobalPinned: Changing to " + std::string(pinned ? "pinned" : "unpinned"), "FlatUIBar");
    
    // Use state manager for the transition
    if (pinned) {
        m_stateManager->TransitionTo(FlatUIBarStateManager::BarState::PINNED);
    } else {
        m_stateManager->TransitionTo(FlatUIBarStateManager::BarState::UNPINNED);
    }
    
    // OnGlobalPinStateChanged handles all UI updates, layout, and button visibility
    // Don't call UpdateButtonVisibility() separately to avoid duplicate refresh
    OnGlobalPinStateChanged(pinned);
    
    LOG_INF("Global pin state changed to: " + std::string(pinned ? "pinned" : "unpinned"), "FlatUIBar");
}

void FlatUIBar::ToggleGlobalPinState()
{
    SetGlobalPinned(!m_stateManager->IsPinned());
}

bool FlatUIBar::ShouldShowPages() const
{
    if (!m_stateManager) {
        return false;
    }
    
    // Pages should be visible if:
    // 1. State is pinned (always show)
    // 2. Not pinned but there's a temporarily shown page
    return m_stateManager->IsPinned() || (m_temporarilyShownPage != nullptr);
}

void FlatUIBar::OnGlobalPinStateChanged(bool isPinned)
{
    LOG_INF("OnGlobalPinStateChanged called with isPinned: " + std::string(isPinned ? "true" : "false"), "FlatUIBar");

    // Enhanced freezing to prevent flickering during panel switching
    // Freeze both this window and parent to prevent any intermediate redraws
    Freeze();
    wxWindow* parent = GetParent();
    if (parent) {
        parent->Freeze();
    }
    
    try {
        if (isPinned) {
            // Pinned state: Show all content including active page
            ShowAllContent();
            LOG_INF("Switched to pinned state - showing all content", "FlatUIBar");
        }
        else {
            // Unpinned state: Hide all content except bar space (tabs area)
            HideAllContentExceptBarSpace();
            LOG_INF("Switched to unpinned state - hiding all content except bar space", "FlatUIBar");
        }

        // Update layout only once after all changes, without immediate refresh
        wxSize clientSize = GetClientSize();
        LOG_INF("OnGlobalPinStateChanged: Calling UpdateLayout with size (" + 
               std::to_string(clientSize.GetWidth()) + ", " + 
               std::to_string(clientSize.GetHeight()) + ")", "FlatUIBar");
        
        m_layoutManager->UpdateLayout(clientSize);
        
        // Update barContainer position based on pin state
        if (m_barContainer) {
            int barHeight = GetBarHeight();
            int topMargin = isPinned ? m_barTopMargin : 0;
            m_barContainer->SetPosition(wxPoint(0, topMargin));
            m_barContainer->SetSize(clientSize.GetWidth(), barHeight);
            m_barContainer->UpdateLayout();
        }
        
        // Verify FixPanel position after layout
        if (m_fixPanel && m_fixPanel->IsShown()) {
            wxPoint fixPanelPos = m_fixPanel->GetPosition();
            wxSize fixPanelSize = m_fixPanel->GetSize();
            LOG_INF("OnGlobalPinStateChanged: FixPanel final position (" + 
                   std::to_string(fixPanelPos.x) + ", " + std::to_string(fixPanelPos.y) + 
                   ") size (" + std::to_string(fixPanelSize.GetWidth()) + ", " + 
                   std::to_string(fixPanelSize.GetHeight()) + ")", "FlatUIBar");
        }
        
        // Update size info without forcing immediate layout
        InvalidateBestSize();
        
        if (parent) {
            parent->InvalidateBestSize();
            // Don't call Layout() immediately - let it happen naturally
        }
    }
    catch (...) {
        // Ensure both Thaw calls happen even if an exception occurs
        if (parent) {
            parent->Thaw();
        }
        Thaw();
        throw;
    }
    
    // Thaw parent first, then this window, then do a single refresh
    if (parent) {
        parent->Thaw();
    }
    Thaw();
    
    // Single deferred refresh to avoid flickering
    CallAfter([this]() {
        if (IsShown()) {
            Refresh();
            // Trigger parent layout only after our refresh is done
            wxWindow* parent = GetParent();
            if (parent) {
                parent->Layout();
            }
        }
    });
}

void FlatUIBar::ShowAllContent()
{
    LOG_INF("ShowAllContent: Showing all page content via FixPanel", "FlatUIBar");
    
    // Show the fix panel and rebuild its content
    if (m_fixPanel) {
        // Re-add all pages to FixPanel to ensure clean state
        for (size_t i = 0; i < m_pageManager->GetPageCount(); ++i) {
            FlatUIPage* page = m_pageManager->GetPage(i);
            if (page) {
                m_fixPanel->AddPage(page);
                LOG_INF("ShowAllContent: Re-added page '" + page->GetLabel().ToStdString() + "' to FixPanel", "FlatUIBar");
            }
        }
        
        if (!m_fixPanel->IsShown()) {
            m_fixPanel->Show();
            LOG_INF("ShowAllContent: Showed FixPanel", "FlatUIBar");
        }

        // Set active page in fix panel
        size_t activePage = m_stateManager->GetActivePage();
        FlatUIPage* page = m_pageManager->GetPage(activePage);
        if (page) {
            m_fixPanel->SetActivePage(activePage);
            LOG_INF("ShowAllContent: Set active page in FixPanel - " + page->GetLabel().ToStdString(), "FlatUIBar");
        }
    }
    
    // Update button visibility without causing refresh (caller handles refresh timing)
    UpdateButtonVisibility();

    // Clear temporarily shown page state since we're in pinned mode
    m_temporarilyShownPage = nullptr;
    LOG_INF("ShowAllContent: Cleared temporarily shown page state", "FlatUIBar");
}

void FlatUIBar::HideAllContentExceptBarSpace()
{
    LOG_INF("HideAllContentExceptBarSpace: Hiding all page content via FixPanel", "FlatUIBar");
    
    // Clear FixPanel content and hide it to prevent display artifacts during state transitions
    if (m_fixPanel) {
        // First clear the content to reset all page states and unpin button
        m_fixPanel->ClearContent();
        
        // Then hide the panel
        if (m_fixPanel->IsShown()) {
            m_fixPanel->Hide();
            LOG_INF("HideAllContentExceptBarSpace: Hidden FixPanel", "FlatUIBar");
        }
    }

    // Clear temporarily shown page state
    m_temporarilyShownPage = nullptr;
    LOG_INF("HideAllContentExceptBarSpace: Cleared temporarily shown page state", "FlatUIBar");
}



// FloatPanel methods implementation
void FlatUIBar::ShowPageInFloatPanel(FlatUIPage* page)
{
    if (!m_floatPanel || !page) {
        return;
    }

    wxWindow* frame = wxGetTopLevelParent(this);
    if (!frame) return;

    // Calculate position and size to match FixPanel positioning
    // Position (0, 30) relative to this bar, converted to screen coordinates
    const int FLOAT_PANEL_Y = 30;
    wxPoint position = this->ClientToScreen(wxPoint(0, FLOAT_PANEL_Y)); 
    
    // Size: bar's width and calculated height based on page content
    wxSize pageSize = page->GetBestSize();
    int floatHeight = wxMax(60, pageSize.GetHeight()); // Minimum 60px height
    wxSize size(GetClientSize().GetWidth(), floatHeight);

    if (m_floatPanel->IsShown()) {
        // Float panel is already shown, just update the content
        LOG_INF("Updating float panel content to: " + page->GetLabel().ToStdString(), "FlatUIBar");
        m_floatPanel->SetPageContent(page);
    } else {
        // Float panel is not shown, show it with the new content
        LOG_INF("Showing float panel with: " + page->GetLabel().ToStdString(), "FlatUIBar");
        m_floatPanel->SetPageContent(page);
        m_floatPanel->ShowAt(position, size);
    }

    // Ensure the activeFloatingPage is set correctly if it wasn't set by the caller
    if (m_stateManager->GetActiveFloatingPage() == static_cast<size_t>(-1)) {
        for (size_t i = 0; i < m_pageManager->GetPageCount(); ++i) {
            if (m_pageManager->GetPage(i) == page) {
                m_stateManager->SetActiveFloatingPage(i);
                break;
            }
        }
    }
}

void FlatUIBar::HideFloatPanel()
{
    if (m_floatPanel && m_floatPanel->IsShown()) {
        m_floatPanel->HidePanel();
        m_stateManager->SetActiveFloatingPage(static_cast<size_t>(-1)); // Reset floating page selection
        Refresh(); // Update tab visual state
        LOG_INF("Hidden float panel", "FlatUIBar");
    }
}

void FlatUIBar::OnFloatPanelDismissed(wxCommandEvent& event)
{
    // Handle the event when the float panel is dismissed
    LOG_INF("Float panel dismissed, resetting active floating page", "FlatUIBar");
    m_stateManager->SetActiveFloatingPage(static_cast<size_t>(-1)); // Reset floating page selection
    Refresh(); // Update tab visual state
    event.Skip();
}

void FlatUIBar::OnTabsDropdown(wxCommandEvent& event)
{
    if (m_hiddenTabsMenu && m_hiddenTabsMenu->GetMenuItemCount() > 0) {
        PopupMenu(m_hiddenTabsMenu);
    }
    event.Skip();
}

void FlatUIBar::OnHiddenTabMenuItem(wxCommandEvent& event)
{
    int selectedIndex = event.GetId() - FlatUIBarConfig::MENU_ID_RANGE_START;
    if (selectedIndex >= 0) {
        SetActivePage(static_cast<size_t>(selectedIndex));
    }
    event.Skip();
}

void FlatUIBar::UpdateHiddenTabsMenu(const std::vector<size_t>& hiddenIndices)
{
    if (!m_hiddenTabsMenu) return;

    // Clear existing menu items
    while (m_hiddenTabsMenu->GetMenuItemCount() > 0) {
        m_hiddenTabsMenu->Destroy(m_hiddenTabsMenu->FindItemByPosition(0));
    }

    for (const auto& index : hiddenIndices) {
        FlatUIPage* page = GetPage(index);
        if (page) {
            // Use a range of IDs for menu items
            int menuId = FlatUIBarConfig::MENU_ID_RANGE_START + index;
            m_hiddenTabsMenu->Append(menuId, page->GetLabel());
        }
    }
}

void FlatUIBar::SetVisibleTabsCount(size_t count)
{
    m_visibleTabsCount = count;
}

size_t FlatUIBar::GetVisibleTabsCount() const
{
    return m_visibleTabsCount;
}

void FlatUIBar::UpdateButtonVisibility()
{
    LOG_INF("UpdateButtonVisibility: Current state is " + std::string(m_stateManager->IsPinned() ? "pinned" : "unpinned"), "FlatUIBar");
    
    // Unpin button logic: Show only when ribbon is pinned and fix panel is shown
    if (m_fixPanel && m_unpinButton) {
        bool shouldShowUnpin = m_stateManager->IsPinned() && m_fixPanel->IsShown();
        m_fixPanel->ShowUnpinButton(shouldShowUnpin);
        LOG_INF("UpdateButtonVisibility: " + std::string(shouldShowUnpin ? "Showed" : "Hidden") + " unpin button in FixPanel", "FlatUIBar");
    }
    
    // Pin button logic: Handled by float panel, should be hidden when ribbon is pinned
    // When ribbon is unpinned, pin button will be shown by float panel when it's displayed
    if (m_floatPanel && m_floatPanel->IsShown()) {
        // Float panel is shown, pin button should be visible (handled by float panel)
        LOG_INF("UpdateButtonVisibility: Float panel is shown, pin button managed by float panel", "FlatUIBar");
    } else {
        // Float panel is hidden, ensure pin button is not visible
        LOG_INF("UpdateButtonVisibility: Float panel is hidden, pin button should be hidden", "FlatUIBar");
    }
    
    // NOTE: Don't call Refresh() here - let the calling code handle refresh timing
    // This prevents redundant refreshes during state transitions
}

void FlatUIBar::HideTemporarilyShownPage()
{
    if (m_temporarilyShownPage) {
        m_temporarilyShownPage->Hide();
        m_temporarilyShownPage = nullptr;
        LOG_INF("Hidden temporarily shown page", "FlatUIBar");
    }
}

void FlatUIBar::RefreshTheme()
{
    // Update all theme-based colors and settings
    m_tabBorderColour = CFG_COLOUR("BarTabBorderColour");
    m_tabBorderTopColour = CFG_COLOUR("BarActiveTabTopBorderColour");
    m_tabBorderBottomColour = CFG_COLOUR("BarTabBorderColour");
    m_tabBorderLeftColour = CFG_COLOUR("BarTabBorderColour");
    m_tabBorderRightColour = CFG_COLOUR("BarTabBorderColour");
    m_activeTabBgColour = CFG_COLOUR("ActBarBackgroundColour");
    m_activeTabTextColour = CFG_COLOUR("BarActiveTextColour");
    m_inactiveTabTextColour = CFG_COLOUR("BarInactiveTextColour");
    m_barTopMargin = CFG_INT("BarTopMargin");
    m_barBottomMargin = CFG_INT("BarBottomMargin");
    m_tabTopSpacing = CFG_INT("TabTopSpacing");
    m_barUnpinnedHeight = CFG_INT("BarUnpinnedHeight");
    
    // Update control properties
    SetFont(CFG_DEFAULTFONT());
    
    // Update barContainer position based on current pin state
    if (m_barContainer) {
        wxSize clientSize = GetClientSize();
        int barHeight = GetBarHeight();
        int topMargin = m_stateManager->IsPinned() ? m_barTopMargin : 0;
        m_barContainer->SetPosition(wxPoint(0, topMargin));
        m_barContainer->SetSize(clientSize.GetWidth(), barHeight);
        m_barContainer->UpdateLayout();
    }
    
    // Refresh all child components
    if (m_homeSpace) {
        m_homeSpace->Refresh(true);
        m_homeSpace->Update();
    }
    if (m_functionSpace) {
        m_functionSpace->Refresh(true);
        m_functionSpace->Update();
    }
    if (m_profileSpace) {
        m_profileSpace->Refresh(true);
        m_profileSpace->Update();
    }
    if (m_systemButtons) {
        m_systemButtons->Refresh(true);
        m_systemButtons->Update();
    }
    if (m_fixPanel) {
        m_fixPanel->Refresh(true);
        m_fixPanel->Update();
    }
    if (m_floatPanel) {
        m_floatPanel->Refresh(true);
        m_floatPanel->Update();
    }
    if (m_tabFunctionSpacer) {
        m_tabFunctionSpacer->Refresh(true);
        m_tabFunctionSpacer->Update();
    }
    if (m_functionProfileSpacer) {
        m_functionProfileSpacer->Refresh(true);
        m_functionProfileSpacer->Update();
    }
    
    // Refresh all pages
    for (size_t i = 0; i < GetPageCount(); ++i) {
        FlatUIPage* page = GetPage(i);
        if (page) {
            page->Refresh(true);
            page->Update();
        }
    }
    
    // Force refresh
    Refresh(true);
    Update();
}

