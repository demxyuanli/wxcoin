#pragma once

#include <wx/wx.h>
#include <wx/dc.h>
#include <vector>
#include "DockArea.h"

namespace ads {

class DockAreaTabBar;
class DockWidget;

struct TabBarTabInfo {
    DockWidget* widget = nullptr;
    wxRect rect;
    wxRect closeButtonRect;
    bool isCurrent = false;
    bool isHovered = false;
    bool closeButtonHovered = false;
    bool showCloseButton = true;
};

struct TabBarRenderContext {
    wxRect clientRect;
    TabPosition tabPosition;
    const DockStyleConfig& style;
    std::vector<TabBarTabInfo> tabs;
    wxRect overflowButtonRect;
    bool hasOverflow = false;
};

class TabBarRenderer {
public:
    TabBarRenderer();
    virtual ~TabBarRenderer();

    void renderBackground(wxDC& dc, const TabBarRenderContext& context);
    void renderTab(wxDC& dc, const TabBarTabInfo& tabInfo, const DockStyleConfig& style);
    void renderTabs(wxDC& dc, const TabBarRenderContext& context);
    void renderOverflowButton(wxDC& dc, const wxRect& rect, const DockStyleConfig& style);

private:
    void renderHorizontalTab(wxDC& dc, const TabBarTabInfo& tabInfo, const DockStyleConfig& style);
    void renderVerticalTab(wxDC& dc, const TabBarTabInfo& tabInfo, const DockStyleConfig& style);
};

} // namespace ads

