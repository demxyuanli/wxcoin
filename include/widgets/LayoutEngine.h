#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include <wx/window.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include "widgets/DockTypes.h"
#include "widgets/UnifiedDockTypes.h"
#include "widgets/IDockManager.h"

class ModernDockManager;
class ModernDockPanel;

// Layout node types
enum class LayoutNodeType {
    Panel, HorizontalSplitter, VerticalSplitter, Root
};

// Layout constraints
// Note: LayoutConstraints is now defined in UnifiedDockTypes.h

// Forward declaration
class LayoutNode;

// Layout transition animation
struct LayoutTransition {
    enum Type { None, SplitterMove, PanelInsert, PanelRemove, Resize };
    
    Type type;
    LayoutNode* node;
    wxRect startRect;
    wxRect targetRect;
    double progress;
    int duration;
    bool active;
    
    LayoutTransition() : type(None), node(nullptr), progress(0.0), 
                        duration(250), active(false) {}
};

// Layout tree node
class LayoutNode {
public:
    LayoutNode(LayoutNodeType type, LayoutNode* parent = nullptr);
    ~LayoutNode();
    
    // Tree structure
    void AddChild(std::unique_ptr<LayoutNode> child);
    void RemoveChild(LayoutNode* child);
    LayoutNode* GetParent() const { return m_parent; }
    const std::vector<std::unique_ptr<LayoutNode>>& GetChildren() const { return m_children; }
    std::vector<std::unique_ptr<LayoutNode>>& GetChildren() { return m_children; }
    bool IsLeaf() const { return m_children.empty(); }
    bool IsRoot() const { return m_parent == nullptr; }
    
    // Properties
    LayoutNodeType GetType() const { return m_type; }
    void SetPanel(ModernDockPanel* panel) { m_panel = panel; }
    ModernDockPanel* GetPanel() const { return m_panel; }
    void SetSplitter(wxSplitterWindow* splitter) { m_splitter = splitter; }
    wxSplitterWindow* GetSplitter() const { return m_splitter; }
    
    // Layout management
    void SetRect(const wxRect& rect) { m_rect = rect; }
    wxRect GetRect() const { return m_rect; }
    void SetConstraints(const LayoutConstraints& constraints) { m_constraints = constraints; }
    const LayoutConstraints& GetConstraints() const { 
        static const LayoutConstraints defaultConstraints;
        return m_constraints; 
    }
    
    // Splitter management
    void SetSplitterRatio(double ratio) { m_splitterRatio = ratio; }
    double GetSplitterRatio() const { return m_splitterRatio; }
    void SetSashPosition(int position);
    int GetSashPosition() const;
    
    // Utility
    LayoutNode* FindPanel(ModernDockPanel* panel);
    LayoutNode* FindNodeAt(const wxPoint& pos);
    void GetAllPanels(std::vector<ModernDockPanel*>& panels);
    void SetDockArea(DockArea area) { m_dockArea = area; }
    DockArea GetDockArea() const { return m_dockArea; }
    
private:
    LayoutNodeType m_type;
    LayoutNode* m_parent;
    std::vector<std::unique_ptr<LayoutNode>> m_children;
    
    // Associated objects
    ModernDockPanel* m_panel;           // For panel nodes
    wxSplitterWindow* m_splitter;       // For splitter nodes
    
    // Layout data
    wxRect m_rect;
    LayoutConstraints m_constraints;
    double m_splitterRatio;             // For splitter nodes (0.0-1.0)
    DockArea m_dockArea;                // For panel nodes
};

// Advanced layout engine with animation support
class LayoutEngine : public wxEvtHandler {
public:
    explicit LayoutEngine(wxWindow* parent, IDockManager* manager);
    ~LayoutEngine();
    
    // Layout tree management
    void InitializeLayout(wxWindow* rootWindow);
    void AddPanel(ModernDockPanel* panel, DockArea area, ModernDockPanel* relativeTo = nullptr);
    void RemovePanel(ModernDockPanel* panel);
    void MovePanel(ModernDockPanel* panel, DockArea newArea, ModernDockPanel* relativeTo = nullptr);
    
    // Docking operations
    bool DockPanel(ModernDockPanel* panel, ModernDockPanel* target, DockPosition position);
    void TabifyPanel(ModernDockPanel* panel, ModernDockPanel* target);
    void FloatPanel(ModernDockPanel* panel);
    void RestorePanel(ModernDockPanel* panel, DockArea area);
    
    // Layout calculation
    void UpdateLayout();
    void UpdateLayout(const wxRect& clientRect);
    void RecalculateLayout();
    void OptimizeLayout();
    
    // Splitter management
    void CreateSplitter(LayoutNode* parent, bool horizontal);
    void RemoveSplitter(LayoutNode* splitterNode);
    void UpdateSplitterConstraints();
    
    // Animation support
    void AnimateLayout(int durationMs = 250);
    void StartTransition(const LayoutTransition& transition);
    void UpdateTransitions();
    bool IsAnimating() const { return !m_activeTransitions.empty(); }
    
    // Query operations
    LayoutNode* GetRootNode() const { return m_rootNode.get(); }
    LayoutNode* FindPanelNode(ModernDockPanel* panel) const;
    std::vector<ModernDockPanel*> GetAllPanels() const;
    wxRect GetPanelRect(ModernDockPanel* panel) const;
    
    // Layout persistence
    wxString SaveLayout() const;
    bool LoadLayout(const wxString& layoutData);
    
    // Configuration
    void SetAnimationEnabled(bool enabled) { m_animationEnabled = enabled; }
    bool IsAnimationEnabled() const { return m_animationEnabled; }
    void SetMinPanelSize(const wxSize& size) { m_minPanelSize = size; }
    wxSize GetMinPanelSize() const { return m_minPanelSize; }

private:
    // Layout tree operations
    std::unique_ptr<LayoutNode> CreatePanelNode(ModernDockPanel* panel);
    std::unique_ptr<LayoutNode> CreateSplitterNode(bool horizontal);
    void InsertPanelIntoTree(ModernDockPanel* panel, LayoutNode* parent, DockPosition position);
    void InsertPanelWithSplitter(std::unique_ptr<LayoutNode> panelNode, LayoutNode* parent, DockPosition position);
    void RemovePanelFromTree(LayoutNode* panelNode);
    void OrganizeByDockAreas(std::unique_ptr<LayoutNode> panelNode, LayoutNode* parent);
    void CreateMainLayoutStructure(LayoutNode* parent);
    
    // Layout calculation helpers
    void CalculateNodeLayout(LayoutNode* node, const wxRect& rect);
    void CalculateSplitterLayout(LayoutNode* splitterNode, const wxRect& rect);
    void ApplyLayoutToWidgets(LayoutNode* node);
    
    // Splitter helpers
    wxSplitterWindow* CreateSplitterWindow(wxWindow* parent, bool horizontal);
    void ConfigureSplitter(wxSplitterWindow* splitter, bool horizontal);
    void UpdateSplitterSashPosition(LayoutNode* splitterNode);
    void UpdateSplitterChildrenLayout(LayoutNode* splitterNode);
    void SetupSplitterPanels(LayoutNode* splitterNode);
    wxWindow* GetChildWindow(LayoutNode* node);
    LayoutNode* FindSplitterNode(wxSplitterWindow* splitter) const;
    
    // Animation helpers
    void InitializeTransition(LayoutTransition& transition, LayoutNode* node);
    void ApplyTransition(const LayoutTransition& transition);
    void CompleteTransition(LayoutTransition& transition);
    void OnAnimationTimer(wxTimerEvent& event);
    
    // Constraint validation
    bool ValidateConstraints(LayoutNode* node, const wxRect& rect) const;
    wxRect EnforceConstraints(LayoutNode* node, const wxRect& rect) const;
    void UpdateConstraintsRecursive(LayoutNode* node);
    
    // Utility functions
    DockArea PositionToDockArea(DockPosition position) const;
    bool CanDockAtPosition(LayoutNode* target, DockPosition position) const;
    LayoutNode* FindBestInsertionPoint(DockArea area) const;
    void CleanupEmptyNodes();
    bool IsNodeValid(LayoutNode* node) const;
    bool IsLeftSidebarSplitter(LayoutNode* splitterNode) const;
    bool IsNodeInHierarchy(LayoutNode* ancestor, LayoutNode* target) const;
    
    wxWindow* m_parent;
    IDockManager* m_manager;
    std::unique_ptr<LayoutNode> m_rootNode;
    
    // Animation system
    wxTimer m_animationTimer;
    std::vector<LayoutTransition> m_activeTransitions;
    bool m_animationEnabled;
    
    // Configuration
    wxSize m_minPanelSize;
    int m_splitterSashSize;
    int m_defaultAnimationDuration;
    
    // Layout state
    wxRect m_lastClientRect;
    bool m_layoutDirty;
    
    // Event handlers
    void OnSplitterMoved(wxSplitterEvent& event);
    void OnSplitterDoubleClick(wxSplitterEvent& event);
    
    // Constants
    static constexpr int DEFAULT_MIN_PANEL_WIDTH = 150;
    static constexpr int DEFAULT_MIN_PANEL_HEIGHT = 100;
    static constexpr int DEFAULT_SPLITTER_SASH_SIZE = 4;
    static constexpr int DEFAULT_ANIMATION_DURATION = 250;
    static constexpr int ANIMATION_TIMER_INTERVAL = 16; // ~60 FPS
    static constexpr double MIN_SPLITTER_RATIO = 0.1;
    static constexpr double MAX_SPLITTER_RATIO = 0.9;

    wxDECLARE_EVENT_TABLE();
};

#endif // LAYOUT_ENGINE_H

