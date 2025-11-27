#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include "config/LoggerConfig.h"
#include "config/Coin3DConfig.h"
#include "config/ThemeManager.h"
#include "config/SelectionColorConfig.h"
#include "config/SelectionHighlightConfig.h"
#include "config/UnifiedConfigManager.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/ffile.h>

ConfigManager::ConfigManager() : initialized(false) {
}

ConfigManager::~ConfigManager() {
}

ConfigManager& ConfigManager::getInstance() {
	static ConfigManager instance;
	return instance;
}

bool ConfigManager::initialize(const std::string& configFilePath) {
	if (initialized) {
		LOG_WRN("Configuration manager already initialized", "ConfigManager");
		return true;
	}

	// Use provided config file path if available
	if (!configFilePath.empty()) {
		this->configFilePath = configFilePath;
	}
	else {
		// Otherwise, find the config file
		this->configFilePath = findConfigFile();
	}

	// Create a new config file if none found
	if (this->configFilePath.empty()) {
		wxString userConfigDir = wxStandardPaths::Get().GetUserConfigDir();
		wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
		wxString appName = exeFile.GetName();

		wxString configDir = userConfigDir + wxFileName::GetPathSeparator() + appName;
		if (!wxDirExists(configDir)) {
			wxMkdir(configDir);
		}

		this->configFilePath = (configDir + wxFileName::GetPathSeparator() + "config.ini").ToStdString();
	}

	// Verify file exists and is readable
	if (!wxFileExists(this->configFilePath)) {
		LOG_ERR("Config file does not exist: " + this->configFilePath, "ConfigManager");
		// Create an empty config file
		wxFFile file(this->configFilePath, "w");
		if (!file.IsOpened()) {
			LOG_ERR("Failed to create config file: " + this->configFilePath, "ConfigManager");
			return false;
		}
		file.Close();
	}
	else {
		// Check if file is readable
		wxFFile file(this->configFilePath, "r");
		if (!file.IsOpened()) {
			LOG_ERR("Config file is not readable: " + this->configFilePath, "ConfigManager");
			return false;
		}
		file.Close();
	}

	// Initialize configuration object
	fileConfig = std::make_unique<wxFileConfig>(wxEmptyString, wxEmptyString,
		wxString(this->configFilePath), wxEmptyString,
		wxCONFIG_USE_LOCAL_FILE);

	initialized = true;

	// Initialize Logger configuration
	LoggerConfig::getInstance().initialize(*this);

	// Initialize Coin3D configuration
	Coin3DConfig::getInstance().initialize(*this);

	// Initialize Theme manager - this should be done last
	// as it depends on other configurations being loaded first
	ThemeManager::getInstance().initialize(*this);

	// Initialize Selection color configuration
	SelectionColorConfig::getInstance().initialize(*this);

	// Initialize Selection highlight configuration
	SelectionHighlightConfigManager::getInstance().initialize(*this);

	// Initialize Unified Configuration Manager
	UnifiedConfigManager::getInstance().initialize(*this);

	return true;
}

std::string ConfigManager::findConfigFile() {
	// Check current directory
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxFileName exeDir(exePath);
	wxString currentDir = exeDir.GetPath();

	// Check config/config.ini in current directory
	wxString configPath = currentDir + wxFileName::GetPathSeparator() + "config" + wxFileName::GetPathSeparator() + "config.ini";
	if (wxFileExists(configPath)) {
		return configPath.ToStdString();
	}

	// Check user configuration directory
	wxString userConfigDir = wxStandardPaths::Get().GetUserConfigDir();
	wxString appName = exeDir.GetName();
	configPath = userConfigDir + wxFileName::GetPathSeparator() + appName + wxFileName::GetPathSeparator() + "config.ini";
	if (wxFileExists(configPath)) {
		return configPath.ToStdString();
	}

	LOG_WRN("No config file found in config or user config directory", "ConfigManager");
	return "";
}

std::string ConfigManager::getString(const std::string& section, const std::string& key, const std::string& defaultValue) {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return defaultValue;
	}

	wxString value;
	fileConfig->SetPath("/" + wxString(section));
	bool success = fileConfig->Read(wxString(key), &value, wxString(defaultValue));
	return value.ToStdString();
}

int ConfigManager::getInt(const std::string& section, const std::string& key, int defaultValue) {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return defaultValue;
	}

	int value;
	fileConfig->SetPath("/" + wxString(section));
	fileConfig->Read(wxString(key), &value, defaultValue);
	return value;
}

double ConfigManager::getDouble(const std::string& section, const std::string& key, double defaultValue) {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return defaultValue;
	}

	double value;
	fileConfig->SetPath("/" + wxString(section));
	fileConfig->Read(wxString(key), &value, defaultValue);
	return value;
}

bool ConfigManager::getBool(const std::string& section, const std::string& key, bool defaultValue) {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return defaultValue;
	}

	bool value;
	fileConfig->SetPath("/" + wxString(section));

	// Debug: check what wxFileConfig reads
	wxString wxKey = wxString(key);
	wxString readValue;
	bool found = fileConfig->Read(wxKey, &readValue);
	if (found) {
		value = readValue.IsSameAs("true", false);
	} else {
		value = defaultValue;
	}

	return value;
}

void ConfigManager::setString(const std::string& section, const std::string& key, const std::string& value) {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return;
	}

	fileConfig->SetPath("/" + wxString(section));
	fileConfig->Write(wxString(key), wxString(value));
}

void ConfigManager::setInt(const std::string& section, const std::string& key, int value) {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return;
	}

	fileConfig->SetPath("/" + wxString(section));
	fileConfig->Write(wxString(key), value);
}

void ConfigManager::setDouble(const std::string& section, const std::string& key, double value) {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return;
	}

	fileConfig->SetPath("/" + wxString(section));
	fileConfig->Write(wxString(key), value);
}

void ConfigManager::setBool(const std::string& section, const std::string& key, bool value) {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return;
	}

	fileConfig->SetPath("/" + wxString(section));
	fileConfig->Write(wxString(key), value);
}

bool ConfigManager::save() {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return false;
	}

	return fileConfig->Flush();
}

bool ConfigManager::reload() {
	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return false;
	}

	fileConfig = std::make_unique<wxFileConfig>(wxEmptyString, wxEmptyString,
		wxString(configFilePath), wxEmptyString,
		wxCONFIG_USE_LOCAL_FILE);
	return true;
}

std::string ConfigManager::getConfigFilePath() const {
	return configFilePath;
}

std::vector<std::string> ConfigManager::getSections() {
	std::vector<std::string> sections;

	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return sections;
	}

	// Save current path and ensure we're at root
	wxString oldPath = fileConfig->GetPath();
	fileConfig->SetPath("/");

	wxString str;
	long index;
	bool cont = fileConfig->GetFirstGroup(str, index);
	int count = 0;
	while (cont) {
		sections.push_back(str.ToStdString());
		count++;
		cont = fileConfig->GetNextGroup(str, index);
	}

	// Restore path
	fileConfig->SetPath(oldPath);

	return sections;
}

std::vector<std::string> ConfigManager::getKeys(const std::string& section) {
	std::vector<std::string> keys;

	if (!initialized) {
		LOG_ERR("Configuration manager not initialized", "ConfigManager");
		return keys;
	}

	// Save current path
	wxString oldPath = fileConfig->GetPath();

	// Set path to section
	fileConfig->SetPath("/" + wxString(section));

	wxString str;
	long index;
	bool cont = fileConfig->GetFirstEntry(str, index);
	int count = 0;
	while (cont) {
		keys.push_back(str.ToStdString());
		count++;
		cont = fileConfig->GetNextEntry(str, index);
	}

	// Restore path
	fileConfig->SetPath(oldPath);

	return keys;
}