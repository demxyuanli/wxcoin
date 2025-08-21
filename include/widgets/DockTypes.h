#ifndef DOCK_TYPES_H
#define DOCK_TYPES_H

// Dock positions following VS2022 style
// Note: DockPosition is now defined in UnifiedDockTypes.h

// Dock area types
enum class DockArea {
    Left, Right, Top, Bottom, Center, Floating
};

// Dock panel states
enum class DockPanelState {
    Normal, Maximized, Minimized, Floating, Hidden
};

// Tab close behavior
enum class TabCloseMode {
    ShowAlways, ShowOnHover, ShowNever
};

// Drag operation types
enum class DragOperation {
    None, TabReorder, TabDetach, PanelMove, PanelResize
};

#endif // DOCK_TYPES_H
