#pragma once

#include <wx/wx.h>

namespace ads {

class DockWidget;
class DockArea;
class DockManager;

class IDragHandler {
public:
    virtual ~IDragHandler() = default;

    virtual bool handleMouseDown(wxMouseEvent& event, int tabIndex) = 0;
    virtual void handleMouseUp(wxMouseEvent& event) = 0;
    virtual void handleMouseMove(wxMouseEvent& event) = 0;
    virtual void cancelDrag() = 0;

    virtual bool isDragging() const = 0;
    virtual bool hasDraggedTab() const = 0;
    virtual int draggedTabIndex() const = 0;

    virtual DockArea* getDockArea() const = 0;
    virtual DockWidget* getDraggedWidget() const = 0;
};

class DragHandlerBase : public IDragHandler {
public:
    DragHandlerBase();
    virtual ~DragHandlerBase();

    bool isDragging() const override { return m_dragStarted && m_draggedTab >= 0; }
    bool hasDraggedTab() const override { return m_draggedTab >= 0; }
    int draggedTabIndex() const override { return m_draggedTab; }

protected:
    int m_draggedTab;
    wxPoint m_dragStartPos;
    bool m_dragStarted;
    wxPoint m_currentDragPos;

    bool shouldStartDrag(const wxPoint& currentPos, const wxPoint& startPos, bool draggingFlag) const;
    void startDragOperation(int tabIndex, const wxPoint& startPos);
    void updateDragPosition(const wxPoint& pos);
    void finishDragOperation();
};

} // namespace ads

