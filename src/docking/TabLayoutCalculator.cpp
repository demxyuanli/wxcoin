#include "docking/TabLayoutCalculator.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include <wx/dcmemory.h>
#include <algorithm>

namespace ads {

TabLayoutCalculator::TabLayoutCalculator() {
}

TabLayoutCalculator::~TabLayoutCalculator() {
}

void TabLayoutCalculator::calculateLayout(const wxSize& containerSize,
                                         const std::vector<DockWidget*>& widgets,
                                         int currentIndex,
                                         TabPosition position,
                                         const DockStyleConfig& style,
                                         bool showCloseButton,
                                         bool showAutoHideButton,
                                         bool showPinButton,
                                         bool showLockButton,
                                         int buttonSize,
                                         std::vector<TabLayoutInfo>& outTabs,
                                         bool& outHasOverflow,
                                         int& outFirstVisibleTab,
                                         wxRect& outOverflowButtonRect) {
    outTabs.clear();
    outTabs.reserve(widgets.size());

    for (auto* widget : widgets) {
        TabLayoutInfo info;
        info.widget = widget;
        info.showCloseButton = widget && widget->hasFeature(DockWidgetClosable);
        outTabs.push_back(info);
    }

    int buttonsWidth = 0;
    int buttonsHeight = 0;
    if (showPinButton) {
        buttonsWidth += buttonSize;
        buttonsHeight += buttonSize;
    }
    if (showCloseButton) {
        buttonsWidth += buttonSize;
        buttonsHeight += buttonSize;
    }
    if (showAutoHideButton) {
        buttonsWidth += buttonSize;
        buttonsHeight += buttonSize;
    }
    if (showLockButton) {
        buttonsWidth += buttonSize;
        buttonsHeight += buttonSize;
    }

    wxMemoryDC tempDC;
    switch (position) {
        case TabPosition::Top:
            calculateHorizontalLayout(containerSize, widgets, currentIndex, true, style,
                                    buttonsWidth, outTabs, outHasOverflow,
                                    outFirstVisibleTab, outOverflowButtonRect);
            break;
        case TabPosition::Bottom:
            calculateHorizontalLayout(containerSize, widgets, currentIndex, false, style,
                                    buttonsWidth, outTabs, outHasOverflow,
                                    outFirstVisibleTab, outOverflowButtonRect);
            break;
        case TabPosition::Left:
            calculateVerticalLayout(containerSize, widgets, currentIndex, true, style,
                                   buttonsHeight, outTabs, outHasOverflow,
                                   outFirstVisibleTab, outOverflowButtonRect);
            break;
        case TabPosition::Right:
            calculateVerticalLayout(containerSize, widgets, currentIndex, false, style,
                                  buttonsHeight, outTabs, outHasOverflow,
                                  outFirstVisibleTab, outOverflowButtonRect);
            break;
    }
}

void TabLayoutCalculator::calculateHorizontalLayout(const wxSize& size,
                                                   const std::vector<DockWidget*>& widgets,
                                                   int currentIndex,
                                                   bool isTop,
                                                   const DockStyleConfig& style,
                                                   int buttonsWidth,
                                                   std::vector<TabLayoutInfo>& outTabs,
                                                   bool& outHasOverflow,
                                                   int& outFirstVisibleTab,
                                                   wxRect& outOverflowButtonRect) {
    wxMemoryDC tempDC;
    int x = 5;
    int tabHeight = style.tabHeight;
    int tabY = isTop ? 4 : (size.GetHeight() - tabHeight);

    const int overflowButtonWidth = 20;
    int availableWidth = size.GetWidth() - buttonsWidth - x;

    int totalTabsWidth = 0;
    for (size_t i = 0; i < widgets.size(); ++i) {
        bool isCurrent = (static_cast<int>(i) == currentIndex);
        int tabWidth = calculateTabWidth(tempDC, widgets[i], isCurrent, style,
                                        outTabs[i].showCloseButton);
        totalTabsWidth += tabWidth;
    }

    if (totalTabsWidth > availableWidth - overflowButtonWidth - 4) {
        outHasOverflow = true;
        availableWidth -= (overflowButtonWidth + 4);

        if (currentIndex >= 0) {
            int visibleTabsWidth = 0;
            int visibleTabsCount = 0;

            for (int i = outFirstVisibleTab; i < static_cast<int>(widgets.size()); ++i) {
                bool isCurrent = (i == currentIndex);
                int tabWidth = calculateTabWidth(tempDC, widgets[i], isCurrent, style,
                                                outTabs[i].showCloseButton);

                if (visibleTabsWidth + tabWidth > availableWidth) {
                    break;
                }

                visibleTabsWidth += tabWidth;
                visibleTabsCount++;
            }

            if (currentIndex < outFirstVisibleTab) {
                outFirstVisibleTab = currentIndex;
            } else if (currentIndex >= outFirstVisibleTab + visibleTabsCount) {
                outFirstVisibleTab = currentIndex - visibleTabsCount + 1;
                if (outFirstVisibleTab < 0) outFirstVisibleTab = 0;
            }
        }
    } else {
        outHasOverflow = false;
        outFirstVisibleTab = 0;
    }

    int lastTabEndX = 5;
    int tabSpacing = DOCK_INT("TabSpacing");
    if (tabSpacing <= 0) tabSpacing = 4;

    for (int i = outFirstVisibleTab; i < static_cast<int>(widgets.size()); ++i) {
        bool isCurrent = (i == currentIndex);
        int tabWidth = calculateTabWidth(tempDC, widgets[i], isCurrent, style,
                                        outTabs[i].showCloseButton);

        if (x + tabWidth > availableWidth) {
            break;
        }

        outTabs[i].rect = wxRect(x, tabY, tabWidth, tabHeight);

        if (outTabs[i].showCloseButton) {
            int closeSize = style.buttonSize;
            outTabs[i].closeButtonRect = wxRect(
                outTabs[i].rect.GetRight() - closeSize - 3,
                outTabs[i].rect.GetTop() + (tabHeight - closeSize) / 2,
                closeSize,
                closeSize
            );
        }

        lastTabEndX = outTabs[i].rect.GetRight();
        x += tabWidth + tabSpacing;
    }

    if (outHasOverflow) {
        int overflowX = lastTabEndX + 4;
        int maxOverflowX = buttonsWidth > 0 ? (size.GetWidth() - buttonsWidth - 4) : (size.GetWidth() - 4);
        
        if (overflowX + overflowButtonWidth > maxOverflowX) {
            overflowX = maxOverflowX - overflowButtonWidth;
        }
        
        outOverflowButtonRect = wxRect(overflowX, tabY, overflowButtonWidth, tabHeight);
    }
}

void TabLayoutCalculator::calculateVerticalLayout(const wxSize& size,
                                                 const std::vector<DockWidget*>& widgets,
                                                 int currentIndex,
                                                 bool isLeft,
                                                 const DockStyleConfig& style,
                                                 int buttonsHeight,
                                                 std::vector<TabLayoutInfo>& outTabs,
                                                 bool& outHasOverflow,
                                                 int& outFirstVisibleTab,
                                                 wxRect& outOverflowButtonRect) {
    wxMemoryDC tempDC;
    int y = 5;
    int tabWidth = 30;
    int tabX = isLeft ? style.tabTopMargin : (size.GetWidth() - style.tabTopMargin - tabWidth);

    const int overflowButtonHeight = 20;
    int availableHeight = size.GetHeight() - buttonsHeight - y;

    int totalTabsHeight = 0;
    for (size_t i = 0; i < widgets.size(); ++i) {
        bool isCurrent = (static_cast<int>(i) == currentIndex);
        int tabHeight = calculateTabHeight(tempDC, widgets[i], isCurrent, style,
                                           outTabs[i].showCloseButton);
        totalTabsHeight += tabHeight;
    }

    if (totalTabsHeight > availableHeight - overflowButtonHeight - 4) {
        outHasOverflow = true;
        availableHeight -= (overflowButtonHeight + 4);

        if (currentIndex >= 0) {
            int visibleTabsHeight = 0;
            int visibleTabsCount = 0;

            for (int i = outFirstVisibleTab; i < static_cast<int>(widgets.size()); ++i) {
                bool isCurrent = (i == currentIndex);
                int tabHeight = calculateTabHeight(tempDC, widgets[i], isCurrent, style,
                                                  outTabs[i].showCloseButton);

                if (visibleTabsHeight + tabHeight > availableHeight) {
                    break;
                }

                visibleTabsHeight += tabHeight;
                visibleTabsCount++;
            }

            if (currentIndex < outFirstVisibleTab) {
                outFirstVisibleTab = currentIndex;
            } else if (currentIndex >= outFirstVisibleTab + visibleTabsCount) {
                outFirstVisibleTab = currentIndex - visibleTabsCount + 1;
                if (outFirstVisibleTab < 0) outFirstVisibleTab = 0;
            }
        }
    } else {
        outHasOverflow = false;
        outFirstVisibleTab = 0;
    }

    int lastTabEndY = 5;
    int tabSpacing = DOCK_INT("TabSpacing");
    if (tabSpacing <= 0) tabSpacing = 4;

    for (int i = outFirstVisibleTab; i < static_cast<int>(widgets.size()); ++i) {
        bool isCurrent = (i == currentIndex);
        int tabHeight = calculateTabHeight(tempDC, widgets[i], isCurrent, style,
                                          outTabs[i].showCloseButton);

        if (y + tabHeight > availableHeight) {
            break;
        }

        outTabs[i].rect = wxRect(tabX, y, tabWidth, tabHeight);

        if (outTabs[i].showCloseButton) {
            int closeSize = style.buttonSize;
            outTabs[i].closeButtonRect = wxRect(
                outTabs[i].rect.GetLeft() + (tabWidth - closeSize) / 2,
                outTabs[i].rect.GetBottom() - closeSize - 3,
                closeSize,
                closeSize
            );
        }

        lastTabEndY = outTabs[i].rect.GetBottom();
        y += tabHeight + tabSpacing;
    }

    if (outHasOverflow) {
        int overflowY = lastTabEndY + 4;
        int maxOverflowY = buttonsHeight > 0 ? (size.GetHeight() - buttonsHeight - 4) : (size.GetHeight() - 4);
        
        if (overflowY + overflowButtonHeight > maxOverflowY) {
            overflowY = maxOverflowY - overflowButtonHeight;
        }
        
        outOverflowButtonRect = wxRect(tabX, overflowY, tabWidth, overflowButtonHeight);
    }
}

int TabLayoutCalculator::calculateTabWidth(wxDC& dc, DockWidget* widget, bool isCurrent,
                                           const DockStyleConfig& style, bool showCloseButton) {
    if (!widget) return 60;

    wxString title = widget->title();
    wxSize textSize = dc.GetTextExtent(title);
    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;

    int tabWidth = textSize.GetWidth() + textPadding * 2;

    if (isCurrent && showCloseButton && widget->hasFeature(DockWidgetClosable)) {
        tabWidth += style.buttonSize + style.contentMargin;
    }
    return std::max(tabWidth, 60);
}

int TabLayoutCalculator::calculateTabHeight(wxDC& dc, DockWidget* widget, bool isCurrent,
                                            const DockStyleConfig& style, bool showCloseButton) {
    if (!widget) return 30;

    wxString title = widget->title();
    wxSize textSize = dc.GetTextExtent(title);
    int textPadding = DOCK_INT("TabPadding");
    if (textPadding <= 0) textPadding = 8;

    int tabHeight = textSize.GetHeight() + textPadding * 2;

    if (isCurrent && showCloseButton && widget->hasFeature(DockWidgetClosable)) {
        tabHeight += style.buttonSize + style.contentMargin;
    }
    return std::max(tabHeight, 30);
}

} // namespace ads

