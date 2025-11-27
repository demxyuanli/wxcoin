#include "config/LoggerConfig.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <sstream>
#include <algorithm>

LoggerConfig& LoggerConfig::getInstance() {
	static LoggerConfig instance;
	return instance;
}

void LoggerConfig::initialize(ConfigManager& configManager) {
	std::string logLevelStr = configManager.getString("Logger", "LogLevel", "WRN");
	configureLoggerLevels(logLevelStr);
}

void LoggerConfig::configureLoggerLevels(const std::string& logLevelStr) {
	std::set<Logger::LogLevel> logLevels;
	bool isSingleLevel = true;

	// ERR should always be included to ensure error messages are always logged
	logLevels.insert(Logger::LogLevel::ERR);

	// Parse comma-separated log levels
	std::stringstream ss(logLevelStr);
	std::string level;
	while (std::getline(ss, level, ',')) {
		// Trim whitespace
		level.erase(0, level.find_first_not_of(" \t"));
		level.erase(level.find_last_not_of(" \t") + 1);
		// Convert to uppercase for case-insensitive parsing
		std::transform(level.begin(), level.end(), level.begin(), ::toupper);

		if (level == "WRN") {
			logLevels.insert(Logger::LogLevel::WRN);
		}
		else if (level == "DBG") {
			logLevels.insert(Logger::LogLevel::DBG);
		}
		else if (level == "INF") {
			logLevels.insert(Logger::LogLevel::INF);
		}
		else if (level == "ERR") {
			logLevels.insert(Logger::LogLevel::ERR);
		}
		else {
			LOG_WRN("Unknown log level in config: " + level, "LoggerConfig");
		}
	}

	if (logLevels.size() == 1 && logLevels.count(Logger::LogLevel::ERR)) {
		// Only ERR was specified, fallback to default warning level
		logLevels.insert(Logger::LogLevel::WRN);
		isSingleLevel = true;
	}

	isSingleLevel = (logLevelStr.find(',') == std::string::npos);

	// Set log levels in Logger
	Logger::getLogger().SetLogLevels(logLevels, isSingleLevel);
}