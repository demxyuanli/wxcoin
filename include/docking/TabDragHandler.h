#pragma once

#include <wx/wx.h>
#include "DockManager.h"

namespace ads {

class DockArea;
class DockAreaMergedTitleBar;
class DockWidget;
class DockManager;
class FloatingDragPreview;
class DockOverlay;

class TabDragHandler {
public:
    TabDragHandler(DockAreaMergedTitleBar* titleBar);
    virtual ~TabDragHandler();

    bool handleMouseDown(wxMouseEvent& event, int tabIndex);
    void handleMouseUp(wxMouseEvent& event);
    void handleMouseMove(wxMouseEvent& event);
    void cancelDrag();

    bool isDragging() const { return m_dragStarted && m_draggedTab >= 0; }
    bool hasDraggedTab() const { return m_draggedTab >= 0; }
    int draggedTabIndex() const { return m_draggedTab; }

private:
    DockAreaMergedTitleBar* m_titleBar;
    int m_draggedTab;
    wxPoint m_dragStartPos;
    bool m_dragStarted;
    FloatingDragPreview* m_dragPreview;
    
    DockArea* getDockArea() const;

    void startDrag(int tabIndex, const wxPoint& startPos);
    void updateDrag(const wxPoint& currentPos);
    void finishDrag(const wxPoint& dropPos, bool cancelled);
    void handleDrop(const wxPoint& screenPos);
    DockArea* findTargetArea(const wxPoint& screenPos);
    wxWindow* findTargetWindowUnderMouse(const wxPoint& screenPos, wxWindow* dragPreview);
    void showOverlayForTarget(DockArea* targetArea, const wxPoint& screenPos);
    void hideAllOverlays();
};

} // namespace ads

