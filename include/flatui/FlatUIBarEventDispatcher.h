#ifndef FLATUIBAR_EVENT_DISPATCHER_H
#define FLATUIBAR_EVENT_DISPATCHER_H

#include <wx/wx.h>

// Forward declarations
class FlatUIBar;
class FlatUIBarStateManager;
class FlatUIPageManager;
class FlatUIBarLayoutManager;

class FlatUIBarEventDispatcher {
public:
    explicit FlatUIBarEventDispatcher(FlatUIBar* bar);
    ~FlatUIBarEventDispatcher() = default;
    
    // Initialization
    void Initialize(FlatUIBarStateManager* stateManager, 
                   FlatUIPageManager* pageManager, 
                   FlatUIBarLayoutManager* layoutManager);
    
    // Primary event handlers
    void HandleTabClick(size_t tabIndex);
    void HandlePinButtonClick();
    void HandleUnpinButtonClick();
    void HandleFloatPanelDismissed();
    
    // Mouse event handlers
    void HandleMouseDown(const wxPoint& position);
    void HandleGlobalMouseDown(const wxPoint& globalPosition);
    void HandleTabAreaClick(const wxPoint& position);
    
    // State change handlers
    void HandleStateTransition(bool isPinned);
    void HandlePageActivation(size_t pageIndex);
    
    // Panel event handlers
    void HandleFixPanelEvent(wxCommandEvent& event);
    void HandleFloatPanelEvent(wxCommandEvent& event);
    
    // Window event handlers
    void HandleSizeEvent(const wxSize& newSize);
    void HandleShowEvent(bool isShown);
    void HandlePaintEvent(wxPaintEvent& event);
    
    // Event validation and routing
    bool ValidateTabClick(size_t tabIndex) const;
    bool ValidateMouseClick(const wxPoint& position) const;
    bool CanHandleEvent() const;
    
    // Event broadcasting
    void BroadcastStateChange(bool isPinned);
    void BroadcastPageChange(size_t oldIndex, size_t newIndex);
    
private:
    FlatUIBar* m_bar;
    FlatUIBarStateManager* m_stateManager;
    FlatUIPageManager* m_pageManager;
    FlatUIBarLayoutManager* m_layoutManager;
    
    // Event processing state
    bool m_initialized;
    bool m_processingEvent;
    wxPoint m_lastMousePosition;
    
    // Helper methods
    void ProcessPinnedTabClick(size_t tabIndex);
    void ProcessUnpinnedTabClick(size_t tabIndex);
    void UpdateLayoutAfterEvent();
    void RefreshDisplayAfterEvent();
    
    // State transition helpers
    void TransitionToPinned();
    void TransitionToUnpinned();
    
    // Event utilities
    size_t GetTabIndexFromPosition(const wxPoint& position) const;
    bool IsPositionInTabArea(const wxPoint& position) const;
    bool IsPositionInBarArea(const wxPoint& position) const;
    
    // Event sequence management
    void BeginEventProcessing();
    void EndEventProcessing();
    bool IsEventProcessing() const { return m_processingEvent; }
    
    // Safety checks
    bool ValidateComponents() const;
    void LogEventInfo(const wxString& eventType, const wxString& details = wxEmptyString) const;
};

#endif // FLATUIBAR_EVENT_DISPATCHER_H 