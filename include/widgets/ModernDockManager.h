#ifndef MODERN_DOCK_MANAGER_H
#define MODERN_DOCK_MANAGER_H

#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <wx/graphics.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include "widgets/DockTypes.h"

// Forward declarations
class wxGraphicsContext;

class ModernDockPanel;
class DockGuides;
class GhostWindow;
class DragDropController;
class LayoutEngine;

// Drag state enumeration
enum class DragState {
    None, Started, Active, Completing
};

// Modern dock manager with VS2022-style features
class ModernDockManager : public wxPanel {
public:
    explicit ModernDockManager(wxWindow* parent);
    ~ModernDockManager() override;

    // Panel management
    void AddPanel(wxWindow* content, const wxString& title, DockArea area = DockArea::Center);
    void RemovePanel(ModernDockPanel* panel);
    ModernDockPanel* FindPanel(const wxString& title) const;
    
    // Drag and drop operations
    void StartDrag(ModernDockPanel* panel, const wxPoint& startPos);
    void UpdateDrag(const wxPoint& currentPos);
    void CompleteDrag(const wxPoint& endPos);
    void CancelDrag();
    
    // Hit testing and region detection
    ModernDockPanel* HitTest(const wxPoint& screenPos) const;
    DockPosition GetDockPosition(ModernDockPanel* target, const wxPoint& screenPos) const;
    
    // Visual feedback
    void ShowDockGuides(ModernDockPanel* target);
    void HideDockGuides();
    void ShowPreviewRect(const wxRect& rect, DockPosition position);
    void HidePreviewRect();
    
    // Layout management
    void SaveLayout();
    void RestoreLayout();
    void OptimizeLayout();
    
    // DPI support
    void OnDPIChanged(double newScale);
    double GetDPIScale() const { return m_dpiScale; }

protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);

private:
    void InitializeComponents();
    void UpdateLayout();
    void RenderPreviewOverlay(wxGraphicsContext* gc);
    void AnimatePreviewRect();
    void DrawPositionIndicator(wxGraphicsContext* gc, const wxRect& rect, DockPosition position);
    void DrawArrow(wxGraphicsContext* gc, const wxPoint& center, int size, double angle);
    void DrawTabIcon(wxGraphicsContext* gc, const wxPoint& center, int size);
    
    // Core components
    std::unique_ptr<DockGuides> m_dockGuides;
    std::unique_ptr<GhostWindow> m_ghostWindow;
    std::unique_ptr<DragDropController> m_dragController;
    std::unique_ptr<LayoutEngine> m_layoutEngine;
    
    // Panel containers by area
    std::unordered_map<DockArea, std::vector<ModernDockPanel*>> m_panels;
    
    // Drag state
    DragState m_dragState;
    ModernDockPanel* m_draggedPanel;
    ModernDockPanel* m_hoveredPanel;
    wxPoint m_dragStartPos;
    wxPoint m_lastMousePos;
    
    // Visual feedback
    bool m_previewVisible;
    wxRect m_previewRect;
    wxRect m_targetPreviewRect;
    DockPosition m_previewPosition;
    wxTimer m_animationTimer;
    double m_animationProgress;
    
    // DPI awareness
    double m_dpiScale;
    
    // Constants
    static constexpr int ANIMATION_STEPS = 10;
    static constexpr int ANIMATION_DURATION_MS = 150;
    static constexpr int HIT_TEST_MARGIN = 20;
    static constexpr double PREVIEW_ALPHA = 0.3;

    wxDECLARE_EVENT_TABLE();
};

#endif // MODERN_DOCK_MANAGER_H
