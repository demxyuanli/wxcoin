#include "widgets/DragDropController.h"
#include "widgets/ModernDockManager.h"
#include "widgets/ModernDockPanel.h"
#include "DPIManager.h"
#include <wx/window.h>
#include <wx/dcmemory.h>
#include <cmath>

wxBEGIN_EVENT_TABLE(DragDropController, wxEvtHandler)
    EVT_TIMER(wxID_ANY, DragDropController::OnAutoScrollTimer)
wxEND_EVENT_TABLE()

DragDropController::DragDropController(IDockManager* manager)
    : wxEvtHandler(),
      m_manager(manager),
      m_dragThreshold(DEFAULT_DRAG_THRESHOLD),
      m_autoScrollEnabled(true),
      m_autoScrollTimer(this, wxID_ANY),
      m_autoScrolling(false),
      m_feedbackVisible(false)
{
    // Initialize DPI-aware threshold
    double dpiScale = DPIManager::getInstance().getDPIScale();
    m_dragThreshold = static_cast<int>(DEFAULT_DRAG_THRESHOLD * dpiScale);
}

DragDropController::~DragDropController()
{
    CancelDrag();
}

bool DragDropController::StartDrag(ModernDockPanel* panel, int tabIndex, const wxPoint& startPos)
{
    if (!panel || tabIndex < 0 || tabIndex >= panel->GetContentCount()) {
        return false;
    }
    
    // Initialize drag session
    InitializeDragSession(panel, tabIndex, startPos);
    
    // Capture content for dragging
    m_dragSession.draggedContent = panel->GetContent(tabIndex);
    m_dragSession.draggedTitle = panel->GetContentTitle(tabIndex);
    m_dragSession.draggedIcon = panel->GetContentIcon(tabIndex);
    
    // Notify callback
    if (m_onDragStart) {
        m_onDragStart(m_dragSession);
    }
    
    return true;
}

void DragDropController::UpdateDrag(const wxPoint& currentPos)
{
    if (m_dragSession.operation == DragOperation::None) return;
    
    m_dragSession.currentPos = currentPos;
    
    // Check if threshold exceeded
    if (!m_dragSession.thresholdExceeded) {
        if (ExceedsThreshold(m_dragSession.startPos, currentPos)) {
            m_dragSession.thresholdExceeded = true;
            
            // Determine drag operation type
            m_dragSession.operation = DetermineDragOperation(m_dragSession.startPos, currentPos);
        } else {
            return; // Still within threshold
        }
    }
    
    // Update drag operation
    UpdateDragOperation(currentPos);
    
    // Update auto-scroll if enabled
    if (m_autoScrollEnabled) {
        UpdateAutoScroll(currentPos);
    }
    
    // Notify callback
    if (m_onDragUpdate) {
        m_onDragUpdate(m_dragSession);
    }
}

void DragDropController::CompleteDrag()
{
    if (m_dragSession.operation == DragOperation::None) return;
    
    // Validate drop
    DropValidation validation = ValidateDrop(m_dragSession.currentPos);
    
    // Stop auto-scroll
    StopAutoScroll();
    
    // Clear visual feedback
    ClearVisualFeedback();
    
    // Notify callback
    if (m_onDragComplete) {
        m_onDragComplete(m_dragSession, validation);
    }
    
    // Reset drag session
    m_dragSession = DragSession();
}

void DragDropController::CancelDrag()
{
    if (m_dragSession.operation == DragOperation::None) return;
    
    // Stop auto-scroll
    StopAutoScroll();
    
    // Clear visual feedback
    ClearVisualFeedback();
    
    // Notify callback
    if (m_onDragCancel) {
        m_onDragCancel(m_dragSession);
    }
    
    // Reset drag session
    m_dragSession = DragSession();
}

DropValidation DragDropController::ValidateDrop(const wxPoint& pos) const
{
    DropValidation validation;
    
    if (m_dragSession.operation == DragOperation::None) {
        return validation;
    }
    
    // Find target panel under cursor
    validation.targetPanel = FindTargetPanel(pos);
    if (!validation.targetPanel) {
        return validation;
    }
    
    // Calculate dock position
    validation.position = CalculateDockPosition(validation.targetPanel, pos);
    if (validation.position == DockPosition::None) {
        return validation;
    }
    
    // Check if drop is allowed
    if (!CanDropOnTarget(validation.targetPanel, validation.position)) {
        return validation;
    }
    
    // Calculate preview rectangle
    validation.previewRect = CalculatePreviewRect(validation.targetPanel, validation.position);
    
    // Calculate insert index for tab operations
    if (validation.position == DockPosition::Center) {
        validation.insertIndex = CalculateTabInsertIndex(validation.targetPanel, pos);
    }
    
    validation.valid = true;
    return validation;
}

bool DragDropController::CanDropOnTarget(ModernDockPanel* target, DockPosition position) const
{
    if (!target) return false;
    
    // Don't allow dropping on the same panel for now (could be enhanced)
    if (target == m_dragSession.sourcePanel && position == DockPosition::Center) {
        return false;
    }
    
    // Check if target accepts drops
    // This could be extended with more sophisticated rules
    return true;
}

void DragDropController::InitializeDragSession(ModernDockPanel* panel, int tabIndex, const wxPoint& startPos)
{
    m_dragSession = DragSession();
    m_dragSession.operation = DragOperation::TabDetach; // Start as potential detach
    m_dragSession.sourcePanel = panel;
    m_dragSession.sourceTabIndex = tabIndex;
    m_dragSession.startPos = startPos;
    m_dragSession.currentPos = startPos;
    m_dragSession.thresholdExceeded = false;
    
    // Calculate offset from panel position
    wxPoint panelPos = panel->GetScreenPosition();
    m_dragSession.offset = startPos - panelPos;
}

void DragDropController::UpdateDragOperation(const wxPoint& currentPos)
{
    switch (m_dragSession.operation) {
        case DragOperation::TabReorder:
            HandleTabReorder(currentPos);
            break;
        case DragOperation::TabDetach:
            HandleTabDetach(currentPos);
            break;
        case DragOperation::PanelMove:
            HandlePanelMove(currentPos);
            break;
        default:
            break;
    }
}

void DragDropController::HandleTabReorder(const wxPoint& pos)
{
    // Implementation for tab reordering within the same panel
    if (!m_dragSession.sourcePanel) return;
    
    wxPoint localPos = m_dragSession.sourcePanel->ScreenToClient(pos);
    int newIndex = CalculateTabInsertIndex(m_dragSession.sourcePanel, pos);
    wxUnusedVar(localPos);
    wxUnusedVar(newIndex);
    
    // Update visual feedback for tab reordering
    // This would typically show an insertion indicator
}

void DragDropController::HandleTabDetach(const wxPoint& pos)
{
    // Implementation for tab detachment and docking
    DropValidation validation = ValidateDrop(pos);
    UpdateVisualFeedback(validation);
}

void DragDropController::HandlePanelMove(const wxPoint& pos)
{
    wxUnusedVar(pos);
    // Implementation for moving entire panels
    // This would be used for docking entire panels to new locations
}

ModernDockPanel* DragDropController::FindTargetPanel(const wxPoint& screenPos) const
{
    if (!m_manager) return nullptr;
    
    static bool inFindTarget = false;
    if (inFindTarget) return nullptr;
    
    inFindTarget = true;
    wxWindow* result = m_manager->HitTest(screenPos);
    inFindTarget = false;
    
    // Convert wxWindow* to ModernDockPanel* if possible
    if (result) {
        // Try to find the panel that contains this window
        // This is a simplified approach - in a real implementation,
        // you might want to maintain a mapping or use a different approach
        return nullptr; // Placeholder - need proper conversion logic
    }
    
    return nullptr;
}

DockPosition DragDropController::CalculateDockPosition(ModernDockPanel* target, const wxPoint& pos) const
{
    if (!target) return DockPosition::None;
    
    // Convert ModernDockPanel* to wxWindow* for the interface call
    wxWindow* targetWindow = target->GetContent();
    return m_manager->GetDockPosition(targetWindow, pos);
}

wxRect DragDropController::CalculatePreviewRect(ModernDockPanel* target, DockPosition position) const
{
    if (!target) return wxRect();
    
    wxRect targetRect = target->GetScreenRect();
    
    switch (position) {
        case DockPosition::Left:
            return wxRect(targetRect.x, targetRect.y, 
                         targetRect.width / 2, targetRect.height);
            
        case DockPosition::Right:
            return wxRect(targetRect.x + targetRect.width / 2, targetRect.y,
                         targetRect.width / 2, targetRect.height);
            
        case DockPosition::Top:
            return wxRect(targetRect.x, targetRect.y,
                         targetRect.width, targetRect.height / 2);
            
        case DockPosition::Bottom:
            return wxRect(targetRect.x, targetRect.y + targetRect.height / 2,
                         targetRect.width, targetRect.height / 2);
            
        case DockPosition::Center:
            return targetRect;
            
        default:
            return wxRect();
    }
}

int DragDropController::CalculateTabInsertIndex(ModernDockPanel* target, const wxPoint& pos) const
{
    if (!target) return -1;
    
    wxPoint localPos = target->ScreenToClient(pos);
    
    // Simple implementation: divide tab area by tab count
    int tabCount = target->GetContentCount();
    if (tabCount == 0) return 0;
    
    wxRect contentRect = target->GetClientRect();
    int tabAreaWidth = contentRect.width;
    int tabWidth = tabAreaWidth / tabCount;
    
    if (tabWidth > 0) {
        int index = localPos.x / tabWidth;
        return std::max(0, std::min(tabCount, index));
    }
    
    return 0;
}

void DragDropController::UpdateAutoScroll(const wxPoint& pos)
{
    if (!m_autoScrollEnabled || !m_manager) return;
    
    // Get manager client rect in screen coordinates
    wxRect managerRect = m_manager->GetScreenRect();
    
    // Check if near edges
    wxPoint direction(0, 0);
    bool shouldScroll = false;
    
    if (pos.x < managerRect.x + AUTO_SCROLL_MARGIN) {
        direction.x = -1;
        shouldScroll = true;
    } else if (pos.x > managerRect.GetRight() - AUTO_SCROLL_MARGIN) {
        direction.x = 1;
        shouldScroll = true;
    }
    
    if (pos.y < managerRect.y + AUTO_SCROLL_MARGIN) {
        direction.y = -1;
        shouldScroll = true;
    } else if (pos.y > managerRect.GetBottom() - AUTO_SCROLL_MARGIN) {
        direction.y = 1;
        shouldScroll = true;
    }
    
    if (shouldScroll) {
        StartAutoScroll(direction);
    } else {
        StopAutoScroll();
    }
}

void DragDropController::StartAutoScroll(const wxPoint& direction)
{
    if (m_autoScrollDirection != direction) {
        m_autoScrollDirection = direction;
        
        if (!m_autoScrollTimer.IsRunning()) {
            m_autoScrollTimer.Start(AUTO_SCROLL_INTERVAL);
        }
        
        m_autoScrolling = true;
    }
}

void DragDropController::StopAutoScroll()
{
    if (m_autoScrolling) {
        m_autoScrolling = false;
        m_autoScrollTimer.Stop();
        m_autoScrollDirection = wxPoint(0, 0);
    }
}

void DragDropController::OnAutoScrollTimer(wxTimerEvent& event)
{
    wxUnusedVar(event);
    if (!m_autoScrolling || !m_manager) return;
    
    // Implement auto-scrolling logic
    // This would typically scroll the viewport when dragging near edges
    
    // For now, just continue the timer
    // Real implementation would depend on the scrollable container
}

void DragDropController::UpdateVisualFeedback(const DropValidation& validation)
{
    if (validation.valid) {
        // Show preview rect and dock guides
        m_manager->ShowPreviewRect(validation.previewRect, validation.position);
        m_manager->ShowDockGuides(validation.targetPanel);
        m_feedbackVisible = true;
    } else {
        ClearVisualFeedback();
    }
    
    m_lastValidation = validation;
}

void DragDropController::ClearVisualFeedback()
{
    if (m_feedbackVisible) {
        m_manager->HidePreviewRect();
        m_manager->HideDockGuides();
        m_feedbackVisible = false;
    }
}

bool DragDropController::ExceedsThreshold(const wxPoint& start, const wxPoint& current) const
{
    int dx = abs(current.x - start.x);
    int dy = abs(current.y - start.y);
    return (dx > m_dragThreshold || dy > m_dragThreshold);
}

DragOperation DragDropController::DetermineDragOperation(const wxPoint& startPos, const wxPoint& currentPos) const
{
    wxUnusedVar(startPos);
    // Simple heuristic: if dragging within the source panel, it's a reorder
    // Otherwise, it's a detach operation
    
    if (m_dragSession.sourcePanel) {
        wxRect panelRect = m_dragSession.sourcePanel->GetScreenRect();
        if (panelRect.Contains(currentPos)) {
            return DragOperation::TabReorder;
        }
    }
    
    return DragOperation::TabDetach;
}

wxBitmap DragDropController::CaptureTabContent(ModernDockPanel* panel, int tabIndex) const
{
    if (!panel || tabIndex < 0 || tabIndex >= panel->GetContentCount()) {
        return wxNullBitmap;
    }
    
    // Create a simplified representation of the tab
    wxSize tabSize(150, 30);
    wxBitmap bitmap(tabSize);
    wxMemoryDC memDC(bitmap);
    
    // Draw tab background
    memDC.SetBackground(wxBrush(wxColour(0, 122, 204)));
    memDC.Clear();
    
    // Draw tab text
    memDC.SetTextForeground(*wxWHITE);
    wxString title = panel->GetContentTitle(tabIndex);
    wxRect textRect(8, 0, tabSize.x - 16, tabSize.y);
    memDC.DrawLabel(title, textRect, wxALIGN_CENTER_VERTICAL);
    
    memDC.SelectObject(wxNullBitmap);
    return bitmap;
}

