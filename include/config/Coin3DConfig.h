#ifndef COIN3D_CONFIG_H
#define COIN3D_CONFIG_H

#include "config/ConfigManager.h"
#include <string>

class Coin3DConfig {
public:
    static Coin3DConfig& getInstance();
    void initialize(ConfigManager& configManager);

    // 获取和设置Coin3D相关配置
    std::string getSceneGraphPath() const;
    void setSceneGraphPath(const std::string& path);
    
    bool getAutoSaveEnabled() const;
    void setAutoSaveEnabled(bool enabled);
    
    int getAutoSaveInterval() const;
    void setAutoSaveInterval(int minutes);
    
    std::string getDefaultMaterial() const;
    void setDefaultMaterial(const std::string& material);

private:
    Coin3DConfig() = default;
    ConfigManager* configManager = nullptr;
    
    static constexpr const char* SECTION_NAME = "Coin3D";
    static constexpr const char* KEY_SCENE_GRAPH_PATH = "SceneGraphPath";
    static constexpr const char* KEY_AUTO_SAVE_ENABLED = "AutoSaveEnabled";
    static constexpr const char* KEY_AUTO_SAVE_INTERVAL = "AutoSaveInterval";
    static constexpr const char* KEY_DEFAULT_MATERIAL = "DefaultMaterial";
};

#endif // COIN3D_CONFIG_H 