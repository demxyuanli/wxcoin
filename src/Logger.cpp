#include "Logger.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Default log file
    setLogFile("wxCoin3D.log");
}

Logger::~Logger() {
    close();
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_logFile.open(filename, std::ios::out | std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

void Logger::log(LogLevel level, const std::string& message, const std::string& file, int line) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string logEntry = "[" + getTimestamp() + "] [" + levelToString(level) + "] ";
    if (!file.empty()) {
        logEntry += "[" + file + ":" + std::to_string(line) + "] ";
    }
    logEntry += message;

    // Write to console
    std::cout << logEntry << std::endl;

    // Write to file
    if (m_logFile.is_open()) {
        m_logFile << logEntry << std::endl;
        m_logFile.flush();
    }
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

std::string Logger::getTimestamp() const {
    auto now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
    case LogLevel::INF: return "INF";
    case LogLevel::DBG: return "DBG";
    case LogLevel::WAR: return "WAR";
    case LogLevel::ERR: return "ERR";
    default: return "UNKNOWN";
    }
}