#ifndef DRAG_DROP_CONTROLLER_H
#define DRAG_DROP_CONTROLLER_H

#include <wx/window.h>
#include <wx/timer.h>
#include <wx/gdicmn.h>
#include <wx/bitmap.h>
#include <memory>
#include <functional>
#include "widgets/DockTypes.h"
#include "widgets/UnifiedDockTypes.h"
#include "widgets/IDockManager.h"

class ModernDockManager;
class ModernDockPanel;
class GhostWindow;
class DockGuides;

// Drop validation result
// Note: DropValidation is now defined in UnifiedDockTypes.h

// Drag session data
struct DragSession {
    DragOperation operation;
    ModernDockPanel* sourcePanel;
    int sourceTabIndex;
    wxWindow* draggedContent;
    wxString draggedTitle;
    wxBitmap draggedIcon;
    wxPoint startPos;
    wxPoint currentPos;
    wxPoint offset;
    bool thresholdExceeded;
    
    DragSession() : operation(DragOperation::None), sourcePanel(nullptr),
                   sourceTabIndex(-1), draggedContent(nullptr),
                   thresholdExceeded(false) {}
};

// Advanced drag and drop controller
class DragDropController : public wxEvtHandler {
public:
    explicit DragDropController(IDockManager* manager);
    ~DragDropController();
    
    // Drag session management
    bool StartDrag(ModernDockPanel* panel, int tabIndex, const wxPoint& startPos);
    void UpdateDrag(const wxPoint& currentPos);
    void CompleteDrag();
    void CancelDrag();
    
    // State queries
    bool IsDragging() const { return m_dragSession.operation != DragOperation::None; }
    DragOperation GetCurrentOperation() const { return m_dragSession.operation; }
    const DragSession& GetDragSession() const { return m_dragSession; }
    
    // Access to drag session data for docking operations
    DropValidation GetLastValidation() const { return m_lastValidation; }
    ModernDockPanel* GetSourcePanel() const { return m_dragSession.sourcePanel; }
    
    // Drop validation
    DropValidation ValidateDrop(const wxPoint& pos) const;
    bool CanDropOnTarget(ModernDockPanel* target, DockPosition position) const;
    
    // Configuration
    void SetDragThreshold(int threshold) { m_dragThreshold = threshold; }
    int GetDragThreshold() const { return m_dragThreshold; }
    void SetAutoScrollEnabled(bool enabled) { m_autoScrollEnabled = enabled; }
    bool IsAutoScrollEnabled() const { return m_autoScrollEnabled; }
    
    // Event callbacks
    using DragStartCallback = std::function<void(const DragSession&)>;
    using DragUpdateCallback = std::function<void(const DragSession&)>;
    using DragCompleteCallback = std::function<void(const DragSession&, const DropValidation&)>;
    using DragCancelCallback = std::function<void(const DragSession&)>;
    
    void SetDragStartCallback(DragStartCallback callback) { m_onDragStart = callback; }
    void SetDragUpdateCallback(DragUpdateCallback callback) { m_onDragUpdate = callback; }
    void SetDragCompleteCallback(DragCompleteCallback callback) { m_onDragComplete = callback; }
    void SetDragCancelCallback(DragCancelCallback callback) { m_onDragCancel = callback; }

private:
    void InitializeDragSession(ModernDockPanel* panel, int tabIndex, const wxPoint& startPos);
    void UpdateDragOperation(const wxPoint& currentPos);
    void HandleTabReorder(const wxPoint& pos);
    void HandleTabDetach(const wxPoint& pos);
    void HandlePanelMove(const wxPoint& pos);
    
    // Drop zone detection
    ModernDockPanel* FindTargetPanel(const wxPoint& screenPos) const;
    DockPosition CalculateDockPosition(ModernDockPanel* target, const wxPoint& pos) const;
    wxRect CalculatePreviewRect(ModernDockPanel* target, DockPosition position) const;
    int CalculateTabInsertIndex(ModernDockPanel* target, const wxPoint& pos) const;
    
    // Auto-scroll support
    void UpdateAutoScroll(const wxPoint& pos);
    void StartAutoScroll(const wxPoint& direction);
    void StopAutoScroll();
    void OnAutoScrollTimer(wxTimerEvent& event);
    
    // Visual feedback management
    void UpdateVisualFeedback(const DropValidation& validation);
    void ClearVisualFeedback();
    
    // Utility functions
    bool ExceedsThreshold(const wxPoint& start, const wxPoint& current) const;
    DragOperation DetermineDragOperation(const wxPoint& startPos, const wxPoint& currentPos) const;
    wxBitmap CaptureTabContent(ModernDockPanel* panel, int tabIndex) const;
    
    IDockManager* m_manager;
    DragSession m_dragSession;
    
    // Configuration
    int m_dragThreshold;
    bool m_autoScrollEnabled;
    
    // Auto-scroll state
    wxTimer m_autoScrollTimer;
    wxPoint m_autoScrollDirection;
    bool m_autoScrolling;
    
    // Visual feedback
    DropValidation m_lastValidation;
    bool m_feedbackVisible;
    
    // Event callbacks
    DragStartCallback m_onDragStart;
    DragUpdateCallback m_onDragUpdate;
    DragCompleteCallback m_onDragComplete;
    DragCancelCallback m_onDragCancel;
    
    // Constants
    static constexpr int DEFAULT_DRAG_THRESHOLD = 8;
    static constexpr int AUTO_SCROLL_MARGIN = 20;
    static constexpr int AUTO_SCROLL_SPEED = 10;
    static constexpr int AUTO_SCROLL_INTERVAL = 50;
    static constexpr double DOCK_REGION_RATIO = 0.25;

    wxDECLARE_EVENT_TABLE();
};

#endif // DRAG_DROP_CONTROLLER_H

