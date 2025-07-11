#ifndef LOGGER_CONFIG_H
#define LOGGER_CONFIG_H

#include "logger/Logger.h"
#include "config/ConfigManager.h"

class LoggerConfig {
public:
    static LoggerConfig& getInstance();
    void initialize(ConfigManager& configManager);

private:
    LoggerConfig() = default;
    void configureLoggerLevels(const std::string& logLevelStr);
};

#endif // LOGGER_CONFIG_H