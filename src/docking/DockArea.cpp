// DockArea.cpp - Main implementation of DockArea class

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

        // Remove widget completely (delete the widget and its panel)
        if (m_containerWidget) {
            m_containerWidget->removeDockWidget(widget);
        }
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

} // namespace ads
