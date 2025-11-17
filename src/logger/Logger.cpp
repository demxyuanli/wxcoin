#include "logger/Logger.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <filesystem>

Logger::Logger() : logCtrl(nullptr) {
	// Generate log file name with timestamp
	std::time_t now = std::time(nullptr);
	std::tm* timeinfo = std::localtime(&now);
	char timestamp[20];
	std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);
	logFileName = "app_" + std::string(timestamp) + ".log";

	logFile.open(logFileName, std::ios::out | std::ios::trunc);
	if (!logFile.is_open()) {
		std::cerr << "Error: Failed to open log file '" << logFileName << "'" << std::endl;
		throw std::runtime_error("Failed to open log file");
	}
	allowedLogLevels = { LogLevel::ERR, LogLevel::WRN, LogLevel::DBG, LogLevel::INF };

	char displayTimestamp[20];
	std::strftime(displayTimestamp, sizeof(displayTimestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

	logFile << "[" << displayTimestamp << "] [INF] [Logger] Logger initialized, output file: " << logFileName << std::endl;
	logFile.flush();
}

Logger::~Logger() {
	if (logFile.is_open()) {
		logFile.close();
	}
}

Logger& Logger::getLogger() {
	static Logger instance;
	return instance;
}

void Logger::SetOutputCtrl(wxTextCtrl* ctrl) {
	logCtrl = ctrl;
}

void Logger::SetLogLevels(const std::set<LogLevel>& levels, bool isSingleLevel) {
	isSingleLevelMode = isSingleLevel;
	allowedLogLevels.clear();

	if (levels.empty()) {
		// Empty config: only ERR
		allowedLogLevels.insert(LogLevel::ERR);
	}
	else if (isSingleLevel && levels.size() == 1) {
		// Single level: treat as minimum level and include that level and above.
		auto baseLevel = *levels.begin();

		auto getRank = [](LogLevel lvl) {
			switch (lvl) {
			case LogLevel::DBG: return 0;
			case LogLevel::INF: return 1;
			case LogLevel::WRN: return 2;
			case LogLevel::ERR: return 3;
			default: return 3;
			}
		};

		int baseRank = getRank(baseLevel);

		for (LogLevel lvl : { LogLevel::DBG, LogLevel::INF, LogLevel::WRN, LogLevel::ERR }) {
			if (getRank(lvl) >= baseRank) {
				allowedLogLevels.insert(lvl);
			}
		}
	}
	else {
		// Multiple levels: include only specified levels and always ERR
		allowedLogLevels = levels;
		allowedLogLevels.insert(LogLevel::ERR);
	}

	// Log final allowed levels
	std::string levelsStr;
	for (const auto& lvl : allowedLogLevels) {
		switch (lvl) {
		case LogLevel::INF: levelsStr += "INF "; break;
		case LogLevel::DBG: levelsStr += "DBG "; break;
		case LogLevel::WRN: levelsStr += "WRN "; break;
		case LogLevel::ERR: levelsStr += "ERR "; break;
		}
	}
	Log(LogLevel::INF, "Allowed log levels set to: " + (levelsStr.empty() ? "none" : levelsStr), "Logger");
}

bool Logger::ShouldLog(LogLevel level) const {
	return allowedLogLevels.find(level) != allowedLogLevels.end();
}

void Logger::Log(LogLevel level, const std::string& message, const std::string& context,
	const std::string& file, int line) {
	if (!ShouldLog(level)) return; // Skip if level is not allowed

	if (!logFile.is_open()) {
		logFile.open(logFileName, std::ios::out | std::ios::app);
		if (!logFile.is_open()) {
			std::cerr << "Error: Failed to open log file '" << logFileName << "' for writing" << std::endl;
			return;
		}
	}

	std::time_t now = std::time(nullptr);
	std::tm* timeinfo = std::localtime(&now);
	char timestamp[20];
	std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

	std::string levelStr;
	switch (level) {
	case LogLevel::INF: levelStr = "INF"; break;
	case LogLevel::DBG: levelStr = "DBG"; break;
	case LogLevel::WRN: levelStr = "WRN"; break;
	case LogLevel::ERR: levelStr = "ERR"; break;
	}

	std::string contextStr = context.empty() ? "" : "[" + context + "] ";

	std::string fileInfo;
	if (!file.empty()) {
		std::string filename = std::filesystem::path(file).filename().string();
		fileInfo = " (" + filename + ":" + std::to_string(line) + ")";
	}

	std::string logMessage = "[" + std::string(timestamp) + "] [" + levelStr + "] " +
		contextStr + message + fileInfo;

	logFile << logMessage << std::endl;
	std::cout << "Logger: " << logMessage << std::endl;
	logFile.flush();

	if (isShuttingDown || !logCtrl || !logCtrl->IsShown()) {
		return;
	}
	logCtrl->AppendText(logMessage + "\n");
}

void Logger::Shutdown() {
	isShuttingDown = true;
	logCtrl = nullptr;
}