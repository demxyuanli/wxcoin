// DockLayoutConfig.cpp - Core implementation of DockLayoutConfig class

#include "docking/DockLayoutConfig.h"
#include <wx/config.h>

namespace ads {

// DockLayoutConfig implementation
void DockLayoutConfig::SaveToConfig() {
    wxConfig config("DockLayout");

    config.Write("TopAreaHeight", topAreaHeight);
    config.Write("BottomAreaHeight", bottomAreaHeight);
    config.Write("LeftAreaWidth", leftAreaWidth);
    config.Write("RightAreaWidth", rightAreaWidth);
    config.Write("CenterMinWidth", centerMinWidth);
    config.Write("CenterMinHeight", centerMinHeight);

    config.Write("UsePercentage", usePercentage);
    config.Write("TopAreaPercent", topAreaPercent);
    config.Write("BottomAreaPercent", bottomAreaPercent);
    config.Write("LeftAreaPercent", leftAreaPercent);
    config.Write("RightAreaPercent", rightAreaPercent);

    config.Write("MinAreaSize", minAreaSize);
    config.Write("SplitterWidth", splitterWidth);

    config.Write("ShowTopArea", showTopArea);
    config.Write("ShowBottomArea", showBottomArea);
    config.Write("ShowLeftArea", showLeftArea);
    config.Write("ShowRightArea", showRightArea);

    config.Write("EnableAnimation", enableAnimation);
    config.Write("AnimationDuration", animationDuration);
}

void DockLayoutConfig::LoadFromConfig() {
    wxConfig config("DockLayout");

    config.Read("TopAreaHeight", &topAreaHeight, 150);
    config.Read("BottomAreaHeight", &bottomAreaHeight, 200);
    config.Read("LeftAreaWidth", &leftAreaWidth, 250);
    config.Read("RightAreaWidth", &rightAreaWidth, 250);
    config.Read("CenterMinWidth", &centerMinWidth, 400);
    config.Read("CenterMinHeight", &centerMinHeight, 300);

    config.Read("UsePercentage", &usePercentage, true);   // Default to true for 15/85 layout
    config.Read("TopAreaPercent", &topAreaPercent, 0);    // Default to 0 for 15/85 layout
    config.Read("BottomAreaPercent", &bottomAreaPercent, 20); // Default to 20 for 15/85 layout
    config.Read("LeftAreaPercent", &leftAreaPercent, 15);   // Default to 15 for 15/85 layout
    config.Read("RightAreaPercent", &rightAreaPercent, 0);   // Default to 0 for 15/85 layout

    config.Read("MinAreaSize", &minAreaSize, 100);
    config.Read("SplitterWidth", &splitterWidth, 4);

    config.Read("ShowTopArea", &showTopArea, false);     // Default to false for 15/85 layout
    config.Read("ShowBottomArea", &showBottomArea, true);  // Default to true for 15/85 layout
    config.Read("ShowLeftArea", &showLeftArea, true);      // Default to true for 15/85 layout
    config.Read("ShowRightArea", &showRightArea, false);   // Default to false for 15/85 layout

    config.Read("EnableAnimation", &enableAnimation, true);
    config.Read("AnimationDuration", &animationDuration, 200);
}

} // namespace ads
