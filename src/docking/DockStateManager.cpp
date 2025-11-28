#include "docking/DockStateManager.h"
#include <algorithm>

namespace ads {

DockStateManager::DockStateManager()
    : m_hoveredTab(-1)
    , m_selectedTab(-1)
    , m_draggedTab(-1)
    , m_focusedTab(-1)
{
}

DockStateManager::~DockStateManager() {
}

void DockStateManager::setHoverState(int tabIndex, bool active) {
    if (active) {
        m_hoveredTab = tabIndex;
    } else if (m_hoveredTab == tabIndex) {
        m_hoveredTab = -1;
    }
}

void DockStateManager::setSelectionState(int tabIndex, bool active) {
    if (active) {
        m_selectedTab = tabIndex;
    } else if (m_selectedTab == tabIndex) {
        m_selectedTab = -1;
    }
}

void DockStateManager::setDragState(int tabIndex, bool active, const wxPoint& position) {
    if (active) {
        m_draggedTab = tabIndex;
        m_dragPosition = position;
    } else if (m_draggedTab == tabIndex) {
        m_draggedTab = -1;
        m_dragPosition = wxPoint();
    }
}

void DockStateManager::setFocusState(int tabIndex, bool active) {
    if (active) {
        m_focusedTab = tabIndex;
    } else if (m_focusedTab == tabIndex) {
        m_focusedTab = -1;
    }
}

void DockStateManager::setLockState(int tabIndex, bool active) {
    if (active) {
        m_lockedTabs[tabIndex] = true;
    } else {
        m_lockedTabs.erase(tabIndex);
    }
}

bool DockStateManager::isHovered(int tabIndex) const {
    return m_hoveredTab == tabIndex;
}

bool DockStateManager::isSelected(int tabIndex) const {
    return m_selectedTab == tabIndex;
}

bool DockStateManager::isDragging(int tabIndex) const {
    return m_draggedTab == tabIndex;
}

bool DockStateManager::isFocused(int tabIndex) const {
    return m_focusedTab == tabIndex;
}

bool DockStateManager::isLocked(int tabIndex) const {
    return m_lockedTabs.find(tabIndex) != m_lockedTabs.end();
}

void DockStateManager::clearHoverState() {
    m_hoveredTab = -1;
}

void DockStateManager::clearSelectionState() {
    m_selectedTab = -1;
}

void DockStateManager::clearDragState() {
    m_draggedTab = -1;
    m_dragPosition = wxPoint();
}

void DockStateManager::clearFocusState() {
    m_focusedTab = -1;
}

void DockStateManager::clearAllStates() {
    clearHoverState();
    clearSelectionState();
    clearDragState();
    clearFocusState();
    m_lockedTabs.clear();
}

std::vector<DockState> DockStateManager::getAllStates() const {
    std::vector<DockState> states;
    
    if (m_hoveredTab >= 0) {
        states.push_back(DockState(DockStateType::Hover, m_hoveredTab, true));
    }
    if (m_selectedTab >= 0) {
        states.push_back(DockState(DockStateType::Selection, m_selectedTab, true));
    }
    if (m_draggedTab >= 0) {
        DockState dragState(DockStateType::Drag, m_draggedTab, true);
        dragState.position = m_dragPosition;
        states.push_back(dragState);
    }
    if (m_focusedTab >= 0) {
        states.push_back(DockState(DockStateType::Focus, m_focusedTab, true));
    }
    
    for (const auto& pair : m_lockedTabs) {
        if (pair.second) {
            states.push_back(DockState(DockStateType::Lock, pair.first, true));
        }
    }
    
    return states;
}

DockState DockStateManager::getState(DockStateType type, int tabIndex) const {
    DockState state;
    state.type = type;
    state.tabIndex = tabIndex;
    
    switch (type) {
        case DockStateType::Hover:
            state.active = (m_hoveredTab == tabIndex);
            break;
        case DockStateType::Selection:
            state.active = (m_selectedTab == tabIndex);
            break;
        case DockStateType::Drag:
            state.active = (m_draggedTab == tabIndex);
            state.position = m_dragPosition;
            break;
        case DockStateType::Focus:
            state.active = (m_focusedTab == tabIndex);
            break;
        case DockStateType::Lock:
            state.active = isLocked(tabIndex);
            break;
    }
    
    return state;
}

void DockStateManager::updateState(DockStateType type, int tabIndex, bool active) {
    switch (type) {
        case DockStateType::Hover:
            setHoverState(tabIndex, active);
            break;
        case DockStateType::Selection:
            setSelectionState(tabIndex, active);
            break;
        case DockStateType::Drag:
            setDragState(tabIndex, active);
            break;
        case DockStateType::Focus:
            setFocusState(tabIndex, active);
            break;
        case DockStateType::Lock:
            setLockState(tabIndex, active);
            break;
    }
}

} // namespace ads

