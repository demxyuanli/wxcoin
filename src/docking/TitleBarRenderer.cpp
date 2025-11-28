#include "docking/TitleBarRenderer.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "config/SvgIconManager.h"
#include "config/ThemeManager.h"
#include <wx/graphics.h>

namespace ads {

TitleBarRenderer::TitleBarRenderer() {
}

TitleBarRenderer::~TitleBarRenderer() {
}

void TitleBarRenderer::renderBackground(wxDC& dc, const wxRect& clientRect) {
    wxColour bg = CFG_COLOUR("DockTitleBarBgColour");
    dc.SetBrush(wxBrush(bg));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(clientRect);

    dc.SetPen(wxPen(CFG_COLOUR("TabBorderBottomColour"), 1));
    dc.DrawLine(0, clientRect.GetHeight() - 1, clientRect.GetWidth(), clientRect.GetHeight() - 1);
}

void TitleBarRenderer::renderTab(wxDC& dc, const TabRenderInfo& tabInfo, TabPosition position) {
    if (!tabInfo.widget) {
        return;
    }

    const DockStyleConfig& style = GetDockStyleConfig();

    switch (position) {
        case TabPosition::Top:
        case TabPosition::Bottom:
            renderHorizontalTab(dc, tabInfo, style);
            break;
        case TabPosition::Left:
        case TabPosition::Right:
            renderVerticalTab(dc, tabInfo, style);
            break;
    }
}

void TitleBarRenderer::renderHorizontalTab(wxDC& dc, const TabRenderInfo& tabInfo, const DockStyleConfig& style) {
    DrawStyledRect(dc, tabInfo.rect, style, tabInfo.isCurrent, false, false);

    dc.SetFont(style.font);
    SetStyledTextColor(dc, style, tabInfo.isCurrent);

    wxString title = tabInfo.widget->title();
    wxRect textRect = tabInfo.rect;

    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;

    textRect.Deflate(textPadding, 0);
    if (tabInfo.isCurrent && tabInfo.showCloseButton && tabInfo.widget->hasFeature(DockWidgetClosable)) {
        textRect.width -= style.buttonSize;
    }
    dc.DrawLabel(title, textRect, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

    if (tabInfo.isCurrent && tabInfo.showCloseButton && tabInfo.widget->hasFeature(DockWidgetClosable)) {
        DrawSvgButton(dc, tabInfo.closeButtonRect, style.closeIconName, style, tabInfo.closeButtonHovered);
    }
}

void TitleBarRenderer::renderVerticalTab(wxDC& dc, const TabRenderInfo& tabInfo, const DockStyleConfig& style) {
    DrawStyledRect(dc, tabInfo.rect, style, tabInfo.isCurrent, false, false);

    dc.SetFont(style.font);
    SetStyledTextColor(dc, style, tabInfo.isCurrent);

    wxString title = tabInfo.widget->title();
    wxRect textRect = tabInfo.rect;

    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;

    textRect.Deflate(0, textPadding);
    if (tabInfo.isCurrent && tabInfo.showCloseButton && tabInfo.widget->hasFeature(DockWidgetClosable)) {
        textRect.height -= style.buttonSize;
    }

    int textX = textRect.GetLeft() + textRect.GetWidth() / 2;
    int textY = textRect.GetTop() + textRect.GetHeight() / 2;

    int charHeight = dc.GetCharHeight();
    int totalTextHeight = charHeight * title.length();
    int startY = textY - totalTextHeight / 2;

    for (size_t i = 0; i < title.length(); ++i) {
        wxString singleChar = title.substr(i, 1);
        int charY = startY + i * charHeight;
        dc.DrawText(singleChar, textX - dc.GetTextExtent(singleChar).GetWidth() / 2, charY);
    }

    if (tabInfo.isCurrent && tabInfo.showCloseButton && tabInfo.widget->hasFeature(DockWidgetClosable)) {
        DrawSvgButton(dc, tabInfo.closeButtonRect, style.closeIconName, style, tabInfo.closeButtonHovered);
    }
}

void TitleBarRenderer::renderButton(wxDC& dc, const ButtonRenderInfo& buttonInfo, const DockStyleConfig& style) {
    DrawStyledRect(dc, buttonInfo.rect, style, false, buttonInfo.isHovered, false);
    SetStyledTextColor(dc, style, false);
    
    if (!buttonInfo.iconName.IsEmpty()) {
        DrawSvgButton(dc, buttonInfo.rect, buttonInfo.iconName, style, buttonInfo.isHovered);
    }
}

void TitleBarRenderer::renderButtons(wxDC& dc, const wxRect& clientRect,
                                    const std::vector<ButtonRenderInfo>& buttons,
                                    TabPosition position, const DockStyleConfig& style) {
    switch (position) {
        case TabPosition::Top:
        case TabPosition::Bottom:
            renderHorizontalButtons(dc, clientRect, buttons, style);
            break;
        case TabPosition::Left:
        case TabPosition::Right:
            renderVerticalButtons(dc, clientRect, buttons, style);
            break;
    }
}

void TitleBarRenderer::renderHorizontalButtons(wxDC& dc, const wxRect& clientRect,
                                              const std::vector<ButtonRenderInfo>& buttons,
                                              const DockStyleConfig& style) {
    for (const auto& button : buttons) {
        renderButton(dc, button, style);
    }
}

void TitleBarRenderer::renderVerticalButtons(wxDC& dc, const wxRect& clientRect,
                                            const std::vector<ButtonRenderInfo>& buttons,
                                            const DockStyleConfig& style) {
    for (const auto& button : buttons) {
        renderButton(dc, button, style);
    }
}

void TitleBarRenderer::renderTitleBarPattern(wxDC& dc, const wxRect& rect,
                                            const std::vector<TabRenderInfo>& tabs,
                                            const std::vector<ButtonRenderInfo>& buttons) {
    wxPen oldPen = dc.GetPen();
    wxBrush oldBrush = dc.GetBrush();

    int leftX = 0;
    int rightX = rect.width;

    for (const auto& tab : tabs) {
        if (!tab.rect.IsEmpty()) {
            leftX = std::max(leftX, tab.rect.GetRight());
        }
    }

    for (const auto& button : buttons) {
        if (!button.rect.IsEmpty()) {
            rightX = std::min(rightX, button.rect.GetLeft());
        }
    }

    leftX += 8;
    rightX -= 8;

    if (rightX > leftX + 20) {
        const DockStyleConfig& style = GetDockStyleConfig();
        wxColour dotColor = style.patternDotColour;

        dc.SetPen(wxPen(dotColor, 1));
        dc.SetBrush(wxBrush(dotColor));

        const int unitWidth = 3;
        const int unitHeight = 3;
        const int dotSize = 1;

        int centerY = rect.y + (rect.height - unitHeight) / 2;

        int currentX = leftX;
        while (currentX + unitWidth <= rightX) {
            dc.DrawCircle(currentX, centerY, dotSize);
            dc.DrawCircle(currentX, centerY + 2, dotSize);
            dc.DrawCircle(currentX + 2, centerY + 1, dotSize);
            currentX += unitWidth;
        }
    }

    dc.SetPen(oldPen);
    dc.SetBrush(oldBrush);
}

void TitleBarRenderer::renderOverflowButton(wxDC& dc, const wxRect& rect, const DockStyleConfig& style) {
    DrawSvgButton(dc, rect, "down", style, false);
}

} // namespace ads

