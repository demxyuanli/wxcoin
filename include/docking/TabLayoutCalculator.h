#pragma once

#include <wx/wx.h>
#include <vector>
#include "DockArea.h"

namespace ads {

class DockWidget;

struct TabLayoutInfo {
    DockWidget* widget = nullptr;
    wxRect rect;
    wxRect closeButtonRect;
    bool showCloseButton = true;
};

class TabLayoutCalculator {
public:
    TabLayoutCalculator();
    virtual ~TabLayoutCalculator();

    void calculateLayout(const wxSize& containerSize,
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
                        wxRect& outOverflowButtonRect);

private:
    void calculateHorizontalLayout(const wxSize& size,
                                   const std::vector<DockWidget*>& widgets,
                                   int currentIndex,
                                   bool isTop,
                                   const DockStyleConfig& style,
                                   int buttonsWidth,
                                   std::vector<TabLayoutInfo>& outTabs,
                                   bool& outHasOverflow,
                                   int& outFirstVisibleTab,
                                   wxRect& outOverflowButtonRect);
    
    void calculateVerticalLayout(const wxSize& size,
                                const std::vector<DockWidget*>& widgets,
                                int currentIndex,
                                bool isLeft,
                                const DockStyleConfig& style,
                                int buttonsHeight,
                                std::vector<TabLayoutInfo>& outTabs,
                                bool& outHasOverflow,
                                int& outFirstVisibleTab,
                                wxRect& outOverflowButtonRect);
    
    int calculateTabWidth(wxDC& dc, DockWidget* widget, bool isCurrent, 
                         const DockStyleConfig& style, bool showCloseButton);
    int calculateTabHeight(wxDC& dc, DockWidget* widget, bool isCurrent,
                          const DockStyleConfig& style, bool showCloseButton);
};

} // namespace ads

