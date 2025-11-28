#include "docking/ButtonLayoutCalculator.h"

namespace ads {

ButtonLayoutCalculator::ButtonLayoutCalculator() {
}

ButtonLayoutCalculator::~ButtonLayoutCalculator() {
}

void ButtonLayoutCalculator::calculateLayout(
    const wxSize& clientSize,
    TabPosition tabPosition,
    int buttonSize,
    int buttonSpacing,
    bool showCloseButton,
    bool showAutoHideButton,
    bool showPinButton,
    bool showLockButton,
    ButtonLayoutInfo& layoutInfo
) {
    wxRect clientRect(0, 0, clientSize.GetWidth(), clientSize.GetHeight());

    switch (tabPosition) {
        case TabPosition::Top:
        case TabPosition::Bottom: {
            int buttonX = clientRect.GetWidth();
            int buttonHeight = clientRect.GetHeight() - 1;
            int buttonY = 0;

            if (showAutoHideButton) {
                buttonX -= buttonSize;
                layoutInfo.autoHideButtonRect = wxRect(buttonX, buttonY, buttonSize, buttonHeight);
                buttonX -= buttonSpacing;
            } else {
                layoutInfo.autoHideButtonRect = wxRect();
            }

            if (showCloseButton) {
                buttonX -= buttonSize;
                layoutInfo.closeButtonRect = wxRect(buttonX, buttonY, buttonSize, buttonHeight);
                buttonX -= buttonSpacing;
            } else {
                layoutInfo.closeButtonRect = wxRect();
            }

            if (showPinButton) {
                buttonX -= buttonSize;
                layoutInfo.pinButtonRect = wxRect(buttonX, buttonY, buttonSize, buttonHeight);
                buttonX -= buttonSpacing;
            } else {
                layoutInfo.pinButtonRect = wxRect();
            }

            if (showLockButton) {
                buttonX -= buttonSize;
                layoutInfo.lockButtonRect = wxRect(buttonX, buttonY, buttonSize, buttonHeight);
            } else {
                layoutInfo.lockButtonRect = wxRect();
            }
            break;
        }
        case TabPosition::Left:
        case TabPosition::Right: {
            int buttonY = clientRect.GetHeight();
            int buttonWidth = clientRect.GetWidth() - 1;
            int buttonX = 0;

            if (showAutoHideButton) {
                buttonY -= buttonSize;
                layoutInfo.autoHideButtonRect = wxRect(buttonX, buttonY, buttonWidth, buttonSize);
                buttonY -= buttonSpacing;
            } else {
                layoutInfo.autoHideButtonRect = wxRect();
            }

            if (showCloseButton) {
                buttonY -= buttonSize;
                layoutInfo.closeButtonRect = wxRect(buttonX, buttonY, buttonWidth, buttonSize);
                buttonY -= buttonSpacing;
            } else {
                layoutInfo.closeButtonRect = wxRect();
            }

            if (showPinButton) {
                buttonY -= buttonSize;
                layoutInfo.pinButtonRect = wxRect(buttonX, buttonY, buttonWidth, buttonSize);
                buttonY -= buttonSpacing;
            } else {
                layoutInfo.pinButtonRect = wxRect();
            }

            if (showLockButton) {
                buttonY -= buttonSize;
                layoutInfo.lockButtonRect = wxRect(buttonX, buttonY, buttonWidth, buttonSize);
            } else {
                layoutInfo.lockButtonRect = wxRect();
            }
            break;
        }
    }
}

} // namespace ads

