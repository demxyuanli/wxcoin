#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/DockManager.h"
#include "docking/DockContainerWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockOverlay.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <wx/menu.h>
#include <wx/button.h>
#include <algorithm>

namespace ads {

// Define custom events
wxEventTypeTag<wxCommandEvent> DockArea::EVT_DOCK_AREA_CURRENT_CHANGED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockArea::EVT_DOCK_AREA_CLOSING(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockArea::EVT_DOCK_AREA_CLOSED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockArea::EVT_DOCK_AREA_TAB_ABOUT_TO_CLOSE(wxNewEventType());

wxEventTypeTag<wxCommandEvent> DockAreaTabBar::EVT_TAB_CLOSE_REQUESTED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockAreaTabBar::EVT_TAB_CURRENT_CHANGED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockAreaTabBar::EVT_TAB_MOVED(wxNewEventType());

wxEventTypeTag<wxCommandEvent> DockAreaTitleBar::EVT_TITLE_BAR_BUTTON_CLICKED(wxNewEventType());

// Event tables
wxBEGIN_EVENT_TABLE(DockArea, wxPanel)
    EVT_SIZE(DockArea::onSize)
    EVT_CLOSE(DockArea::onClose)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(DockAreaTabBar, wxPanel)
    EVT_PAINT(DockAreaTabBar::onPaint)
    EVT_LEFT_DOWN(DockAreaTabBar::onMouseLeftDown)
    EVT_LEFT_UP(DockAreaTabBar::onMouseLeftUp)
    EVT_RIGHT_DOWN(DockAreaTabBar::onMouseRightDown)
    EVT_MOTION(DockAreaTabBar::onMouseMotion)
    EVT_LEAVE_WINDOW(DockAreaTabBar::onMouseLeave)
    EVT_SIZE(DockAreaTabBar::onSize)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(DockAreaTitleBar, wxPanel)
    EVT_PAINT(DockAreaTitleBar::onPaint)
wxEND_EVENT_TABLE()

// Private implementation
class DockArea::Private {
public:
    Private(DockArea* parent) : q(parent) {}
    
    DockArea* q;
    bool allowTabs = true;
    bool titleBarVisible = true;
};

// DockArea implementation
DockArea::DockArea(DockManager* dockManager, DockContainerWidget* parent)
    : wxPanel(parent)
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockManager)
    , m_containerWidget(parent)
    , m_currentDockWidget(nullptr)
    , m_currentIndex(-1)
    , m_flags(DefaultFlags)
    , m_updateTitleBarButtons(false)
    , m_menuOutdated(true)
    , m_isClosing(false)
{
    // Create layout
    m_layout = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_layout);
    
    // Create merged title bar (combines tabs and title buttons)
    m_mergedTitleBar = new DockAreaMergedTitleBar(this);
    m_layout->Add(m_mergedTitleBar, 0, wxEXPAND | wxALL, 0);

    // Create legacy title bar (for backward compatibility, but not used in merged mode)
    m_titleBar = new DockAreaTitleBar(this);
    m_titleBar->Hide(); // Hide by default in merged mode
    
    // Create legacy tab bar (for backward compatibility, but not used in merged mode)
    m_tabBar = new DockAreaTabBar(this);
    m_tabBar->Hide(); // Hide by default in merged mode
    
    // Create content area (placeholder for dock widgets)
    m_contentArea = new wxPanel(this);
    m_contentSizer = new wxBoxSizer(wxVERTICAL);
    m_contentArea->SetSizer(m_contentSizer);
    m_layout->Add(m_contentArea, 1, wxEXPAND);  // Content area takes remaining space
    
    // Register with manager
    if (m_dockManager) {
        m_dockManager->registerDockArea(this);
    }
    
    // Update initial button states
    updateTitleBarButtonStates();
}

DockArea::~DockArea() {
    wxLogDebug("DockArea::~DockArea() - destroying area %p with %d widgets", this, (int)m_dockWidgets.size());
    
    // Clear all dock widgets to prevent them from being destroyed
    for (auto* widget : m_dockWidgets) {
        if (widget) {
            widget->setDockArea(nullptr);
            // Don't destroy the widgets, they should be managed elsewhere
        }
    }
    m_dockWidgets.clear();
    
    // Unregister from manager
    if (m_dockManager) {
        m_dockManager->unregisterDockArea(this);
    }
}

void DockArea::addDockWidget(DockWidget* dockWidget) {
    insertDockWidget(m_dockWidgets.size(), dockWidget, true);
}

void DockArea::removeDockWidget(DockWidget* dockWidget) {
    if (!dockWidget) {
        return;
    }
    
    // Find the widget
    auto it = std::find(m_dockWidgets.begin(), m_dockWidgets.end(), dockWidget);
    if (it == m_dockWidgets.end()) {
        return;
    }
    
    // Get index
    int index = std::distance(m_dockWidgets.begin(), it);
    
    // Remove from list
    m_dockWidgets.erase(it);
    
    // Remove from merged title bar or tab bar
    if (m_mergedTitleBar) {
        m_mergedTitleBar->removeTab(dockWidget);
    } else if (m_tabBar) {
    m_tabBar->removeTab(dockWidget);
    }
    
    // Clear dock area reference
    dockWidget->setDockArea(nullptr);
    
    // Update current widget
    if (m_currentDockWidget == dockWidget) {
        if (!m_dockWidgets.empty()) {
            // Select next widget
            int newIndex = std::min(index, static_cast<int>(m_dockWidgets.size()) - 1);
            setCurrentIndex(newIndex);
        } else {
            m_currentDockWidget = nullptr;
            m_currentIndex = -1;
        }
    }
    
    // Hide widget and remove from content area
    dockWidget->Hide();
    m_contentSizer->Detach(dockWidget);
    
    // Reparent to dock manager's container to keep it alive
    if (m_dockManager && m_dockManager->containerWidget()) {
        dockWidget->Reparent(m_dockManager->containerWidget());
    }
    
    // Update UI
    updateTitleBarVisibility();
    updateTabBar();
    
    // Close area if empty
    if (m_dockWidgets.empty()) {
        // Mark for deletion but don't delete immediately
        m_isClosing = true;
        
        // Notify container to remove this area
        if (m_containerWidget) {
            m_containerWidget->removeDockArea(this);
        }
        
        // The container will handle the actual destruction
    }
}

void DockArea::insertDockWidget(int index, DockWidget* dockWidget, bool activate) {
    if (!dockWidget) {
        return;
    }
    
    // Ensure valid index
    index = std::max(0, std::min(index, static_cast<int>(m_dockWidgets.size())));
    
    // Remove from old area if needed
    if (dockWidget->dockAreaWidget() && dockWidget->dockAreaWidget() != this) {
        dockWidget->dockAreaWidget()->removeDockWidget(dockWidget);
    }
    
    // Insert into list
    m_dockWidgets.insert(m_dockWidgets.begin() + index, dockWidget);
    
    // Set dock area
    dockWidget->setDockArea(this);
    
    // Add to merged title bar or tab bar
    if (m_mergedTitleBar) {
        m_mergedTitleBar->insertTab(index, dockWidget);
    } else if (m_tabBar) {
    m_tabBar->insertTab(index, dockWidget);
    }
    
    // Reparent widget to content area
    dockWidget->Reparent(m_contentArea);
    m_contentSizer->Add(dockWidget, 1, wxEXPAND);
    dockWidget->Hide(); // Initially hidden until activated
    
    // Activate if requested or if this is the first widget
    if (activate || m_dockWidgets.size() == 1) {
        setCurrentIndex(index);
    } else if (m_currentIndex < 0 && !m_dockWidgets.empty()) {
        // If no widget is currently active, activate the first one
        setCurrentIndex(0);
    }
    
    // Update UI
    updateTitleBarVisibility();
    updateTabBar();
}

DockWidget* DockArea::currentDockWidget() const {
    return m_currentDockWidget;
}

void DockArea::setCurrentDockWidget(DockWidget* dockWidget) {
    int index = indexOfDockWidget(dockWidget);
    if (index >= 0) {
        setCurrentIndex(index);
    }
}

int DockArea::currentIndex() const {
    return m_currentIndex;
}

void DockArea::setCurrentIndex(int index) {
    if (index < 0 || index >= static_cast<int>(m_dockWidgets.size())) {
        return;
    }
    
    if (m_currentIndex == index) {
        return;
    }
    
    // Hide old widget
    if (m_currentDockWidget && m_currentDockWidget->GetParent()) {
        // Verify the widget is still valid before calling Hide()
        if (std::find(m_dockWidgets.begin(), m_dockWidgets.end(), m_currentDockWidget) != m_dockWidgets.end()) {
            m_currentDockWidget->Hide();
        }
    }
    
    // Update current
    m_currentIndex = index;
    m_currentDockWidget = m_dockWidgets[index];
    
    // Show new widget
    if (m_currentDockWidget && m_currentDockWidget->GetParent()) {
        m_currentDockWidget->Show();
        m_currentDockWidget->SetFocus();
        m_contentArea->Layout();
    }
    
    // Update merged title bar or tab bar
    if (m_mergedTitleBar) {
        m_mergedTitleBar->setCurrentIndex(index);
    } else if (m_tabBar) {
    m_tabBar->setCurrentIndex(index);
    }
    
    // Update title in merged title bar
    if (m_mergedTitleBar) {
        m_mergedTitleBar->updateTitle();
    } else if (m_titleBar) {
    m_titleBar->updateTitle();
    }
    
    // Notify change
    wxCommandEvent event(EVT_DOCK_AREA_CURRENT_CHANGED);
    event.SetEventObject(this);
    event.SetInt(index);
    ProcessWindowEvent(event);
    
    Layout();
}

int DockArea::indexOfDockWidget(DockWidget* dockWidget) const {
    auto it = std::find(m_dockWidgets.begin(), m_dockWidgets.end(), dockWidget);
    if (it != m_dockWidgets.end()) {
        return std::distance(m_dockWidgets.begin(), it);
    }
    return -1;
}

DockWidget* DockArea::dockWidget(int index) const {
    if (index >= 0 && index < static_cast<int>(m_dockWidgets.size())) {
        return m_dockWidgets[index];
    }
    return nullptr;
}

void DockArea::setDockAreaFlags(DockAreaFlags flags) {
    m_flags = flags;
    updateTitleBarVisibility();
}

void DockArea::setDockAreaFlag(DockAreaFlag flag, bool on) {
    if (on) {
        m_flags |= flag;
    } else {
        m_flags &= ~flag;
    }
    updateTitleBarVisibility();
}

bool DockArea::testDockAreaFlag(DockAreaFlag flag) const {
    return (m_flags & flag) == flag;
}

void DockArea::toggleView(bool open) {
    Show(open);
}

void DockArea::setVisible(bool visible) {
    Show(visible);
    
    // Update widgets visibility
    for (auto* widget : m_dockWidgets) {
        if (widget == m_currentDockWidget) {
            widget->Show(visible);
        } else {
            widget->Show(false);
        }
    }
}

void DockArea::updateTitleBarVisibility() {
    bool visible = true;
    
    // Hide if only one widget and flag is set
    if (testDockAreaFlag(HideSingleWidgetTitleBar) && m_dockWidgets.size() <= 1) {
        visible = false;
    }
    
    // In merged mode, title bar is always visible
    if (m_mergedTitleBar) {
        m_mergedTitleBar->Show(true);
    } else if (m_titleBar) {
    m_titleBar->Show(visible);
    }
    
    // In merged mode, tab bar is part of title bar, so we don't show separate tab bar
    if (!m_mergedTitleBar) {
    // Always show tab bar if there are widgets
    bool showTabBar = m_dockWidgets.size() > 0;
    if (m_dockManager && !m_dockManager->testConfigFlag(AlwaysShowTabs) && m_dockWidgets.size() == 1) {
        showTabBar = false;
    }
        if (m_tabBar) {
    m_tabBar->Show(showTabBar);
        }
    
    // Make sure tab rects are updated
        if (showTabBar && m_tabBar) {
        m_tabBar->updateTabRects();
        }
    } else {
        // In merged mode, update tab rects in merged title bar
        if (m_mergedTitleBar) {
            // No need to call updateTabRects here as it's called in onSize
        }
    }
    
    Layout();
    Refresh();
}

bool DockArea::isAutoHide() const {
    // TODO: Implement auto-hide
    return false;
}

bool DockArea::isCurrent() const {
    // TODO: Implement current area tracking
    return false;
}

void DockArea::saveState(wxString& xmlData) const {
    // TODO: Implement state saving
    xmlData = "<DockArea />";
}

void DockArea::closeArea() {
    // Prevent recursive calls
    if (m_isClosing) {
        return;
    }
    
    // Check if this is the last dock area
    if (m_containerWidget && m_containerWidget->dockAreaCount() <= 1) {
        wxLogDebug("Cannot close the last dock area");
        // Optionally show a message to the user
        wxMessageBox("Cannot close the last dock area", "Warning", wxOK | wxICON_WARNING);
        return;
    }
    
    m_isClosing = true;
    
    // Notify closing
    wxCommandEvent closingEvent(EVT_DOCK_AREA_CLOSING);
    closingEvent.SetEventObject(this);
    ProcessWindowEvent(closingEvent);
    
    // Copy widget list to avoid modification during iteration
    std::vector<DockWidget*> widgetsToClose = m_dockWidgets;
    
    // Clear the widget list first to prevent recursive closeArea calls
    m_dockWidgets.clear();
    
    // Close all widgets
    for (auto* widget : widgetsToClose) {
        widget->setDockArea(nullptr);  // Clear reference first
        widget->closeDockWidget();
    }
    
    // Notify closed (before removal to ensure object is still valid)
    wxCommandEvent closedEvent(EVT_DOCK_AREA_CLOSED);
    closedEvent.SetEventObject(this);
    ProcessWindowEvent(closedEvent);
    
    // Remove from container (this will destroy the area)
    if (m_containerWidget) {
        m_containerWidget->removeDockArea(this);
    }
    
    // Don't call Destroy() here - the container will handle it
}

void DockArea::closeOtherAreas() {
    if (!m_containerWidget) {
        return;
    }
    
    std::vector<DockArea*> areas = m_containerWidget->dockAreas();
    for (auto* area : areas) {
        if (area != this) {
            area->closeArea();
        }
    }
}

wxString DockArea::currentTabTitle() const {
    if (m_currentDockWidget) {
        return m_currentDockWidget->title();
    }
    return wxEmptyString;
}

void DockArea::onTabCloseRequested(int index) {
    DockWidget* widget = dockWidget(index);
    if (widget) {
        // Notify about tab close
        wxCommandEvent event(EVT_DOCK_AREA_TAB_ABOUT_TO_CLOSE);
        event.SetEventObject(this);
        event.SetInt(index);
        ProcessWindowEvent(event);
        
        // Close widget
        widget->closeDockWidget();
    }
}

void DockArea::onCurrentTabChanged(int index) {
    setCurrentIndex(index);
}

void DockArea::onTitleBarButtonClicked() {
    // Handle title bar button clicks
}

void DockArea::updateTitleBarButtonStates() {
    if (m_mergedTitleBar) {
        m_mergedTitleBar->updateButtonStates();
    } else if (m_titleBar) {
    m_titleBar->updateButtonStates();
    }
}

void DockArea::updateTabBar() {
    // Tab bar updates itself
}

void DockArea::internalSetCurrentDockWidget(DockWidget* dockWidget) {
    m_currentDockWidget = dockWidget;
}

void DockArea::markTitleBarMenuOutdated() {
    m_menuOutdated = true;
}

void DockArea::onSize(wxSizeEvent& event) {
    // Force refresh of merged title bar to prevent ghosting during window resize
    if (m_mergedTitleBar) {
        m_mergedTitleBar->Refresh();
        m_mergedTitleBar->Update();
    }
    
    event.Skip();
}

void DockArea::onClose(wxCloseEvent& event) {
    closeArea();
}

// DockAreaTabBar implementation
DockAreaTabBar::DockAreaTabBar(DockArea* dockArea)
    : wxPanel(dockArea)
    , m_dockArea(dockArea)
    , m_currentIndex(-1)
    , m_hoveredTab(-1)
    , m_draggedTab(-1)
    , m_dragStarted(false)
    , m_dragPreview(nullptr)
    , m_hasOverflow(false)
    , m_firstVisibleTab(0)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, 30));
}

DockAreaTabBar::~DockAreaTabBar() {
}

void DockAreaTabBar::insertTab(int index, DockWidget* dockWidget) {
    TabInfo tab;
    tab.widget = dockWidget;
    tab.closeButtonHovered = false;
    
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        m_tabs.push_back(tab);
    } else {
        m_tabs.insert(m_tabs.begin() + index, tab);
    }
    
    checkTabOverflow();
    updateTabRects();
    Refresh();
}

void DockAreaTabBar::removeTab(DockWidget* dockWidget) {
    auto it = std::find_if(m_tabs.begin(), m_tabs.end(),
        [dockWidget](const TabInfo& tab) { return tab.widget == dockWidget; });
    
    if (it != m_tabs.end()) {
        m_tabs.erase(it);
        checkTabOverflow();
        updateTabRects();
        Refresh();
    }
}

void DockAreaTabBar::setCurrentIndex(int index) {
    if (m_currentIndex != index) {
        m_currentIndex = index;
        
        // Ensure current tab is visible
        if (m_hasOverflow) {
            checkTabOverflow();
            updateTabRects();
        }
        
        Refresh();
    }
}

bool DockAreaTabBar::isTabOpen(int index) const {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        return !m_tabs[index].widget->isClosed();
    }
    return false;
}

void DockAreaTabBar::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    
    // Clear background
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.Clear();
    
    // Draw tabs
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (m_tabs[i].rect.IsEmpty()) {
            continue; // Skip non-visible tabs
        }
        drawTab(dc, i);
    }
    
    // Draw overflow button if needed
    if (m_hasOverflow) {
        dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
        dc.DrawRectangle(m_overflowButtonRect);
        
        // Draw arrow down symbol
        int centerX = m_overflowButtonRect.GetLeft() + m_overflowButtonRect.GetWidth() / 2;
        int centerY = m_overflowButtonRect.GetTop() + m_overflowButtonRect.GetHeight() / 2;
        
        wxPoint arrow[3];
        arrow[0] = wxPoint(centerX - 5, centerY - 2);
        arrow[1] = wxPoint(centerX + 5, centerY - 2);
        arrow[2] = wxPoint(centerX, centerY + 3);
        
        dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawPolygon(3, arrow);
    }
}

void DockAreaTabBar::onMouseLeftDown(wxMouseEvent& event) {
    // Check if overflow button clicked
    if (m_hasOverflow && m_overflowButtonRect.Contains(event.GetPosition())) {
        showTabOverflowMenu();
        return;
    }
    
    int tab = getTabAt(event.GetPosition());
    if (tab >= 0) {
        // Check if close button clicked
        if (m_tabs[tab].closeButtonRect.Contains(event.GetPosition())) {
            m_dockArea->onTabCloseRequested(tab);
        } else {
            // Start dragging
            m_draggedTab = tab;
            m_dragStartPos = event.GetPosition();
            
            // Also handle right-click for context menu
            if (event.RightDown()) {
                showTabContextMenu(tab, event.GetPosition());
                return;
            }
            
            // Select tab
            if (tab != m_currentIndex) {
                wxCommandEvent evt(EVT_TAB_CURRENT_CHANGED);
                evt.SetEventObject(this);
                evt.SetInt(tab);
                ProcessWindowEvent(evt);
                
                m_dockArea->onCurrentTabChanged(tab);
            }
            
            CaptureMouse();
        }
    }
}

void DockAreaTabBar::onMouseLeftUp(wxMouseEvent& event) {
    if (HasCapture()) {
        ReleaseMouse();
    }
    
    // Handle drop if we were dragging
    if (m_dragStarted && m_draggedTab >= 0) {
        // Clean up drag preview
        if (m_dragPreview) {
            m_dragPreview->finishDrag();
            m_dragPreview->Destroy();
            m_dragPreview = nullptr;
        }
        
        // Get the widget being dragged
        DockWidget* draggedWidget = m_dockArea->dockWidget(m_draggedTab);
        DockManager* manager = m_dockArea ? m_dockArea->dockManager() : nullptr;
        if (draggedWidget && manager) {
            // End batch operation when drag finishes
            manager->endBatchOperation();
            // Check for drop target
            wxPoint screenPos = ClientToScreen(event.GetPosition());
            wxWindow* windowUnderMouse = wxFindWindowAtPoint(screenPos);
            
            // Find target area
            DockArea* targetArea = nullptr;
            wxWindow* checkWindow = windowUnderMouse;
            while (checkWindow && !targetArea) {
                targetArea = dynamic_cast<DockArea*>(checkWindow);
                if (!targetArea) {
                    checkWindow = checkWindow->GetParent();
                }
            }
            
            bool docked = false;
            
            wxLogDebug("DockAreaTabBar::onMouseLeftUp - targetArea: %p", targetArea);
            
            // Try to dock if we have a target
            if (targetArea) {
                // Check overlay for drop position
                DockOverlay* overlay = manager->dockAreaOverlay();
                wxLogDebug("Area overlay: %p, IsShown: %d", overlay, overlay ? overlay->IsShown() : 0);
                
                if (overlay && overlay->IsShown()) {
                    DockWidgetArea dropArea = overlay->dropAreaUnderCursor();
                    wxLogDebug("Drop area under cursor: %d", dropArea);
                    
                    if (dropArea != InvalidDockWidgetArea) {
                        // Remove widget from current area if needed
                        if (draggedWidget->dockAreaWidget() == m_dockArea) {
                            m_dockArea->removeDockWidget(draggedWidget);
                        }
                        
                        if (dropArea == CenterDockWidgetArea) {
                            // Add as tab
                            wxLogDebug("Adding widget as tab to target area");
                            targetArea->addDockWidget(draggedWidget);
                            docked = true;
                        } else {
                            // Dock to side
                            wxLogDebug("Docking widget to side: %d", dropArea);
                            targetArea->dockContainer()->addDockWidget(dropArea, draggedWidget, targetArea);
                            docked = true;
                        }
                        
                        // Hide overlays before returning
                        if (manager) {
                            DockOverlay* areaOverlay = manager->dockAreaOverlay();
                            if (areaOverlay) {
                                areaOverlay->hideOverlay();
                            }
                            DockOverlay* containerOverlay = manager->containerOverlay();
                            if (containerOverlay) {
                                containerOverlay->hideOverlay();
                            }
                        }
                        
                        // Return early since the area might be destroyed
                        return;
                    }
                }
            }
            
            // If not docked to a specific area, check container overlay
            if (!docked && manager) {
                DockOverlay* containerOverlay = manager->containerOverlay();
                wxLogDebug("Container overlay: %p, IsShown: %d", containerOverlay, containerOverlay ? containerOverlay->IsShown() : 0);
                
                if (containerOverlay && containerOverlay->IsShown()) {
                    DockWidgetArea dropArea = containerOverlay->dropAreaUnderCursor();
                    wxLogDebug("Container drop area under cursor: %d", dropArea);
                    
                    if (dropArea != InvalidDockWidgetArea) {
                        // Remove widget from current area
                        if (draggedWidget->dockAreaWidget() == m_dockArea) {
                            wxLogDebug("Removing widget from current area");
                            m_dockArea->removeDockWidget(draggedWidget);
                        }
                        
                        // Verify widget is still valid
                        if (!draggedWidget->GetParent()) {
                            wxLogDebug("ERROR: Widget has no parent after removal!");
                            return;
                        }
                        
                        // Add to container at specified position
                        wxLogDebug("Adding widget to container at position %d", dropArea);
                        wxLogDebug("Widget ptr: %p, title: %s", draggedWidget, draggedWidget->title().c_str());
                        manager->addDockWidget(dropArea, draggedWidget);
                        docked = true;
                        
                        // Hide overlays before returning
                        if (manager) {
                            DockOverlay* areaOverlay = manager->dockAreaOverlay();
                            if (areaOverlay) {
                                areaOverlay->hideOverlay();
                            }
                            DockOverlay* containerOverlay = manager->containerOverlay();
                            if (containerOverlay) {
                                containerOverlay->hideOverlay();
                            }
                        }
                        
                        // Return early since the area might be destroyed
                        return;
                    }
                }
            }
            
            // If not docked, create floating container
            if (!docked) {
                wxLogDebug("Not docked - creating floating container");
                
                // Remove from current area if still there
                if (draggedWidget->dockAreaWidget() == m_dockArea) {
                    m_dockArea->removeDockWidget(draggedWidget);
                }
                
                // Set as floating
                draggedWidget->setFloating();
                
                FloatingDockContainer* floatingContainer = draggedWidget->floatingDockContainer();
                if (floatingContainer) {
                    floatingContainer->SetPosition(screenPos - wxPoint(50, 10));
                    floatingContainer->Show();
                    floatingContainer->Raise();
                }
                
                // Hide overlays using saved manager reference
                if (manager) {
                    DockOverlay* areaOverlay = manager->dockAreaOverlay();
                    if (areaOverlay) {
                        areaOverlay->hideOverlay();
                    }
                    DockOverlay* containerOverlay = manager->containerOverlay();
                    if (containerOverlay) {
                        containerOverlay->hideOverlay();
                    }
                }
                
                // Return early since the area might be destroyed
                return;
            }
        }
        
        // Hide any overlays that might be showing
        if (m_dockArea && m_dockArea->dockManager()) {
            DockOverlay* areaOverlay = m_dockArea->dockManager()->dockAreaOverlay();
            if (areaOverlay) {
                areaOverlay->hideOverlay();
            }
            DockOverlay* containerOverlay = m_dockArea->dockManager()->containerOverlay();
            if (containerOverlay) {
                containerOverlay->hideOverlay();
            }
        }
    }
    
    // Clear tooltip
    UnsetToolTip();
    
    m_draggedTab = -1;
    m_dragStarted = false;
}

void DockAreaTabBar::onMouseMotion(wxMouseEvent& event) {
    // Save manager reference at the beginning
    DockManager* manager = m_dockArea ? m_dockArea->dockManager() : nullptr;
    
    // Update hovered tab
    int oldHovered = m_hoveredTab;
    m_hoveredTab = getTabAt(event.GetPosition());
    
    // Update close button hover state
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        bool wasHovered = m_tabs[i].closeButtonHovered;
        m_tabs[i].closeButtonHovered = (i == m_hoveredTab && 
            m_tabs[i].closeButtonRect.Contains(event.GetPosition()));
        
        if (wasHovered != m_tabs[i].closeButtonHovered) {
            RefreshRect(m_tabs[i].closeButtonRect);
        }
    }
    
    if (oldHovered != m_hoveredTab) {
        Refresh();
    }
    
    // Handle dragging
    if (m_draggedTab >= 0 && event.Dragging()) {
        wxPoint delta = event.GetPosition() - m_dragStartPos;
        
        // Check if we should start drag operation (require minimum drag distance)
        if (!m_dragStarted && (abs(delta.x) > 5 || abs(delta.y) > 5)) {
            m_dragStarted = true;
            
            // Get the dock widget being dragged
            DockWidget* draggedWidget = m_dockArea->dockWidget(m_draggedTab);
            if (draggedWidget && draggedWidget->hasFeature(DockWidgetMovable) && manager) {
                // Begin batch operation to prevent excessive refreshes during drag
                manager->beginBatchOperation();
                
                // Create a floating drag preview
                FloatingDragPreview* preview = new FloatingDragPreview(draggedWidget, manager->containerWidget());
                wxPoint screenPos = ClientToScreen(event.GetPosition());
                preview->startDrag(screenPos);
                
                // Store preview reference
                m_dragPreview = preview;
                
                // Mark widget as being dragged (don't actually float yet)
                // We'll handle the actual move on drop
            }
        }
        
        if (m_dragStarted) {
            wxPoint screenPos = ClientToScreen(event.GetPosition());
            
            // Update drag preview position
            if (m_dragPreview) {
                m_dragPreview->moveFloating(screenPos);
            }
            
            // Check for drop targets under mouse
            wxWindow* windowUnderMouse = wxFindWindowAtPoint(screenPos);
            
            // Skip the drag preview window itself
            if (windowUnderMouse && m_dragPreview) {
                if (windowUnderMouse == m_dragPreview || windowUnderMouse->GetParent() == m_dragPreview) {
                    // Try to find window below the preview
                    m_dragPreview->Hide();
                    windowUnderMouse = wxFindWindowAtPoint(screenPos);
                    m_dragPreview->Show();
                }
            }
            
            // Show overlay on potential drop targets
            DockArea* targetArea = nullptr;
            wxWindow* checkWindow = windowUnderMouse;
            
            // First check if we're over a tab bar
            DockAreaTabBar* targetTabBar = dynamic_cast<DockAreaTabBar*>(windowUnderMouse);
            if (!targetTabBar && windowUnderMouse) {
                // Check parent in case we're over a child of tab bar
                targetTabBar = dynamic_cast<DockAreaTabBar*>(windowUnderMouse->GetParent());
            }
            
            // Find DockArea in parent hierarchy
            while (checkWindow && !targetArea) {
                targetArea = dynamic_cast<DockArea*>(checkWindow);
                if (!targetArea) {
                    checkWindow = checkWindow->GetParent();
                }
            }
            
            if (targetArea && manager) {
                wxLogDebug("Found target DockArea, showing overlay");
                DockOverlay* overlay = manager->dockAreaOverlay();
                if (overlay) {
                    // If over tab bar, only show center drop area for tab merge
                    if (targetTabBar && targetTabBar->GetParent() == targetArea) {
                        wxLogDebug("Over tab bar - showing center drop area only");
                        overlay->showOverlay(targetArea);
                        overlay->setAllowedAreas(CenterDockWidgetArea);  // Only allow center drop
                    } else {
                        overlay->showOverlay(targetArea);
                        overlay->setAllowedAreas(AllDockAreas);  // Allow all areas
                    }
                } else {
                    wxLogDebug("No area overlay available");
                }
            } else if (manager) {
                // Check for container overlay
                DockContainerWidget* container = manager->containerWidget() ? 
                    dynamic_cast<DockContainerWidget*>(manager->containerWidget()) : nullptr;
                    
                if (container && container->GetScreenRect().Contains(screenPos)) {
                    wxLogDebug("Over container, showing container overlay");
                    DockOverlay* overlay = manager->containerOverlay();
                    if (overlay) {
                        overlay->showOverlay(container);
                    }
                } else {
                    // Hide overlays if not over any target
                    if (manager->dockAreaOverlay()) {
                        manager->dockAreaOverlay()->hideOverlay();
                    }
                    if (manager->containerOverlay()) {
                        manager->containerOverlay()->hideOverlay();
                    }
                }
            }
        }
    }
}

void DockAreaTabBar::onMouseLeave(wxMouseEvent& event) {
    m_hoveredTab = -1;
    
    // Clear close button hover states
    for (auto& tab : m_tabs) {
        tab.closeButtonHovered = false;
    }
    
    Refresh();
}

int DockAreaTabBar::getTabAt(const wxPoint& pos) {
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (m_tabs[i].rect.Contains(pos)) {
            return i;
        }
    }
    return -1;
}

void DockAreaTabBar::updateTabRects() {
    wxSize size = GetClientSize();
    const int defaultTabWidth = 120;
    const int overflowButtonWidth = 30;
    int x = 0;
    
    // Clear all tab rects first
    for (auto& tab : m_tabs) {
        tab.rect = wxRect();
        tab.closeButtonRect = wxRect();
    }
    
    // Calculate available width
    int maxWidth = m_hasOverflow ? size.GetWidth() - overflowButtonWidth : size.GetWidth();
    
    // Layout visible tabs
    for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
        auto& tab = m_tabs[i];
        
        // Check if this tab would exceed available width
        if (x + defaultTabWidth > maxWidth) {
            break; // Stop laying out tabs
        }
        
        tab.rect = wxRect(x, 0, defaultTabWidth, size.GetHeight());
        
        // Close button rect
        int closeSize = 16;
        int closePadding = (size.GetHeight() - closeSize) / 2;
        tab.closeButtonRect = wxRect(
            tab.rect.GetRight() - closeSize - closePadding,
            closePadding,
            closeSize,
            closeSize
        );
        
        x += defaultTabWidth;
    }
}

void DockAreaTabBar::drawTab(wxDC& dc, int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        return;
    }
    
    const TabInfo& tab = m_tabs[index];
    bool isCurrent = (index == m_currentIndex);
    bool isHovered = (index == m_hoveredTab);
    
    // Clear the tab area first to prevent ghosting
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(tab.rect);
    
    // Draw tab background
    wxColour bgColor;
    if (isCurrent) {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    } else if (isHovered) {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT);
    } else {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    }
    
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(tab.rect);
    
    // Draw tab border
    if (isCurrent) {
        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
        dc.DrawLine(tab.rect.GetLeft(), tab.rect.GetTop(), 
                   tab.rect.GetLeft(), tab.rect.GetBottom());
        dc.DrawLine(tab.rect.GetLeft(), tab.rect.GetTop(), 
                   tab.rect.GetRight(), tab.rect.GetTop());
        dc.DrawLine(tab.rect.GetRight() - 1, tab.rect.GetTop(), 
                   tab.rect.GetRight() - 1, tab.rect.GetBottom());
    }
    
    // Draw tab text
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    
    wxString title = tab.widget->title();
    wxRect textRect = tab.rect;
    textRect.Deflate(5, 0);
    textRect.width -= 20; // Space for close button
    
    dc.DrawLabel(title, textRect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
    
    // Draw close button
    if (tab.widget->hasFeature(DockWidgetClosable)) {
        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT)));
        
        if (tab.closeButtonHovered) {
            dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT)));
            dc.DrawRectangle(tab.closeButtonRect);
        }
        
        // Draw X
        int margin = 4;
        dc.DrawLine(
            tab.closeButtonRect.GetLeft() + margin,
            tab.closeButtonRect.GetTop() + margin,
            tab.closeButtonRect.GetRight() - margin,
            tab.closeButtonRect.GetBottom() - margin
        );
        dc.DrawLine(
            tab.closeButtonRect.GetRight() - margin,
            tab.closeButtonRect.GetTop() + margin,
            tab.closeButtonRect.GetLeft() + margin,
            tab.closeButtonRect.GetBottom() - margin
        );
    }
}

void DockAreaTabBar::onSize(wxSizeEvent& event) {
    checkTabOverflow();
    updateTabRects();
    Refresh();
    event.Skip();
}

void DockAreaTabBar::checkTabOverflow() {
    if (m_tabs.empty()) {
        m_hasOverflow = false;
        m_firstVisibleTab = 0;
        return;
    }
    
    // Calculate total width needed for all tabs
    const int tabWidth = 120;
    const int closeButtonWidth = 20;
    const int overflowButtonWidth = 30;
    
    int totalTabsWidth = 0;
    for (const auto& tab : m_tabs) {
        totalTabsWidth += tabWidth;
        if (tab.widget->hasFeature(DockWidgetClosable)) {
            totalTabsWidth += closeButtonWidth;
        }
    }
    
    int availableWidth = GetClientSize().GetWidth();
    
    // Check if we need overflow
    if (totalTabsWidth > availableWidth - overflowButtonWidth) {
        m_hasOverflow = true;
        
        // Reserve space for overflow button
        m_overflowButtonRect = wxRect(
            availableWidth - overflowButtonWidth, 0,
            overflowButtonWidth, GetClientSize().GetHeight()
        );
        
        // Ensure current tab is visible
        if (m_currentIndex >= 0) {
            // Calculate how many tabs can fit
            int visibleTabsWidth = 0;
            int visibleTabsCount = 0;
            
            for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
                int tabWidth = 120;
                if (m_tabs[i].widget->hasFeature(DockWidgetClosable)) {
                    tabWidth += closeButtonWidth;
                }
                
                if (visibleTabsWidth + tabWidth > availableWidth - overflowButtonWidth) {
                    break;
                }
                
                visibleTabsWidth += tabWidth;
                visibleTabsCount++;
            }
            
            // Adjust first visible tab if current tab is not visible
            if (m_currentIndex < m_firstVisibleTab) {
                m_firstVisibleTab = m_currentIndex;
            } else if (m_currentIndex >= m_firstVisibleTab + visibleTabsCount) {
                m_firstVisibleTab = m_currentIndex - visibleTabsCount + 1;
                if (m_firstVisibleTab < 0) m_firstVisibleTab = 0;
            }
        }
    } else {
        m_hasOverflow = false;
        m_firstVisibleTab = 0;
    }
}

void DockAreaTabBar::showTabOverflowMenu() {
    wxMenu menu;
    
    // Add all tabs to menu
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        wxString title = m_tabs[i].widget->title();
        if (i == m_currentIndex) {
            title = "> " + title; // Add marker for current tab
        }
        
        wxMenuItem* item = menu.Append(wxID_ANY, title);
        
        // Bind menu item to tab selection
        menu.Bind(wxEVT_MENU, [this, i](wxCommandEvent&) {
            setCurrentIndex(i);
        }, item->GetId());
    }
    
    // Show menu at overflow button position
    wxPoint pos = m_overflowButtonRect.GetBottomLeft();
    PopupMenu(&menu, pos);
}



// Event table for DockAreaMergedTitleBar
wxBEGIN_EVENT_TABLE(DockAreaMergedTitleBar, wxPanel)
    EVT_PAINT(DockAreaMergedTitleBar::onPaint)
    EVT_LEFT_DOWN(DockAreaMergedTitleBar::onMouseLeftDown)
    EVT_LEFT_UP(DockAreaMergedTitleBar::onMouseLeftUp)
    EVT_MOTION(DockAreaMergedTitleBar::onMouseMotion)
    EVT_LEAVE_WINDOW(DockAreaMergedTitleBar::onMouseLeave)
    EVT_SIZE(DockAreaMergedTitleBar::onSize)
wxEND_EVENT_TABLE()

// DockAreaMergedTitleBar implementation
DockAreaMergedTitleBar::DockAreaMergedTitleBar(DockArea* dockArea)
    : wxPanel(dockArea)
    , m_dockArea(dockArea)
    , m_currentIndex(-1)
    , m_hoveredTab(-1)
    , m_buttonSize(20)
    , m_buttonSpacing(2)
    , m_showCloseButton(true)
    , m_showAutoHideButton(false)
    , m_showPinButton(true)
    , m_draggedTab(-1)
    , m_dragStarted(false)
    , m_dragPreview(nullptr)
    , m_pinButtonHovered(false)
    , m_closeButtonHovered(false)
    , m_autoHideButtonHovered(false)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, 30)); // Slightly taller than original to accommodate both tabs and buttons
}

DockAreaMergedTitleBar::~DockAreaMergedTitleBar() {
    // Clean up drag preview if it exists
    if (m_dragPreview) {
        if (!m_dragPreview->IsBeingDeleted()) {
            m_dragPreview->finishDrag();
            m_dragPreview->Destroy();
        }
        m_dragPreview = nullptr;
    }
}

void DockAreaMergedTitleBar::updateTitle() {
    Refresh();
}

void DockAreaMergedTitleBar::updateButtonStates() {
    // Update button visibility based on features
    if (m_dockArea && m_dockArea->dockContainer()) {
        bool canCloseArea = m_dockArea->dockContainer()->dockAreaCount() > 1;
        m_showCloseButton = canCloseArea;
    }

    // Update tab close button visibility - hide when only one tab
    bool hasMultipleTabs = (m_tabs.size() > 1);
    for (auto& tab : m_tabs) {
        tab.showCloseButton = hasMultipleTabs && tab.widget &&
                              tab.widget->hasFeature(DockWidgetClosable);
    }

    Refresh();
}

void DockAreaMergedTitleBar::insertTab(int index, DockWidget* dockWidget) {
    if (!dockWidget) return;

    TabInfo tab;
    tab.widget = dockWidget;

    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        m_tabs.push_back(tab);
    } else {
        m_tabs.insert(m_tabs.begin() + index, tab);
    }

    updateButtonStates(); // Update button visibility after tab insertion
    updateTabRects();
    Refresh();
}

void DockAreaMergedTitleBar::removeTab(DockWidget* dockWidget) {
    auto it = std::find_if(m_tabs.begin(), m_tabs.end(),
        [dockWidget](const TabInfo& tab) { return tab.widget == dockWidget; });

    if (it != m_tabs.end()) {
        m_tabs.erase(it);
        updateButtonStates(); // Update button visibility after tab removal
        updateTabRects();
        Refresh();
    }
}

void DockAreaMergedTitleBar::setCurrentIndex(int index) {
    if (m_currentIndex != index) {
        m_currentIndex = index;
        Refresh();
    }
}

DockWidget* DockAreaMergedTitleBar::getTabWidget(int index) const {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        return m_tabs[index].widget;
    }
    return nullptr;
}

void DockAreaMergedTitleBar::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    wxRect clientRect = GetClientRect();

    // Clear the entire area first to prevent ghosting
    dc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.Clear();

    // Draw background
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(clientRect);

    // Draw bottom border
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
    dc.DrawLine(0, clientRect.GetHeight() - 1, clientRect.GetWidth(), clientRect.GetHeight() - 1);

    // Draw tabs on the left side
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (!m_tabs[i].rect.IsEmpty()) {
            drawTab(dc, i);
        }
    }

    // Draw buttons on the right side
    int buttonX = clientRect.GetWidth() - m_buttonSpacing;
    int buttonCount = 0;

    if (m_showAutoHideButton) {
        buttonX -= m_buttonSize;
        m_autoHideButtonRect = wxRect(buttonX, (clientRect.GetHeight() - m_buttonSize) / 2,
                                     m_buttonSize, m_buttonSize);
        drawButton(dc, m_autoHideButtonRect, "^", m_autoHideButtonHovered);
        buttonX -= m_buttonSpacing;
        buttonCount++;
    }

    if (m_showCloseButton) {
        buttonX -= m_buttonSize;
        m_closeButtonRect = wxRect(buttonX, (clientRect.GetHeight() - m_buttonSize) / 2,
                                  m_buttonSize, m_buttonSize);
        drawButton(dc, m_closeButtonRect, "X", m_closeButtonHovered);
        buttonX -= m_buttonSpacing;
        buttonCount++;
    }

    if (m_showPinButton) {
        buttonX -= m_buttonSize;
        m_pinButtonRect = wxRect(buttonX, (clientRect.GetHeight() - m_buttonSize) / 2,
                                m_buttonSize, m_buttonSize);
        drawButton(dc, m_pinButtonRect, "P", m_pinButtonHovered);
        buttonCount++;
    }
}

void DockAreaMergedTitleBar::onMouseLeftDown(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();

    // Check if clicked on a tab
    int tabIndex = getTabAt(pos);
    if (tabIndex >= 0) {
        // Check if close button clicked - only if close button should be shown
        if (tabIndex < static_cast<int>(m_tabs.size()) &&
            m_tabs[tabIndex].showCloseButton &&
            m_tabs[tabIndex].closeButtonRect.Contains(pos) &&
            m_dockArea->dockWidget(tabIndex)->hasFeature(DockWidgetClosable)) {
            // Handle close button click
            m_dockArea->onTabCloseRequested(tabIndex);
            return;
        }

        // Start dragging
        m_draggedTab = tabIndex;
        m_dragStartPos = event.GetPosition();

        // Select tab
        if (tabIndex != m_currentIndex) {
            m_dockArea->setCurrentIndex(tabIndex);
        }

        // TODO: Re-enable cursor setting after fixing wxWidgets resource issues
        // For now, focus on core drag functionality

        CaptureMouse();
        return;
    }

    // Check if clicked on a button
    if (m_showCloseButton && m_closeButtonRect.Contains(pos)) {
        m_dockArea->closeArea();
    } else if (m_showAutoHideButton && m_autoHideButtonRect.Contains(pos)) {
        // TODO: Implement auto-hide
        wxMessageBox("Auto-hide feature not yet implemented", "Info", wxOK | wxICON_INFORMATION);
    } else if (m_showPinButton && m_pinButtonRect.Contains(pos)) {
        // TODO: Implement pin/unpin
        wxMessageBox("Pin/Unpin feature not yet implemented", "Info", wxOK | wxICON_INFORMATION);
    }
}

void DockAreaMergedTitleBar::onMouseLeftUp(wxMouseEvent& event) {
    if (HasCapture()) {
        ReleaseMouse();
    }

    // Handle drop if we were dragging
    if (m_dragStarted && m_draggedTab >= 0) {
        // Clean up drag preview
        if (m_dragPreview) {
            m_dragPreview->finishDrag();
            m_dragPreview->Destroy();
            m_dragPreview = nullptr;
        }

        // Get the widget being dragged
        DockWidget* draggedWidget = m_dockArea->dockWidget(m_draggedTab);
        DockManager* manager = m_dockArea ? m_dockArea->dockManager() : nullptr;
        if (draggedWidget && manager) {
            // Check for drop target
            wxPoint screenPos = ClientToScreen(event.GetPosition());
            wxWindow* windowUnderMouse = findTargetWindowUnderMouse(screenPos, m_dragPreview);

            // Find target area
            DockArea* targetArea = nullptr;
            wxWindow* checkWindow = windowUnderMouse;
            while (checkWindow && !targetArea) {
                targetArea = dynamic_cast<DockArea*>(checkWindow);
                if (!targetArea) {
                    checkWindow = checkWindow->GetParent();
                }
            }

            bool docked = false;

            wxLogDebug("DockAreaMergedTitleBar::onMouseLeftUp - targetArea: %p", targetArea);

            // Try to dock if we have a target
            if (targetArea) {
                // Check overlay for drop position
                DockOverlay* overlay = manager->dockAreaOverlay();
                wxLogDebug("Area overlay: %p, IsShown: %d", overlay, overlay ? overlay->IsShown() : 0);

                if (overlay && overlay->IsShown()) {
                    DockWidgetArea dropArea = overlay->dropAreaUnderCursor();
                    wxLogDebug("Drop area under cursor: %d", dropArea);

                    if (dropArea != InvalidDockWidgetArea) {
                        // Remove widget from current area if needed
                        if (draggedWidget->dockAreaWidget() == m_dockArea) {
                            m_dockArea->removeDockWidget(draggedWidget);
                        }

                        if (dropArea == CenterDockWidgetArea) {
                            // Add as tab - merge with existing tabs
                            wxLogDebug("Adding widget as tab to target area (merging tabs)");
                            targetArea->addDockWidget(draggedWidget);

                            // If the target area has merged title bar, make sure the new tab becomes current
                            if (targetArea->mergedTitleBar()) {
                                targetArea->setCurrentDockWidget(draggedWidget);
                            }

                            docked = true;
                        } else {
                            // Dock to side
                            wxLogDebug("Docking widget to side: %d", dropArea);
                            targetArea->dockContainer()->addDockWidget(dropArea, draggedWidget, targetArea);
                            docked = true;
                        }

                        // Hide overlays before returning
                        if (manager) {
                            DockOverlay* areaOverlay = manager->dockAreaOverlay();
                            if (areaOverlay) {
                                areaOverlay->hideOverlay();
                            }
                            DockOverlay* containerOverlay = manager->containerOverlay();
                            if (containerOverlay) {
                                containerOverlay->hideOverlay();
                            }
                        }

                        // Return early since the area might be destroyed
                        return;
                    }
                }
            }

            // If not docked to a specific area, check container overlay
            if (!docked && manager) {
                DockOverlay* containerOverlay = manager->containerOverlay();
                wxLogDebug("Container overlay: %p, IsShown: %d", containerOverlay, containerOverlay ? containerOverlay->IsShown() : 0);

                if (containerOverlay && containerOverlay->IsShown()) {
                    DockWidgetArea dropArea = containerOverlay->dropAreaUnderCursor();
                    wxLogDebug("Container drop area under cursor: %d", dropArea);

                    if (dropArea != InvalidDockWidgetArea) {
                        // Remove widget from current area
                        if (draggedWidget->dockAreaWidget() == m_dockArea) {
                            wxLogDebug("Removing widget from current area");
                            m_dockArea->removeDockWidget(draggedWidget);
                        }

                        // Verify widget is still valid
                        if (!draggedWidget->GetParent()) {
                            wxLogDebug("ERROR: Widget has no parent after removal!");
                            return;
                        }

                        // Add to container at specified position
                        wxLogDebug("Adding widget to container at position %d", dropArea);
                        wxLogDebug("Widget ptr: %p, title: %s", draggedWidget, draggedWidget->title().c_str());
                        manager->addDockWidget(dropArea, draggedWidget);
                        docked = true;

                        // Hide overlays before returning
                        if (manager) {
                            DockOverlay* areaOverlay = manager->dockAreaOverlay();
                            if (areaOverlay) {
                                areaOverlay->hideOverlay();
                            }
                            DockOverlay* containerOverlay = manager->containerOverlay();
                            if (containerOverlay) {
                                containerOverlay->hideOverlay();
                            }
                        }

                        // Return early since the area might be destroyed
                        return;
                    }
                }
            }

            // If not docked, create floating container
            if (!docked) {
                wxLogDebug("Not docked - creating floating container");

                // Remove from current area if still there
                if (draggedWidget->dockAreaWidget() == m_dockArea) {
                    m_dockArea->removeDockWidget(draggedWidget);
                }

                // Set as floating
                draggedWidget->setFloating();

                FloatingDockContainer* floatingContainer = draggedWidget->floatingDockContainer();
                if (floatingContainer) {
                    floatingContainer->SetPosition(screenPos - wxPoint(50, 10));
                    floatingContainer->Show();
                    floatingContainer->Raise();
                }

                // Hide overlays using saved manager reference
                if (manager) {
                    DockOverlay* areaOverlay = manager->dockAreaOverlay();
                    if (areaOverlay) {
                        areaOverlay->hideOverlay();
                    }
                    DockOverlay* containerOverlay = manager->containerOverlay();
                    if (containerOverlay) {
                        containerOverlay->hideOverlay();
                    }
                }

                // Return early since the area might be destroyed
                return;
            }
        }

        // Hide any overlays that might be showing
        if (m_dockArea && m_dockArea->dockManager()) {
            DockOverlay* areaOverlay = m_dockArea->dockManager()->dockAreaOverlay();
            if (areaOverlay) {
                areaOverlay->hideOverlay();
            }
            DockOverlay* containerOverlay = m_dockArea->dockManager()->containerOverlay();
            if (containerOverlay) {
                containerOverlay->hideOverlay();
            }
        }
    }

    // Clear tooltip
    UnsetToolTip();

    // Reset cursor - temporarily disabled
    // wxSetCursor(wxCursor(wxCURSOR_ARROW));

    m_draggedTab = -1;
    m_dragStarted = false;
}

void DockAreaMergedTitleBar::onMouseMotion(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();
    int oldHoveredTab = m_hoveredTab;
    m_hoveredTab = getTabAt(pos);

    // Update tab hover states
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        m_tabs[i].hovered = (i == m_hoveredTab);
    }

    // Update close button hover states - only if close button should be shown
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        bool wasHovered = m_tabs[i].closeButtonHovered;
        m_tabs[i].closeButtonHovered = (i == m_hoveredTab &&
            m_tabs[i].showCloseButton &&
            m_tabs[i].closeButtonRect.Contains(pos) &&
            m_dockArea->dockWidget(i)->hasFeature(DockWidgetClosable));

        if (wasHovered != m_tabs[i].closeButtonHovered) {
            RefreshRect(m_tabs[i].closeButtonRect);
        }
    }

    // Update button hover states
    bool oldPinHovered = m_pinButtonHovered;
    bool oldCloseHovered = m_closeButtonHovered;
    bool oldAutoHideHovered = m_autoHideButtonHovered;

    m_pinButtonHovered = m_showPinButton && m_pinButtonRect.Contains(pos);
    m_closeButtonHovered = m_showCloseButton && m_closeButtonRect.Contains(pos);
    m_autoHideButtonHovered = m_showAutoHideButton && m_autoHideButtonRect.Contains(pos);

    if (oldHoveredTab != m_hoveredTab ||
        oldPinHovered != m_pinButtonHovered ||
        oldCloseHovered != m_closeButtonHovered ||
        oldAutoHideHovered != m_autoHideButtonHovered) {
        Refresh();
    }

    // Handle dragging
    if (m_draggedTab >= 0 && event.Dragging()) {
        wxPoint delta = event.GetPosition() - m_dragStartPos;

        // Check if we should start drag operation (require minimum drag distance)
        if (!m_dragStarted && (abs(delta.x) > 5 || abs(delta.y) > 5)) {
            m_dragStarted = true;

            // Get the dock widget being dragged
            DockWidget* draggedWidget = m_dockArea->dockWidget(m_draggedTab);
            if (draggedWidget && draggedWidget->hasFeature(DockWidgetMovable) && m_dockArea && m_dockArea->dockManager()) {
                DockManager* manager = m_dockArea->dockManager();

                // Create a floating drag preview
                FloatingDragPreview* preview = new FloatingDragPreview(draggedWidget, manager->containerWidget());
                wxPoint screenPos = ClientToScreen(event.GetPosition());
                preview->startDrag(screenPos);

                // Store preview reference
                m_dragPreview = preview;

                // Mark widget as being dragged (don't actually float yet)
                // We'll handle the actual move on drop
            }
        }

        if (m_dragStarted) {
            wxPoint screenPos = ClientToScreen(event.GetPosition());

            // Update drag preview position
            if (m_dragPreview) {
                m_dragPreview->moveFloating(screenPos);
            }

            // Check for drop targets under mouse
            if (m_dockArea && m_dockArea->dockManager()) {
                DockManager* manager = m_dockArea->dockManager();
                wxWindow* windowUnderMouse = wxFindWindowAtPoint(screenPos);

                // Find target window, skipping the drag preview
                wxWindow* targetWindow = findTargetWindowUnderMouse(screenPos, m_dragPreview);

                // Show overlay on potential drop targets
                DockArea* targetArea = nullptr;
                wxWindow* checkWindow = targetWindow;

                // First check if we're over a tab bar
                DockAreaMergedTitleBar* targetTabBar = dynamic_cast<DockAreaMergedTitleBar*>(targetWindow);
                if (!targetTabBar && targetWindow) {
                    // Check parent in case we're over a child of tab bar
                    targetTabBar = dynamic_cast<DockAreaMergedTitleBar*>(targetWindow->GetParent());
                }

                // Find DockArea in parent hierarchy
                while (checkWindow && !targetArea) {
                    targetArea = dynamic_cast<DockArea*>(checkWindow);
                    if (!targetArea) {
                        checkWindow = checkWindow->GetParent();
                    }
                }

                // If we found a target area, check if we're specifically over its title bar
                // This helps determine if we should merge as a tab or dock to the side
                bool isOverTitleBar = false;
                if (targetArea) {
                    wxPoint targetLocalPos = targetArea->ScreenToClient(screenPos);
                    if (targetArea->mergedTitleBar()) {
                        wxRect titleRect = targetArea->mergedTitleBar()->GetRect();
                        isOverTitleBar = titleRect.Contains(targetLocalPos);
                    } else if (targetArea->tabBar()) {
                        wxRect tabRect = targetArea->tabBar()->GetRect();
                        isOverTitleBar = tabRect.Contains(targetLocalPos);
                    }
                }

                if (targetArea && manager) {
                    wxLogDebug("Found target DockArea, showing overlay");
                    DockOverlay* overlay = manager->dockAreaOverlay();
                    if (overlay) {
                        // If over title bar/tab bar, prioritize tab merge
                        if (isOverTitleBar) {
                            wxLogDebug("Over title bar - prioritizing tab merge");
                            overlay->showOverlay(targetArea);
                            overlay->setAllowedAreas(CenterDockWidgetArea);  // Only allow center drop for tab merge

                            // Show tooltip to indicate tab merge is possible
                            if (m_dockArea && m_dockArea->mergedTitleBar()) {
                                m_dockArea->mergedTitleBar()->showDragFeedback(true);
                            }

                            // Temporarily disable cursor updates
                            // m_dockArea->mergedTitleBar()->updateDragCursor(CenterDockWidgetArea);
                        } else {
                            wxLogDebug("Not over title bar - showing all areas");
                            overlay->showOverlay(targetArea);
                            overlay->setAllowedAreas(AllDockAreas);  // Allow all areas

                            // Temporarily disable cursor updates for general docking
                            // wxSetCursor(wxCursor(wxCURSOR_SIZING));

                            // Hide merge feedback
                            if (m_dockArea && m_dockArea->mergedTitleBar()) {
                                m_dockArea->mergedTitleBar()->showDragFeedback(false);
                            }
                        }
                    } else {
                        wxLogDebug("No area overlay available");
                    }
                } else if (manager) {
                    // Check for container overlay
                    DockContainerWidget* container = manager->containerWidget() ?
                        dynamic_cast<DockContainerWidget*>(manager->containerWidget()) : nullptr;

                    if (container && container->GetScreenRect().Contains(screenPos)) {
                        wxLogDebug("Over container, showing container overlay");
                        DockOverlay* overlay = manager->containerOverlay();
                        if (overlay) {
                            overlay->showOverlay(container);
                        }
                    } else {
                        // Hide overlays if not over any target
                        if (manager->dockAreaOverlay()) {
                            manager->dockAreaOverlay()->hideOverlay();
                        }
                        if (manager->containerOverlay()) {
                            manager->containerOverlay()->hideOverlay();
                        }
                    }
                }
            }
        }
    }
}

void DockAreaMergedTitleBar::onMouseLeave(wxMouseEvent& event) {
    m_hoveredTab = -1;
    for (auto& tab : m_tabs) {
        tab.hovered = false;
        tab.closeButtonHovered = false;
    }

    // Reset button hover states
    m_pinButtonHovered = false;
    m_closeButtonHovered = false;
    m_autoHideButtonHovered = false;

    Refresh();
}

void DockAreaMergedTitleBar::onSize(wxSizeEvent& event) {
    updateTabRects();
    
    // Force complete refresh to prevent ghosting during window resize
    Refresh();
    Update();
    
    event.Skip();
}

void DockAreaMergedTitleBar::updateTabRects() {
    wxSize size = GetClientSize();
    int x = 5; // Left margin
    int tabHeight = size.GetHeight() - 4; // Leave some margin
    int tabSpacing = 2;

    // Calculate available width for tabs (leave space for buttons)
    int buttonsWidth = 0;
    if (m_showPinButton) buttonsWidth += m_buttonSize + m_buttonSpacing;
    if (m_showCloseButton) buttonsWidth += m_buttonSize + m_buttonSpacing;
    if (m_showAutoHideButton) buttonsWidth += m_buttonSize + m_buttonSpacing;
    buttonsWidth += m_buttonSpacing; // Extra margin

    int availableWidth = size.GetWidth() - buttonsWidth - x;

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        auto& tab = m_tabs[i];

        // Simple fixed width for now (can be improved with text measurement)
        int tabWidth = 100;

        // Check if we have enough space
        if (x + tabWidth > availableWidth) {
            tabWidth = std::max(50, availableWidth - x - 10);
        }

        if (tabWidth > 0) {
            tab.rect = wxRect(x, 2, tabWidth, tabHeight);

            // Close button rect within tab - only if should be shown
            if (tab.showCloseButton) {
                int closeSize = 12;
                tab.closeButtonRect = wxRect(
                    tab.rect.GetRight() - closeSize - 3,
                    tab.rect.GetTop() + (tab.rect.GetHeight() - closeSize) / 2,
                    closeSize,
                    closeSize
                );
            } else {
                tab.closeButtonRect = wxRect(); // Empty rect means no close button
            }

            x += tabWidth + tabSpacing;
        } else {
            tab.rect = wxRect();
            tab.closeButtonRect = wxRect();
        }
    }
}

void DockAreaMergedTitleBar::drawTab(wxDC& dc, int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        return;
    }

    const TabInfo& tab = m_tabs[index];
    bool isCurrent = (index == m_currentIndex);
    bool isHovered = tab.hovered;

    // Clear the tab area first to prevent ghosting
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(tab.rect);

    // Draw tab background
    wxColour bgColor;
    if (isCurrent) {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    } else if (isHovered) {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT);
    } else {
        bgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    }

    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(isCurrent ? wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW))
                       : *wxTRANSPARENT_PEN);
    dc.DrawRectangle(tab.rect);

    // Draw tab text
    dc.SetTextForeground(isCurrent ? wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
                                  : wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));

    wxString title = tab.widget->title();
    wxRect textRect = tab.rect;
    textRect.Deflate(5, 0);

    // Adjust text width based on whether close button is shown
    if (tab.showCloseButton && tab.widget->hasFeature(DockWidgetClosable)) {
        textRect.width -= 20; // Space for close button
    }

    dc.DrawLabel(title, textRect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    // Draw close button if should be shown
    if (tab.showCloseButton && tab.widget->hasFeature(DockWidgetClosable)) {
        // Draw close button background
        wxColour closeBgColor = wxColour(255, 255, 255, 0); // Default transparent
        if (tab.closeButtonHovered) {
            closeBgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT);
        }

        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT)));
        dc.SetBrush(wxBrush(closeBgColor));
        dc.DrawRectangle(tab.closeButtonRect);

        // Draw X
        int margin = 2;
        dc.DrawLine(tab.closeButtonRect.GetLeft() + margin,
                  tab.closeButtonRect.GetTop() + margin,
                  tab.closeButtonRect.GetRight() - margin,
                  tab.closeButtonRect.GetBottom() - margin);
        dc.DrawLine(tab.closeButtonRect.GetRight() - margin,
                  tab.closeButtonRect.GetTop() + margin,
                  tab.closeButtonRect.GetLeft() + margin,
                  tab.closeButtonRect.GetBottom() - margin);
    }
}

void DockAreaMergedTitleBar::drawButton(wxDC& dc, const wxRect& rect, const wxString& text, bool hovered) {
    // Clear the button area first to prevent ghosting
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rect);

    // Draw button background
    wxColour bgColor = hovered ? wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT)
                              : wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
    dc.DrawRectangle(rect);

    // Draw button text
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    dc.DrawLabel(text, rect, wxALIGN_CENTER);
}

int DockAreaMergedTitleBar::getTabAt(const wxPoint& pos) const {
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (m_tabs[i].rect.Contains(pos)) {
            return i;
        }
    }
    return -1;
}

void DockAreaMergedTitleBar::updateDragCursor(int dropArea) {
    // Temporarily disabled cursor setting to focus on core functionality
    // TODO: Implement proper cursor handling later
    wxLogDebug("Drag cursor update requested for area: %d", dropArea);
}

wxWindow* DockAreaMergedTitleBar::findTargetWindowUnderMouse(const wxPoint& screenPos, wxWindow* dragPreview) const {
    if (!m_dockArea || !m_dockArea->dockManager()) {
        return nullptr;
    }

    DockManager* manager = m_dockArea->dockManager();
    const std::vector<DockArea*>& dockAreas = manager->dockAreas();

    // First try to find a dock area that contains the screen position
    for (DockArea* dockArea : dockAreas) {
        if (dockArea && dockArea != m_dockArea) { // Skip the source dock area
            wxRect areaRect = dockArea->GetScreenRect();
            if (areaRect.Contains(screenPos)) {
                // Check if we're specifically over the title bar
                if (dockArea->mergedTitleBar()) {
                    wxRect titleRect = dockArea->mergedTitleBar()->GetScreenRect();
                    if (titleRect.Contains(screenPos)) {
                        return dockArea->mergedTitleBar();
                    }
                }
                // Return the dock area itself
                return dockArea;
            }
        }
    }

    // If no dock area found, try the original wxFindWindowAtPoint but skip drag preview
    wxWindow* windowUnderMouse = wxFindWindowAtPoint(screenPos);
    if (windowUnderMouse && dragPreview) {
        // Check if the found window is part of the drag preview
        wxWindow* checkWindow = windowUnderMouse;
        while (checkWindow) {
            if (checkWindow == dragPreview) {
                // Found drag preview, try to find the container widget
                if (manager->containerWidget()) {
                    wxRect containerRect = manager->containerWidget()->GetScreenRect();
                    if (containerRect.Contains(screenPos)) {
                        return manager->containerWidget();
                    }
                }
                return nullptr; // Can't find a suitable target
            }
            checkWindow = checkWindow->GetParent();
        }
    }

    return windowUnderMouse;
}

void DockAreaMergedTitleBar::showDragFeedback(bool showMergeHint) {
    // This method can be used to show additional visual feedback during dragging
    // For now, we rely on cursor changes and overlay display
    // Future enhancements could include:
    // - Highlighting potential drop targets
    // - Showing preview of tab merge
    // - Displaying drag hints/tooltips

    if (showMergeHint) {
        // Set tooltip to indicate tab merge is possible
        SetToolTip("Drop here to merge tabs");
    } else {
        UnsetToolTip();
    }
}

// DockAreaTitleBar implementation (legacy)
DockAreaTitleBar::DockAreaTitleBar(DockArea* dockArea)
    : wxPanel(dockArea)
    , m_dockArea(dockArea)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, 25));
    
    // Create layout
    m_layout = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(m_layout);
    
    // Create title label
    m_titleLabel = new wxStaticText(this, wxID_ANY, "");
    m_layout->Add(m_titleLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    
    // Create buttons
    createButtons();
    
    updateTitle();
}

DockAreaTitleBar::~DockAreaTitleBar() {
}

void DockAreaTitleBar::updateTitle() {
    wxString title = m_dockArea->currentTabTitle();
    m_titleLabel->SetLabel(title);
    Layout();
}

void DockAreaTitleBar::updateButtonStates() {
    // Update button visibility based on features
    
    // Check if we should disable close button
    if (m_dockArea && m_dockArea->dockContainer()) {
        bool canClose = m_dockArea->dockContainer()->dockAreaCount() > 1;
        m_closeButton->Enable(canClose);
        
        // Update tooltip
        if (!canClose) {
            m_closeButton->SetToolTip("Cannot close the last dock area");
        } else {
            m_closeButton->SetToolTip("Close this dock area");
        }
    }
}

void DockAreaTitleBar::showCloseButton(bool show) {
    m_closeButton->Show(show);
    Layout();
}

void DockAreaTitleBar::showAutoHideButton(bool show) {
    m_autoHideButton->Show(show);
    Layout();
}

void DockAreaTitleBar::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    
    // Draw background
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
    dc.Clear();
    
    // Draw bottom border
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
    wxSize size = GetClientSize();
    dc.DrawLine(0, size.GetHeight() - 1, size.GetWidth(), size.GetHeight() - 1);
}

void DockAreaTitleBar::onCloseButtonClicked(wxCommandEvent& event) {
    m_dockArea->closeArea();
}

void DockAreaTitleBar::onAutoHideButtonClicked(wxCommandEvent& event) {
    // TODO: Implement auto-hide
}

void DockAreaTitleBar::onMenuButtonClicked(wxCommandEvent& event) {
    // TODO: Show dock area menu
}

void DockAreaTitleBar::onPinButtonClicked(wxCommandEvent& event) {
    // TODO: Auto-hide feature not yet fully implemented
    // For now, just show a message
    wxMessageBox("Auto-hide feature is not yet implemented", "Info", wxOK | wxICON_INFORMATION);
}

void DockAreaTitleBar::createButtons() {
    // Create pin button (for auto-hide)
    wxButton* pinButton = new wxButton(this, wxID_ANY, "P", wxDefaultPosition, wxSize(20, 20));
    pinButton->SetToolTip("Pin/Auto-hide");
    pinButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onPinButtonClicked, this);
    m_layout->Add(pinButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    
    // Create close button
    m_closeButton = new wxButton(this, wxID_ANY, "X", wxDefaultPosition, wxSize(20, 20));
    m_closeButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onCloseButtonClicked, this);
    m_layout->Add(m_closeButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    
    // Create auto-hide button
    m_autoHideButton = new wxButton(this, wxID_ANY, "^", wxDefaultPosition, wxSize(20, 20));
    m_autoHideButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onAutoHideButtonClicked, this);
    m_layout->Add(m_autoHideButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    m_autoHideButton->Hide(); // Hidden by default
    
    // Create menu button
    m_menuButton = new wxButton(this, wxID_ANY, "v", wxDefaultPosition, wxSize(20, 20));
    m_menuButton->Bind(wxEVT_BUTTON, &DockAreaTitleBar::onMenuButtonClicked, this);
    m_layout->Add(m_menuButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
}

void DockAreaTabBar::onMouseRightDown(wxMouseEvent& event) {
    int tab = getTabAt(event.GetPosition());
    if (tab >= 0) {
        // Select the tab
        if (tab != m_currentIndex) {
            wxCommandEvent evt(EVT_TAB_CURRENT_CHANGED);
            evt.SetEventObject(this);
            evt.SetInt(tab);
            ProcessWindowEvent(evt);
            
            m_dockArea->onCurrentTabChanged(tab);
        }
        
        // Show context menu
        showTabContextMenu(tab, event.GetPosition());
    }
}

void DockAreaTabBar::showTabContextMenu(int tab, const wxPoint& pos) {
    if (tab < 0 || tab >= static_cast<int>(m_tabs.size())) {
        return;
    }
    
    DockWidget* widget = m_tabs[tab].widget;
    if (!widget) {
        return;
    }
    
    wxMenu menu;
    
    // Add docking options
    wxMenu* dockMenu = new wxMenu();
    dockMenu->Append(wxID_ANY, "Dock Left");
    dockMenu->Append(wxID_ANY, "Dock Right");
    dockMenu->Append(wxID_ANY, "Dock Top");
    dockMenu->Append(wxID_ANY, "Dock Bottom");
    menu.AppendSubMenu(dockMenu, "Dock To");
    
    menu.AppendSeparator();
    
    // Add float option
    if (widget->hasFeature(DockWidgetFloatable)) {
        menu.Append(wxID_ANY, "Float");
    }
    
    // Add close option
    if (widget->hasFeature(DockWidgetClosable)) {
        menu.AppendSeparator();
        menu.Append(wxID_ANY, "Close");
    }
    
    // Show menu
    wxPoint screenPos = ClientToScreen(pos);
    int selection = GetPopupMenuSelectionFromUser(menu, pos);
    
    // Handle selection
    if (selection != wxID_NONE) {
        wxString itemText = menu.GetLabelText(selection);
        
        // For now, just show what would happen
        wxString msg = wxString::Format(
            "Action '%s' selected for tab '%s'.\n\n"
            "Full docking functionality is not yet implemented.",
            itemText, widget->title()
        );
        wxMessageBox(msg, "Docking Action", wxOK | wxICON_INFORMATION);
    }
}

wxRect DockAreaTabBar::getTabCloseRect(int index) const {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        return wxRect();
    }
    
    const TabInfo& tab = m_tabs[index];
    return tab.closeButtonRect;
}

bool DockAreaTabBar::isOverCloseButton(int tabIndex, const wxPoint& pos) const {
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_tabs.size())) {
        return false;
    }
    
    const TabInfo& tab = m_tabs[tabIndex];
    if (!tab.widget->hasFeature(DockWidgetClosable)) {
        return false;
    }
    
    return tab.closeButtonRect.Contains(pos);
}

} // namespace ads
