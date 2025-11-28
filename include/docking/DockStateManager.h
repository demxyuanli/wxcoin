#pragma once

#include <wx/wx.h>
#include <vector>
#include <map>
#include <memory>

namespace ads {

class DockWidget;
class DockArea;

enum class DockStateType {
    Hover,
    Selection,
    Drag,
    Focus,
    Lock
};

struct DockState {
    DockStateType type;
    int tabIndex = -1;
    bool active = false;
    wxPoint position;
    wxRect rect;
    
    DockState() = default;
    DockState(DockStateType t, int idx, bool act = false) 
        : type(t), tabIndex(idx), active(act) {}
};

class DockStateManager {
public:
    DockStateManager();
    virtual ~DockStateManager();

    void setHoverState(int tabIndex, bool active);
    void setSelectionState(int tabIndex, bool active);
    void setDragState(int tabIndex, bool active, const wxPoint& position = wxPoint());
    void setFocusState(int tabIndex, bool active);
    void setLockState(int tabIndex, bool active);

    bool isHovered(int tabIndex) const;
    bool isSelected(int tabIndex) const;
    bool isDragging(int tabIndex) const;
    bool isFocused(int tabIndex) const;
    bool isLocked(int tabIndex) const;

    int hoveredTab() const { return m_hoveredTab; }
    int selectedTab() const { return m_selectedTab; }
    int draggedTab() const { return m_draggedTab; }
    int focusedTab() const { return m_focusedTab; }

    void clearHoverState();
    void clearSelectionState();
    void clearDragState();
    void clearFocusState();
    void clearAllStates();

    wxPoint dragPosition() const { return m_dragPosition; }
    void setDragPosition(const wxPoint& pos) { m_dragPosition = pos; }

    std::vector<DockState> getAllStates() const;
    DockState getState(DockStateType type, int tabIndex) const;

private:
    int m_hoveredTab;
    int m_selectedTab;
    int m_draggedTab;
    int m_focusedTab;
    wxPoint m_dragPosition;
    
    std::map<int, bool> m_lockedTabs;
    
    void updateState(DockStateType type, int tabIndex, bool active);
};

} // namespace ads

