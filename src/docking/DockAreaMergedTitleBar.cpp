// DockAreaMergedTitleBar.cpp - Implementation of DockAreaMergedTitleBar class

#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/DockManager.h"
#include "docking/DockContainerWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockOverlay.h"
#include "config/SvgIconManager.h"
#include <wx/dcbuffer.h>
#include <wx/menu.h>
#include <algorithm>

namespace ads {

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
    , m_buttonSpacing(0)  // Set to 0 as requested
    , m_showCloseButton(true)
    , m_showAutoHideButton(false)
    , m_showPinButton(true)
    , m_draggedTab(-1)
    , m_dragStarted(false)
    , m_dragPreview(nullptr)
    , m_pinButtonHovered(false)
    , m_closeButtonHovered(false)
    , m_autoHideButtonHovered(false)
    , m_hasOverflow(false)
    , m_firstVisibleTab(0)
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

        // Ensure current tab is visible when overflow is active
        if (m_hasOverflow) {
            updateTabRects();
        }

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

    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();

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

    // Draw overflow button if needed
    if (m_hasOverflow) {
        // Use the new styled drawing system for overflow button
        DrawStyledRect(dc, m_overflowButtonRect, style, false, false, false);  // No hover effect

        // Draw dropdown arrow
        int centerX = m_overflowButtonRect.GetLeft() + m_overflowButtonRect.GetWidth() / 2;
        int centerY = m_overflowButtonRect.GetTop() + m_overflowButtonRect.GetHeight() / 2;
        wxPoint arrow[3] = {
            wxPoint(centerX - 4, centerY - 2),
            wxPoint(centerX + 4, centerY - 2),
            wxPoint(centerX, centerY + 2)
        };
        dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawPolygon(3, arrow);
    }

    // Draw buttons on the right side with 0 margin from top, right, and bottom
    int buttonX = clientRect.GetWidth();  // Start from right edge
    int buttonHeight = clientRect.GetHeight() - 1;  // -1 for bottom border
    int buttonY = 0;  // Start from top edge
    int buttonCount = 0;

    if (m_showAutoHideButton) {
        buttonX -= m_buttonSize;
        m_autoHideButtonRect = wxRect(buttonX, buttonY, m_buttonSize, buttonHeight);
        drawButton(dc, m_autoHideButtonRect, "^", m_autoHideButtonHovered);
        buttonX -= m_buttonSpacing;
        buttonCount++;
    }

    if (m_showCloseButton) {
        buttonX -= m_buttonSize;
        m_closeButtonRect = wxRect(buttonX, buttonY, m_buttonSize, buttonHeight);
        drawButton(dc, m_closeButtonRect, "X", m_closeButtonHovered);
        buttonX -= m_buttonSpacing;
        buttonCount++;
    }

    if (m_showPinButton) {
        buttonX -= m_buttonSize;
        m_pinButtonRect = wxRect(buttonX, buttonY, m_buttonSize, buttonHeight);
        drawButton(dc, m_pinButtonRect, "P", m_pinButtonHovered);
        buttonCount++;
    }
}

void DockAreaMergedTitleBar::onMouseLeftDown(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();

    // Check if overflow button clicked
    if (m_hasOverflow && m_overflowButtonRect.Contains(pos)) {
        showTabOverflowMenu();
        return;
    }

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
    // Only process events that originate from this widget
    if (event.GetEventObject() != this) {
        event.Skip();
        return;
    }

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
        // Use targeted refresh instead of refreshing the whole bar
        if (oldHoveredTab != m_hoveredTab) {
            if (oldHoveredTab >= 0 && oldHoveredTab < static_cast<int>(m_tabs.size())) {
                RefreshRect(m_tabs[oldHoveredTab].rect);
            }
            if (m_hoveredTab >= 0 && m_hoveredTab < static_cast<int>(m_tabs.size())) {
                RefreshRect(m_tabs[m_hoveredTab].rect);
            }
        }
        if (oldPinHovered != m_pinButtonHovered) {
            RefreshRect(m_pinButtonRect);
        }
        if (oldCloseHovered != m_closeButtonHovered) {
            RefreshRect(m_closeButtonRect);
        }
        if (oldAutoHideHovered != m_autoHideButtonHovered) {
            RefreshRect(m_autoHideButtonRect);
        }
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

    // Clear all tab rects first
    for (auto& tab : m_tabs) {
        tab.rect = wxRect();
        tab.closeButtonRect = wxRect();
    }

    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();
    int tabHeight = style.tabHeight; // Use configured tab height
    int tabSpacing = DOCK_INT("TabSpacing");
    if (tabSpacing <= 0) tabSpacing = 4; // Default to 4

    // Calculate available width for tabs (leave space for buttons)
    int buttonsWidth = 0;
    if (m_showPinButton) buttonsWidth += style.buttonSize;
    if (m_showCloseButton) buttonsWidth += style.buttonSize;
    if (m_showAutoHideButton) buttonsWidth += style.buttonSize;

    const int overflowButtonWidth = 20; // Reduced width as requested
    int availableWidth = size.GetWidth() - buttonsWidth - x;

    // Get text padding from theme
    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;  // Default to 8

    // Calculate total width needed for all tabs
    int totalTabsWidth = 0;
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        auto& tab = m_tabs[i];

        // Calculate tab width based on text content (adaptive width)
        wxString title = tab.widget->title();
        wxSize textSize = GetTextExtent(title);
        int tabWidth = textSize.GetWidth() + textPadding * 2;

        // Add space for close button if this is the current tab
        if (static_cast<int>(i) == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
            tabWidth += style.buttonSize + style.contentMargin;
        }

        // Ensure minimum width
        tabWidth = std::max(tabWidth, 60);

        totalTabsWidth += tabWidth;
    }

    // Check if we need overflow
    if (totalTabsWidth > availableWidth - overflowButtonWidth - 4) { // -4 for min distance to button
        m_hasOverflow = true;

        // Adjust available width to account for overflow button and spacing
        availableWidth -= (overflowButtonWidth + 4);

        // Ensure current tab is visible
        if (m_currentIndex >= 0) {
            // Calculate how many tabs can fit with adaptive widths
            int visibleTabsWidth = 0;
            int visibleTabsCount = 0;

            for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
                auto& tab = m_tabs[i];

                // Calculate adaptive tab width
                wxString title = tab.widget->title();
                wxSize textSize = GetTextExtent(title);
                int tabWidth = textSize.GetWidth() + textPadding * 2;

                // Add space for close button if this is the current tab
                if (i == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
                    tabWidth += style.buttonSize + style.contentMargin;
                }

                // Ensure minimum width
                tabWidth = std::max(tabWidth, 60);

                if (visibleTabsWidth + tabWidth > availableWidth) {
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

    // Layout visible tabs
    int lastTabEndX = 5; // Default to left margin if no tabs
    for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
        auto& tab = m_tabs[i];

        // Calculate tab width based on text content (adaptive width)
        wxString title = tab.widget->title();
        wxSize textSize = GetTextExtent(title);
        int tabWidth = textSize.GetWidth() + textPadding * 2;

        // Add space for close button if this is the current tab
        if (i == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
            tabWidth += style.buttonSize + style.contentMargin;
        }

        // Ensure minimum width
        tabWidth = std::max(tabWidth, 60);

        // Check if this tab would exceed available width
        if (x + tabWidth > availableWidth) {
            break; // Stop laying out tabs
        }

        // Use configured tab top margin
        tab.rect = wxRect(x, style.tabTopMargin, tabWidth, tabHeight);

        // Close button rect within tab - only if should be shown
        if (tab.showCloseButton) {
            int closeSize = style.buttonSize;
            tab.closeButtonRect = wxRect(
                tab.rect.GetRight() - closeSize - 3,
                tab.rect.GetTop() + (tabHeight - closeSize) / 2,
                closeSize,
                closeSize
            );
        } else {
            tab.closeButtonRect = wxRect(); // Empty rect means no close button
        }

        lastTabEndX = tab.rect.GetRight();
        x += tabWidth + tabSpacing;
    }

    // Position overflow button if needed
    if (m_hasOverflow) {
        // Position overflow button 4 pixels after the last visible tab
        // Ensure it's at least 4 pixels away from title bar buttons
        int overflowX = lastTabEndX + 4;
        int maxOverflowX = buttonsWidth > 0 ? (size.GetWidth() - buttonsWidth - 4) : (size.GetWidth() - 4);
        
        // Limit overflow button position to not overlap with title bar buttons
        if (overflowX + overflowButtonWidth > maxOverflowX) {
            overflowX = maxOverflowX - overflowButtonWidth;
        }
        
        m_overflowButtonRect = wxRect(
            overflowX, style.tabTopMargin,
            overflowButtonWidth, tabHeight
        );
    }
}

void DockAreaMergedTitleBar::showTabOverflowMenu() {
    wxMenu menu;

    // Add all tabs to menu
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        wxString title = m_tabs[i].widget->title();
        if (i == m_currentIndex) {
            title = "> " + title; // Add marker for current tab
        }

        wxMenuItem* item = menu.Append(wxID_ANY, title);

        // Bind menu item to tab selection - call DockArea's setCurrentIndex
        menu.Bind(wxEVT_MENU, [this, i](wxCommandEvent&) {
            if (m_dockArea) {
                m_dockArea->setCurrentIndex(i);
            }
        }, item->GetId());
    }

    // Show menu at overflow button position
    wxPoint pos = m_overflowButtonRect.GetBottomLeft();
    PopupMenu(&menu, pos);
}

void DockAreaMergedTitleBar::drawTab(wxDC& dc, int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        return;
    }

    const TabInfo& tab = m_tabs[index];
    bool isCurrent = (index == m_currentIndex);
    // Remove hover detection - no hover effects

    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();

    // Use the new styled drawing system
    DrawStyledRect(dc, tab.rect, style, isCurrent, false, false);  // No hover effect

    // Set font from ThemeManager
    dc.SetFont(style.font);

    // Draw tab text with styled colors
    SetStyledTextColor(dc, style, isCurrent);

    wxString title = tab.widget->title();
    wxRect textRect = tab.rect;

    // Get text padding from theme for drawing
    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;  // Default to 8
    textRect.Deflate(textPadding, 0);

    // Only adjust text width for close button if this is the current tab
    if (isCurrent && tab.showCloseButton && tab.widget->hasFeature(DockWidgetClosable)) {
        textRect.width -= style.buttonSize;
    }

    dc.DrawLabel(title, textRect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    // Only draw close button for the current tab
    if (isCurrent && tab.showCloseButton && tab.widget->hasFeature(DockWidgetClosable)) {
        DrawSvgButton(dc, tab.closeButtonRect, style.closeIconName,
                     style, false);  // No hover effect
    }
}

void DockAreaMergedTitleBar::drawButton(wxDC& dc, const wxRect& rect, const wxString& text, bool hovered) {
    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();

    // Use the new styled drawing system for buttons
    DrawStyledRect(dc, rect, style, false, hovered, false);

    // Draw button text with styled colors
    SetStyledTextColor(dc, style, false); // Buttons are not "active" in the same way as tabs
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

} // namespace ads
