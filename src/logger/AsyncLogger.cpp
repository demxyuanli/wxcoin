#include "logger/AsyncLogger.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <wx/app.h>

AsyncLogger::AsyncLogger() : logCtrl(nullptr) {
    // Generate log file name with timestamp
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);
    logFileName = "app_async_" + std::string(timestamp) + ".log";

    logFile.open(logFileName, std::ios::out | std::ios::trunc);
    if (!logFile.is_open()) {
        std::cerr << "Error: Failed to open async log file '" << logFileName << "'" << std::endl;
        throw std::runtime_error("Failed to open async log file");
    }
    
    allowedLogLevels = { LogLevel::ERR, LogLevel::WRN, LogLevel::DBG, LogLevel::INF };

    // Start worker thread
    workerThread_ = std::thread(&AsyncLogger::workerThread, this);
    
    // Log initialization message
    Log(LogLevel::INF, "AsyncLogger initialized, output file: " + logFileName, "AsyncLogger");
}

AsyncLogger::~AsyncLogger() {
    Shutdown();
}

AsyncLogger& AsyncLogger::getLogger() {
    static AsyncLogger instance;
    return instance;
}

void AsyncLogger::SetOutputCtrl(wxTextCtrl* ctrl) {
    std::lock_guard<std::mutex> lock(queueMutex);
    logCtrl = ctrl;
}

void AsyncLogger::SetLogLevels(const std::set<LogLevel>& levels, bool isSingleLevel) {
    std::lock_guard<std::mutex> lock(queueMutex);
    isSingleLevelMode = isSingleLevel;
    allowedLogLevels.clear();

    if (levels.empty()) {
        // Empty config: only ERR
        allowedLogLevels.insert(LogLevel::ERR);
    }
    else if (isSingleLevel && levels.size() == 1) {
        // Single level: include the specified level and above
        auto level = *levels.begin();
        if (level <= LogLevel::INF) allowedLogLevels.insert(LogLevel::INF);
        if (level <= LogLevel::DBG) allowedLogLevels.insert(LogLevel::DBG);
        if (level <= LogLevel::WRN) allowedLogLevels.insert(LogLevel::WRN);
        allowedLogLevels.insert(LogLevel::ERR); // Always include ERR
    }
    else {
        // Multiple levels: include only specified levels and ERR
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
    Log(LogLevel::INF, "Allowed log levels set to: " + (levelsStr.empty() ? "none" : levelsStr), "AsyncLogger");
}

bool AsyncLogger::ShouldLog(LogLevel level) const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return allowedLogLevels.find(level) != allowedLogLevels.end();
}

void AsyncLogger::Log(LogLevel level, const std::string& message, const std::string& context,
                     const std::string& file, int line) {
    if (!ShouldLog(level)) return; // Skip if level is not allowed

    // Check queue size to prevent memory overflow
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (logQueue.size() >= maxQueueSize.load()) {
            // Drop oldest entries if queue is full
            while (logQueue.size() >= maxQueueSize.load() / 2) {
                logQueue.pop();
            }
        }
        
        // Add new entry to queue
        logQueue.emplace(level, message, context, file, line);
    }
    
    // Notify worker thread
    queueCondition.notify_one();
}

void AsyncLogger::Shutdown() {
    if (isShuttingDown.exchange(true)) {
        return; // Already shutting down
    }
    
    // Signal worker thread to stop
    shouldStop.store(true);
    queueCondition.notify_all();
    
    // Wait for worker thread to finish
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    
    // Flush any pending UI logs before shutdown
    flushPendingLogs();
    
    // Close log file
    if (logFile.is_open()) {
        logFile.close();
    }
    
    logCtrl = nullptr;
}

void AsyncLogger::workerThread() {
    while (!shouldStop.load()) {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        // Wait for entries or shutdown signal
        queueCondition.wait(lock, [this] { 
            return !logQueue.empty() || shouldStop.load(); 
        });
        
        // Process all available entries
        while (!logQueue.empty()) {
            LogEntry entry = std::move(logQueue.front());
            logQueue.pop();
            lock.unlock();
            
            // Process entry (I/O operations outside of lock)
            processLogEntry(entry);
            totalLogged.fetch_add(1);
            
            lock.lock();
        }
    }
}

void AsyncLogger::processLogEntry(const LogEntry& entry) {
    std::string logMessage = formatLogMessage(entry);
    
    // File output (if enabled)
    if (enableFileOutput.load()) {
        if (!logFile.is_open()) {
            logFile.open(logFileName, std::ios::out | std::ios::app);
        }
        if (logFile.is_open()) {
            logFile << logMessage << std::endl;
            // Flush periodically to ensure data is written
            static int flushCounter = 0;
            if (++flushCounter % 100 == 0) {
                logFile.flush();
            }
        }
    }
    
    // Console output (if enabled)
    if (enableConsoleOutput.load()) {
        std::cout << "AsyncLogger: " << logMessage << std::endl;
    }
    
    // UI output (if enabled and available) - show only 1 log every 500 logs
    if (enableUIOutput.load() && !isShuttingDown.load()) {
        static std::atomic<int> uiUpdateCounter{0};
        
        // Show only 1 log every N logs
        bool shouldUpdate = (++uiUpdateCounter % uiUpdateInterval.load() == 0);
        
        if (shouldUpdate) {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (logCtrl && logCtrl->IsShown() && wxTheApp) {
                // Show only the current log message
                wxTheApp->CallAfter([this, logMessage]() {
                    if (logCtrl && !isShuttingDown.load()) {
                        logCtrl->AppendText(logMessage + "\n");
                    }
                });
            }
        }
    }
}

std::string AsyncLogger::formatLogMessage(const LogEntry& entry) {
    // Convert timestamp to string
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    auto tm = *std::localtime(&time_t);
    
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);
    
    std::string levelStr;
    switch (entry.level) {
    case LogLevel::INF: levelStr = "INF"; break;
    case LogLevel::DBG: levelStr = "DBG"; break;
    case LogLevel::WRN: levelStr = "WRN"; break;
    case LogLevel::ERR: levelStr = "ERR"; break;
    }
    
    std::string contextStr = entry.context.empty() ? "" : "[" + entry.context + "] ";
    
    std::string fileInfo;
    if (!entry.file.empty()) {
        std::string filename = std::filesystem::path(entry.file).filename().string();
        fileInfo = " (" + filename + ":" + std::to_string(entry.line) + ")";
    }
    
    return "[" + std::string(timestamp) + "] [" + levelStr + "] " +
           contextStr + entry.message + fileInfo;
}

size_t AsyncLogger::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return logQueue.size();
}

size_t AsyncLogger::getTotalLogged() const {
    return totalLogged.load();
}

void AsyncLogger::setMaxQueueSize(size_t maxSize) {
    maxQueueSize.store(maxSize);
}

void AsyncLogger::flushPendingLogs() {
    // Force flush any pending UI logs
    if (enableUIOutput.load() && !isShuttingDown.load()) {
        // Since we now show only 1 log every 500, there's nothing to flush
        // The last log will be shown when the counter reaches the interval
        std::lock_guard<std::mutex> lock(queueMutex);
        if (logCtrl && logCtrl->IsShown() && wxTheApp) {
            // Force show the last log if we're close to the interval
            static std::atomic<int> uiUpdateCounter{0};
            if (uiUpdateCounter.load() % uiUpdateInterval.load() != 0) {
                // Show a completion message
                wxTheApp->CallAfter([this]() {
                    if (logCtrl && !isShuttingDown.load()) {
                        logCtrl->AppendText("[AsyncLogger] Computation completed\n");
                    }
                });
            }
        }
    }
}

void AsyncLogger::setUIUpdateInterval(size_t interval) {
    uiUpdateInterval.store(interval);
}
