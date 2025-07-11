#ifndef FLATUIBAR_STATE_MANAGER_H
#define FLATUIBAR_STATE_MANAGER_H

#include <cstddef>

class FlatUIBarStateManager {
public:
    enum class BarState { 
        PINNED,     // All content visible, fixed panels shown
        UNPINNED    // Only bar space visible, float panels for content
    };
    
    FlatUIBarStateManager();
    ~FlatUIBarStateManager() = default;
    
    // State transitions
    void TransitionTo(BarState newState);
    BarState GetCurrentState() const { return m_currentState; }
    bool IsPinned() const { return m_currentState == BarState::PINNED; }
    bool IsUnpinned() const { return m_currentState == BarState::UNPINNED; }
    
    // Page management
    void SetActivePage(size_t pageIndex);
    size_t GetActivePage() const;
    size_t GetActiveFloatingPage() const { return m_activeFloatingPage; }
    void SetActiveFloatingPage(size_t pageIndex) { m_activeFloatingPage = pageIndex; }
    
    // State preservation
    void SaveActivePageBeforeUnpin();
    void RestoreActivePageAfterPin();
    
    // Validation
    bool IsValidPageIndex(size_t index, size_t totalPages) const;
    
    // State queries
    bool ShouldShowFixedPanels() const;
    bool ShouldShowFloatPanels() const;
    bool HasValidActivePage(size_t totalPages) const;
    
private:
    BarState m_currentState;
    size_t m_activePage;
    size_t m_activeFloatingPage;
    size_t m_lastPinnedActivePage;
    
    static constexpr size_t INVALID_PAGE = static_cast<size_t>(-1);
    
    // Helper methods
    void ResetFloatingState();
    void ValidateActivePageIndex(size_t totalPages);
};

#endif // FLATUIBAR_STATE_MANAGER_H 