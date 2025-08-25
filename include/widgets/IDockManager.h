#pragma once

#include "UnifiedDockTypes.h"
#include <wx/panel.h>
#include <wx/window.h>
#include <wx/string.h>
#include <wx/event.h>
#include <functional>
#include <vector>

// Forward declarations
class LayoutNode;
class ModernDockPanel;

// Unified dock manager interface
class IDockManager {
public:
    virtual ~IDockManager() = default;

    // Core panel management
    virtual void AddPanel(wxWindow* content, const wxString& title, UnifiedDockArea area) = 0;
    virtual void RemovePanel(wxWindow* content) = 0;
    virtual void ShowPanel(wxWindow* content) = 0;
    virtual void HidePanel(wxWindow* content) = 0;
    virtual bool HasPanel(wxWindow* content) const = 0;
    
    // Layout strategy management
    virtual void SetLayoutStrategy(LayoutStrategy strategy) = 0;
    virtual LayoutStrategy GetLayoutStrategy() const = 0;
    virtual void SetLayoutConstraints(const LayoutConstraints& constraints) = 0;
    virtual LayoutConstraints GetLayoutConstraints() const = 0;
    
    // Layout persistence
    virtual void SaveLayout() = 0;
    virtual void RestoreLayout() = 0;
    virtual void ResetToDefaultLayout() = 0;
    virtual bool LoadLayoutFromFile(const wxString& filename) = 0;
    virtual bool SaveLayoutToFile(const wxString& filename) = 0;
    
    // Panel positioning and docking
    virtual void DockPanel(wxWindow* panel, wxWindow* target, DockPosition position) = 0;
    virtual void UndockPanel(wxWindow* panel) = 0;
    virtual void FloatPanel(wxWindow* panel) = 0;
    virtual void TabifyPanel(wxWindow* panel, wxWindow* target) = 0;
    
    // Layout information
    virtual wxRect GetPanelRect(wxWindow* panel) const = 0;
    virtual UnifiedDockArea GetPanelArea(wxWindow* panel) const = 0;
    virtual bool IsPanelFloating(wxWindow* panel) const = 0;
    virtual bool IsPanelDocked(wxWindow* panel) const = 0;
    
    // Visual feedback control
    virtual void ShowDockGuides() = 0;
    virtual void HideDockGuides() = 0;
    virtual void ShowDockGuides(wxWindow* target) = 0;  // Overloaded version
    virtual void SetDockGuideConfig(const DockGuideConfig& config) = 0;
    virtual DockGuideConfig GetDockGuideConfig() const = 0;
    
    // Preview and hit testing
    virtual void ShowPreviewRect(const wxRect& rect, DockPosition position) = 0;
    virtual void HidePreviewRect() = 0;
    virtual wxWindow* HitTest(const wxPoint& screenPos) const = 0;
    virtual DockPosition GetDockPosition(wxWindow* target, const wxPoint& screenPos) const = 0;
    virtual wxRect GetScreenRect() const = 0;
    
    // Event handling
    virtual void BindDockEvent(wxEventType eventType, 
                              std::function<void(const DockEventData&)> handler) = 0;
    virtual void UnbindDockEvent(wxEventType eventType) = 0;
    
    // Drag and drop
    virtual void StartDrag(wxWindow* panel, const wxPoint& startPos) = 0;
    virtual void UpdateDrag(const wxPoint& currentPos) = 0;
    virtual void EndDrag(const wxPoint& endPos) = 0;
    virtual bool IsDragging() const = 0;
    
    // Layout tree access
    virtual LayoutNode* GetRootNode() const = 0;
    virtual LayoutNode* FindNode(wxWindow* panel) const = 0;
    virtual void TraverseNodes(std::function<void(LayoutNode*)> visitor) const = 0;
    
    // Utility functions
    virtual void RefreshLayout() = 0;
    virtual void UpdateLayout() = 0;
    virtual void FitLayout() = 0;
    virtual wxSize GetMinimumSize() const = 0;
    virtual wxSize GetBestSize() const = 0;
    
    // wxWidgets compatibility methods
    virtual wxRect GetClientRect() const = 0;
    virtual wxPoint ClientToScreen(const wxPoint& pt) const = 0;
    virtual wxPoint ScreenToClient(const wxPoint& pt) const = 0;
    
    // Configuration
    virtual void SetAutoSaveLayout(bool autoSave) = 0;
    virtual bool GetAutoSaveLayout() const = 0;
    virtual void SetLayoutUpdateInterval(int milliseconds) = 0;
    virtual int GetLayoutUpdateInterval() const = 0;
    
    // Statistics and debugging
    virtual int GetPanelCount() const = 0;
    virtual int GetContainerCount() const = 0;
    virtual int GetSplitterCount() const = 0;
    virtual wxString GetLayoutStatistics() const = 0;
    virtual void DumpLayoutTree() const = 0;
    
    // Panel collection access
    virtual std::vector<ModernDockPanel*> GetAllPanels() const = 0;
    
    // Dock guide target access
    virtual ModernDockPanel* GetDockGuideTarget() const = 0;
};


