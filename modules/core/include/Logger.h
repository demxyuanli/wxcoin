#pragma once

#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    enum class LogLevel {
        INF,
        DBG,
        WAR,
        ERR
    };

    static Logger& getInstance();
    void log(LogLevel level, const std::string& message, const std::string& file = "", int line = 0);
    void setLogFile(const std::string& filename);
    void close();

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream m_logFile;
    std::mutex m_mutex;
    std::string getTimestamp() const;
    std::string levelToString(LogLevel level) const;
};

// Convenience macros for logging
#define LOG_INF(message) Logger::getInstance().log(Logger::LogLevel::INF, message, __FILE__, __LINE__)
#define LOG_DBG(message) Logger::getInstance().log(Logger::LogLevel::DBG, message, __FILE__, __LINE__)
#define LOG_WAR(message) Logger::getInstance().log(Logger::LogLevel::WAR, message, __FILE__, __LINE__)
#define LOG_ERR(message) Logger::getInstance().log(Logger::LogLevel::ERR, message, __FILE__, __LINE__)