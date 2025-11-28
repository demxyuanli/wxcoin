#include "docking/IDragHandler.h"

namespace ads {

DragHandlerBase::DragHandlerBase()
    : m_draggedTab(-1)
    , m_dragStarted(false)
{
}

DragHandlerBase::~DragHandlerBase() {
}

bool DragHandlerBase::shouldStartDrag(const wxPoint& currentPos, const wxPoint& startPos, bool draggingFlag) const {
    if (draggingFlag) {
        return true;
    }
    
    wxPoint delta = currentPos - startPos;
    const int dragThreshold = 10;
    return (abs(delta.x) > dragThreshold || abs(delta.y) > dragThreshold);
}

void DragHandlerBase::startDragOperation(int tabIndex, const wxPoint& startPos) {
    m_draggedTab = tabIndex;
    m_dragStartPos = startPos;
    m_currentDragPos = startPos;
    m_dragStarted = false;
}

void DragHandlerBase::updateDragPosition(const wxPoint& pos) {
    m_currentDragPos = pos;
    if (!m_dragStarted && shouldStartDrag(pos, m_dragStartPos, false)) {
        m_dragStarted = true;
    }
}

void DragHandlerBase::finishDragOperation() {
    m_draggedTab = -1;
    m_dragStarted = false;
    m_dragStartPos = wxPoint();
    m_currentDragPos = wxPoint();
}

} // namespace ads

