// DockLayoutConfig.cpp - Core implementation of DockLayoutConfig class

#include "docking/DockLayoutConfig.h"
#include "config/ConfigManager.h"

namespace ads {

// DockLayoutConfig implementation
void DockLayoutConfig::SaveToConfig() {
    ConfigManager& config = ConfigManager::getInstance();
    const std::string section = "DockLayout";

    config.setInt(section, "TopAreaHeight", topAreaHeight);
    config.setInt(section, "BottomAreaHeight", bottomAreaHeight);
    config.setInt(section, "LeftAreaWidth", leftAreaWidth);
    config.setInt(section, "RightAreaWidth", rightAreaWidth);
    config.setInt(section, "CenterMinWidth", centerMinWidth);
    config.setInt(section, "CenterMinHeight", centerMinHeight);

    config.setBool(section, "UsePercentage", usePercentage);
    config.setInt(section, "TopAreaPercent", topAreaPercent);
    config.setInt(section, "BottomAreaPercent", bottomAreaPercent);
    config.setInt(section, "LeftAreaPercent", leftAreaPercent);
    config.setInt(section, "RightAreaPercent", rightAreaPercent);

    config.setInt(section, "MinAreaSize", minAreaSize);
    config.setInt(section, "SplitterWidth", splitterWidth);

    config.setBool(section, "ShowTopArea", showTopArea);
    config.setBool(section, "ShowBottomArea", showBottomArea);
    config.setBool(section, "ShowLeftArea", showLeftArea);
    config.setBool(section, "ShowRightArea", showRightArea);

    config.setBool(section, "EnableAnimation", enableAnimation);
    config.setInt(section, "AnimationDuration", animationDuration);
    
    config.save();
}

void DockLayoutConfig::LoadFromConfig() {
    ConfigManager& config = ConfigManager::getInstance();
    const std::string section = "DockLayout";

    topAreaHeight = config.getInt(section, "TopAreaHeight", 150);
    bottomAreaHeight = config.getInt(section, "BottomAreaHeight", 200);
    leftAreaWidth = config.getInt(section, "LeftAreaWidth", 250);
    rightAreaWidth = config.getInt(section, "RightAreaWidth", 250);
    centerMinWidth = config.getInt(section, "CenterMinWidth", 400);
    centerMinHeight = config.getInt(section, "CenterMinHeight", 300);

    usePercentage = config.getBool(section, "UsePercentage", true);   // Default to true for 15/85 layout
    topAreaPercent = config.getInt(section, "TopAreaPercent", 0);    // Default to 0 for 15/85 layout
    bottomAreaPercent = config.getInt(section, "BottomAreaPercent", 20); // Default to 20 for 15/85 layout
    leftAreaPercent = config.getInt(section, "LeftAreaPercent", 15);   // Default to 15 for 15/85 layout
    rightAreaPercent = config.getInt(section, "RightAreaPercent", 0);   // Default to 0 for 15/85 layout

    minAreaSize = config.getInt(section, "MinAreaSize", 100);
    splitterWidth = config.getInt(section, "SplitterWidth", 4);

    showTopArea = config.getBool(section, "ShowTopArea", false);     // Default to false for 15/85 layout
    showBottomArea = config.getBool(section, "ShowBottomArea", true);  // Default to true for 15/85 layout
    showLeftArea = config.getBool(section, "ShowLeftArea", true);      // Default to true for 15/85 layout
    showRightArea = config.getBool(section, "ShowRightArea", false);   // Default to false for 15/85 layout

    enableAnimation = config.getBool(section, "EnableAnimation", true);
    animationDuration = config.getInt(section, "AnimationDuration", 200);
}

} // namespace ads
