#include "docking/DockContainerWidget.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/DockSplitter.h"
#include "docking/DockLayoutConfig.h"
#include <algorithm>

namespace ads {

// Layout management implementation
void DockContainerWidget::addDockAreaSimple(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::addDockAreaSimple - area: %d", area);

    wxWindow* window1 = rootSplitter->GetWindow1();
    wxWindow* window2 = rootSplitter->GetWindow2();

    // First area - just add it
    if (!window1 && !window2) {
        wxLogDebug("  -> First area, initializing root");
        dockArea->Reparent(rootSplitter);
        rootSplitter->Initialize(dockArea);
        return;
    }

    // Only one window exists
    if (!window2) {
        wxLogDebug("  -> Only one window exists");
        dockArea->Reparent(rootSplitter);

        if (area == LeftDockWidgetArea) {
            // Split: [Left | Existing]
            rootSplitter->SplitVertically(dockArea, window1);
            rootSplitter->SetSashPosition(getConfiguredAreaSize(area));
        } else if (area == RightDockWidgetArea) {
            // Split: [Existing | Right]
            rootSplitter->SplitVertically(window1, dockArea);
            rootSplitter->SetSashPosition(rootSplitter->GetSize().GetWidth() - getConfiguredAreaSize(area));
        } else if (area == TopDockWidgetArea) {
            // Split: [Top / Existing]
            rootSplitter->SplitHorizontally(dockArea, window1);
            rootSplitter->SetSashPosition(getConfiguredAreaSize(area));
        } else if (area == BottomDockWidgetArea) {
            // Split: [Existing / Bottom]
            rootSplitter->SplitHorizontally(window1, dockArea);
            rootSplitter->SetSashPosition(rootSplitter->GetSize().GetHeight() - getConfiguredAreaSize(area));
        } else { // CenterDockWidgetArea
            rootSplitter->SplitVertically(window1, dockArea);
        }
        return;
    }

    // Both windows exist - need to handle complex cases
    wxLogDebug("  -> Both windows exist, complex layout needed");
    wxLogDebug("  -> Root splitter mode: %s",
               rootSplitter->GetSplitMode() == wxSPLIT_VERTICAL ? "VERTICAL" : "HORIZONTAL");

    // Debug: Check what's in the windows
    if (DockArea* area1 = dynamic_cast<DockArea*>(window1)) {
        wxLogDebug("  -> Window1 is DockArea");
    } else if (DockSplitter* split1 = dynamic_cast<DockSplitter*>(window1)) {
        wxLogDebug("  -> Window1 is DockSplitter");
    }

    if (DockArea* area2 = dynamic_cast<DockArea*>(window2)) {
        wxLogDebug("  -> Window2 is DockArea");
    } else if (DockSplitter* split2 = dynamic_cast<DockSplitter*>(window2)) {
        wxLogDebug("  -> Window2 is DockSplitter");
    }

    // For SimpleDockingFrame's specific order: Center, Left, Right, Top, Bottom
    // At this point we should have [Left | Center] and we're adding Right
    if (area == RightDockWidgetArea && rootSplitter->GetSplitMode() == wxSPLIT_VERTICAL) {
        wxLogDebug("  -> Adding Right to existing [Left | Center]");

        // Create sub-splitter for Center and Right
        DockSplitter* subSplitter = new DockSplitter(rootSplitter);

        // Store the current sash position
        int currentSashPos = rootSplitter->GetSashPosition();

        // First, unsplit to safely remove windows
        rootSplitter->Unsplit();

        // Reparent windows
        window1->Reparent(rootSplitter);  // Keep Left in root
        window2->Reparent(subSplitter);   // Move Center to sub-splitter
        dockArea->Reparent(subSplitter);  // Add Right to sub-splitter

        // Set up the sub-splitter with Center and Right
        subSplitter->SplitVertically(window2, dockArea);

        // Set up the root splitter with Left and the sub-splitter
        rootSplitter->SplitVertically(window1, subSplitter);

        // Restore sash position
        rootSplitter->SetSashPosition(currentSashPos);

        // Set appropriate sash position for sub-splitter
        // We want Right panel to be about 250 pixels wide
        wxSize subSize = subSplitter->GetSize();
        if (subSize.GetWidth() > 0) {
            subSplitter->SetSashPosition(subSize.GetWidth() - getConfiguredAreaSize(area));
        }

        // Ensure all windows are shown
        window1->Show();
        window2->Show();
        dockArea->Show();
        subSplitter->Show();

        // Force complete layout update
        rootSplitter->UpdateSize();
        subSplitter->UpdateSize();
        if (wxWindow* parent = rootSplitter->GetParent()) {
            parent->Layout();
            parent->Refresh();
        }

        return;
    }

    // For other cases, use the existing logic
    if (area == TopDockWidgetArea || area == BottomDockWidgetArea) {
        ensureTopBottomLayout(rootSplitter, dockArea, area);
    } else {
        addToMiddleLayer(rootSplitter, dockArea, area);
    }
}

void DockContainerWidget::ensureTopBottomLayout(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::ensureTopBottomLayout - area: %d", area);

    // Check if root is already horizontal (correct for top/bottom)
    if (rootSplitter->GetSplitMode() == wxSPLIT_HORIZONTAL) {
        // Already horizontal, add to appropriate position
        addToHorizontalLayout(rootSplitter, dockArea, area);
    } else {
        // Root is vertical, need to restructure
        restructureForTopBottom(rootSplitter, dockArea, area);
    }
}

void DockContainerWidget::addToMiddleLayer(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::addToMiddleLayer - area: %d", area);

    // Find or create the middle layer
    wxWindow* middleLayer = nullptr;

    if (rootSplitter->GetSplitMode() == wxSPLIT_HORIZONTAL) {
        // Root is horizontal, look for middle layer
        wxWindow* w1 = rootSplitter->GetWindow1();
        wxWindow* w2 = rootSplitter->GetWindow2();

        // Check if either window is a vertical splitter (middle layer)
        if (DockSplitter* s1 = dynamic_cast<DockSplitter*>(w1)) {
            if (s1->GetSplitMode() == wxSPLIT_VERTICAL) {
                middleLayer = w1;
            }
        }
        if (!middleLayer && w2) {
            if (DockSplitter* s2 = dynamic_cast<DockSplitter*>(w2)) {
                if (s2->GetSplitMode() == wxSPLIT_VERTICAL) {
                    middleLayer = w2;
                }
            }
        }

        // If no middle layer found, check if we have a dock area that should be in middle
        if (!middleLayer) {
            if (DockArea* a1 = dynamic_cast<DockArea*>(w1)) {
                // w1 is a dock area, could be the future middle layer
                middleLayer = w1;
            } else if (DockArea* a2 = dynamic_cast<DockArea*>(w2)) {
                // w2 is a dock area, could be the future middle layer
                middleLayer = w2;
            }
        }
    } else {
        // Root is vertical, it IS the middle layer
        middleLayer = rootSplitter;
    }

    if (!middleLayer) {
        wxLogDebug("  -> ERROR: Could not find middle layer");
        return;
    }

    // Add to the middle layer
    if (middleLayer == rootSplitter) {
        // Root splitter is the middle layer
        addToVerticalSplitter(rootSplitter, dockArea, area);
    } else if (DockSplitter* middleSplitter = dynamic_cast<DockSplitter*>(middleLayer)) {
        // Middle layer is a splitter
        addToVerticalSplitter(middleSplitter, dockArea, area);
    } else if (DockArea* existingArea = dynamic_cast<DockArea*>(middleLayer)) {
        // Middle layer is a single dock area, need to create splitter
        createMiddleSplitter(rootSplitter, existingArea, dockArea, area);
    }
}

void DockContainerWidget::addToVerticalSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::addToVerticalSplitter - area: %d", area);

    wxWindow* w1 = splitter->GetWindow1();
    wxWindow* w2 = splitter->GetWindow2();

    if (!w2) {
        // Only one window in splitter
        dockArea->Reparent(splitter);

        if (area == LeftDockWidgetArea) {
            splitter->SplitVertically(dockArea, w1);
            splitter->SetSashPosition(getConfiguredAreaSize(area));
        } else if (area == RightDockWidgetArea) {
            splitter->SplitVertically(w1, dockArea);
            splitter->SetSashPosition(splitter->GetSize().GetWidth() - getConfiguredAreaSize(area));
        } else { // CenterDockWidgetArea
            splitter->SplitVertically(w1, dockArea);
            splitter->SetSashPosition(splitter->GetSize().GetWidth() / 2);
        }
    } else {
        // Both windows exist, need sub-splitter for 3-way split
        create3WaySplit(splitter, dockArea, area);
    }
}

void DockContainerWidget::createMiddleSplitter(DockSplitter* rootSplitter, DockArea* existingArea, DockArea* newArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::createMiddleSplitter - area: %d", area);

    // Create new vertical splitter for middle layer
    DockSplitter* middleSplitter = new DockSplitter(rootSplitter);

    // Replace existing area with new splitter
    rootSplitter->ReplaceWindow(existingArea, middleSplitter);

    // Reparent areas to new splitter
    existingArea->Reparent(middleSplitter);
    newArea->Reparent(middleSplitter);

    // Split based on area type
    if (area == LeftDockWidgetArea) {
        middleSplitter->SplitVertically(newArea, existingArea);
        middleSplitter->SetSashPosition(getConfiguredAreaSize(area));
    } else if (area == RightDockWidgetArea) {
        middleSplitter->SplitVertically(existingArea, newArea);
        middleSplitter->SetSashPosition(middleSplitter->GetSize().GetWidth() - getConfiguredAreaSize(area));
    } else { // CenterDockWidgetArea
        middleSplitter->SplitVertically(existingArea, newArea);
        middleSplitter->SetSashPosition(middleSplitter->GetSize().GetWidth() / 2);
    }
}

void DockContainerWidget::handleTopBottomArea(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::handleTopBottomArea - area: %d", area);

    wxWindow* window1 = rootSplitter->GetWindow1();
    wxWindow* window2 = rootSplitter->GetWindow2();

    // If root splitter is empty or has only one window
    if (!window2) {
        dockArea->Reparent(rootSplitter);
        if (!window1) {
            rootSplitter->Initialize(dockArea);
        } else {
            // Split horizontally for top/bottom
            if (area == TopDockWidgetArea) {
                rootSplitter->SplitHorizontally(dockArea, window1);
            } else {
                rootSplitter->SplitHorizontally(window1, dockArea);
            }
        }
        return;
    }

    // Root splitter has both windows - need to restructure
    // Create a new splitter for the existing content
    DockSplitter* contentSplitter = new DockSplitter(rootSplitter);

    // Move existing windows to the content splitter
    window1->Reparent(contentSplitter);
    if (window2) {
        window2->Reparent(contentSplitter);

        // Determine if the existing split is horizontal or vertical
        if (rootSplitter->GetSplitMode() == wxSPLIT_HORIZONTAL) {
            contentSplitter->SplitHorizontally(window1, window2);
        } else {
            contentSplitter->SplitVertically(window1, window2);
        }

        // Copy the sash position
        contentSplitter->SetSashPosition(rootSplitter->GetSashPosition());
    } else {
        contentSplitter->Initialize(window1);
    }

    // Now restructure the root splitter
    rootSplitter->Unsplit();
    dockArea->Reparent(rootSplitter);

    if (area == TopDockWidgetArea) {
        rootSplitter->SplitHorizontally(dockArea, contentSplitter);
        rootSplitter->SetSashPosition(getConfiguredAreaSize(area));
    } else { // BottomDockWidgetArea
        rootSplitter->SplitHorizontally(contentSplitter, dockArea);
        rootSplitter->SetSashPosition(rootSplitter->GetSize().GetHeight() - getConfiguredAreaSize(area));
    }
}

void DockContainerWidget::handleMiddleLayerArea(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::handleMiddleLayerArea - area: %d", area);

    // For left/center/right areas, we need to find or create the middle layer
    wxWindow* middleLayer = findOrCreateMiddleLayer(rootSplitter);

    if (!middleLayer) {
        wxLogDebug("  -> ERROR: Could not find or create middle layer");
        return;
    }

    // If middle layer is a dock area, we need to create a splitter for it
    if (DockArea* existingArea = dynamic_cast<DockArea*>(middleLayer)) {
        wxLogDebug("  -> Middle layer is a dock area, need to split it");

        // Get the parent splitter
        DockSplitter* parentSplitter = dynamic_cast<DockSplitter*>(middleLayer->GetParent());
        if (!parentSplitter) {
            wxLogDebug("  -> ERROR: Parent is not a splitter");
            return;
        }

        // Create a new splitter to hold the middle layer items
        DockSplitter* newSplitter = new DockSplitter(parentSplitter);

        // Replace the existing area with the new splitter in the parent
        parentSplitter->ReplaceWindow(existingArea, newSplitter);

        // Reparent the existing area to the new splitter
        existingArea->Reparent(newSplitter);
        dockArea->Reparent(newSplitter);

        // Now determine the layout based on what areas we're dealing with
        // The existing area is likely the center, so position accordingly
        if (area == LeftDockWidgetArea) {
            // New area goes on the left
            newSplitter->SplitVertically(dockArea, existingArea);
            newSplitter->SetSashPosition(getConfiguredAreaSize(area)); // Default left width
        } else if (area == RightDockWidgetArea) {
            // New area goes on the right
            newSplitter->SplitVertically(existingArea, dockArea);
            newSplitter->SetSashPosition(newSplitter->GetSize().GetWidth() - getConfiguredAreaSize(area));
        } else { // CenterDockWidgetArea
            // This is tricky - if we're adding center to an existing area,
            // we need to determine which one should be left/right
            newSplitter->SplitVertically(existingArea, dockArea);
            newSplitter->SetSashPosition(newSplitter->GetSize().GetWidth() / 2);
        }
    } else if (DockSplitter* middleSplitter = dynamic_cast<DockSplitter*>(middleLayer)) {
        wxLogDebug("  -> Middle layer is already a splitter");
        // Add to the existing middle splitter using improved logic
        addDockAreaToMiddleSplitter(middleSplitter, dockArea, area);
    }
}

void DockContainerWidget::addDockAreaToMiddleSplitter(DockSplitter* middleSplitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::addDockAreaToMiddleSplitter - area: %d", area);

    wxWindow* window1 = middleSplitter->GetWindow1();
    wxWindow* window2 = middleSplitter->GetWindow2();

    // If the splitter is empty, just add the area
    if (!window1 && !window2) {
        dockArea->Reparent(middleSplitter);
        middleSplitter->Initialize(dockArea);
        return;
    }

    // If only one window exists, split based on the area type
    if (!window2) {
        dockArea->Reparent(middleSplitter);

        // Determine split based on what we're adding and what exists
        if (area == LeftDockWidgetArea) {
            middleSplitter->SplitVertically(dockArea, window1);
            middleSplitter->SetSashPosition(getConfiguredAreaSize(area));
        } else if (area == RightDockWidgetArea) {
            middleSplitter->SplitVertically(window1, dockArea);
            middleSplitter->SetSashPosition(middleSplitter->GetSize().GetWidth() - getConfiguredAreaSize(area));
        } else { // CenterDockWidgetArea
            // If we're adding center and something exists, put center in the middle
            middleSplitter->SplitVertically(window1, dockArea);
            middleSplitter->SetSashPosition(middleSplitter->GetSize().GetWidth() / 2);
        }
        return;
    }

    // Both windows exist - need to create a more complex layout
    // This is the case where we need to handle left-center-right properly
    wxLogDebug("  -> Both windows exist, need to create 3-way split");

    // We need to determine what's already there and add the new area appropriately
    // For now, let's create a sub-splitter
    DockSplitter* subSplitter = new DockSplitter(middleSplitter);

    if (area == LeftDockWidgetArea) {
        // Move everything to the right and add new area on the left
        window1->Reparent(subSplitter);
        window2->Reparent(subSplitter);
        subSplitter->SplitVertically(window1, window2);

        middleSplitter->Unsplit();
        dockArea->Reparent(middleSplitter);
        middleSplitter->SplitVertically(dockArea, subSplitter);
        middleSplitter->SetSashPosition(getConfiguredAreaSize(area));
    } else if (area == RightDockWidgetArea) {
        // Move everything to the left and add new area on the right
        window1->Reparent(subSplitter);
        window2->Reparent(subSplitter);
        subSplitter->SplitVertically(window1, window2);

        middleSplitter->Unsplit();
        dockArea->Reparent(middleSplitter);
        middleSplitter->SplitVertically(subSplitter, dockArea);
        middleSplitter->SetSashPosition(middleSplitter->GetSize().GetWidth() - getConfiguredAreaSize(area));
    } else { // CenterDockWidgetArea
        // Add in the middle - this is the most complex case
        // We'll put the new center between the existing windows
        window2->Reparent(subSplitter);
        dockArea->Reparent(subSplitter);
        subSplitter->SplitVertically(dockArea, window2);

        middleSplitter->ReplaceWindow(window2, subSplitter);
    }
}

void DockContainerWidget::ensureAllChildrenVisible(wxWindow* window) {
    if (!window) return;

    window->Show();

    // If it's a splitter, ensure its children are visible too
    if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(window)) {
        if (splitter->GetWindow1()) {
            ensureAllChildrenVisible(splitter->GetWindow1());
        }
        if (splitter->GetWindow2()) {
            ensureAllChildrenVisible(splitter->GetWindow2());
        }
    }
}

void DockContainerWidget::addDockAreaRelativeTo(DockArea* newArea, DockWidgetArea area, DockArea* targetArea) {
    if (!newArea || !targetArea || !m_rootSplitter) {
        return;
    }

    wxLogDebug("DockContainerWidget::addDockAreaRelativeTo - area: %d, target: %p", area, targetArea);

    // Find the parent splitter of the target area
    wxWindow* parent = targetArea->GetParent();
    DockSplitter* parentSplitter = dynamic_cast<DockSplitter*>(parent);

    if (!parentSplitter) {
        wxLogDebug("  -> Target area has no parent splitter, using general addDockArea");
        addDockArea(newArea, area);
        return;
    }

    // Register the new area
    m_dockAreas.push_back(newArea);
    wxLogDebug("  -> Dock areas count after add: %d", (int)m_dockAreas.size());

    // Determine which window in the splitter is our target
    wxWindow* window1 = parentSplitter->GetWindow1();
    wxWindow* window2 = parentSplitter->GetWindow2();
    bool targetIsWindow1 = (window1 == targetArea);

    // Create a new sub-splitter for the target area and new area
    DockSplitter* subSplitter = new DockSplitter(parentSplitter);

    // First, replace the target area with the sub-splitter in the parent
    // This is important to do before reparenting to avoid wxWidgets assertions
    if (targetIsWindow1) {
        parentSplitter->ReplaceWindow(window1, subSplitter);
    } else {
        parentSplitter->ReplaceWindow(window2, subSplitter);
    }

    // Now reparent the windows to the sub-splitter
    targetArea->Reparent(subSplitter);
    newArea->Reparent(subSplitter);

    // Configure the sub-splitter based on docking direction
    switch (area) {
    case TopDockWidgetArea:
        subSplitter->SplitHorizontally(newArea, targetArea);
        subSplitter->SetSashPosition(getConfiguredAreaSize(area));
        break;
    case BottomDockWidgetArea:
        subSplitter->SplitHorizontally(targetArea, newArea);
        // For bottom docking, we'll set the sash position later after layout
        break;
    case LeftDockWidgetArea:
        subSplitter->SplitVertically(newArea, targetArea);
        subSplitter->SetSashPosition(getConfiguredAreaSize(area));
        break;
    case RightDockWidgetArea:
        subSplitter->SplitVertically(targetArea, newArea);
        // For right docking, we'll set the sash position later after layout
        break;
    default:
        wxLogDebug("  -> Invalid docking area for relative positioning");
        delete subSplitter;
        return;
    }

    // For bottom and right docking, adjust sash position after layout
    if (area == BottomDockWidgetArea || area == RightDockWidgetArea) {
        // Force layout to get correct sizes
        subSplitter->Layout();
        wxSize size = subSplitter->GetSize();

        if (area == BottomDockWidgetArea && size.GetHeight() > 0) {
            subSplitter->SetSashPosition(size.GetHeight() - getConfiguredAreaSize(area));
        } else if (area == RightDockWidgetArea && size.GetWidth() > 0) {
            subSplitter->SetSashPosition(size.GetWidth() - getConfiguredAreaSize(area));
        }
    }

    // Ensure visibility
    newArea->Show();
    targetArea->Show();
    subSplitter->Show();

    // Update layout
    Layout();
    Refresh();

    // Notify about the change
    wxCommandEvent event(EVT_DOCK_AREAS_ADDED);
    event.SetEventObject(this);
    ProcessEvent(event);
}

int DockContainerWidget::getConfiguredAreaSize(DockWidgetArea area) const {
    if (!m_dockManager) {
        // Default sizes
        switch (area) {
        case TopDockWidgetArea: return 150;
        case BottomDockWidgetArea: return 200;
        case LeftDockWidgetArea: return 250;
        case RightDockWidgetArea: return 250;
        default: return 250;
        }
    }

    const DockLayoutConfig& config = m_dockManager->getLayoutConfig();

    if (config.usePercentage) {
        // Calculate from percentage
        wxSize containerSize = GetSize();
        switch (area) {
        case TopDockWidgetArea:
            return containerSize.GetHeight() * config.topAreaPercent / 100;
        case BottomDockWidgetArea:
            return containerSize.GetHeight() * config.bottomAreaPercent / 100;
        case LeftDockWidgetArea:
            return containerSize.GetWidth() * config.leftAreaPercent / 100;
        case RightDockWidgetArea:
            return containerSize.GetWidth() * config.rightAreaPercent / 100;
        default:
            return 250;
        }
    } else {
        // Use pixel values
        switch (area) {
        case TopDockWidgetArea: return config.topAreaHeight;
        case BottomDockWidgetArea: return config.bottomAreaHeight;
        case LeftDockWidgetArea: return config.leftAreaWidth;
        case RightDockWidgetArea: return config.rightAreaWidth;
        default: return 250;
        }
    }
}

wxWindow* DockContainerWidget::findOrCreateMiddleLayer(DockSplitter* rootSplitter) {
    wxWindow* window1 = rootSplitter->GetWindow1();
    wxWindow* window2 = rootSplitter->GetWindow2();

    wxLogDebug("DockContainerWidget::findOrCreateMiddleLayer");
    wxLogDebug("  -> Root splitter mode: %s",
               rootSplitter->GetSplitMode() == wxSPLIT_HORIZONTAL ? "HORIZONTAL" : "VERTICAL");
    wxLogDebug("  -> Window1: %p, Window2: %p", window1, window2);

    // If root splitter is empty, return the splitter itself as the middle layer
    if (!window1 && !window2) {
        wxLogDebug("  -> Root splitter is empty, returning root");
        return rootSplitter;
    }

    // If only one window exists
    if (!window2) {
        wxLogDebug("  -> Only window1 exists");
        // If the root is split horizontally and only has one window,
        // that window is the middle layer
        if (rootSplitter->GetSplitMode() == wxSPLIT_HORIZONTAL ||
            rootSplitter->IsSplit() == false) {
            return window1;
        }
        return rootSplitter;
    }

    // Both windows exist
    if (rootSplitter->GetSplitMode() == wxSPLIT_HORIZONTAL) {
        wxLogDebug("  -> Root is split horizontally (top/bottom layout)");

        // In a proper 5-zone layout with horizontal split at root,
        // we need to find which window is the middle layer

        // Check if window1 is a splitter (could be the middle layer)
        if (DockSplitter* splitter1 = dynamic_cast<DockSplitter*>(window1)) {
            if (splitter1->GetSplitMode() == wxSPLIT_VERTICAL) {
                wxLogDebug("  -> Window1 is a vertical splitter - likely middle layer");
                return window1;
            }
        }

        // Check if window2 is a splitter (could be the middle layer)
        if (DockSplitter* splitter2 = dynamic_cast<DockSplitter*>(window2)) {
            if (splitter2->GetSplitMode() == wxSPLIT_VERTICAL) {
                wxLogDebug("  -> Window2 is a vertical splitter - likely middle layer");
                return window2;
            }
        }

        // Neither is a vertical splitter, so we need to create the middle layer
        // This happens when we have top and bottom but no middle yet
        wxLogDebug("  -> No middle layer found, need to create one");

        // We'll return window2 as the position for the middle layer
        // The caller will need to handle restructuring
        return window2;
    } else {
        wxLogDebug("  -> Root is split vertically");
        // If root is already split vertically, it IS the middle layer
        return rootSplitter;
    }
}

void DockContainerWidget::create3WaySplit(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::create3WaySplit - area: %d", area);

    wxWindow* w1 = splitter->GetWindow1();
    wxWindow* w2 = splitter->GetWindow2();

    // Create a sub-splitter to hold two of the three areas
    DockSplitter* subSplitter = new DockSplitter(splitter);

    if (area == LeftDockWidgetArea) {
        // New area goes left, existing windows go to sub-splitter on right
        w1->Reparent(subSplitter);
        w2->Reparent(subSplitter);
        subSplitter->SplitVertically(w1, w2);

        splitter->Unsplit();
        dockArea->Reparent(splitter);
        splitter->SplitVertically(dockArea, subSplitter);
        splitter->SetSashPosition(getConfiguredAreaSize(area));
    } else if (area == RightDockWidgetArea) {
        // New area goes right, existing windows go to sub-splitter on left
        w1->Reparent(subSplitter);
        w2->Reparent(subSplitter);
        subSplitter->SplitVertically(w1, w2);

        splitter->Unsplit();
        dockArea->Reparent(splitter);
        splitter->SplitVertically(subSplitter, dockArea);
        splitter->SetSashPosition(splitter->GetSize().GetWidth() - getConfiguredAreaSize(area));
    } else { // CenterDockWidgetArea
        // New area goes in middle
        w2->Reparent(subSplitter);
        dockArea->Reparent(subSplitter);
        subSplitter->SplitVertically(dockArea, w2);

        splitter->ReplaceWindow(w2, subSplitter);
    }
}

void DockContainerWidget::addToHorizontalLayout(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::addToHorizontalLayout - area: %d", area);

    // TODO: Implement adding to existing horizontal layout
    // For now, just add using the existing logic
    addDockAreaToSplitter(splitter, dockArea, area);
}

void DockContainerWidget::restructureForTopBottom(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::restructureForTopBottom - area: %d", area);

    wxWindow* w1 = rootSplitter->GetWindow1();
    wxWindow* w2 = rootSplitter->GetWindow2();

    // Debug what we're restructuring
    wxLogDebug("  -> w1: %p, w2: %p", w1, w2);
    if (DockSplitter* split2 = dynamic_cast<DockSplitter*>(w2)) {
        wxLogDebug("  -> w2 is a splitter with children: %p, %p",
                   split2->GetWindow1(), split2->GetWindow2());
    }

    // Store current sash position
    int sashPos = rootSplitter->GetSashPosition();

    // Create a new splitter to hold the existing vertical content
    DockSplitter* middleSplitter = new DockSplitter(rootSplitter);

    // First unsplit the root
    rootSplitter->Unsplit();

    // Move existing windows to middle splitter
    w1->Reparent(middleSplitter);
    if (w2) {
        w2->Reparent(middleSplitter);
        middleSplitter->SplitVertically(w1, w2);
        middleSplitter->SetSashPosition(sashPos);

        // IMPORTANT: Ensure sub-windows are visible
        w1->Show();
        w2->Show();

        // If w2 is a splitter, ensure its children are visible too
        if (DockSplitter* subSplitter = dynamic_cast<DockSplitter*>(w2)) {
            if (subSplitter->GetWindow1()) subSplitter->GetWindow1()->Show();
            if (subSplitter->GetWindow2()) subSplitter->GetWindow2()->Show();
            subSplitter->Show();
        }
    } else {
        middleSplitter->Initialize(w1);
        w1->Show();
    }

    // Show the middle splitter
    middleSplitter->Show();

    // Reparent new area to root
    dockArea->Reparent(rootSplitter);
    dockArea->Show();

    // Set up the root splitter
    if (area == TopDockWidgetArea) {
        rootSplitter->SplitHorizontally(dockArea, middleSplitter);
        rootSplitter->SetSashPosition(getConfiguredAreaSize(area));
    } else { // BottomDockWidgetArea
        rootSplitter->SplitHorizontally(middleSplitter, dockArea);
        rootSplitter->SetSashPosition(rootSplitter->GetSize().GetHeight() - getConfiguredAreaSize(area));
    }

    // Force layout update
    middleSplitter->UpdateSize();
    rootSplitter->UpdateSize();

    // Force parent layout update
    if (wxWindow* parent = rootSplitter->GetParent()) {
        parent->Layout();
        parent->Refresh();
    }

    wxLogDebug("  -> Restructure complete");
}

void DockContainerWidget::addDockAreaToSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area) {
    if (!splitter || !dockArea) {
        return;
    }

    wxLogDebug("DockContainerWidget::addDockAreaToSplitter - area: %d", area);

    if (splitter->GetWindow1() == nullptr) {
        wxLogDebug("  -> Window1 is null, initializing");
        dockArea->Reparent(splitter);
        splitter->Initialize(dockArea);
    } else if (splitter->GetWindow2() == nullptr) {
        wxLogDebug("  -> Window2 is null, splitting");
        dockArea->Reparent(splitter);
        // Ensure minimum sizes
        splitter->GetWindow1()->SetMinSize(wxSize(100, 100));
        dockArea->SetMinSize(wxSize(100, 100));

        if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
            if (area == LeftDockWidgetArea) {
                splitter->SplitVertically(dockArea, splitter->GetWindow1());
            } else { // RightDockWidgetArea
                splitter->SplitVertically(splitter->GetWindow1(), dockArea);
            }
        } else if (area == TopDockWidgetArea || area == BottomDockWidgetArea) {
            if (area == TopDockWidgetArea) {
                splitter->SplitHorizontally(dockArea, splitter->GetWindow1());
            } else { // BottomDockWidgetArea
                splitter->SplitHorizontally(splitter->GetWindow1(), dockArea);
            }
        } else {
            // Default case
            splitter->SplitHorizontally(splitter->GetWindow1(), dockArea);
        }
    } else {
        wxLogDebug("  -> Both windows occupied, creating sub-splitter");
        // Both windows occupied, same logic as in addDockArea
        wxWindow* targetWindow = nullptr;

                 // Choose target window based on desired area position
         if (area == LeftDockWidgetArea) {
             targetWindow = splitter->GetWindow1();
         } else if (area == RightDockWidgetArea) {
             targetWindow = splitter->GetWindow2();
         } else if (area == TopDockWidgetArea) {
             targetWindow = splitter->GetWindow1();
         } else if (area == BottomDockWidgetArea) {
             targetWindow = splitter->GetWindow2();
         } else {
             // Default to window2 for unknown areas (CenterDockWidgetArea)
             targetWindow = splitter->GetWindow2();
         }

        if (targetWindow) {
            DockSplitter* newSplitter = new DockSplitter(splitter);
            targetWindow->Reparent(newSplitter);
            splitter->ReplaceWindow(targetWindow, newSplitter);
            dockArea->Reparent(newSplitter);

            // Ensure minimum size for both windows
            dockArea->SetMinSize(wxSize(100, 100));
            targetWindow->SetMinSize(wxSize(100, 100));

            // Split based on area - ensure proper positioning
            if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
                if (area == LeftDockWidgetArea) {
                    newSplitter->SplitVertically(dockArea, targetWindow);
                } else { // RightDockWidgetArea
                    newSplitter->SplitVertically(targetWindow, dockArea);
                }
            } else if (area == TopDockWidgetArea || area == BottomDockWidgetArea) {
                if (area == TopDockWidgetArea) {
                    newSplitter->SplitHorizontally(dockArea, targetWindow);
                } else { // BottomDockWidgetArea
                    newSplitter->SplitHorizontally(targetWindow, dockArea);
                }
            } else {
                // Default to horizontal split for CenterDockWidgetArea
                newSplitter->SplitHorizontally(targetWindow, dockArea);
            }

            // Set splitter properties
            newSplitter->SetSashGravity(0.5);
            newSplitter->SetMinimumPaneSize(50);
        }
    }
}

void DockContainerWidget::applyLayoutConfig() {
    if (!m_dockManager || !m_rootSplitter) {
        return;
    }

    const DockLayoutConfig& config = m_dockManager->getLayoutConfig();
    if (!config.usePercentage) {
        return; // Only apply for percentage-based layouts
    }

    // Get the current size of the container
    wxSize containerSize = GetSize();
    if (containerSize.GetWidth() <= 0 || containerSize.GetHeight() <= 0) {
        return; // Invalid size
    }

    // Cache check to avoid unnecessary recalculations
    static wxSize lastContainerSize;
    static DockLayoutConfig lastConfig;
    static bool lastConfigValid = false;
    
    if (lastConfigValid && 
        lastContainerSize == containerSize && 
        lastConfig == config) {
        return; // No changes, skip recalculation
    }
    
    // Update cache
    lastContainerSize = containerSize;
    lastConfig = config;
    lastConfigValid = true;

    // Apply the configuration to all splitters
    DockSplitter* rootSplitter = dynamic_cast<DockSplitter*>(m_rootSplitter);
    if (!rootSplitter || !rootSplitter->IsSplit()) {
        return;
    }

    // Update root splitter position based on its orientation
    if (rootSplitter->GetSplitMode() == wxSPLIT_HORIZONTAL) {
        // Horizontal split - check if it's top/bottom layout
        wxWindow* w1 = rootSplitter->GetWindow1();
        wxWindow* w2 = rootSplitter->GetWindow2();

        // Determine if window1 is top or bottom area
        bool isTopBottomLayout = false;
        for (auto* area : m_dockAreas) {
            if (area == w1 || dynamic_cast<DockSplitter*>(w1) != nullptr) {
                isTopBottomLayout = true;
                break;
            }
        }

        if (isTopBottomLayout && config.showTopArea) {
            int topHeight = containerSize.GetHeight() * config.topAreaPercent / 100;
            rootSplitter->SetSashPosition(topHeight);
        } else if (config.showBottomArea) {
            int bottomHeight = containerSize.GetHeight() * config.bottomAreaPercent / 100;
            rootSplitter->SetSashPosition(containerSize.GetHeight() - bottomHeight);
        }
    } else {
        // Vertical split - check for left/right layout
        wxWindow* w1 = rootSplitter->GetWindow1();
        wxWindow* w2 = rootSplitter->GetWindow2();

        // Check if we have a sub-splitter for center+right
        if (DockSplitter* subSplitter = dynamic_cast<DockSplitter*>(w2)) {
            // This is likely [Left | [Center | Right]] layout
            if (config.showLeftArea) {
                int leftWidth = containerSize.GetWidth() * config.leftAreaPercent / 100;
                rootSplitter->SetSashPosition(leftWidth);
            }

            // Update sub-splitter for right area
            if (subSplitter->IsSplit() && config.showRightArea) {
                wxSize subSize = subSplitter->GetSize();
                if (subSize.GetWidth() > 0) {
                    int rightWidth = subSize.GetWidth() * config.rightAreaPercent /
                                   (100 - config.leftAreaPercent) * 100;
                    subSplitter->SetSashPosition(subSize.GetWidth() - rightWidth);
                }
            }
        } else {
            // Simple left/right split
            if (config.showLeftArea) {
                int leftWidth = containerSize.GetWidth() * config.leftAreaPercent / 100;
                rootSplitter->SetSashPosition(leftWidth);
            } else if (config.showRightArea) {
                int rightWidth = containerSize.GetWidth() * config.rightAreaPercent / 100;
                rootSplitter->SetSashPosition(containerSize.GetWidth() - rightWidth);
            }
        }
    }

    // Use deferred layout update to avoid excessive refreshes
    if (!m_layoutUpdateTimer) {
        m_layoutUpdateTimer = new wxTimer(this);
        Bind(wxEVT_TIMER, &DockContainerWidget::onLayoutUpdateTimer, this, m_layoutUpdateTimer->GetId());
    }

    // Cancel any pending layout update
    if (m_layoutUpdateTimer->IsRunning()) {
        m_layoutUpdateTimer->Stop();
    }

    // Schedule layout update with debounce delay
    m_layoutUpdateTimer->Start(8, wxTIMER_ONE_SHOT); // ~120fps debounce
}

void DockContainerWidget::applyProportionalResize(const wxSize& oldSize, const wxSize& newSize) {
    if (oldSize.GetWidth() <= 0 || oldSize.GetHeight() <= 0 || 
        newSize.GetWidth() <= 0 || newSize.GetHeight() <= 0) {
        return;
    }
    
    // Calculate scale factors
    double scaleX = static_cast<double>(newSize.GetWidth()) / oldSize.GetWidth();
    double scaleY = static_cast<double>(newSize.GetHeight()) / oldSize.GetHeight();
    
    // Apply proportional scaling to all cached splitter ratios
    for (auto& ratio : m_splitterRatios) {
        if (!ratio.isValid || !ratio.splitter) {
            continue;
        }
        
        DockSplitter* splitter = dynamic_cast<DockSplitter*>(ratio.splitter);
        if (!splitter || !splitter->IsSplit()) {
            continue;
        }
        
        // Calculate new position based on scale factor
        wxSize splitterSize = splitter->GetSize();
        int newPosition;
        
        if (splitter->GetSplitMode() == wxSPLIT_VERTICAL) {
            // Vertical splitter - scale horizontally
            newPosition = static_cast<int>(ratio.ratio * splitterSize.GetWidth());
        } else {
            // Horizontal splitter - scale vertically
            newPosition = static_cast<int>(ratio.ratio * splitterSize.GetHeight());
        }
        
        // Ensure position is within valid range
        int minSize = splitter->GetMinimumPaneSize();
        int maxPosition = (splitter->GetSplitMode() == wxSPLIT_VERTICAL) ? 
                         splitterSize.GetWidth() - minSize : 
                         splitterSize.GetHeight() - minSize;
        
        newPosition = std::max(minSize, std::min(newPosition, maxPosition));
        
        // Apply new position
        splitter->SetSashPosition(newPosition);
    }
    
    // Update layout without full refresh
    Layout();
    
    // Use RefreshRect for better performance
    wxRect dirtyRect = GetClientRect();
    RefreshRect(dirtyRect, false);
}

void DockContainerWidget::cacheSplitterRatios() {
    m_splitterRatios.clear();
    
    // Recursively collect all splitters and their ratios
    collectSplitterRatios(m_rootSplitter);
    
    wxLogDebug("DockContainerWidget::cacheSplitterRatios - cached %d splitter ratios", 
               static_cast<int>(m_splitterRatios.size()));
}

void DockContainerWidget::collectSplitterRatios(wxWindow* window) {
    if (!window) {
        return;
    }
    
    DockSplitter* splitter = dynamic_cast<DockSplitter*>(window);
    if (splitter && splitter->IsSplit()) {
        SplitterRatio ratio;
        ratio.splitter = splitter;
        ratio.isValid = true;
        
        // Calculate current ratio
        wxSize splitterSize = splitter->GetSize();
        int sashPosition = splitter->GetSashPosition();
        
        if (splitter->GetSplitMode() == wxSPLIT_VERTICAL) {
            // Vertical splitter
            if (splitterSize.GetWidth() > 0) {
                ratio.ratio = static_cast<double>(sashPosition) / splitterSize.GetWidth();
            } else {
                ratio.ratio = 0.5; // Default to center
            }
        } else {
            // Horizontal splitter
            if (splitterSize.GetHeight() > 0) {
                ratio.ratio = static_cast<double>(sashPosition) / splitterSize.GetHeight();
            } else {
                ratio.ratio = 0.5; // Default to center
            }
        }
        
        // Ensure ratio is within valid range
        ratio.ratio = std::max(0.1, std::min(0.9, ratio.ratio));
        
        m_splitterRatios.push_back(ratio);
        
        wxLogDebug("Cached splitter ratio: %.3f (position: %d, size: %dx%d)", 
                   ratio.ratio, sashPosition, splitterSize.GetWidth(), splitterSize.GetHeight());
    }
    
    // Recursively process child windows
    wxWindowList& children = window->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        collectSplitterRatios(*it);
    }
}

void DockContainerWidget::restoreSplitterRatios() {
    // This function can be used to restore ratios after layout changes
    // For now, we'll just recache the current ratios
    cacheSplitterRatios();
}

void DockContainerWidget::markUserAdjustedLayout() {
    m_hasUserAdjustedLayout = true;
    
    // Cache current splitter ratios when user adjusts layout
    cacheSplitterRatios();
    
    wxLogDebug("DockContainerWidget::markUserAdjustedLayout - User adjusted layout, cached ratios");
}

} // namespace ads