#include "config/Coin3DConfig.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"

Coin3DConfig& Coin3DConfig::getInstance() {
	static Coin3DConfig instance;
	return instance;
}

void Coin3DConfig::initialize(ConfigManager& configManager) {
	this->configManager = &configManager;
	LOG_INF("Coin3D configuration initialized", "Coin3DConfig");
}

std::string Coin3DConfig::getSceneGraphPath() const {
	return configManager->getString(SECTION_NAME, KEY_SCENE_GRAPH_PATH, "");
}

void Coin3DConfig::setSceneGraphPath(const std::string& path) {
	configManager->setString(SECTION_NAME, KEY_SCENE_GRAPH_PATH, path);
	configManager->save();
}

bool Coin3DConfig::getAutoSaveEnabled() const {
	return configManager->getBool(SECTION_NAME, KEY_AUTO_SAVE_ENABLED, true);
}

void Coin3DConfig::setAutoSaveEnabled(bool enabled) {
	configManager->setBool(SECTION_NAME, KEY_AUTO_SAVE_ENABLED, enabled);
	configManager->save();
}

int Coin3DConfig::getAutoSaveInterval() const {
	return configManager->getInt(SECTION_NAME, KEY_AUTO_SAVE_INTERVAL, 5);
}

void Coin3DConfig::setAutoSaveInterval(int minutes) {
	configManager->setInt(SECTION_NAME, KEY_AUTO_SAVE_INTERVAL, minutes);
	configManager->save();
}

std::string Coin3DConfig::getDefaultMaterial() const {
	return configManager->getString(SECTION_NAME, KEY_DEFAULT_MATERIAL, "Default");
}

void Coin3DConfig::setDefaultMaterial(const std::string& material) {
	configManager->setString(SECTION_NAME, KEY_DEFAULT_MATERIAL, material);
	configManager->save();
}