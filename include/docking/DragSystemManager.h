#pragma once

#include <wx/wx.h>
#include <vector>
#include "DockManager.h"

namespace ads {

class DockManager;
class DockWidget;
class DockArea;
class DockOverlay;

struct DragContext {
    DockWidget* draggedWidget = nullptr;
    wxLongLong startTime = 0;
    wxLongLong lastUpdateTime = 0;
    double dragDistance = 0.0;
    double dragVelocity = 0.0;
    bool isGlobalDocking = false;
    DockArea* lastTargetArea = nullptr;

    DragContext() = default;
};

enum class DragState {
    Inactive,
    Started,
    Active,
    Ending
};

class DragSystemManager {
public:
    DragSystemManager(DockManager* manager);
    virtual ~DragSystemManager();

    void optimizeDragOperation(DockWidget* draggedWidget);
    void startDragOperation(DockWidget* draggedWidget);
    void updateDragOperation(DockWidget* draggedWidget);
    void finishDragOperation(DockWidget* draggedWidget, bool cancelled = false);

    void checkGlobalDockingConditions();
    void enableGlobalDocking();
    void disableGlobalDocking();
    void showInitialDragHints(DockWidget* draggedWidget);
    void updateOverlayHints();
    void updateLocalDockingHints(const wxPoint& mousePos);
    void hideAllOverlays();
    void setOptimizedRendering(bool enabled);
    void updateDragTargets();
    void collectDropTargets(wxWindow* window);

    DragState getDragState() const { return m_dragState; }
    bool isProcessingDrag() const { return m_isProcessingDrag; }

private:
    DockManager* m_manager;
    DragState m_dragState;
    DragContext m_dragContext;
    bool m_isProcessingDrag;
    wxPoint m_lastMousePos;
    std::vector<wxWindow*> m_cachedDropTargets;
    DockOverlay* m_dockAreaOverlay;
    DockOverlay* m_containerOverlay;
    wxWindow* m_containerWidget;
};

} // namespace ads

