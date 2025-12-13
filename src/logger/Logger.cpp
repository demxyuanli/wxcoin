#include "logger/Logger.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <filesystem>

#ifdef USE_LOG4CXX
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/fileappender.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/level.h>
#include <log4cxx/ndc.h>
#include <log4cxx/logmanager.h>
#include <log4cxx/spi/appenderattachable.h>
// RollingFileAppender is accessed via dynamic cast, no need for separate header
#endif

Logger::Logger() : logCtrl(nullptr) {
#ifdef USE_LOG4CXX
    try {
        // Try to load configuration from file - check multiple possible locations
        std::filesystem::path configPaths[] = {
            "log4cxx.properties",
            "config/log4cxx.properties",
            std::filesystem::current_path() / "log4cxx.properties"
        };
        
        bool configLoaded = false;
        for (const auto& configPath : configPaths) {
            if (std::filesystem::exists(configPath)) {
                try {
                    // Ensure logs directory exists before loading config
                    std::filesystem::path logsDir = "logs";
                    if (!std::filesystem::exists(logsDir)) {
                        std::filesystem::create_directories(logsDir);
                    }
                    
                    log4cxx::PropertyConfigurator::configure(configPath.string());
                    log4cxxLogger = log4cxx::Logger::getLogger("CADVisBird");
                    log4cxxLogger->setLevel(log4cxx::Level::getInfo());
                    
                    // Ensure ImmediateFlush is enabled for all file appenders in Release builds
                    log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
                    log4cxx::AppenderList appenders = rootLogger->getAllAppenders();
                    for (auto appender : appenders) {
                        // Use FileAppender base class - works for both FileAppender and RollingFileAppender
                        log4cxx::FileAppenderPtr fileAppender = 
                            log4cxx::cast<log4cxx::FileAppender>(appender);
                        if (fileAppender) {
                            fileAppender->setImmediateFlush(true);
                        }
                    }
                    
                    configLoaded = true;
                    std::cerr << "log4cxx: Configuration loaded from " << configPath.string() << std::endl;
                    break;
                } catch (const std::exception& e) {
                    std::cerr << "log4cxx: Failed to load config from " << configPath.string() << ": " << e.what() << std::endl;
                }
            }
        }
        
        if (!configLoaded) {
            // Use basic configuration if no config file found
            log4cxx::BasicConfigurator::configure();
            
            // Ensure logs directory exists
            std::filesystem::path logsDir = "logs";
            if (!std::filesystem::exists(logsDir)) {
                std::filesystem::create_directories(logsDir);
            }
            
            // Set up file appender - use logs/app.log
            std::string logFilePath = (logsDir / "app.log").string();
            
            log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
            log4cxx::LayoutPtr layout(new log4cxx::PatternLayout("%d{yyyy-MM-dd HH:mm:ss} [%p] [%c{1}] %m%n"));
            
            log4cxx::FileAppenderPtr fileAppender(new log4cxx::FileAppender(layout, logFilePath, false));
            fileAppender->setImmediateFlush(true);  // Ensure immediate flush in Release builds
            rootLogger->addAppender(fileAppender);
            
            log4cxx::ConsoleAppenderPtr consoleAppender(new log4cxx::ConsoleAppender(layout));
            rootLogger->addAppender(consoleAppender);
            
            rootLogger->setLevel(log4cxx::Level::getInfo());
            
            log4cxxLogger = log4cxx::Logger::getLogger("CADVisBird");
            log4cxxLogger->setLevel(log4cxx::Level::getInfo());
            std::cerr << "log4cxx: Basic configuration initialized, log file: " << logFilePath << std::endl;
        }
    } catch (const log4cxx::helpers::Exception& e) {
        std::cerr << "Failed to initialize log4cxx: " << e.what() << std::endl;
        // Fallback to basic file logging
        log4cxxLogger.reset();
        initializeFallbackLogging();
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize log4cxx: " << e.what() << std::endl;
        log4cxxLogger.reset();
        initializeFallbackLogging();
    } catch (...) {
        std::cerr << "Failed to initialize log4cxx: Unknown exception" << std::endl;
        log4cxxLogger.reset();
        initializeFallbackLogging();
    }
#else
    initializeFallbackLogging();
#endif
    
    allowedLogLevels = { LogLevel::ERR, LogLevel::WRN, LogLevel::DBG, LogLevel::INF };
}

void Logger::initializeFallbackLogging() {
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

    char displayTimestamp[20];
    std::strftime(displayTimestamp, sizeof(displayTimestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    logFile << "[" << displayTimestamp << "] [INF] [Logger] Logger initialized, output file: " << logFileName << std::endl;
    logFile.flush();
}

Logger::~Logger() {
#ifdef USE_LOG4CXX
    log4cxx::LogManager::shutdown();
#else
    if (logFile.is_open()) {
        logFile.close();
    }
#endif
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

    // ERR must ALWAYS be logged regardless of configuration - this is critical for error tracking
    // We ensure this at the start so it's never missed in any code path
    allowedLogLevels.insert(LogLevel::ERR);

    if (levels.empty()) {
        // Empty config: only ERR (already added above)
        // No additional levels to add
    }
    else if (isSingleLevel && levels.size() == 1) {
        // Single level: treat as minimum level and include that level and above.
        // ERR is already included above, now add the cascading levels.
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

        // Add all levels at or above the base level
        for (LogLevel lvl : { LogLevel::DBG, LogLevel::INF, LogLevel::WRN, LogLevel::ERR }) {
            if (getRank(lvl) >= baseRank) {
                allowedLogLevels.insert(lvl);
            }
        }
    }
    else {
        // Multiple levels: include specified levels and always include ERR
        // ERR is already included above, now add the specified levels
        for (LogLevel lvl : levels) {
            allowedLogLevels.insert(lvl);
        }
        // Ensure ERR is explicitly included (redundant but makes intent clear)
        allowedLogLevels.insert(LogLevel::ERR);
    }

#ifdef USE_LOG4CXX
    // Update log4cxx level based on allowed levels
    if (log4cxxLogger) {
        log4cxx::LevelPtr log4cxxLevel = log4cxx::Level::getInfo();
        if (allowedLogLevels.find(LogLevel::DBG) != allowedLogLevels.end()) {
            log4cxxLevel = log4cxx::Level::getDebug();
        } else if (allowedLogLevels.find(LogLevel::INF) != allowedLogLevels.end()) {
            log4cxxLevel = log4cxx::Level::getInfo();
        } else if (allowedLogLevels.find(LogLevel::WRN) != allowedLogLevels.end()) {
            log4cxxLevel = log4cxx::Level::getWarn();
        } else {
            log4cxxLevel = log4cxx::Level::getError();
        }
        log4cxxLogger->setLevel(log4cxxLevel);
    }
#endif

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

#ifdef USE_LOG4CXX
    if (log4cxxLogger) {
        try {
            log4cxx::LoggerPtr logger = log4cxxLogger;
            if (!context.empty()) {
                logger = log4cxx::Logger::getLogger("CADVisBird." + context);
            }

            std::string fullMessage = message;
            if (!file.empty()) {
                std::string filename = std::filesystem::path(file).filename().string();
                fullMessage += " (" + filename + ":" + std::to_string(line) + ")";
            }

            switch (level) {
            case LogLevel::DBG:
                logger->debug(fullMessage);
                break;
            case LogLevel::INF:
                logger->info(fullMessage);
                break;
            case LogLevel::WRN:
                logger->warn(fullMessage);
                break;
            case LogLevel::ERR:
                logger->error(fullMessage);
                break;
            }
            
            // Note: ImmediateFlush is already set to true for all file appenders during initialization
            // log4cxx will automatically flush after each log message, so no manual flush is needed

            // Also output to wxTextCtrl if available
            if (!isShuttingDown && logCtrl && logCtrl->IsShown()) {
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
                std::string logMessage = "[" + std::string(timestamp) + "] [" + levelStr + "] " +
                    contextStr + fullMessage + "\n";
                logCtrl->AppendText(logMessage);
            }
            return;
        } catch (const log4cxx::helpers::Exception& e) {
            // Fallback to file logging if log4cxx fails
        } catch (...) {
            // Fallback to file logging if log4cxx fails
        }
    }
#endif
    // Fallback to file logging if log4cxx is not available or failed
    fallbackLog(level, message, context, file, line);
}

void Logger::fallbackLog(LogLevel level, const std::string& message, const std::string& context,
    const std::string& file, int line) {
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
#ifdef USE_LOG4CXX
    if (log4cxxLogger) {
        log4cxx::LogManager::shutdown();
        log4cxxLogger.reset();
    }
#endif
}
