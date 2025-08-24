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
#include "widgets/UnifiedDockTypes.h"
#include "widgets/IDockManager.h"

// Forward declarations
class wxGraphicsContext;

class ModernDockPanel;
class DockGuides;
class GhostWindow;
class DragDropController;
class LayoutEngine;

// Modern dock manager with VS2022-style features
class ModernDockManager : public wxPanel, public IDockManager {
public:
    explicit ModernDockManager(wxWindow* parent);
    ~ModernDockManager() override;

    // Panel management
    void AddPanel(wxWindow* content, const wxString& title, DockArea area = DockArea::Center);
    void RemovePanel(ModernDockPanel* panel);
    ModernDockPanel* FindPanel(const wxString& title) const;
    ModernDockPanel* FindDockPanel(wxWindow* content) const;
    
    // Docking control
    void SetPanelDockingEnabled(ModernDockPanel* panel, bool enabled);
    void SetPanelSystemButtonsVisible(ModernDockPanel* panel, bool visible);
    void SetAreaDockingEnabled(DockArea area, bool enabled);
    bool IsAreaDockingEnabled(DockArea area) const;
    
    // Drag and drop operations
    void StartDrag(ModernDockPanel* panel, const wxPoint& startPos);
    void CompleteDrag(const wxPoint& endPos);
    void CancelDrag();
    

    
    // Visual feedback
    void ShowDockGuides(ModernDockPanel* target);
    
    // Layout management
    void OptimizeLayout();
    
    // IDockManager interface implementation
    void AddPanel(wxWindow* content, const wxString& title, UnifiedDockArea area) override;
    void RemovePanel(wxWindow* content) override;
    void ShowPanel(wxWindow* content) override;
    void HidePanel(wxWindow* content) override;
    bool HasPanel(wxWindow* content) const override;
    
    // Layout strategy management
    void SetLayoutStrategy(LayoutStrategy strategy) override;
    LayoutStrategy GetLayoutStrategy() const override;
    void SetLayoutConstraints(const LayoutConstraints& constraints) override;
    LayoutConstraints GetLayoutConstraints() const override;
    
    // Layout persistence
    void SaveLayout() override;
    void RestoreLayout() override;
    void ResetToDefaultLayout() override;
    bool LoadLayoutFromFile(const wxString& filename) override;
    bool SaveLayoutToFile(const wxString& filename) override;
    
    // Panel positioning and docking
    void DockPanel(wxWindow* panel, wxWindow* target, DockPosition position) override;
    void UndockPanel(wxWindow* panel) override;
    void FloatPanel(wxWindow* panel) override;
    void TabifyPanel(wxWindow* panel, wxWindow* target) override;
    
    // Layout information
    wxRect GetPanelRect(wxWindow* panel) const override;
    UnifiedDockArea GetPanelArea(wxWindow* panel) const override;
    bool IsPanelFloating(wxWindow* panel) const override;
    bool IsPanelDocked(wxWindow* panel) const override;
    
    // Visual feedback control
    void ShowDockGuides() override;
    void HideDockGuides() override;
    void ShowDockGuides(wxWindow* target) override;
    void SetDockGuideConfig(const DockGuideConfig& config) override;
    DockGuideConfig GetDockGuideConfig() const override;
    
    // Preview and hit testing
    void ShowPreviewRect(const wxRect& rect, DockPosition position) override;
    void HidePreviewRect() override;
    wxWindow* HitTest(const wxPoint& screenPos) const override;
    DockPosition GetDockPosition(wxWindow* target, const wxPoint& screenPos) const override;
    wxRect GetScreenRect() const override;
    
    // Event handling
    void BindDockEvent(wxEventType eventType, 
                       std::function<void(const DockEventData&)> handler) override;
    void UnbindDockEvent(wxEventType eventType) override;
    
    // Drag and drop
    void StartDrag(wxWindow* panel, const wxPoint& startPos) override;
    void UpdateDrag(const wxPoint& currentPos) override;
    void EndDrag(const wxPoint& endPos) override;
    bool IsDragging() const override;
    
    // Layout tree access
    LayoutNode* GetRootNode() const override;
    LayoutNode* FindNode(wxWindow* panel) const override;
    void TraverseNodes(std::function<void(LayoutNode*)> visitor) const override;
    
    // Utility functions
    void RefreshLayout() override;
    void FitLayout() override;
    wxSize GetMinimumSize() const override;
    wxSize GetBestSize() const override;
    
    // wxWidgets compatibility methods
    wxRect GetClientRect() const override;
    wxPoint ClientToScreen(const wxPoint& pt) const override;
    wxPoint ScreenToClient(const wxPoint& pt) const override;
    
    // Configuration
    void SetAutoSaveLayout(bool autoSave) override;
    bool GetAutoSaveLayout() const override;
    void SetLayoutUpdateInterval(int milliseconds) override;
    int GetLayoutUpdateInterval() const override;
    
    // Statistics and debugging
    int GetPanelCount() const override;
    int GetContainerCount() const override;
    int GetSplitterCount() const override;
    wxString GetLayoutStatistics() const override;
    void DumpLayoutTree() const override;
    
    // Configuration
    void EnableLayoutCaching(bool enabled);
    void SetLayoutUpdateMode(LayoutUpdateMode mode);
    
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
    
    // Layout strategy and configuration
    LayoutStrategy m_currentStrategy;
    bool m_autoSaveLayout;
    int m_layoutUpdateInterval;
    bool m_layoutCachingEnabled;
    LayoutUpdateMode m_layoutUpdateMode;
    LayoutConstraints m_layoutConstraints;
    
    // Constants
    static constexpr int ANIMATION_STEPS = 10;
    static constexpr int ANIMATION_DURATION_MS = 150;
    static constexpr int HIT_TEST_MARGIN = 20;
    static constexpr double PREVIEW_ALPHA = 0.3;

    wxDECLARE_EVENT_TABLE();
};

#endif // MODERN_DOCK_MANAGER_H
