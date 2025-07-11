#include "flatui/FlatUIBarStateManager.h"
#include "logger/Logger.h"

FlatUIBarStateManager::FlatUIBarStateManager()
    : m_currentState(BarState::PINNED),
      m_activePage(0),
      m_activeFloatingPage(INVALID_PAGE),
      m_lastPinnedActivePage(0)
{
    LOG_INF("FlatUIBarStateManager initialized in PINNED state", "StateManager");
}

void FlatUIBarStateManager::TransitionTo(BarState newState)
{
    if (m_currentState == newState) {
        return; // No change needed
    }
    
    BarState oldState = m_currentState;
    
    if (newState == BarState::UNPINNED) {
        SaveActivePageBeforeUnpin();
        ResetFloatingState();
        LOG_INF("State transition: PINNED -> UNPINNED", "StateManager");
    } else {
        RestoreActivePageAfterPin();
        ResetFloatingState();
        LOG_INF("State transition: UNPINNED -> PINNED", "StateManager");
    }
    
    m_currentState = newState;
}

void FlatUIBarStateManager::SetActivePage(size_t pageIndex)
{
    if (m_currentState == BarState::PINNED) {
        m_activePage = pageIndex;
        LOG_INF("Active page set to " + std::to_string(pageIndex) + " in PINNED state", "StateManager");
    } else {
        // In unpinned state, we might want to track both floating and future pinned page
        m_activePage = pageIndex; // Keep this for when we pin again
        LOG_INF("Active page set to " + std::to_string(pageIndex) + " for future PINNED state", "StateManager");
    }
}

size_t FlatUIBarStateManager::GetActivePage() const
{
    return m_activePage;
}

void FlatUIBarStateManager::SaveActivePageBeforeUnpin()
{
    m_lastPinnedActivePage = m_activePage;
}

void FlatUIBarStateManager::RestoreActivePageAfterPin()
{
    m_activePage = m_lastPinnedActivePage;
}

bool FlatUIBarStateManager::IsValidPageIndex(size_t index, size_t totalPages) const
{
    return index < totalPages;
}

bool FlatUIBarStateManager::ShouldShowFixedPanels() const
{
    return m_currentState == BarState::PINNED;
}

bool FlatUIBarStateManager::ShouldShowFloatPanels() const
{
    return m_currentState == BarState::UNPINNED;
}

bool FlatUIBarStateManager::HasValidActivePage(size_t totalPages) const
{
    return totalPages > 0 && IsValidPageIndex(m_activePage, totalPages);
}

void FlatUIBarStateManager::ResetFloatingState()
{
    m_activeFloatingPage = INVALID_PAGE;
}

void FlatUIBarStateManager::ValidateActivePageIndex(size_t totalPages)
{
    if (totalPages == 0) {
        m_activePage = INVALID_PAGE;
        m_activeFloatingPage = INVALID_PAGE;
        return;
    }
    
    if (m_activePage >= totalPages) {
        m_activePage = 0; // Fallback to first page
    }
    
    if (m_activeFloatingPage >= totalPages) {
        m_activeFloatingPage = INVALID_PAGE;
    }
} 
