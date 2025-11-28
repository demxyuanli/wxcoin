// DockAreaMergedTitleBar.cpp - Implementation of DockAreaMergedTitleBar class

#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/DockManager.h"
#include "docking/DockContainerWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockOverlay.h"
#include "docking/TitleBarRenderer.h"
#include "docking/TabLayoutCalculator.h"
#include "docking/TabDragHandler.h"
#include "docking/ButtonLayoutCalculator.h"
#include "docking/RenderOptimizer.h"
#include "docking/PerformanceMonitor.h"
#include "docking/RenderObjectPool.h"
#include "config/SvgIconManager.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>
#include <wx/menu.h>
#include <wx/graphics.h>
#include <wx/cursor.h>
#include <algorithm>
#include <memory>

namespace ads {

// Event table for DockAreaMergedTitleBar
wxBEGIN_EVENT_TABLE(DockAreaMergedTitleBar, wxPanel)
    EVT_PAINT(DockAreaMergedTitleBar::onPaint)
    EVT_LEFT_DOWN(DockAreaMergedTitleBar::onMouseLeftDown)
    EVT_LEFT_UP(DockAreaMergedTitleBar::onMouseLeftUp)
    EVT_MOTION(DockAreaMergedTitleBar::onMouseMotion)
    EVT_LEAVE_WINDOW(DockAreaMergedTitleBar::onMouseLeave)
    EVT_ENTER_WINDOW(DockAreaMergedTitleBar::onMouseEnter)
    EVT_SET_CURSOR(DockAreaMergedTitleBar::onSetCursor)
    EVT_SIZE(DockAreaMergedTitleBar::onSize)
    EVT_TIMER(wxID_ANY, DockAreaMergedTitleBar::onResizeRefreshTimer)
    EVT_MOUSE_CAPTURE_LOST(DockAreaMergedTitleBar::onMouseCaptureLost)
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
    , m_resizeRefreshTimer(nullptr)
    , m_pendingRefresh(false)
    , m_refreshFlags(0)
    , m_renderer(std::make_unique<TitleBarRenderer>())
    , m_layoutCalculator(std::make_unique<TabLayoutCalculator>())
    , m_dragHandler(std::make_unique<TabDragHandler>(this))
    , m_buttonLayoutCalculator(std::make_unique<ButtonLayoutCalculator>())
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, 28)); // Reduced by 2px as requested

    // Enable double buffering for smoother rendering
    SetDoubleBuffered(true);

    // Register theme change listener
    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        RefreshTheme();
    });
}

DockAreaMergedTitleBar::~DockAreaMergedTitleBar() {
    // Remove theme change listener
    ThemeManager::getInstance().removeThemeChangeListener(this);

    // TabDragHandler will clean up drag preview in its destructor

    // Clean up resize refresh timer
    if (m_resizeRefreshTimer) {
        if (m_resizeRefreshTimer->IsRunning()) {
            m_resizeRefreshTimer->Stop();
        }
        delete m_resizeRefreshTimer;
        m_resizeRefreshTimer = nullptr;
    }

    // unique_ptr will automatically clean up m_renderer, m_layoutCalculator, m_dragHandler
}

void DockAreaMergedTitleBar::updateTitle() {
    scheduleRefresh(RefreshTabs);
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

    scheduleRefresh(RefreshButtons);
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
    scheduleRefresh(RefreshTabs);
}

void DockAreaMergedTitleBar::removeTab(DockWidget* dockWidget) {
    auto it = std::find_if(m_tabs.begin(), m_tabs.end(),
        [dockWidget](const TabInfo& tab) { return tab.widget == dockWidget; });

    if (it != m_tabs.end()) {
        m_tabs.erase(it);
        updateButtonStates(); // Update button visibility after tab removal
        updateTabRects();
        scheduleRefresh(RefreshTabs);
    }
}

void DockAreaMergedTitleBar::setCurrentIndex(int index) {
    if (m_currentIndex != index) {
        m_currentIndex = index;

        // Ensure current tab is visible when overflow is active
        if (m_hasOverflow) {
            updateTabRects();
        }

        scheduleRefresh(RefreshTabs);
    }
}

DockWidget* DockAreaMergedTitleBar::getTabWidget(int index) const {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        return m_tabs[index].widget;
    }
    return nullptr;
}

void DockAreaMergedTitleBar::onPaint(wxPaintEvent& event) {
    ScopedPerformanceTimer timer("DockAreaMergedTitleBar::onPaint");
    wxAutoBufferedPaintDC dc(this);
    wxRect clientRect = GetClientRect();

    // Reset pending refresh flag since we're painting now
    if (m_pendingRefresh) {
        m_pendingRefresh = false;
        m_refreshFlags = 0;
        Unbind(wxEVT_IDLE, &DockAreaMergedTitleBar::onIdleRefresh, this);
    }

    // Render background using TitleBarRenderer
    if (m_renderer) {
        m_renderer->renderBackground(dc, clientRect);
    }

    // Draw decorative pattern on title bar (skip during active resize)
    bool skipHeavyDecor = false;
    if (m_dockArea && m_dockArea->dockContainer()) {
        if (DockContainerWidget* c = m_dockArea->dockContainer()) {
            skipHeavyDecor = c->isResizeInProgress();
        }
    }
    
    if (!skipHeavyDecor && m_renderer) {
        // Use object pool for render info vectors
        auto tabRenderInfos = RenderObjectPool::getInstance().acquireTabRenderInfoVector();
        auto buttonRenderInfos = RenderObjectPool::getInstance().acquireButtonRenderInfoVector();

        // Prepare tab render info
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            TabRenderInfo info;
            info.widget = m_tabs[i].widget;
            info.rect = m_tabs[i].rect;
            info.closeButtonRect = m_tabs[i].closeButtonRect;
            info.isCurrent = (static_cast<int>(i) == m_currentIndex);
            info.isHovered = m_tabs[i].hovered;
            info.closeButtonHovered = m_tabs[i].closeButtonHovered;
            info.showCloseButton = m_tabs[i].showCloseButton;
            tabRenderInfos->push_back(info);
        }

        // Prepare button render info
        if (m_showLockButton && !m_lockButtonRect.IsEmpty()) {
            ButtonRenderInfo info;
            info.rect = m_lockButtonRect;
            info.isHovered = m_lockButtonHovered;
            info.iconName = isAnyTabLocked() ? "lock" : "lock";
            buttonRenderInfos->push_back(info);
        }
        if (m_showPinButton && !m_pinButtonRect.IsEmpty()) {
            ButtonRenderInfo info;
            info.rect = m_pinButtonRect;
            info.isHovered = m_pinButtonHovered;
            info.iconName = isAnyTabPinned() ? "pinned" : "pin";
            buttonRenderInfos->push_back(info);
        }
        if (m_showCloseButton && !m_closeButtonRect.IsEmpty()) {
            ButtonRenderInfo info;
            info.rect = m_closeButtonRect;
            info.isHovered = m_closeButtonHovered;
            info.iconName = "close";
            buttonRenderInfos->push_back(info);
        }
        if (m_showAutoHideButton && !m_autoHideButtonRect.IsEmpty()) {
            ButtonRenderInfo info;
            info.rect = m_autoHideButtonRect;
            info.isHovered = m_autoHideButtonHovered;
            info.iconName = "auto_hide";
            buttonRenderInfos->push_back(info);
        }

        m_renderer->renderTitleBarPattern(dc, clientRect, *tabRenderInfos, *buttonRenderInfos);

        // Release vectors back to pool
        RenderObjectPool::getInstance().releaseTabRenderInfoVector(std::move(tabRenderInfos));
        RenderObjectPool::getInstance().releaseButtonRenderInfoVector(std::move(buttonRenderInfos));
    }

    // Draw tabs using TitleBarRenderer
    if (m_renderer) {
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (!m_tabs[i].rect.IsEmpty()) {
                TabRenderInfo info;
                info.widget = m_tabs[i].widget;
                info.rect = m_tabs[i].rect;
                info.closeButtonRect = m_tabs[i].closeButtonRect;
                info.isCurrent = (i == m_currentIndex);
                info.isHovered = m_tabs[i].hovered;
                info.closeButtonHovered = m_tabs[i].closeButtonHovered;
                info.showCloseButton = m_tabs[i].showCloseButton;
                m_renderer->renderTab(dc, info, m_tabPosition);
            }
        }
    }
     
    // Draw overflow button if needed
    if (m_hasOverflow && m_renderer) {
        const DockStyleConfig& style = GetDockStyleConfig();
        m_renderer->renderOverflowButton(dc, m_overflowButtonRect, style);
    }

    // Draw buttons using TitleBarRenderer
    if (m_renderer) {
        // Use object pool for button render info vector
        auto buttonInfos = RenderObjectPool::getInstance().acquireButtonRenderInfoVector();
        
        if (m_showLockButton && !m_lockButtonRect.IsEmpty()) {
            ButtonRenderInfo info;
            info.rect = m_lockButtonRect;
            info.isHovered = m_lockButtonHovered;
            info.iconName = isAnyTabLocked() ? "lock" : "unlock";
            buttonInfos->push_back(info);
        }
        if (m_showPinButton && !m_pinButtonRect.IsEmpty()) {
            ButtonRenderInfo info;
            info.rect = m_pinButtonRect;
            info.isHovered = m_pinButtonHovered;
            info.iconName = isAnyTabPinned() ? "pinned" : "pin";
            buttonInfos->push_back(info);
        }
        if (m_showCloseButton && !m_closeButtonRect.IsEmpty()) {
            ButtonRenderInfo info;
            info.rect = m_closeButtonRect;
            info.isHovered = m_closeButtonHovered;
            info.iconName = "close";
            buttonInfos->push_back(info);
        }
        if (m_showAutoHideButton && !m_autoHideButtonRect.IsEmpty()) {
            ButtonRenderInfo info;
            info.rect = m_autoHideButtonRect;
            info.isHovered = m_autoHideButtonHovered;
            info.iconName = "auto_hide";
            buttonInfos->push_back(info);
        }

        const DockStyleConfig& style = GetDockStyleConfig();
        m_renderer->renderButtons(dc, clientRect, *buttonInfos, m_tabPosition, style);

        // Release vector back to pool
        RenderObjectPool::getInstance().releaseButtonRenderInfoVector(std::move(buttonInfos));
    }

    // Redraw bottom border last so it's not covered by subsequent drawing
    dc.SetPen(wxPen(CFG_COLOUR("TabBorderBottomColour"), 1));
    int bottomY = GetClientSize().GetHeight() - 1;
    dc.DrawLine(0, bottomY, GetClientSize().GetWidth(), bottomY);
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

        // Select tab first
        if (tabIndex != m_currentIndex) {
            m_dockArea->setCurrentIndex(tabIndex);
        }

        // Delegate drag handling to TabDragHandler
        if (m_dragHandler && m_dragHandler->handleMouseDown(event, tabIndex)) {
            // Indicate drag start with grab-like cursor
            SetCursor(wxCursor(wxCURSOR_SIZING));
        CaptureMouse();
        }
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

void DockAreaMergedTitleBar::onMouseCaptureLost(wxMouseCaptureLostEvent& event) {
    // Clean up drag state when mouse capture is lost
    if (m_dragHandler && m_dragHandler->hasDraggedTab()) {
        // Cancel drag operation
        m_dragHandler->cancelDrag();
        
        // Hide any overlays
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
    
    // Restore default cursor
    SetCursor(wxCursor(wxCURSOR_ARROW));
    
    // Clear tooltip
    UnsetToolTip();
    
    event.Skip();
}

void DockAreaMergedTitleBar::onMouseLeftUp(wxMouseEvent& event) {
    // Always release mouse capture first
    if (HasCapture()) {
        ReleaseMouse();
    }

    // Delegate drag handling to TabDragHandler
    // This handles both started drags and cancelled drags
    // CRITICAL: After handleMouseUp, the DockArea (and this title bar) may be destroyed
    // if the last widget was moved. Check if we're still valid before accessing members.
    if (m_dragHandler) {
        m_dragHandler->handleMouseUp(event);
    }

    // CRITICAL: Check if this object is still valid before accessing members
    // The DockArea may have been destroyed during handleMouseUp->handleDrop
    if (IsBeingDeleted()) {
        return; // Object is being destroyed, don't access members
    }
    
    // Check if parent (DockArea) is still valid
    if (!GetParent() || GetParent()->IsBeingDeleted()) {
        return; // Parent is being destroyed, don't access members
    }

    // Clear tooltip
    UnsetToolTip();

    // Restore default cursor after drag ends
    SetCursor(wxCursor(wxCURSOR_ARROW));
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
            RenderOptimizer::getInstance().optimizeRefresh(this, &m_tabs[i].closeButtonRect);
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
                RenderOptimizer::getInstance().optimizeRefresh(this, &m_tabs[oldHoveredTab].rect);
            }
            if (m_hoveredTab >= 0 && m_hoveredTab < static_cast<int>(m_tabs.size())) {
                RenderOptimizer::getInstance().optimizeRefresh(this, &m_tabs[m_hoveredTab].rect);
            }
        }
        if (oldPinHovered != m_pinButtonHovered) {
            RenderOptimizer::getInstance().optimizeRefresh(this, &m_pinButtonRect);
        }
        if (oldCloseHovered != m_closeButtonHovered) {
            RenderOptimizer::getInstance().optimizeRefresh(this, &m_closeButtonRect);
        }
        if (oldAutoHideHovered != m_autoHideButtonHovered) {
            RenderOptimizer::getInstance().optimizeRefresh(this, &m_autoHideButtonRect);
        }
        if (oldLockHovered != m_lockButtonHovered) {
            RenderOptimizer::getInstance().optimizeRefresh(this, &m_lockButtonRect);
        }
    }

    // Update cursor for hover/drag state
    if (m_dragHandler && m_dragHandler->isDragging()) {
        SetCursor(wxCursor(wxCURSOR_SIZING));
    } else if (m_hasOverflow && m_overflowButtonRect.Contains(pos)) {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    } else if (m_hoveredTab >= 0) {
        SetCursor(wxCursor(wxCURSOR_HAND));
    } else if (m_showCloseButton && m_closeButtonRect.Contains(pos)) {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    } else if (m_showPinButton && m_pinButtonRect.Contains(pos)) {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    } else if (m_showAutoHideButton && m_autoHideButtonRect.Contains(pos)) {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    } else if (m_showLockButton && m_lockButtonRect.Contains(pos)) {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    } else {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }

    // Delegate drag handling to TabDragHandler
    // Always call handleMouseMove if we have a dragged tab, even if drag hasn't started yet
    // This allows handleMouseMove to decide when to start the drag
    if (m_dragHandler && m_dragHandler->hasDraggedTab()) {
        m_dragHandler->handleMouseMove(event);
        
        // Update cursor for drag state
        if (m_dragHandler->isDragging()) {
            SetCursor(wxCursor(wxCURSOR_SIZING));
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

    // Cancel drag if mouse leaves while dragging
    if (m_dragHandler && m_dragHandler->hasDraggedTab() && !m_dragHandler->isDragging()) {
        // Only cancel if drag hasn't started yet (just mouse down)
        m_dragHandler->cancelDrag();
        if (HasCapture()) {
            ReleaseMouse();
        }
    }

    RenderOptimizer::getInstance().optimizeRefresh(this, nullptr);

    // Restore default cursor when leaving title bar
    SetCursor(wxCursor(wxCURSOR_ARROW));
    
    event.Skip();
}

// Ensure cursor stays updated when system queries cursor
void DockAreaMergedTitleBar::onSetCursor(wxSetCursorEvent& event) {
    if (m_dragHandler && m_dragHandler->isDragging()) {
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

void DockAreaMergedTitleBar::onMouseEnter(wxMouseEvent& event) {
    // Update cursor immediately upon entering
    if (m_hoveredTab >= 0) {
        SetCursor(wxCursor(wxCURSOR_HAND));
    } else {
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
    event.Skip();
}

void DockAreaMergedTitleBar::onSize(wxSizeEvent& event) {
    updateTabRects();

    // Use deferred refresh to prevent excessive redraws during resize
    if (!m_resizeRefreshTimer) {
        m_resizeRefreshTimer = new wxTimer(this);
    }

    // Cancel any pending refresh
    if (m_resizeRefreshTimer->IsRunning()) {
        m_resizeRefreshTimer->Stop();
    }

    // Schedule refresh with debounce delay
    m_resizeRefreshTimer->Start(50, wxTIMER_ONE_SHOT); // 50ms debounce

    event.Skip();
}

void DockAreaMergedTitleBar::updateTabRects() {
    if (!m_layoutCalculator) {
        return;
    }

    wxSize size = GetClientSize();
    
    // Prepare widget list
    std::vector<DockWidget*> widgets;
    for (const auto& tab : m_tabs) {
        widgets.push_back(tab.widget);
    }

    // Get style config
    const DockStyleConfig& style = GetDockStyleConfig();

    // Calculate layout using TabLayoutCalculator
    std::vector<TabLayoutInfo> layoutInfos;
    m_layoutCalculator->calculateLayout(
        size, widgets, m_currentIndex, m_tabPosition, style,
        m_showCloseButton, m_showAutoHideButton, m_showPinButton, m_showLockButton,
        m_buttonSize, layoutInfos, m_hasOverflow, m_firstVisibleTab, m_overflowButtonRect
    );

    // Update tab rects from layout info
    for (size_t i = 0; i < layoutInfos.size() && i < m_tabs.size(); ++i) {
        m_tabs[i].rect = layoutInfos[i].rect;
        m_tabs[i].closeButtonRect = layoutInfos[i].closeButtonRect;
        m_tabs[i].showCloseButton = layoutInfos[i].showCloseButton;
    }

    // Update button rects
    updateButtonRects();
}

void DockAreaMergedTitleBar::updateButtonRects() {
    if (!m_buttonLayoutCalculator) {
        return;
    }

    wxSize clientSize = GetClientSize();
    ButtonLayoutInfo layoutInfo;
    
    m_buttonLayoutCalculator->calculateLayout(
        clientSize,
        m_tabPosition,
        m_buttonSize,
        m_buttonSpacing,
        m_showCloseButton,
        m_showAutoHideButton,
        m_showPinButton,
        m_showLockButton,
        layoutInfo
    );

    m_pinButtonRect = layoutInfo.pinButtonRect;
    m_closeButtonRect = layoutInfo.closeButtonRect;
    m_autoHideButtonRect = layoutInfo.autoHideButtonRect;
    m_lockButtonRect = layoutInfo.lockButtonRect;
}

// Removed: updateHorizontalTabRects
// This method has been replaced by TabLayoutCalculator::calculateHorizontalLayout

// Removed: updateVerticalTabRects
// This method has been replaced by TabLayoutCalculator::calculateVerticalLayout

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

// Removed: drawTab, drawButtons, drawHorizontalButtons, drawVerticalButtons
// These methods have been replaced by TitleBarRenderer::renderTab and TitleBarRenderer::renderButtons

void DockAreaMergedTitleBar::onLockButtonClicked() {
    // Toggle lock state for all widgets in this dock area
    bool shouldLock = !isAnyTabLocked();

    for (auto& tab : m_tabs) {
        if (tab.widget) {
            tab.widget->setPositionLocked(shouldLock);
        }
    }

    RenderOptimizer::getInstance().optimizeRefresh(this, nullptr); // Refresh to update button appearance
}

bool DockAreaMergedTitleBar::isAnyTabLocked() const {
    for (const auto& tab : m_tabs) {
        if (tab.widget && tab.widget->isPositionLocked()) {
            return true;
        }
    }
    return false;
}

bool DockAreaMergedTitleBar::isAnyTabPinned() const {
    for (const auto& tab : m_tabs) {
        if (tab.widget && tab.widget->isPinned()) {
            return true;
        }
    }
    return false;
}

// Removed: drawButton
// This method has been replaced by TitleBarRenderer::renderButton

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

wxRect DockAreaMergedTitleBar::getTabRect(int tabIndex) const {
    if (tabIndex >= 0 && tabIndex < static_cast<int>(m_tabs.size())) {
        return m_tabs[tabIndex].rect;
    }
    return wxRect();
}

// Removed: findTargetWindowUnderMouse
// This method has been moved to TabDragHandler::findTargetWindowUnderMouse

bool DockAreaMergedTitleBar::isDraggingTab() const {
    return m_dragHandler && m_dragHandler->isDragging();
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
    scheduleRefresh(RefreshAll);
}

// Removed: drawTitleBarPattern
// This method has been replaced by TitleBarRenderer::renderTitleBarPattern

void DockAreaMergedTitleBar::scheduleRefresh(unsigned int flags) {
    m_refreshFlags |= flags;

    if (!m_pendingRefresh) {
        m_pendingRefresh = true;
        // Use idle time to perform refresh for better responsiveness
        Bind(wxEVT_IDLE, &DockAreaMergedTitleBar::onIdleRefresh, this);
    }
}

void DockAreaMergedTitleBar::performRefresh() {
    if (m_refreshFlags != 0) {
        RenderOptimizer::getInstance().optimizeRefresh(this, nullptr);
        m_refreshFlags = 0;
        m_pendingRefresh = false;
        Unbind(wxEVT_IDLE, &DockAreaMergedTitleBar::onIdleRefresh, this);
    }
}

void DockAreaMergedTitleBar::onIdleRefresh(wxIdleEvent& event) {
    performRefresh();
    event.Skip();
}

void DockAreaMergedTitleBar::onResizeRefreshTimer(wxTimerEvent& event) {
    // Perform the deferred refresh after resize debounce period
    RenderOptimizer::getInstance().optimizeRefresh(this, nullptr);
    Update();
}

void DockAreaMergedTitleBar::RefreshTheme() {
    // Apply theme colors to background
    SetBackgroundColour(CFG_COLOUR("DockTitleBarBgColour"));

    // Refresh the display to apply new theme colors
    RenderOptimizer::getInstance().optimizeRefresh(this, nullptr);
    Update();
}

} // namespace ads
