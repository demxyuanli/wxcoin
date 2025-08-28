#include "docking/AutoHideContainer.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/DockContainerWidget.h"
#include "docking/DockManager.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <wx/graphics.h>
#include <wx/button.h>
#include <cmath>

namespace ads {

// AutoHideTab implementation
wxBEGIN_EVENT_TABLE(AutoHideTab, wxPanel)
    EVT_PAINT(AutoHideTab::OnPaint)
    EVT_ENTER_WINDOW(AutoHideTab::OnMouseEnter)
    EVT_LEAVE_WINDOW(AutoHideTab::OnMouseLeave)
    EVT_LEFT_DOWN(AutoHideTab::OnLeftDown)
wxEND_EVENT_TABLE()

AutoHideTab::AutoHideTab(DockWidget* dockWidget, AutoHideSideBarLocation location)
    : wxPanel(dockWidget->dockManager()->containerWidget())
    , m_dockWidget(dockWidget)
    , m_location(location)
    , m_isActive(false)
    , m_isHovered(false)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    // Set size based on orientation
    if (location == SideBarLeft || location == SideBarRight) {
        SetMinSize(wxSize(25, 100));
    } else {
        SetMinSize(wxSize(100, 25));
    }
    
    updateIcon();
    updateTitle();
}

AutoHideTab::~AutoHideTab() {
}

void AutoHideTab::updateIcon() {
    if (m_dockWidget) {
        m_icon = m_dockWidget->icon();
    }
}

void AutoHideTab::updateTitle() {
    if (m_dockWidget) {
        SetToolTip(m_dockWidget->title());
    }
}

void AutoHideTab::setActive(bool active) {
    if (m_isActive != active) {
        m_isActive = active;
        Refresh();
    }
}

void AutoHideTab::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    wxRect rect = GetClientRect();
    
    // Draw background
    wxColour bgColor;
    if (m_isActive) {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    } else if (m_isHovered) {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT);
    } else {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    }
    
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rect);
    
    // Draw border
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
    dc.DrawRectangle(rect);
    
    // Draw content based on orientation
    bool isVertical = (m_location == SideBarLeft || m_location == SideBarRight);
    
    if (isVertical) {
        // Draw vertical text
        dc.SetFont(GetFont());
        dc.SetTextForeground(m_isActive ? 
            wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT) :
            wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
        
        wxString title = m_dockWidget->title();
        wxSize textSize = dc.GetTextExtent(title);
        
        // Save current transform
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            // Rotate 90 degrees
            gc->Translate(rect.width / 2, rect.height / 2);
            gc->Rotate(-M_PI / 2);
            
            // Draw rotated text
            gc->SetFont(GetFont(), dc.GetTextForeground());
            gc->DrawText(title, -textSize.GetWidth() / 2, -rect.width / 2 + 5);
            
            delete gc;
        }
    } else {
        // Draw horizontal text and icon
        int x = 5;
        
        // Draw icon if available
        if (m_icon.IsOk()) {
            int iconY = (rect.height - m_icon.GetHeight()) / 2;
            dc.DrawBitmap(m_icon, x, iconY);
            x += m_icon.GetWidth() + 5;
        }
        
        // Draw text
        dc.SetFont(GetFont());
        dc.SetTextForeground(m_isActive ? 
            wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT) :
            wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
        
        wxString title = m_dockWidget->title();
        int textY = (rect.height - dc.GetCharHeight()) / 2;
        dc.DrawText(title, x, textY);
    }
}

void AutoHideTab::OnMouseEnter(wxMouseEvent& event) {
    m_isHovered = true;
    Refresh();
    
    // Show the dock widget through the container
    AutoHideSideBar* sideBar = dynamic_cast<AutoHideSideBar*>(GetParent());
    if (sideBar && sideBar->getContainer()) {
        // The container should have the auto-hide manager
        // For now, just activate the tab
        sideBar->showDockWidget(m_dockWidget);
    }
    
    event.Skip();
}

void AutoHideTab::OnMouseLeave(wxMouseEvent& event) {
    m_isHovered = false;
    Refresh();
    event.Skip();
}

void AutoHideTab::OnLeftDown(wxMouseEvent& event) {
    // Toggle visibility
    AutoHideSideBar* sideBar = dynamic_cast<AutoHideSideBar*>(GetParent());
    if (sideBar) {
        sideBar->showDockWidget(m_dockWidget);
    }
    event.Skip();
}

// AutoHideSideBar implementation
wxBEGIN_EVENT_TABLE(AutoHideSideBar, wxPanel)
    EVT_SIZE(AutoHideSideBar::OnSize)
wxEND_EVENT_TABLE()

AutoHideSideBar::AutoHideSideBar(DockContainerWidget* container, AutoHideSideBarLocation location)
    : wxPanel(container)
    , m_container(container)
    , m_location(location)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    
    // Create sizer based on orientation
    if (location == SideBarLeft || location == SideBarRight) {
        m_sizer = new wxBoxSizer(wxVERTICAL);
        SetMinSize(wxSize(25, -1));
    } else {
        m_sizer = new wxBoxSizer(wxHORIZONTAL);
        SetMinSize(wxSize(-1, 25));
    }
    
    SetSizer(m_sizer);
}

AutoHideSideBar::~AutoHideSideBar() {
    // Tabs are deleted by wxWidgets parent-child relationship
}

void AutoHideSideBar::addAutoHideWidget(DockWidget* dockWidget) {
    if (!dockWidget) return;
    
    // Check if already added
    for (auto* tab : m_tabs) {
        if (tab->dockWidget() == dockWidget) {
            return;
        }
    }
    
    // Create new tab
    AutoHideTab* tab = new AutoHideTab(dockWidget, m_location);
    m_tabs.push_back(tab);
    
    // Add to sizer
    if (m_location == SideBarLeft || m_location == SideBarRight) {
        m_sizer->Add(tab, 0, wxEXPAND | wxALL, 1);
    } else {
        m_sizer->Add(tab, 0, wxEXPAND | wxALL, 1);
    }
    
    updateLayout();
}

void AutoHideSideBar::removeAutoHideWidget(DockWidget* dockWidget) {
    auto it = std::find_if(m_tabs.begin(), m_tabs.end(),
        [dockWidget](AutoHideTab* tab) { return tab->dockWidget() == dockWidget; });
    
    if (it != m_tabs.end()) {
        AutoHideTab* tab = *it;
        m_sizer->Detach(tab);
        tab->Destroy();
        m_tabs.erase(it);
        updateLayout();
    }
}

AutoHideTab* AutoHideSideBar::tab(int index) const {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        return m_tabs[index];
    }
    return nullptr;
}

void AutoHideSideBar::showDockWidget(DockWidget* dockWidget) {
    if (!dockWidget || !m_container) return;
    
    // Find the auto-hide manager
    DockManager* dockManager = m_container->dockManager();
    if (!dockManager) return;
    
    // Activate the tab
    for (auto* tab : m_tabs) {
        tab->setActive(tab->dockWidget() == dockWidget);
    }
    
    // Show the auto-hide container through the manager
    // This will be called through the AutoHideManager which is owned by the container
    // For now, we just mark the tab as active
}

void AutoHideSideBar::hideDockWidget(DockWidget* dockWidget) {
    // Deactivate the tab
    for (auto* tab : m_tabs) {
        if (tab->dockWidget() == dockWidget) {
            tab->setActive(false);
            break;
        }
    }
}

bool AutoHideSideBar::hasVisibleTabs() const {
    return !m_tabs.empty();
}

void AutoHideSideBar::OnSize(wxSizeEvent& event) {
    updateLayout();
    event.Skip();
}

void AutoHideSideBar::updateLayout() {
    Layout();
    Refresh();
}

// AutoHideDockContainer implementation
wxBEGIN_EVENT_TABLE(AutoHideDockContainer, wxPanel)
    EVT_PAINT(AutoHideDockContainer::OnPaint)
    EVT_TIMER(wxID_ANY, AutoHideDockContainer::OnTimer)
    EVT_MOUSE_CAPTURE_LOST(AutoHideDockContainer::OnMouseCaptureLost)
    EVT_KILL_FOCUS(AutoHideDockContainer::OnKillFocus)
    EVT_BUTTON(wxID_ANY, AutoHideDockContainer::OnPinButtonClick)
wxEND_EVENT_TABLE()

AutoHideDockContainer::AutoHideDockContainer(DockWidget* dockWidget, 
                                           AutoHideSideBarLocation location,
                                           DockContainerWidget* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_RAISED)
    , m_dockWidget(dockWidget)
    , m_sideBarLocation(location)
    , m_container(parent)
    , m_animationTimer(this)
    , m_isAnimating(false)
    , m_isVisible(false)
    , m_animationProgress(0)
    , m_size(300, 400) // Default size
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    
    // Add the dock widget
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Add title bar
    wxPanel* titleBar = new wxPanel(this);
    titleBar->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION));
    titleBar->SetMinSize(wxSize(-1, 25));
    
    wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* titleText = new wxStaticText(titleBar, wxID_ANY, dockWidget->title());
    titleText->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_CAPTIONTEXT));
    titleSizer->Add(titleText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    
    // Add pin button
    wxButton* pinButton = new wxButton(titleBar, wxID_ANY, "P", wxDefaultPosition, wxSize(20, 20));
    pinButton->SetToolTip("Pin/Unpin");
    // Use a proper event handler instead of lambda for better compatibility
    pinButton->Bind(wxEVT_BUTTON, &AutoHideDockContainer::OnPinButtonClick, this);
    titleSizer->Add(pinButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    
    titleBar->SetSizer(titleSizer);
    
    sizer->Add(titleBar, 0, wxEXPAND);
    sizer->Add(dockWidget, 1, wxEXPAND);
    
    SetSizer(sizer);
    
    // Initially hidden
    Hide();
}

AutoHideDockContainer::~AutoHideDockContainer() {
    if (m_animationTimer.IsRunning()) {
        m_animationTimer.Stop();
    }
}

void AutoHideDockContainer::slideIn() {
    if (m_isVisible && !m_isAnimating) return;
    
    m_isVisible = true;
    m_isAnimating = true;
    m_animationProgress = 0;
    
    Show();
    m_animationTimer.Start(16); // ~60 FPS
}

void AutoHideDockContainer::slideOut() {
    if (!m_isVisible && !m_isAnimating) return;
    
    m_isVisible = false;
    m_isAnimating = true;
    m_animationProgress = 100;
    
    m_animationTimer.Start(16); // ~60 FPS
}

void AutoHideDockContainer::setSize(const wxSize& size) {
    m_size = size;
}

void AutoHideDockContainer::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    // Additional painting if needed
}

void AutoHideDockContainer::OnTimer(wxTimerEvent& event) {
    updateAnimation();
}

void AutoHideDockContainer::OnMouseCaptureLost(wxMouseCaptureLostEvent& event) {
    // Handle mouse capture loss
    if (m_isVisible) {
        slideOut();
    }
}

void AutoHideDockContainer::OnKillFocus(wxFocusEvent& event) {
    // Check if focus went to a child window
    wxWindow* focusWindow = wxFindFocusDescendant(this);
    if (!focusWindow) {
        // Focus went outside this container, hide
        slideOut();
    }
    event.Skip();
}

void AutoHideDockContainer::OnPinButtonClick(wxCommandEvent& event) {
    // Restore to normal docked state
    if (m_container && m_container->dockManager()) {
        m_container->dockManager()->restoreFromAutoHide(m_dockWidget);
    }
}

void AutoHideDockContainer::updateAnimation() {
    const int animationStep = 10; // Percentage per frame
    
    if (m_isVisible) {
        // Sliding in
        m_animationProgress += animationStep;
        if (m_animationProgress >= 100) {
            m_animationProgress = 100;
            m_isAnimating = false;
            m_animationTimer.Stop();
        }
    } else {
        // Sliding out
        m_animationProgress -= animationStep;
        if (m_animationProgress <= 0) {
            m_animationProgress = 0;
            m_isAnimating = false;
            m_animationTimer.Stop();
            Hide();
        }
    }
    
    // Update position and size
    SetSize(calculateGeometry(m_animationProgress));
    Refresh();
}

wxRect AutoHideDockContainer::calculateGeometry(int progress) {
    wxRect containerRect = m_container->GetClientRect();
    wxRect targetRect;
    
    // Calculate full size position
    switch (m_sideBarLocation) {
    case SideBarLeft:
        targetRect = wxRect(0, 0, m_size.GetWidth(), containerRect.height);
        break;
    case SideBarRight:
        targetRect = wxRect(containerRect.width - m_size.GetWidth(), 0, 
                          m_size.GetWidth(), containerRect.height);
        break;
    case SideBarTop:
        targetRect = wxRect(0, 0, containerRect.width, m_size.GetHeight());
        break;
    case SideBarBottom:
        targetRect = wxRect(0, containerRect.height - m_size.GetHeight(),
                          containerRect.width, m_size.GetHeight());
        break;
    }
    
    // Apply animation progress
    if (progress < 100) {
        int offset = ((100 - progress) * targetRect.width) / 100;
        switch (m_sideBarLocation) {
        case SideBarLeft:
            targetRect.x -= offset;
            break;
        case SideBarRight:
            targetRect.x += offset;
            break;
        case SideBarTop:
            targetRect.y -= (((100 - progress) * targetRect.height) / 100);
            break;
        case SideBarBottom:
            targetRect.y += (((100 - progress) * targetRect.height) / 100);
            break;
        }
    }
    
    return targetRect;
}

// AutoHideManager implementation
AutoHideManager::AutoHideManager(DockContainerWidget* container)
    : m_container(container)
    , m_activeContainer(nullptr)
{
    // Initialize side bars array
    for (int i = 0; i < SideBarCount; ++i) {
        m_sideBars[i] = nullptr;
    }
    
    createSideBars();
}

AutoHideManager::~AutoHideManager() {
    // Clean up auto-hide containers
    for (auto& pair : m_autoHideContainers) {
        delete pair.second;
    }
    
    // Side bars are deleted by parent
}

void AutoHideManager::addAutoHideWidget(DockWidget* dockWidget, AutoHideSideBarLocation location) {
    if (!dockWidget) return;
    
    // Create side bar if needed
    if (!m_sideBars[location]) {
        createSideBars();
    }
    
    // Add to side bar
    m_sideBars[location]->addAutoHideWidget(dockWidget);
    
    // Create auto-hide container
    AutoHideDockContainer* container = new AutoHideDockContainer(dockWidget, location, m_container);
    m_autoHideContainers[dockWidget] = container;
    
    // Update visibility
    updateSideBarVisibility();
}

void AutoHideManager::removeAutoHideWidget(DockWidget* dockWidget) {
    if (!dockWidget) return;
    
    // Find and remove from side bar
    for (int i = 0; i < SideBarCount; ++i) {
        if (m_sideBars[i]) {
            m_sideBars[i]->removeAutoHideWidget(dockWidget);
        }
    }
    
    // Remove container
    auto it = m_autoHideContainers.find(dockWidget);
    if (it != m_autoHideContainers.end()) {
        delete it->second;
        m_autoHideContainers.erase(it);
    }
    
    updateSideBarVisibility();
}

void AutoHideManager::restoreDockWidget(DockWidget* dockWidget) {
    if (!dockWidget || !m_container) return;
    
    // Remove from auto-hide
    removeAutoHideWidget(dockWidget);
    
    // Add back to normal docking
    if (m_container->dockManager()) {
        m_container->dockManager()->addDockWidget(CenterDockWidgetArea, dockWidget);
    }
}

AutoHideSideBar* AutoHideManager::sideBar(AutoHideSideBarLocation location) const {
    if (location >= 0 && location < SideBarCount) {
        return m_sideBars[location];
    }
    return nullptr;
}

void AutoHideManager::showAutoHideWidget(DockWidget* dockWidget) {
    if (!dockWidget) return;
    
    auto it = m_autoHideContainers.find(dockWidget);
    if (it != m_autoHideContainers.end()) {
        // Hide current active container
        if (m_activeContainer && m_activeContainer != it->second) {
            m_activeContainer->slideOut();
        }
        
        // Show new container
        m_activeContainer = it->second;
        m_activeContainer->slideIn();
    }
}

void AutoHideManager::hideAutoHideWidget(DockWidget* dockWidget) {
    if (!dockWidget) return;
    
    auto it = m_autoHideContainers.find(dockWidget);
    if (it != m_autoHideContainers.end() && it->second == m_activeContainer) {
        m_activeContainer->slideOut();
        m_activeContainer = nullptr;
    }
}

AutoHideDockContainer* AutoHideManager::autoHideContainer(DockWidget* dockWidget) const {
    auto it = m_autoHideContainers.find(dockWidget);
    if (it != m_autoHideContainers.end()) {
        return it->second;
    }
    return nullptr;
}

bool AutoHideManager::hasAutoHideWidgets() const {
    return !m_autoHideContainers.empty();
}

std::vector<DockWidget*> AutoHideManager::autoHideWidgets() const {
    std::vector<DockWidget*> widgets;
    for (const auto& pair : m_autoHideContainers) {
        widgets.push_back(pair.first);
    }
    return widgets;
}

void AutoHideManager::saveState(wxString& xmlData) const {
    // TODO: Implement state saving
    xmlData = "<AutoHide />";
}

bool AutoHideManager::restoreState(const wxString& xmlData) {
    // TODO: Implement state restoration
    return true;
}

void AutoHideManager::createSideBars() {
    if (!m_container) return;
    
    // Get container's sizer
    wxSizer* containerSizer = m_container->GetSizer();
    if (!containerSizer) {
        containerSizer = new wxBoxSizer(wxVERTICAL);
        m_container->SetSizer(containerSizer);
    }
    
    // Create side bars if they don't exist
    for (int i = 0; i < SideBarCount; ++i) {
        if (!m_sideBars[i]) {
            m_sideBars[i] = new AutoHideSideBar(m_container, static_cast<AutoHideSideBarLocation>(i));
            
            // Position side bars
            // This is simplified - in real implementation, we'd need to properly integrate
            // with the container's layout system
            switch (i) {
            case SideBarLeft:
                // Add to left side
                break;
            case SideBarRight:
                // Add to right side
                break;
            case SideBarTop:
                // Add to top
                break;
            case SideBarBottom:
                // Add to bottom
                break;
            }
            
            m_sideBars[i]->Hide(); // Initially hidden
        }
    }
}

void AutoHideManager::updateSideBarVisibility() {
    for (int i = 0; i < SideBarCount; ++i) {
        if (m_sideBars[i]) {
            m_sideBars[i]->Show(m_sideBars[i]->hasVisibleTabs());
        }
    }
    
    if (m_container) {
        m_container->Layout();
    }
}

} // namespace ads
