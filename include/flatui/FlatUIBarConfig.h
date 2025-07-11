#ifndef FLATUIBAR_CONFIG_H
#define FLATUIBAR_CONFIG_H

namespace FlatUIBarConfig {
    // Layout constants
    constexpr int FIXED_PANEL_Y = 30;
    constexpr int PIN_BUTTON_MARGIN = 4;
    constexpr int ELEMENT_SPACING = 5;
    constexpr int BAR_PADDING = 2;
    constexpr int TAB_PADDING = 10;
    constexpr int TAB_SPACING = 1;
    
    // Timing constants
    constexpr int AUTO_HIDE_DELAY_MS = 500;
    
    // Size constants
    constexpr int CONTROL_WIDTH = 20;
    constexpr int CONTROL_HEIGHT = 20;
    constexpr int MIN_PANEL_WIDTH = 100;
    constexpr int MIN_PANEL_HEIGHT = 50;
    
    // Visual constants
    constexpr int SHADOW_OFFSET = 1;
    constexpr int BORDER_WIDTH = 1;
    constexpr int CORNER_RADIUS = 4;
    
    // Interaction constants
    constexpr int MOUSE_MARGIN = 5;
    constexpr int DRAG_THRESHOLD = 3;

    // Menu ID range for hidden tabs dropdown
    constexpr int MENU_ID_RANGE_START = 5000;
    constexpr int MENU_ID_RANGE_END = 5100;
}

#endif // FLATUIBAR_CONFIG_H 