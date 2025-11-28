#include "docking/TabBarRenderer.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"
#include <wx/dcbuffer.h>

namespace ads {

TabBarRenderer::TabBarRenderer() {
}

TabBarRenderer::~TabBarRenderer() {
}

void TabBarRenderer::renderBackground(wxDC& dc, const TabBarRenderContext& context) {
    wxColour tabBarBg = CFG_COLOUR("DockTabBarBgColour");
    dc.SetBrush(wxBrush(tabBarBg));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(context.clientRect);
}

void TabBarRenderer::renderTab(wxDC& dc, const TabBarTabInfo& tabInfo, const DockStyleConfig& style) {
    if (tabInfo.rect.IsEmpty()) {
        return;
    }

    bool isCurrent = tabInfo.isCurrent;

    DrawStyledRect(dc, tabInfo.rect, style, isCurrent, false, false);

    dc.SetFont(style.font);
    SetStyledTextColor(dc, style, isCurrent);

    wxString title = tabInfo.widget->title();
    wxRect textRect = tabInfo.rect;

    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;
    textRect.Deflate(textPadding, 0);

    if (isCurrent && tabInfo.widget->hasFeature(DockWidgetClosable)) {
        textRect.width -= style.buttonSize;
    }

    dc.DrawLabel(title, textRect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    if (isCurrent && tabInfo.widget->hasFeature(DockWidgetClosable)) {
        DrawSvgButton(dc, tabInfo.closeButtonRect, style.closeIconName, style, false);
    }
}

void TabBarRenderer::renderTabs(wxDC& dc, const TabBarRenderContext& context) {
    for (const auto& tab : context.tabs) {
        if (tab.rect.IsEmpty()) {
            continue;
        }
        renderTab(dc, tab, context.style);
    }
}

void TabBarRenderer::renderOverflowButton(wxDC& dc, const wxRect& rect, const DockStyleConfig& style) {
    DrawSvgButton(dc, rect, "down", style, false);
}

void TabBarRenderer::renderHorizontalTab(wxDC& dc, const TabBarTabInfo& tabInfo, const DockStyleConfig& style) {
    renderTab(dc, tabInfo, style);
}

void TabBarRenderer::renderVerticalTab(wxDC& dc, const TabBarTabInfo& tabInfo, const DockStyleConfig& style) {
    renderTab(dc, tabInfo, style);
}

} // namespace ads

