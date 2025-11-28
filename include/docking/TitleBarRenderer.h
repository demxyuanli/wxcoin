#pragma once

#include <wx/wx.h>
#include <wx/dc.h>
#include <vector>
#include "DockArea.h"

namespace ads {

class DockAreaMergedTitleBar;
class DockWidget;

struct TabRenderInfo {
    DockWidget* widget = nullptr;
    wxRect rect;
    wxRect closeButtonRect;
    bool isCurrent = false;
    bool isHovered = false;
    bool closeButtonHovered = false;
    bool showCloseButton = true;
};

struct ButtonRenderInfo {
    wxRect rect;
    bool isHovered = false;
    wxString iconName;
};

class TitleBarRenderer {
public:
    TitleBarRenderer();
    virtual ~TitleBarRenderer();

    void renderBackground(wxDC& dc, const wxRect& clientRect);
    void renderTab(wxDC& dc, const TabRenderInfo& tabInfo, TabPosition position);
    void renderButton(wxDC& dc, const ButtonRenderInfo& buttonInfo, const DockStyleConfig& style);
    void renderButtons(wxDC& dc, const wxRect& clientRect, 
                      const std::vector<ButtonRenderInfo>& buttons, 
                      TabPosition position, const DockStyleConfig& style);
    void renderTitleBarPattern(wxDC& dc, const wxRect& rect, 
                               const std::vector<TabRenderInfo>& tabs,
                               const std::vector<ButtonRenderInfo>& buttons);
    void renderOverflowButton(wxDC& dc, const wxRect& rect, const DockStyleConfig& style);

private:
    void renderHorizontalTab(wxDC& dc, const TabRenderInfo& tabInfo, const DockStyleConfig& style);
    void renderVerticalTab(wxDC& dc, const TabRenderInfo& tabInfo, const DockStyleConfig& style);
    void renderHorizontalButtons(wxDC& dc, const wxRect& clientRect,
                                const std::vector<ButtonRenderInfo>& buttons,
                                const DockStyleConfig& style);
    void renderVerticalButtons(wxDC& dc, const wxRect& clientRect,
                              const std::vector<ButtonRenderInfo>& buttons,
                              const DockStyleConfig& style);
};

} // namespace ads

