#pragma once

#include <wx/wx.h>
#include "DockArea.h"

namespace ads {

struct ButtonLayoutInfo {
    wxRect pinButtonRect;
    wxRect closeButtonRect;
    wxRect autoHideButtonRect;
    wxRect lockButtonRect;
};

class ButtonLayoutCalculator {
public:
    ButtonLayoutCalculator();
    virtual ~ButtonLayoutCalculator();

    void calculateLayout(
        const wxSize& clientSize,
        TabPosition tabPosition,
        int buttonSize,
        int buttonSpacing,
        bool showCloseButton,
        bool showAutoHideButton,
        bool showPinButton,
        bool showLockButton,
        ButtonLayoutInfo& layoutInfo
    );
};

} // namespace ads

