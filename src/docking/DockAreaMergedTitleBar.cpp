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
#include <wx/graphics.h>
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
    , m_showLockButton(true)
    , m_draggedTab(-1)
    , m_dragStarted(false)
    , m_dragPreview(nullptr)
    , m_pinButtonHovered(false)
    , m_closeButtonHovered(false)
    , m_autoHideButtonHovered(false)
    , m_lockButtonHovered(false)
    , m_hasOverflow(false)
    , m_firstVisibleTab(0)
    , m_tabPosition(TabPosition::Top)
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
        // Check if this dock area is in a floating container
        bool isFloating = false;
        if (FloatingDockContainer* floatingWidget = m_dockArea->dockContainer()->floatingWidget()) {
            isFloating = true;
        }
        
        // Show close button if:
        // 1. Multiple dock areas (normal docked behavior)
        // 2. Single dock area in floating window (close floating window)
        bool canCloseArea = m_dockArea->dockContainer()->dockAreaCount() > 1 || isFloating;
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

    // Draw decorative pattern on title bar
    wxLogDebug("DockAreaMergedTitleBar::onPaint - Drawing pattern on rect: %dx%d at (%d,%d)", 
               clientRect.width, clientRect.height, clientRect.x, clientRect.y);
    drawTitleBarPattern(dc, clientRect);

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
        // Use SVG icon for dropdown button
        DrawSvgButton(dc, m_overflowButtonRect, "down", style, false);  // Use "down" SVG icon
    }

    // Draw buttons based on tab position
    drawButtons(dc, clientRect);
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

        // Check if widget is position locked
        DockWidget* draggedWidget = m_dockArea->dockWidget(tabIndex);
        if (draggedWidget && draggedWidget->isPositionLocked()) {
            // Position locked widget cannot be dragged
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
    } else if (m_showLockButton && m_lockButtonRect.Contains(pos)) {
        onLockButtonClicked();
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
                        if (dropArea == CenterDockWidgetArea) {
                            // Add as tab - merge with existing tabs
                            wxLogDebug("Adding widget as tab to target area (merging tabs)");
                            
                            // Sync tab position with target area
                            TabPosition targetTabPosition = targetArea->tabPosition();
                            wxLogDebug("Target area tab position: %d", static_cast<int>(targetTabPosition));
                            
                            // Get source area before removing widget
                            DockArea* sourceArea = draggedWidget->dockAreaWidget();

                            // Remove widget from current area if needed
                            if (sourceArea && sourceArea != targetArea) {
                                // Store the original tab position before removal
                                TabPosition sourceTabPosition = sourceArea->tabPosition();

                                sourceArea->removeDockWidget(draggedWidget);

                                // After removal, the sourceArea might be destroyed, so we can't access it anymore
                                // The container will handle cleanup of empty areas
                                wxLogDebug("Widget removed from source area, original tab position was %d, target is %d",
                                          static_cast<int>(sourceTabPosition),
                                          static_cast<int>(targetTabPosition));
                            }
                            
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
    bool oldLockHovered = m_lockButtonHovered;

    m_pinButtonHovered = m_showPinButton && m_pinButtonRect.Contains(pos);
    m_closeButtonHovered = m_showCloseButton && m_closeButtonRect.Contains(pos);
    m_autoHideButtonHovered = m_showAutoHideButton && m_autoHideButtonRect.Contains(pos);
    m_lockButtonHovered = m_showLockButton && m_lockButtonRect.Contains(pos);

    if (oldHoveredTab != m_hoveredTab ||
        oldPinHovered != m_pinButtonHovered ||
        oldCloseHovered != m_closeButtonHovered ||
        oldAutoHideHovered != m_autoHideButtonHovered ||
        oldLockHovered != m_lockButtonHovered) {
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
        if (oldLockHovered != m_lockButtonHovered) {
            RefreshRect(m_lockButtonRect);
        }
    }

    // Handle dragging
    if (m_draggedTab >= 0 && event.Dragging()) {
        wxPoint delta = event.GetPosition() - m_dragStartPos;

        // Check if we should start drag operation (require minimum drag distance)
        // AND ensure mouse is still within the tab area to prevent accidental drag triggers
        if (!m_dragStarted && (abs(delta.x) > 15 || abs(delta.y) > 15)) {
            // Additional check: mouse must still be within the dragged tab's rectangle
            // or within a reasonable distance from the drag start position
            wxPoint currentPos = event.GetPosition();
            int draggedTabIndex = m_draggedTab;

            // Check if mouse is still within the tab area or close to drag start
            bool isWithinTabArea = false;
            if (draggedTabIndex >= 0 && draggedTabIndex < static_cast<int>(m_tabs.size())) {
                wxRect tabRect = m_tabs[draggedTabIndex].rect;
                // Inflate the tab rectangle slightly to allow for some tolerance
                tabRect.Inflate(10, 10);
                isWithinTabArea = tabRect.Contains(currentPos);
            }

            // If mouse moved outside the tab area, don't start drag
            if (!isWithinTabArea) {
                wxLogDebug("DockAreaMergedTitleBar::onMouseMotion - Mouse moved outside tab area, canceling drag");
                m_draggedTab = -1; // Reset drag state
                return;
            }

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
    
    // Clear all tab rects first
    for (auto& tab : m_tabs) {
        tab.rect = wxRect();
        tab.closeButtonRect = wxRect();
    }

    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();
    int tabSpacing = DOCK_INT("TabSpacing");
    if (tabSpacing <= 0) tabSpacing = 4; // Default to 4

    // Get text padding from theme
    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;  // Default to 8

    // Layout tabs based on position
    switch (m_tabPosition) {
        case TabPosition::Top:
            updateHorizontalTabRects(size, style, tabSpacing, textPadding, true);
            break;
        case TabPosition::Bottom:
            updateHorizontalTabRects(size, style, tabSpacing, textPadding, false);
            break;
        case TabPosition::Left:
            updateVerticalTabRects(size, style, tabSpacing, textPadding, true);
            break;
        case TabPosition::Right:
            updateVerticalTabRects(size, style, tabSpacing, textPadding, false);
            break;
    }
}

void DockAreaMergedTitleBar::updateHorizontalTabRects(const wxSize& size, const DockStyleConfig& style, 
                                                      int tabSpacing, int textPadding, bool isTop) {
    int x = 5; // Left margin
    int tabHeight = style.tabHeight;
    int tabY = isTop ? style.tabTopMargin : (size.GetHeight() - style.tabTopMargin - tabHeight);

    // Calculate available width for tabs (leave space for buttons)
    int buttonsWidth = 0;
    if (m_showPinButton) buttonsWidth += style.buttonSize;
    if (m_showCloseButton) buttonsWidth += style.buttonSize;
    if (m_showAutoHideButton) buttonsWidth += style.buttonSize;

    const int overflowButtonWidth = 20;
    int availableWidth = size.GetWidth() - buttonsWidth - x;

    // Calculate total width needed for all tabs
    int totalTabsWidth = 0;
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        auto& tab = m_tabs[i];
        wxString title = tab.widget->title();
        wxSize textSize = GetTextExtent(title);
        int tabWidth = textSize.GetWidth() + textPadding * 2;

        if (static_cast<int>(i) == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
            tabWidth += style.buttonSize + style.contentMargin;
        }
        tabWidth = std::max(tabWidth, 60);
        totalTabsWidth += tabWidth;
    }

    // Check if we need overflow
    if (totalTabsWidth > availableWidth - overflowButtonWidth - 4) {
        m_hasOverflow = true;
        availableWidth -= (overflowButtonWidth + 4);

        // Ensure current tab is visible
        if (m_currentIndex >= 0) {
            int visibleTabsWidth = 0;
            int visibleTabsCount = 0;

            for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
                auto& tab = m_tabs[i];
                wxString title = tab.widget->title();
                wxSize textSize = GetTextExtent(title);
                int tabWidth = textSize.GetWidth() + textPadding * 2;

                if (i == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
                    tabWidth += style.buttonSize + style.contentMargin;
                }
                tabWidth = std::max(tabWidth, 60);

                if (visibleTabsWidth + tabWidth > availableWidth) {
                    break;
                }

                visibleTabsWidth += tabWidth;
                visibleTabsCount++;
            }

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
    int lastTabEndX = 5;
    for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
        auto& tab = m_tabs[i];
        wxString title = tab.widget->title();
        wxSize textSize = GetTextExtent(title);
        int tabWidth = textSize.GetWidth() + textPadding * 2;

        if (i == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
            tabWidth += style.buttonSize + style.contentMargin;
        }
        tabWidth = std::max(tabWidth, 60);

        if (x + tabWidth > availableWidth) {
            break;
        }

        tab.rect = wxRect(x, tabY, tabWidth, tabHeight);

        if (tab.showCloseButton) {
            int closeSize = style.buttonSize;
            tab.closeButtonRect = wxRect(
                tab.rect.GetRight() - closeSize - 3,
                tab.rect.GetTop() + (tabHeight - closeSize) / 2,
                closeSize,
                closeSize
            );
        }

        lastTabEndX = tab.rect.GetRight();
        x += tabWidth + tabSpacing;
    }

    // Position overflow button
    if (m_hasOverflow) {
        int overflowX = lastTabEndX + 4;
        int maxOverflowX = buttonsWidth > 0 ? (size.GetWidth() - buttonsWidth - 4) : (size.GetWidth() - 4);
        
        if (overflowX + overflowButtonWidth > maxOverflowX) {
            overflowX = maxOverflowX - overflowButtonWidth;
        }
        
        m_overflowButtonRect = wxRect(overflowX, tabY, overflowButtonWidth, tabHeight);
    }
}

void DockAreaMergedTitleBar::updateVerticalTabRects(const wxSize& size, const DockStyleConfig& style, 
                                                    int tabSpacing, int textPadding, bool isLeft) {
    int y = 5; // Top margin
    int tabWidth = 30; // Fixed width for vertical tabs
    int tabX = isLeft ? style.tabTopMargin : (size.GetWidth() - style.tabTopMargin - tabWidth);

    // Calculate available height for tabs (leave space for buttons)
    int buttonsHeight = 0;
    if (m_showPinButton) buttonsHeight += style.buttonSize;
    if (m_showCloseButton) buttonsHeight += style.buttonSize;
    if (m_showAutoHideButton) buttonsHeight += style.buttonSize;

    const int overflowButtonHeight = 20;
    int availableHeight = size.GetHeight() - buttonsHeight - y;

    // Calculate total height needed for all tabs
    int totalTabsHeight = 0;
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        auto& tab = m_tabs[i];
        wxString title = tab.widget->title();
        wxSize textSize = GetTextExtent(title);
        int tabHeight = textSize.GetHeight() + textPadding * 2;

        if (static_cast<int>(i) == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
            tabHeight += style.buttonSize + style.contentMargin;
        }
        tabHeight = std::max(tabHeight, 30);
        totalTabsHeight += tabHeight;
    }

    // Check if we need overflow
    if (totalTabsHeight > availableHeight - overflowButtonHeight - 4) {
        m_hasOverflow = true;
        availableHeight -= (overflowButtonHeight + 4);

        // Ensure current tab is visible
        if (m_currentIndex >= 0) {
            int visibleTabsHeight = 0;
            int visibleTabsCount = 0;

            for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
                auto& tab = m_tabs[i];
                wxString title = tab.widget->title();
                wxSize textSize = GetTextExtent(title);
                int tabHeight = textSize.GetHeight() + textPadding * 2;

                if (i == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
                    tabHeight += style.buttonSize + style.contentMargin;
                }
                tabHeight = std::max(tabHeight, 30);

                if (visibleTabsHeight + tabHeight > availableHeight) {
                    break;
                }

                visibleTabsHeight += tabHeight;
                visibleTabsCount++;
            }

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
    int lastTabEndY = 5;
    for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
        auto& tab = m_tabs[i];
        wxString title = tab.widget->title();
        wxSize textSize = GetTextExtent(title);
        int tabHeight = textSize.GetHeight() + textPadding * 2;

        if (i == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
            tabHeight += style.buttonSize + style.contentMargin;
        }
        tabHeight = std::max(tabHeight, 30);

        if (y + tabHeight > availableHeight) {
            break;
        }

        tab.rect = wxRect(tabX, y, tabWidth, tabHeight);

        if (tab.showCloseButton) {
            int closeSize = style.buttonSize;
            tab.closeButtonRect = wxRect(
                tab.rect.GetLeft() + (tabWidth - closeSize) / 2,
                tab.rect.GetBottom() - closeSize - 3,
                closeSize,
                closeSize
            );
        }

        lastTabEndY = tab.rect.GetBottom();
        y += tabHeight + tabSpacing;
    }

    // Position overflow button
    if (m_hasOverflow) {
        int overflowY = lastTabEndY + 4;
        int maxOverflowY = buttonsHeight > 0 ? (size.GetHeight() - buttonsHeight - 4) : (size.GetHeight() - 4);
        
        if (overflowY + overflowButtonHeight > maxOverflowY) {
            overflowY = maxOverflowY - overflowButtonHeight;
        }
        
        m_overflowButtonRect = wxRect(tabX, overflowY, tabWidth, overflowButtonHeight);
    }
}

void DockAreaMergedTitleBar::showTabOverflowMenu() {
    wxMenu menu;

    // Get style config for theme settings
    const DockStyleConfig& style = GetDockStyleConfig();
    
    // Add all tabs to menu
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        wxString title = m_tabs[i].widget->title();
        if (i == m_currentIndex) {
            title = "-> " + title; // Use arrow indicator for current tab
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
    
    // Note: Standard wxMenu appearance is controlled by the system theme.
    // For full customization, a custom popup window would be needed.
}

void DockAreaMergedTitleBar::drawTab(wxDC& dc, int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        return;
    }

    const TabInfo& tab = m_tabs[index];
    bool isCurrent = (index == m_currentIndex);

    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();

    // Use the new styled drawing system
    DrawStyledRect(dc, tab.rect, style, isCurrent, false, false);

    // Set font from ThemeManager
    dc.SetFont(style.font);

    // Draw tab text with styled colors
    SetStyledTextColor(dc, style, isCurrent);

    wxString title = tab.widget->title();
    wxRect textRect = tab.rect;

    // Get text padding from theme for drawing
    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;

    // Adjust text rect based on tab position
    switch (m_tabPosition) {
        case TabPosition::Top:
        case TabPosition::Bottom:
            // Horizontal tabs
            textRect.Deflate(textPadding, 0);
            if (isCurrent && tab.showCloseButton && tab.widget->hasFeature(DockWidgetClosable)) {
                textRect.width -= style.buttonSize;
            }
            dc.DrawLabel(title, textRect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
            break;
            
        case TabPosition::Left:
        case TabPosition::Right:
            // Vertical tabs - draw rotated text using wxGraphicsContext
            textRect.Deflate(0, textPadding);
            if (isCurrent && tab.showCloseButton && tab.widget->hasFeature(DockWidgetClosable)) {
                textRect.height -= style.buttonSize;
            }
            
            // For vertical tabs, draw text rotated using manual character positioning
            // Since wxGraphicsContext requires specific DC types, we'll use a simpler approach
            
            // Set font and text color
            dc.SetFont(style.font);
            SetStyledTextColor(dc, style, isCurrent);
            
            // Calculate text position (center of the tab)
            int textX = textRect.GetLeft() + textRect.GetWidth() / 2;
            int textY = textRect.GetTop() + textRect.GetHeight() / 2;
            
            // For vertical text, we'll draw each character individually
            // This is a simple approach that works with any DC type
            wxString title = tab.widget->title();
            int charHeight = dc.GetCharHeight();
            int totalTextHeight = charHeight * title.length();
            int startY = textY - totalTextHeight / 2;
            
            // Draw each character vertically
            for (size_t i = 0; i < title.length(); ++i) {
                wxString singleChar = title.substr(i, 1);
                int charY = startY + i * charHeight;
                dc.DrawText(singleChar, textX - dc.GetTextExtent(singleChar).GetWidth() / 2, charY);
            }
            break;
    }

    // Only draw close button for the current tab
    if (isCurrent && tab.showCloseButton && tab.widget->hasFeature(DockWidgetClosable)) {
        DrawSvgButton(dc, tab.closeButtonRect, style.closeIconName, style, false);
    }
}

void DockAreaMergedTitleBar::drawButtons(wxDC& dc, const wxRect& clientRect) {
    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();
    
    switch (m_tabPosition) {
        case TabPosition::Top:
        case TabPosition::Bottom:
            drawHorizontalButtons(dc, clientRect, style);
            break;
        case TabPosition::Left:
        case TabPosition::Right:
            drawVerticalButtons(dc, clientRect, style);
            break;
    }
}

void DockAreaMergedTitleBar::drawHorizontalButtons(wxDC& dc, const wxRect& clientRect, const DockStyleConfig& style) {
    // Draw buttons on the right side with 0 margin from top, right, and bottom
    int buttonX = clientRect.GetWidth();  // Start from right edge
    int buttonHeight = clientRect.GetHeight() - 1;  // -1 for bottom border
    int buttonY = 0;  // Start from top edge

    if (m_showAutoHideButton) {
        buttonX -= m_buttonSize;
        m_autoHideButtonRect = wxRect(buttonX, buttonY, m_buttonSize, buttonHeight);
        drawButton(dc, m_autoHideButtonRect, "^", m_autoHideButtonHovered);
        buttonX -= m_buttonSpacing;
    }

    if (m_showCloseButton) {
        buttonX -= m_buttonSize;
        m_closeButtonRect = wxRect(buttonX, buttonY, m_buttonSize, buttonHeight);
        drawButton(dc, m_closeButtonRect, "X", m_closeButtonHovered);
        buttonX -= m_buttonSpacing;
    }

    if (m_showPinButton) {
        buttonX -= m_buttonSize;
        m_pinButtonRect = wxRect(buttonX, buttonY, m_buttonSize, buttonHeight);
        drawButton(dc, m_pinButtonRect, "P", m_pinButtonHovered);
        buttonX -= m_buttonSpacing;
    }

    if (m_showLockButton) {
        buttonX -= m_buttonSize;
        m_lockButtonRect = wxRect(buttonX, buttonY, m_buttonSize, buttonHeight);
        // Use same style as pin button, but different text based on lock state
        wxString lockText = isAnyTabLocked() ? "c" : "o";
        drawButton(dc, m_lockButtonRect, lockText, m_lockButtonHovered);
    }
}

void DockAreaMergedTitleBar::drawVerticalButtons(wxDC& dc, const wxRect& clientRect, const DockStyleConfig& style) {
    // Draw buttons at the bottom for vertical tabs
    int buttonY = clientRect.GetHeight();  // Start from bottom edge
    int buttonWidth = clientRect.GetWidth() - 1;  // -1 for right border
    int buttonX = 0;  // Start from left edge

    if (m_showAutoHideButton) {
        buttonY -= m_buttonSize;
        m_autoHideButtonRect = wxRect(buttonX, buttonY, buttonWidth, m_buttonSize);
        drawButton(dc, m_autoHideButtonRect, "^", m_autoHideButtonHovered);
        buttonY -= m_buttonSpacing;
    }

    if (m_showCloseButton) {
        buttonY -= m_buttonSize;
        m_closeButtonRect = wxRect(buttonX, buttonY, buttonWidth, m_buttonSize);
        drawButton(dc, m_closeButtonRect, "X", m_closeButtonHovered);
        buttonY -= m_buttonSpacing;
    }

    if (m_showPinButton) {
        buttonY -= m_buttonSize;
        m_pinButtonRect = wxRect(buttonX, buttonY, buttonWidth, m_buttonSize);
        drawButton(dc, m_pinButtonRect, "P", m_pinButtonHovered);
        buttonY -= m_buttonSpacing;
    }

    if (m_showLockButton) {
        buttonY -= m_buttonSize;
        m_lockButtonRect = wxRect(buttonX, buttonY, buttonWidth, m_buttonSize);
        // Use same style as pin button, but different text based on lock state
        wxString lockText = isAnyTabLocked() ? "🔒" : "🔓";
        drawButton(dc, m_lockButtonRect, lockText, m_lockButtonHovered);
    }
}

void DockAreaMergedTitleBar::onLockButtonClicked() {
    // Toggle lock state for all widgets in this dock area
    bool shouldLock = !isAnyTabLocked();

    for (auto& tab : m_tabs) {
        if (tab.widget) {
            tab.widget->setPositionLocked(shouldLock);
        }
    }

    Refresh(); // Refresh to update button appearance
}

bool DockAreaMergedTitleBar::isAnyTabLocked() const {
    for (const auto& tab : m_tabs) {
        if (tab.widget && tab.widget->isPositionLocked()) {
            return true;
        }
    }
    return false;
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

void DockAreaMergedTitleBar::setTabPosition(TabPosition position) {
    // Safety check: ensure this object is still valid
    if (!this) {
        wxLogDebug("DockAreaMergedTitleBar::setTabPosition called on invalid object");
        return;
    }

    if (m_tabPosition == position) {
        return;
    }
    
    m_tabPosition = position;
    
    // Update minimum size based on tab position
    switch (position) {
        case TabPosition::Top:
        case TabPosition::Bottom:
            SetMinSize(wxSize(-1, 30)); // Horizontal tabs
            break;
        case TabPosition::Left:
        case TabPosition::Right:
            SetMinSize(wxSize(30, -1)); // Vertical tabs
            break;
    }
    
    // Hide buttons for non-top positions (independent title bar mode)
    if (position != TabPosition::Top) {
        m_showCloseButton = false;
        m_showAutoHideButton = false;
        m_showPinButton = false;
    } else {
        // Restore button visibility for top position (merged mode)
        updateButtonStates();
    }
    
    // Update tab rectangles and refresh
    updateTabRects();
    Refresh();
}

void DockAreaMergedTitleBar::drawTitleBarPattern(wxDC& dc, const wxRect& rect) {
    // Draw decorative horizontal dot bar between tabs and buttons
    // Create a 3x5 pixel dot pattern decoration in the middle area
    
    wxLogDebug("DockAreaMergedTitleBar::drawTitleBarPattern - Drawing horizontal dot bar on rect: %dx%d at (%d,%d)", 
               rect.width, rect.height, rect.x, rect.y);
    
    // Save current pen and brush
    wxPen oldPen = dc.GetPen();
    wxBrush oldBrush = dc.GetBrush();
    
    // Calculate the position for the decoration bar
    // Find the rightmost tab and leftmost button to position the bar between them
    int leftX = 0;
    int rightX = rect.width;
    
    // Find rightmost tab position
    for (const auto& tab : m_tabs) {
        if (!tab.rect.IsEmpty()) {
            leftX = std::max(leftX, tab.rect.GetRight());
        }
    }
    
    // Find leftmost button position
    if (m_showAutoHideButton && !m_autoHideButtonRect.IsEmpty()) {
        rightX = std::min(rightX, m_autoHideButtonRect.GetLeft());
    }
    if (m_showPinButton && !m_pinButtonRect.IsEmpty()) {
        rightX = std::min(rightX, m_pinButtonRect.GetLeft());
    }
    if (m_showCloseButton && !m_closeButtonRect.IsEmpty()) {
        rightX = std::min(rightX, m_closeButtonRect.GetLeft());
    }
    if (m_showLockButton && !m_lockButtonRect.IsEmpty()) {
        rightX = std::min(rightX, m_lockButtonRect.GetLeft());
    }
    
    // Add some margin
    leftX += 8;   // 8px margin from tabs
    rightX -= 8;  // 8px margin from buttons
    
    // Only draw if there's enough space
    if (rightX > leftX + 20) {
        // Get style config with theme initialization
        const DockStyleConfig& style = GetDockStyleConfig();
        
        // Set dot pattern colors from theme
        wxColour dotColor = style.patternDotColour;
        
        // Set pen and brush for dots
        dc.SetPen(wxPen(dotColor, 1));
        dc.SetBrush(wxBrush(dotColor));
        
        // Pattern parameters from theme configuration
        int patternWidth = style.patternWidth;   // Pattern width from theme
        int patternHeight = style.patternHeight; // Pattern height from theme
        int dotSize = 1;        // 1 pixel dots
        
        // Calculate vertical center position
        int centerY = rect.y + (rect.height - patternHeight) / 2;
        
        // Draw horizontal tiling pattern across the available space (no spacing)
        int currentX = leftX;
        int patternCount = 0;
        
        while (currentX + patternWidth <= rightX) {
            // Draw the 3 specific dots in 3x3 pattern at current position
            // Top-left dot (position 0,0)
            dc.DrawCircle(currentX, centerY, dotSize);
            
            // Bottom-left dot (position 0,2)
            dc.DrawCircle(currentX, centerY + 2, dotSize);
            
            // Right-middle dot (position 2,1)
            dc.DrawCircle(currentX + 2, centerY + 1, dotSize);
            
            // Move to next pattern position (no spacing)
            currentX += patternWidth;
            patternCount++;
        }
        
        wxLogDebug("DockAreaMergedTitleBar::drawTitleBarPattern - Drawing tiling pattern from x=%d to x=%d, patterns=%d", 
                   leftX, rightX, patternCount);
    }
    
    // Restore original pen and brush
    dc.SetPen(oldPen);
    dc.SetBrush(oldBrush);
}

} // namespace ads
