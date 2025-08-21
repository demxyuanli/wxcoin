#pragma once

#include <wx/gdicmn.h>  // For wxRect

// Forward declarations for wxWidgets types to avoid include issues
class wxString;
class wxPoint;
class wxSize;
class wxColour;
class wxDateTime;
class wxArrayString;
class wxVariant;

#include <string>
#include <vector>
#include <memory>

// Forward declarations
class ModernDockPanel;

// Unified dock area definitions
enum class UnifiedDockArea {
    Left,       // Left sidebar
    Center,     // Main canvas area
    Right,      // Right sidebar
    Top,        // Top toolbar area
    Bottom,     // Bottom status bar
    Tab,        // Tabbed container
    Floating    // Floating window
};

// Layout strategy types
enum class LayoutStrategy {
    Fixed,      // Fixed 4-region layout (like FlatDock)
    IDE,        // IDE-style layout (like current LayoutEngine)
    Flexible,   // Completely dynamic tree layout
    Hybrid      // Mixed mode
};

// Layout update modes
enum class LayoutUpdateMode {
    Immediate,      // Update immediately
    Deferred,      // Update on next frame
    Lazy,          // Update only when needed
    Manual         // Manual update only
};

// Visual feedback types
enum class VisualFeedbackType {
    DockGuides,         // Dock position indicators
    DragPreview,        // Drag preview window
    AreaHighlight,      // Drop area highlighting
    SplitterIndicator   // Splitter line indicators
};

// Dock position for panel insertion
enum class DockPosition {
    None,       // No position
    Left,       // Dock to left
    Right,      // Dock to right
    Top,        // Dock to top
    Bottom,     // Dock to bottom
    Center,     // Dock to center
    Tab,        // Add as tab
    Floating    // Make floating
};

// Splitter orientation
enum class SplitterOrientation {
    Horizontal,     // Left-right split
    Vertical        // Top-bottom split
};

// Dock guide directions
enum class DockGuideDirection {
    Left, Right, Top, Bottom, Center
};

// Drag state
enum class DragState {
    None,           // No drag operation
    Started,        // Drag started
    Active,         // Drag in progress
    Completing      // Drop operation
};

// Drop validation result
struct DropValidation {
    bool valid;
    DockPosition position;
    class ModernDockPanel* targetPanel;  // Forward declaration
    int insertIndex;
    wxRect previewRect;  // Use value type
    
    DropValidation() : valid(false), position(DockPosition::None), 
                      targetPanel(nullptr), insertIndex(-1) {}
};

// Dock event types
enum class DockEventType {
    PanelAdded,         // Panel was added
    PanelRemoved,       // Panel was removed
    LayoutChanged,      // Layout structure changed
    StrategyChanged,    // Layout strategy changed
    PanelDocked,        // Panel was docked
    PanelUndocked,      // Panel was undocked
    DragStarted,        // Drag operation started
    DragEnded          // Drag operation ended
};

// Forward declaration for wxWindow
class wxWindow;

// Layout constraints
struct LayoutConstraints {
    int minWidth;
    int minHeight;
    int maxWidth;
    int maxHeight;
    bool resizable;
    bool dockable;
    
    LayoutConstraints(int minW = 100, int minH = 100, 
                     int maxW = -1, int maxH = -1,
                     bool res = true, bool dock = true)
        : minWidth(minW), minHeight(minH), maxWidth(maxW), maxHeight(maxH),
          resizable(res), dockable(dock) {}
};

// Dock guide configuration
struct DockGuideConfig {
    bool showCentral;           // Show central guides
    bool showEdges;             // Show edge guides
    bool showTabs;              // Show tab indicators
    int guideSize;              // Size of guide buttons
    int edgePadding;            // Padding from edges
    wxColour* guideColor;       // Guide button color (pointer)
    wxColour* highlightColor;   // Highlight color (pointer)
    
    DockGuideConfig()
        : showCentral(true), showEdges(true), showTabs(true),
          guideSize(32), edgePadding(8),
          guideColor(nullptr), highlightColor(nullptr) {}
};

// Dock event data
struct DockEventData {
    DockEventType type;
    wxWindow* panel;
    UnifiedDockArea area;
    wxRect* rect;  // Use pointer to avoid direct wxRect usage
    std::string title;
    
    DockEventData(DockEventType t, wxWindow* p = nullptr, 
                  UnifiedDockArea a = UnifiedDockArea::Center,
                  wxRect* r = nullptr,
                  const std::string& tit = "")
        : type(t), panel(p), area(a), rect(r), title(tit) {}
};

// Layout persistence data
struct LayoutPersistence {
    std::string strategyName;      // Layout strategy name
    std::string layoutData;        // Serialized layout data
    std::string panelStates;       // Panel states and positions
    wxDateTime* lastModified;      // Last modification time (pointer)
    
    LayoutPersistence()
        : strategyName("IDE"), lastModified(nullptr) {}
};


