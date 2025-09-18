// DockAreaTabBar.cpp - Implementation of DockAreaTabBar class

#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/DockManager.h"
#include "docking/DockContainerWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockOverlay.h"
#include "config/SvgIconManager.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>
#include <wx/menu.h>
#include <wx/cursor.h>
#include <algorithm>

namespace ads {

// Event table for DockAreaTabBar
wxBEGIN_EVENT_TABLE(DockAreaTabBar, wxPanel)
    EVT_PAINT(DockAreaTabBar::onPaint)
    EVT_LEFT_DOWN(DockAreaTabBar::onMouseLeftDown)
    EVT_LEFT_UP(DockAreaTabBar::onMouseLeftUp)
    EVT_RIGHT_DOWN(DockAreaTabBar::onMouseRightDown)
    EVT_MOTION(DockAreaTabBar::onMouseMotion)
    EVT_LEAVE_WINDOW(DockAreaTabBar::onMouseLeave)
    EVT_ENTER_WINDOW(DockAreaTabBar::onMouseEnter)
    EVT_SET_CURSOR(DockAreaTabBar::onSetCursor)
    EVT_SIZE(DockAreaTabBar::onSize)
wxEND_EVENT_TABLE()

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

    // Improve visual smoothness on resize
    SetDoubleBuffered(true);

    // Register theme change listener
    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        RefreshTheme();
    });
}

DockAreaTabBar::~DockAreaTabBar() {
    // Remove theme change listener
    ThemeManager::getInstance().removeThemeChangeListener(this);
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

    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();

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
        // Use SVG icon for dropdown button
        DrawSvgButton(dc, m_overflowButtonRect, "down", style, false);  // Use "down" SVG icon
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
            // Check if widget is position locked
            DockWidget* draggedWidget = m_dockArea->dockWidget(tab);
            if (draggedWidget && draggedWidget->isPositionLocked()) {
                // Position locked widget cannot be dragged
                return;
            }

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
                        if (dropArea == CenterDockWidgetArea) {
                            // Add as tab
                            wxLogDebug("Adding widget as tab to target area");
                            
                            // Sync tab position with target area
                            TabPosition targetTabPosition = targetArea->tabPosition();
                            wxLogDebug("Target area tab position: %d", static_cast<int>(targetTabPosition));
                            
                            // Get source area before removing widget
                            DockArea* sourceArea = draggedWidget->dockAreaWidget();
                            
                            // Remove widget from current area if needed
                            if (sourceArea && sourceArea != targetArea) {
                                sourceArea->removeDockWidget(draggedWidget);
                                
                                // Sync source area tab position with target area
                                if (sourceArea->tabPosition() != targetTabPosition) {
                                    wxLogDebug("Syncing source area tab position from %d to %d", 
                                              static_cast<int>(sourceArea->tabPosition()), 
                                              static_cast<int>(targetTabPosition));
                                    sourceArea->setTabPosition(targetTabPosition);
                                }
                            }
                            
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

    // Restore default cursor after drag ends
    SetCursor(wxCursor(wxCURSOR_ARROW));
}

void DockAreaTabBar::onMouseMotion(wxMouseEvent& event) {
    // Only process events that originate from this widget
    if (event.GetEventObject() != this) {
        event.Skip();
        return;
    }

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
        // Only refresh the affected tab areas instead of the whole bar
        if (oldHovered >= 0 && oldHovered < static_cast<int>(m_tabs.size())) {
            RefreshRect(m_tabs[oldHovered].rect);
        }
        if (m_hoveredTab >= 0 && m_hoveredTab < static_cast<int>(m_tabs.size())) {
            RefreshRect(m_tabs[m_hoveredTab].rect);
        }
    }

    // Update cursor for hover/drag state
    if (m_dragStarted) {
        SetCursor(wxCursor(wxCURSOR_SIZING)); // Grab-like cursor while dragging
    } else if (m_hasOverflow && m_overflowButtonRect.Contains(event.GetPosition())) {
        SetCursor(wxCursor(wxCURSOR_ARROW)); // Keep default over overflow button
    } else if (m_hoveredTab >= 0) {
        SetCursor(wxCursor(wxCURSOR_HAND)); // Hand over tab labels
    } else {
        SetCursor(wxCursor(wxCURSOR_ARROW));
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
                wxLogDebug("DockAreaTabBar::onMouseMotion - Mouse moved outside tab area, canceling drag");
                m_draggedTab = -1; // Reset drag state
                return;
            }

            m_dragStarted = true;

            // Switch cursor when drag starts
            SetCursor(wxCursor(wxCURSOR_SIZING));

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
                    // Set drag preview callback to update preview size
                    if (m_dragPreview) {
                        overlay->setDragPreviewCallback([this](DockWidgetArea area, const wxSize& size) {
                            wxLogDebug("Drag preview callback: area=%d, size=%dx%d", area, size.GetWidth(), size.GetHeight());
                            if (area != InvalidDockWidgetArea && size.GetWidth() > 0 && size.GetHeight() > 0) {
                                wxLogDebug("Setting preview size to %dx%d for area %d", size.GetWidth(), size.GetHeight(), area);
                                m_dragPreview->setPreviewSize(area, size);
                            } else {
                                wxLogDebug("Resetting to default size");
                                m_dragPreview->resetToDefaultSize();
                            }
                        });
                    }
                    
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

    // Restore default cursor when leaving tab bar
    SetCursor(wxCursor(wxCURSOR_ARROW));
}

// Ensure cursor stays updated when system queries cursor
void DockAreaTabBar::onSetCursor(wxSetCursorEvent& event) {
    if (m_dragStarted) {
        SetCursor(wxCursor(wxCURSOR_SIZING));
        event.Skip(false);
        return;
    }
    if (m_hoveredTab >= 0) {
        SetCursor(wxCursor(wxCURSOR_HAND));
        event.Skip(false);
        return;
    }
    SetCursor(wxCursor(wxCURSOR_ARROW));
    event.Skip(false);
}

void DockAreaTabBar::onMouseEnter(wxMouseEvent& event) {
    // Update cursor immediately upon entering
    if (m_hoveredTab >= 0) {
        SetCursor(wxCursor(wxCURSOR_HAND));
    } else {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
    event.Skip();
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
    const int overflowButtonWidth = 20; // Reduced width as requested
    int x = 0;

    // Clear all tab rects first
    for (auto& tab : m_tabs) {
        tab.rect = wxRect();
        tab.closeButtonRect = wxRect();
    }

    // Calculate available width
    int maxWidth = m_hasOverflow ? size.GetWidth() - overflowButtonWidth - 4 : size.GetWidth(); // -4 for min distance to button

    // Get style config with theme initialization
    const DockStyleConfig& style = GetDockStyleConfig();

    // Layout visible tabs
    int lastTabEndX = 0;
    for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
        auto& tab = m_tabs[i];

        // Calculate tab width based on text content
        wxString title = tab.widget->title();
        wxSize textSize = GetTextExtent(title);

        // Add padding for text (left and right) - from theme
        int textPadding = DOCK_INT("TabPadding");
        if (textPadding <= 0) textPadding = 8;  // Default to 8
        int tabWidth = textSize.GetWidth() + textPadding * 2;

        // Add space for close button if this is the current tab
        if (i == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
            tabWidth += style.buttonSize + style.contentMargin;
        }

        // Ensure minimum width
        tabWidth = std::max(tabWidth, 60);

        // Check if this tab would exceed available width
        if (x + tabWidth > maxWidth) {
            break; // Stop laying out tabs
        }

        // Use configured tab height and top margin
        int tabY = style.tabTopMargin;
        int tabHeight = style.tabHeight;
        tab.rect = wxRect(x, tabY, tabWidth, tabHeight);

        // Close button rect with configured size (only for current tab)
        if (i == m_currentIndex && tab.widget->hasFeature(DockWidgetClosable)) {
            int closeSize = style.buttonSize;
            int closePadding = (tabHeight - closeSize) / 2;
            tab.closeButtonRect = wxRect(
                tab.rect.GetRight() - closeSize - style.contentMargin,
                tabY + closePadding,
                closeSize,
                closeSize
            );
        }

        lastTabEndX = tab.rect.GetRight();
        x += tabWidth + style.tabSpacing;
    }

    // Position overflow button if needed
    if (m_hasOverflow) {
        // Position overflow button 4 pixels after the last visible tab
        // Use same Y position and height as tabs for alignment
        m_overflowButtonRect = wxRect(
            lastTabEndX + 4,
            style.tabTopMargin,
            overflowButtonWidth,
            style.tabHeight
        );
    }
}

void DockAreaTabBar::drawTab(wxDC& dc, int index) {
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
    if (isCurrent && tab.widget->hasFeature(DockWidgetClosable)) {
        textRect.width -= style.buttonSize;
    }

    dc.DrawLabel(title, textRect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    // Only draw close button for the current tab
    if (isCurrent && tab.widget->hasFeature(DockWidgetClosable)) {
        DrawSvgButton(dc, tab.closeButtonRect, style.closeIconName,
                     style, false);  // No hover effect
    }
}

void DockAreaTabBar::onSize(wxSizeEvent& event) {
    // Cheap work now; actual repaint is coalesced by parent/container
    checkTabOverflow();
    updateTabRects();
    Refresh(false);
    event.Skip();
}

void DockAreaTabBar::checkTabOverflow() {
    if (m_tabs.empty()) {
        m_hasOverflow = false;
        m_firstVisibleTab = 0;
        return;
    }

    // Get style config for consistent sizing
    const DockStyleConfig& style = GetDockStyleConfig();
    const int overflowButtonWidth = 20; // Reduced width as requested
    const int closeButtonWidth = style.buttonSize;

    // Get text padding from theme
    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;

    int totalTabsWidth = 0;
    for (const auto& tab : m_tabs) {
        // Calculate adaptive tab width
        wxString title = tab.widget->title();
        wxSize textSize = GetTextExtent(title);
        int tabWidth = textSize.GetWidth() + textPadding * 2;

        // Add space for close button if this is the current tab
        // Note: In overflow calculation, we don't need to check current index
        // as we want to calculate total width for all tabs
        if (tab.widget->hasFeature(DockWidgetClosable)) {
            tabWidth += style.buttonSize + style.contentMargin;
        }

        // Ensure minimum width
        tabWidth = std::max(tabWidth, 60);

        totalTabsWidth += tabWidth;
    }

    int availableWidth = GetClientSize().GetWidth();

    // Check if we need overflow
    if (totalTabsWidth > availableWidth - overflowButtonWidth) {
        m_hasOverflow = true;

        // Ensure current tab is visible first
        if (m_currentIndex >= 0) {
            // Calculate how many tabs can fit with adaptive widths
            int visibleTabsWidth = 0;
            int visibleTabsCount = 0;

            for (int i = m_firstVisibleTab; i < static_cast<int>(m_tabs.size()); ++i) {
                // Calculate adaptive tab width
                wxString title = m_tabs[i].widget->title();
                wxSize textSize = GetTextExtent(title);
                int tabWidth = textSize.GetWidth() + textPadding * 2;

                // Add space for close button if this is the current tab
                if (i == m_currentIndex && m_tabs[i].widget->hasFeature(DockWidgetClosable)) {
                    tabWidth += style.buttonSize + style.contentMargin;
                }

                // Ensure minimum width
                tabWidth = std::max(tabWidth, 60);

                if (visibleTabsWidth + tabWidth > availableWidth - overflowButtonWidth - 4) {
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

    // Get style config for theme settings
    const DockStyleConfig& style = GetDockStyleConfig();
    
    // Set menu font (note: wxMenu doesn't directly support font setting,
    // but we can set it on individual items)
    
    // Add all tabs to menu
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        wxString title = m_tabs[i].widget->title();
        if (i == m_currentIndex) {
            title = "-> " + title; // Use arrow indicator for current tab
        }

        wxMenuItem* item = menu.Append(wxID_ANY, title);
        
        // Apply font to menu item if possible
        // Note: Font customization for menu items is limited in wxWidgets
        // The actual appearance will depend on the system theme
        
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
    // For full customization, a custom popup window would be needed,
    // but that adds significant complexity.
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

void DockAreaTabBar::RefreshTheme() {
    // Apply theme colors to background
    SetBackgroundColour(CFG_COLOUR("DockTabBarBgColour"));

    // Refresh the display to apply new theme colors
    Refresh(true);
    Update();
}

} // namespace ads

