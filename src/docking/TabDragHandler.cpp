#include "docking/TabDragHandler.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/DockManager.h"
#include "docking/DockContainerWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockOverlay.h"
#include <wx/log.h>

namespace ads {

TabDragHandler::TabDragHandler(DockAreaMergedTitleBar* titleBar)
    : m_titleBar(titleBar)
    , m_draggedTab(-1)
    , m_dragStarted(false)
    , m_dragPreview(nullptr)
{
}

DockArea* TabDragHandler::getDockArea() const {
    // CRITICAL: Add null check and verify titleBar is still valid
    if (!m_titleBar) {
        return nullptr;
    }
    
    // Check if titleBar is being deleted
    if (m_titleBar->IsBeingDeleted()) {
        return nullptr;
    }
    
    DockArea* area = m_titleBar->dockArea();
    
    // Additional safety check: verify the area is still valid
    if (area && area->IsBeingDeleted()) {
        return nullptr;
    }
    
    return area;
}

TabDragHandler::~TabDragHandler() {
    if (m_dragPreview) {
        if (!m_dragPreview->IsBeingDeleted()) {
            m_dragPreview->finishDrag();
            m_dragPreview->Destroy();
        }
        m_dragPreview = nullptr;
    }
}

bool TabDragHandler::handleMouseDown(wxMouseEvent& event, int tabIndex) {
    DockArea* dockArea = getDockArea();
    if (tabIndex < 0 || !dockArea) {
        return false;
    }

    DockWidget* draggedWidget = dockArea->dockWidget(tabIndex);
    if (!draggedWidget || draggedWidget->isPositionLocked()) {
        return false;
    }

    startDrag(tabIndex, event.GetPosition());
    return true;
}

void TabDragHandler::handleMouseUp(wxMouseEvent& event) {
    // Migrated original logic from DockAreaMergedTitleBar::onMouseLeftUp
    // CRITICAL: Save DockManager reference BEFORE handleDrop, because handleDrop
    // may destroy the source DockArea (if it's the last widget), making getDockArea() invalid
    DockManager* manager = nullptr;
    DockArea* sourceDockArea = nullptr;
    bool sourceAreaWillBeDestroyed = false;
    
    if (m_draggedTab >= 0) {
        sourceDockArea = getDockArea();
        if (sourceDockArea) {
            manager = sourceDockArea->dockManager();
            // Check if source area will be destroyed (if it's the last widget)
            if (sourceDockArea->dockWidgetsCount() == 1) {
                sourceAreaWillBeDestroyed = true;
                wxLogDebug("TabDragHandler::handleMouseUp - Source area will be destroyed after drop");
            }
        }
    }
    
    if (m_draggedTab >= 0) {
        // Handle drop if drag actually started
        if (m_dragStarted) {
            // Clean up drag preview
            if (m_dragPreview) {
                m_dragPreview->finishDrag();
                m_dragPreview->Destroy();
                m_dragPreview = nullptr;
            }

            // Get the widget being dragged
            DockArea* dockArea = getDockArea();
            DockWidget* draggedWidget = dockArea ? dockArea->dockWidget(m_draggedTab) : nullptr;
            
            if (draggedWidget && manager) {
                wxPoint screenPos = m_titleBar ? m_titleBar->ClientToScreen(event.GetPosition()) : event.GetPosition();
                handleDrop(screenPos);
                
                // CRITICAL: After handleDrop, if source area was destroyed, m_titleBar is now invalid
                // Don't access m_titleBar or any members after this point
                if (sourceAreaWillBeDestroyed) {
                    // Mark that we should not access m_titleBar anymore
                    m_titleBar = nullptr;
                }
            } else {
                // Widget or manager is invalid, cancel drag
                cancelDrag();
            }
        } else {
            // Drag didn't start (just a click), clean up state
            m_draggedTab = -1;
            m_dragStarted = false;
        }
    }

    // Always hide overlays on mouse up - use saved manager reference
    // because getDockArea() may be invalid if source area was destroyed
    if (manager) {
        DockOverlay* areaOverlay = manager->dockAreaOverlay();
        if (areaOverlay) {
            areaOverlay->hideOverlay();
        }
        DockOverlay* containerOverlay = manager->containerOverlay();
        if (containerOverlay) {
            containerOverlay->hideOverlay();
        }
    } else {
        // Fallback: try to hide overlays through getDockArea if manager wasn't saved
        // But only if m_titleBar is still valid
        if (m_titleBar && !m_titleBar->IsBeingDeleted()) {
            hideAllOverlays();
        }
    }
    
    // Always reset drag state
    m_draggedTab = -1;
    m_dragStarted = false;
}

void TabDragHandler::handleMouseMove(wxMouseEvent& event) {
    // Migrated original logic from DockAreaMergedTitleBar::onMouseMotion
    // Process drag if we have a dragged tab (even if Dragging() flag is not set yet)
    if (m_draggedTab >= 0) {
        wxPoint pos = event.GetPosition();
        wxPoint delta = pos - m_dragStartPos;

        // Check if we should start drag operation (require minimum drag distance)
        // AND ensure mouse is still within the tab area to prevent accidental drag triggers
        // Also check if dragging flag is set OR we've moved enough to start dragging
        if (!m_dragStarted && (event.Dragging() || (abs(delta.x) > 10 || abs(delta.y) > 10))) {
            // Additional check: mouse must still be within the dragged tab's rectangle
            // or within a reasonable distance from the drag start position
            wxPoint currentPos = event.GetPosition();
            int draggedTabIndex = m_draggedTab;

            // Check if mouse is still within the dragged tab's rectangle
            // Migrated original logic from DockAreaMergedTitleBar::onMouseMotion
            bool isWithinTabArea = false;
            if (draggedTabIndex >= 0 && m_titleBar) {
                wxRect tabRect = m_titleBar->getTabRect(draggedTabIndex);
                // Inflate the tab rectangle slightly to allow for some tolerance
                tabRect.Inflate(10, 10);
                isWithinTabArea = tabRect.Contains(currentPos);
            }

            // If mouse moved outside the tab area, don't start drag
            if (!isWithinTabArea) {
                wxLogDebug("TabDragHandler::handleMouseMove - Mouse moved outside tab area, canceling drag");
                m_draggedTab = -1;
                return;
            }
            
            // Start the drag operation
            m_dragStarted = true;

            // Get the dock widget being dragged
            DockArea* dockArea = getDockArea();
            if (dockArea && dockArea->dockManager()) {
                DockWidget* draggedWidget = dockArea->dockWidget(m_draggedTab);
                if (draggedWidget && draggedWidget->hasFeature(DockWidgetMovable)) {
                    DockManager* manager = dockArea->dockManager();

                    // Create a floating drag preview
                    FloatingDragPreview* preview = new FloatingDragPreview(draggedWidget, manager->containerWidget());
                    wxPoint screenPos = m_titleBar->ClientToScreen(event.GetPosition());
                    preview->startDrag(screenPos);

                    // Store preview reference
                    m_dragPreview = preview;

                    // Mark widget as being dragged (don't actually float yet)
                    // We'll handle the actual move on drop
                }
            }
        }

        // Continue processing if drag has started OR if dragging flag is set
        if (m_dragStarted || event.Dragging()) {
            wxPoint screenPos = m_titleBar->ClientToScreen(event.GetPosition());

            // Update drag preview position
            if (m_dragPreview) {
                m_dragPreview->moveFloating(screenPos);
            }

            // Check for drop targets under mouse - migrated original logic
            DockArea* dockArea = getDockArea();
            if (dockArea && dockArea->dockManager()) {
                DockManager* manager = dockArea->dockManager();
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
                            overlay->setAllowedAreas(CenterDockWidgetArea);

                            // Show tooltip to indicate tab merge is possible
                            DockArea* sourceDockArea = getDockArea();
                            if (sourceDockArea && sourceDockArea->mergedTitleBar()) {
                                sourceDockArea->mergedTitleBar()->showDragFeedback(true);
                            }
                        } else {
                            wxLogDebug("Not over title bar - showing all areas");
                            overlay->showOverlay(targetArea);
                            overlay->setAllowedAreas(AllDockAreas);

                            // Hide merge feedback
                            DockArea* sourceDockArea = getDockArea();
                            if (sourceDockArea && sourceDockArea->mergedTitleBar()) {
                                sourceDockArea->mergedTitleBar()->showDragFeedback(false);
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

void TabDragHandler::cancelDrag() {
    // Clean up drag preview
    if (m_dragPreview) {
        if (!m_dragPreview->IsBeingDeleted()) {
            m_dragPreview->finishDrag();
            m_dragPreview->Destroy();
        }
        m_dragPreview = nullptr;
    }
    
    // Hide all overlays
    hideAllOverlays();
    
    // Reset drag state
    m_draggedTab = -1;
    m_dragStarted = false;
}

void TabDragHandler::startDrag(int tabIndex, const wxPoint& startPos) {
    m_draggedTab = tabIndex;
    m_dragStartPos = startPos;
    m_dragStarted = false;
}

void TabDragHandler::updateDrag(const wxPoint& currentPos) {
    if (m_dragPreview) {
        m_dragPreview->moveFloating(currentPos);
    }

    DockArea* dockArea = getDockArea();
    if (dockArea && dockArea->dockManager()) {
        DockArea* targetArea = findTargetArea(currentPos);
        if (targetArea) {
            showOverlayForTarget(targetArea, currentPos);
        } else {
            hideAllOverlays();
        }
    }
}

void TabDragHandler::finishDrag(const wxPoint& dropPos, bool cancelled) {
    if (cancelled) {
        cancelDrag();
        return;
    }

    if (m_dragPreview) {
        m_dragPreview->finishDrag();
        m_dragPreview->Destroy();
        m_dragPreview = nullptr;
    }

    DockArea* dockArea = getDockArea();
    DockWidget* draggedWidget = dockArea ? dockArea->dockWidget(m_draggedTab) : nullptr;
    DockManager* manager = dockArea ? dockArea->dockManager() : nullptr;

    if (draggedWidget && manager) {
        handleDrop(dropPos);
    }

    hideAllOverlays();
    
    m_draggedTab = -1;
    m_dragStarted = false;
}

void TabDragHandler::handleDrop(const wxPoint& screenPos) {
    // Migrated complete original logic from DockAreaMergedTitleBar::onMouseLeftUp
    DockArea* dockArea = getDockArea();
    DockWidget* draggedWidget = dockArea ? dockArea->dockWidget(m_draggedTab) : nullptr;
    DockManager* manager = dockArea ? dockArea->dockManager() : nullptr;
    
    if (!draggedWidget || !manager) {
        // Reset drag state if widget or manager is invalid
        m_draggedTab = -1;
        m_dragStarted = false;
        return;
    }

    // CRITICAL: Save original position BEFORE any removal operations
    // This allows us to restore the widget if drop fails
    DockArea* originalArea = draggedWidget->dockAreaWidget();
    int originalIndex = -1;
    bool originalAreaWillBeDestroyed = false;
    
    if (originalArea) {
        originalIndex = originalArea->indexOfDockWidget(draggedWidget);
        if (originalArea->dockWidgetsCount() == 1) {
            originalAreaWillBeDestroyed = true;
            wxLogDebug("TabDragHandler::handleDrop - Original area will be destroyed if widget is removed");
        }
        wxLogDebug("TabDragHandler::handleDrop - Original position: area %p, index %d", originalArea, originalIndex);
    }

    // Check for drop target - migrated original logic
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

    wxLogDebug("TabDragHandler::handleDrop - targetArea: %p", targetArea);

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
                    // CRITICAL: Save sourceArea reference before removal, as it may be destroyed
                    DockArea* sourceArea = draggedWidget->dockAreaWidget();
                    bool sourceAreaWillBeDestroyed = false;
                    
                    // Check if source area will be destroyed (if it's the last widget)
                    if (sourceArea && sourceArea->dockWidgetsCount() == 1) {
                        sourceAreaWillBeDestroyed = true;
                        wxLogDebug("Source area will be destroyed after removing last widget");
                    }

                    // Remove widget from current area if needed
                    if (sourceArea && sourceArea != targetArea) {
                        // Store the original tab position before removal
                        TabPosition sourceTabPosition = sourceArea->tabPosition();

                        sourceArea->removeDockWidget(draggedWidget);
                        
                        // CRITICAL: After removeDockWidget, sourceArea may be destroyed
                        // Do NOT access sourceArea after this point if it was the last widget
                        if (sourceAreaWillBeDestroyed) {
                            sourceArea = nullptr; // Mark as invalid
                        }
                        
                        // Ensure widget is properly reparented before adding to new area
                        // This prevents sizer parent mismatch issues
                        if (draggedWidget && draggedWidget->GetParent()) {
                            // Widget should now be parented to containerWidget
                            // The addDockWidget will reparent it to the target area's content area
                            wxLogDebug("Widget removed from source area, parent: %p", draggedWidget->GetParent());
                        }

                        wxLogDebug("Widget removed from source area, original tab position was %d, target is %d",
                                  static_cast<int>(sourceTabPosition),
                                  static_cast<int>(targetTabPosition));
                    }
                    
                    // Add to target area - this will reparent the widget correctly
                    // CRITICAL: Check if widget is still valid before adding
                    if (draggedWidget && draggedWidget->GetParent()) {
                        targetArea->addDockWidget(draggedWidget);

                        // If the target area has merged title bar, make sure the new tab becomes current
                        if (targetArea->mergedTitleBar()) {
                            targetArea->setCurrentDockWidget(draggedWidget);
                        }

                        docked = true;
                        wxLogDebug("TabDragHandler::handleDrop - Successfully docked widget to target area");
                    } else {
                        wxLogDebug("TabDragHandler::handleDrop - ERROR: Widget invalid after removal, restoring to original position");
                        // Restore to original position
                        if (originalArea && !originalAreaWillBeDestroyed && originalIndex >= 0) {
                            originalArea->insertDockWidget(originalIndex, draggedWidget, true);
                        } else if (originalArea && !originalAreaWillBeDestroyed) {
                            originalArea->addDockWidget(draggedWidget);
                        }
                        // Reset drag state even on failure
                        m_draggedTab = -1;
                        m_dragStarted = false;
                        return;
                    }
                } else {
                    // Dock to side
                    wxLogDebug("Docking widget to side: %d", dropArea);
                    // CRITICAL: Remove widget before docking to side
                    if (draggedWidget->dockAreaWidget() == dockArea) {
                        dockArea->removeDockWidget(draggedWidget);
                    }
                    
                    // Verify widget is still valid
                    if (draggedWidget && draggedWidget->GetParent()) {
                        targetArea->dockContainer()->addDockWidget(dropArea, draggedWidget, targetArea);
                        docked = true;
                        wxLogDebug("TabDragHandler::handleDrop - Successfully docked widget to side");
                    } else {
                        wxLogDebug("TabDragHandler::handleDrop - ERROR: Widget invalid after removal, restoring to original position");
                        // Restore to original position
                        if (originalArea && !originalAreaWillBeDestroyed && originalIndex >= 0) {
                            originalArea->insertDockWidget(originalIndex, draggedWidget, true);
                        } else if (originalArea && !originalAreaWillBeDestroyed) {
                            originalArea->addDockWidget(draggedWidget);
                        }
                        // Reset drag state even on failure
                        m_draggedTab = -1;
                        m_dragStarted = false;
                        return;
                    }
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

                // Reset drag state immediately after successful drop to target area
                m_draggedTab = -1;
                m_dragStarted = false;

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
                if (draggedWidget->dockAreaWidget() == dockArea) {
                    wxLogDebug("Removing widget from current area");
                    dockArea->removeDockWidget(draggedWidget);
                }

                // Verify widget is still valid
                if (!draggedWidget->GetParent()) {
                    wxLogDebug("ERROR: Widget has no parent after removal!");
                    return;
                }

                // Add to container at specified position
                wxLogDebug("Adding widget to container at position %d", dropArea);
                wxLogDebug("Widget ptr: %p, title: %s", draggedWidget, draggedWidget->title().c_str());
                
                // Verify widget is still valid before adding
                if (draggedWidget && draggedWidget->GetParent()) {
                    manager->addDockWidget(dropArea, draggedWidget);
                    docked = true;
                    wxLogDebug("TabDragHandler::handleDrop - Successfully docked widget to container");
                    
                    // Reset drag state immediately after successful drop to container
                    m_draggedTab = -1;
                    m_dragStarted = false;
                } else {
                    wxLogDebug("TabDragHandler::handleDrop - ERROR: Widget invalid after removal, restoring to original position");
                    // Restore to original position
                    if (originalArea && !originalAreaWillBeDestroyed && originalIndex >= 0) {
                        originalArea->insertDockWidget(originalIndex, draggedWidget, true);
                    } else if (originalArea && !originalAreaWillBeDestroyed) {
                        originalArea->addDockWidget(draggedWidget);
                    }
                    // Reset drag state even on failure
                    m_draggedTab = -1;
                    m_dragStarted = false;
                    return;
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

    // If not docked, restore to original position
    if (!docked) {
        wxLogDebug("TabDragHandler::handleDrop - Not docked, restoring widget to original position");

        // Restore widget to original position if it was removed
        if (draggedWidget->dockAreaWidget() != originalArea) {
            if (originalArea && !originalAreaWillBeDestroyed) {
                // Restore to original position
                if (originalIndex >= 0 && originalIndex < originalArea->dockWidgetsCount()) {
                    originalArea->insertDockWidget(originalIndex, draggedWidget, true);
                    wxLogDebug("TabDragHandler::handleDrop - Restored widget to original position at index %d", originalIndex);
                } else {
                    originalArea->addDockWidget(draggedWidget);
                    wxLogDebug("TabDragHandler::handleDrop - Restored widget to original area (appended)");
                }
            } else {
                // Original area was destroyed or doesn't exist, create floating container as fallback
                wxLogDebug("TabDragHandler::handleDrop - Original area destroyed, creating floating container as fallback");
                
                // Remove from current area if still there
                if (draggedWidget->dockAreaWidget() == dockArea) {
                    dockArea->removeDockWidget(draggedWidget);
                }

                // Set as floating
                draggedWidget->setFloating();

                FloatingDockContainer* floatingContainer = draggedWidget->floatingDockContainer();
                if (floatingContainer) {
                    floatingContainer->SetPosition(screenPos - wxPoint(50, 10));
                    floatingContainer->Show();
                    floatingContainer->Raise();
                }
            }
        } else {
            wxLogDebug("TabDragHandler::handleDrop - Widget still in original position, no action needed");
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

        // Reset drag state after restoring
        m_draggedTab = -1;
        m_dragStarted = false;

        // Return early since the area might be destroyed
        return;
    }

    // Hide any overlays that might be showing
    if (dockArea && dockArea->dockManager()) {
        DockOverlay* areaOverlay = dockArea->dockManager()->dockAreaOverlay();
        if (areaOverlay) {
            areaOverlay->hideOverlay();
        }
        DockOverlay* containerOverlay = dockArea->dockManager()->containerOverlay();
        if (containerOverlay) {
            containerOverlay->hideOverlay();
        }
    }
    
    // Final cleanup: reset drag state if not already reset
    // This is a safety net in case we reach here without resetting
    m_draggedTab = -1;
    m_dragStarted = false;
}

DockArea* TabDragHandler::findTargetArea(const wxPoint& screenPos) {
    DockArea* dockArea = getDockArea();
    if (!dockArea || !dockArea->dockManager()) {
        return nullptr;
    }

    wxWindow* targetWindow = findTargetWindowUnderMouse(screenPos, m_dragPreview);
    wxWindow* checkWindow = targetWindow;
    
    while (checkWindow) {
        DockArea* area = dynamic_cast<DockArea*>(checkWindow);
        if (area && area != dockArea) {
            return area;
        }
        checkWindow = checkWindow->GetParent();
    }
    
    return nullptr;
}

wxWindow* TabDragHandler::findTargetWindowUnderMouse(const wxPoint& screenPos, wxWindow* dragPreview) {
    DockArea* dockArea = getDockArea();
    if (!dockArea || !dockArea->dockManager()) {
        return nullptr;
    }

    DockManager* manager = dockArea->dockManager();
    const std::vector<DockArea*>& dockAreas = manager->dockAreas();

    for (DockArea* area : dockAreas) {
        if (area && area != dockArea) {
            wxRect areaRect = area->GetScreenRect();
            if (areaRect.Contains(screenPos)) {
                if (area->mergedTitleBar()) {
                    wxRect titleRect = area->mergedTitleBar()->GetScreenRect();
                    if (titleRect.Contains(screenPos)) {
                        return area->mergedTitleBar();
                    }
                }
                return area;
            }
        }
    }

    wxWindow* windowUnderMouse = wxFindWindowAtPoint(screenPos);
    if (windowUnderMouse && dragPreview) {
        wxWindow* checkWindow = windowUnderMouse;
        while (checkWindow) {
            if (checkWindow == dragPreview) {
                if (manager->containerWidget()) {
                    wxRect containerRect = manager->containerWidget()->GetScreenRect();
                    if (containerRect.Contains(screenPos)) {
                        return manager->containerWidget();
                    }
                }
                return nullptr;
            }
            checkWindow = checkWindow->GetParent();
        }
    }

    return windowUnderMouse;
}

void TabDragHandler::showOverlayForTarget(DockArea* targetArea, const wxPoint& screenPos) {
    DockArea* dockArea = getDockArea();
    if (!dockArea || !dockArea->dockManager() || !targetArea) {
        return;
    }

    DockManager* manager = dockArea->dockManager();
    DockOverlay* overlay = manager->dockAreaOverlay();
    
    if (overlay) {
        wxPoint targetLocalPos = targetArea->ScreenToClient(screenPos);
        bool isOverTitleBar = false;
        
        if (targetArea->mergedTitleBar()) {
            wxRect titleRect = targetArea->mergedTitleBar()->GetRect();
            isOverTitleBar = titleRect.Contains(targetLocalPos);
        }
        
        overlay->showOverlay(targetArea);
        if (isOverTitleBar) {
            overlay->setAllowedAreas(CenterDockWidgetArea);
        } else {
            overlay->setAllowedAreas(AllDockAreas);
        }
    }
}

void TabDragHandler::hideAllOverlays() {
    DockArea* dockArea = getDockArea();
    if (!dockArea || !dockArea->dockManager()) {
        return;
    }

    DockManager* manager = dockArea->dockManager();
    
    DockOverlay* areaOverlay = manager->dockAreaOverlay();
    if (areaOverlay) {
        areaOverlay->hideOverlay();
    }
    
    DockOverlay* containerOverlay = manager->containerOverlay();
    if (containerOverlay) {
        containerOverlay->hideOverlay();
    }
}

} // namespace ads

