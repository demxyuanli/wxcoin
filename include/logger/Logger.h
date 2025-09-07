#ifndef LOGGER_H
#define LOGGER_H

#include <wx/wx.h>
#include <fstream>
#include <string>
#include <set>

class Logger {
public:
	enum class LogLevel { INF, DBG, WRN, ERR };

	static Logger& getLogger();
	void SetOutputCtrl(wxTextCtrl* ctrl);
	void Log(LogLevel level, const std::string& message, const std::string& context = "",
		const std::string& file = "", int line = 0);

	// Helper methods for wxString conversion
	void LogWx(LogLevel level, const wxString& message, const wxString& context = wxEmptyString,
		const std::string& file = "", int line = 0) {
		Log(level, message.ToStdString(), context.ToStdString(), file, line);
	}
	void Shutdown();
	void SetLogLevels(const std::set<LogLevel>& levels, bool isSingleLevel); // Set allowed log levels
	bool ShouldLog(LogLevel level) const; // Check if a level should be logged

private:
	Logger();
	~Logger();
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	std::ofstream logFile;
	wxTextCtrl* logCtrl;
	bool isShuttingDown = false;
	std::set<LogLevel> allowedLogLevels; // Set of allowed log levels
	bool isSingleLevelMode = false; // True for single-level mode (log level and above)
};

// Macros that explicitly use std::string conversion
#define LOG_INF(message, context) Logger::getLogger().Log(Logger::LogLevel::INF, std::string(message), std::string(context), __FILE__, __LINE__)
#define LOG_DBG(message, context) Logger::getLogger().Log(Logger::LogLevel::DBG, std::string(message), std::string(context), __FILE__, __LINE__)
#define LOG_WRN(message, context) Logger::getLogger().Log(Logger::LogLevel::WRN, std::string(message), std::string(context), __FILE__, __LINE__)
#define LOG_ERR(message, context) Logger::getLogger().Log(Logger::LogLevel::ERR, std::string(message), std::string(context), __FILE__, __LINE__)

// Compatibility macros for single parameter (no context)
#define LOG_INF_S(message) Logger::getLogger().Log(Logger::LogLevel::INF, std::string(message), "", __FILE__, __LINE__)
#define LOG_DBG_S(message) Logger::getLogger().Log(Logger::LogLevel::DBG, std::string(message), "", __FILE__, __LINE__)
#define LOG_WRN_S(message) Logger::getLogger().Log(Logger::LogLevel::WRN, std::string(message), "", __FILE__, __LINE__)
#define LOG_ERR_S(message) Logger::getLogger().Log(Logger::LogLevel::ERR, std::string(message), "", __FILE__, __LINE__)

// Additional macros for wxString
#define LOG_INF_WX(message, context) Logger::getLogger().LogWx(Logger::LogLevel::INF, message, context, __FILE__, __LINE__)
#define LOG_DBG_WX(message, context) Logger::getLogger().LogWx(Logger::LogLevel::DBG, message, context, __FILE__, __LINE__)
#define LOG_WRN_WX(message, context) Logger::getLogger().LogWx(Logger::LogLevel::WRN, message, context, __FILE__, __LINE__)
#define LOG_ERR_WX(message, context) Logger::getLogger().LogWx(Logger::LogLevel::ERR, message, context, __FILE__, __LINE__)

#endif