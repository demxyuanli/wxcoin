#ifndef ASYNC_LOGGER_H
#define ASYNC_LOGGER_H

#include <wx/wx.h>
#include <fstream>
#include <string>
#include <set>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

class AsyncLogger {
public:
    enum class LogLevel { INF, DBG, WRN, ERR };

    struct LogEntry {
        LogLevel level;
        std::string message;
        std::string context;
        std::string file;
        int line;
        std::chrono::time_point<std::chrono::system_clock> timestamp;
        
        LogEntry(LogLevel lvl, const std::string& msg, const std::string& ctx = "",
                const std::string& f = "", int ln = 0)
            : level(lvl), message(msg), context(ctx), file(f), line(ln),
              timestamp(std::chrono::system_clock::now()) {}
    };

    static AsyncLogger& getLogger();
    void SetOutputCtrl(wxTextCtrl* ctrl);
    void Log(LogLevel level, const std::string& message, const std::string& context = "",
            const std::string& file = "", int line = 0);
    
    // Helper methods for wxString conversion
    void LogWx(LogLevel level, const wxString& message, const wxString& context = wxEmptyString,
               const std::string& file = "", int line = 0) {
        Log(level, message.ToStdString(), context.ToStdString(), file, line);
    }
    
    void Shutdown();
    void SetLogLevels(const std::set<LogLevel>& levels, bool isSingleLevel);
    bool ShouldLog(LogLevel level) const;
    
    // Performance monitoring
    size_t getQueueSize() const;
    size_t getTotalLogged() const;
    void setMaxQueueSize(size_t maxSize);
    
    // UI batch update control
    void flushPendingLogs(); // Force flush pending UI logs
    void setUIUpdateInterval(size_t interval); // Set UI update interval (default: 500)

private:
    AsyncLogger();
    ~AsyncLogger();
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;

    void workerThread();
    void processLogEntry(const LogEntry& entry);
    std::string formatLogMessage(const LogEntry& entry);
    
    // Thread-safe queue
    std::queue<LogEntry> logQueue;
    mutable std::mutex queueMutex;
    std::condition_variable queueCondition;
    
    // Worker thread
    std::thread workerThread_;
    std::atomic<bool> shouldStop{false};
    
    // Output targets
    std::ofstream logFile;
    std::string logFileName;
    wxTextCtrl* logCtrl;
    std::atomic<bool> isShuttingDown{false};
    
    // Configuration
    std::set<LogLevel> allowedLogLevels;
    bool isSingleLevelMode = false;
    std::atomic<size_t> maxQueueSize{10000}; // Prevent memory overflow
    std::atomic<size_t> totalLogged{0};
    
    // Performance optimization
    std::atomic<bool> enableFileOutput{true};
    std::atomic<bool> enableConsoleOutput{true};
    std::atomic<bool> enableUIOutput{true};
    std::atomic<size_t> uiUpdateInterval{500}; // Update UI every 500 logs
};

// Async logging macros
#define LOG_INF_ASYNC(message, context) AsyncLogger::getLogger().Log(AsyncLogger::LogLevel::INF, std::string(message), std::string(context), __FILE__, __LINE__)
#define LOG_DBG_ASYNC(message, context) AsyncLogger::getLogger().Log(AsyncLogger::LogLevel::DBG, std::string(message), std::string(context), __FILE__, __LINE__)
#define LOG_WRN_ASYNC(message, context) AsyncLogger::getLogger().Log(AsyncLogger::LogLevel::WRN, std::string(message), std::string(context), __FILE__, __LINE__)
#define LOG_ERR_ASYNC(message, context) AsyncLogger::getLogger().Log(AsyncLogger::LogLevel::ERR, std::string(message), std::string(context), __FILE__, __LINE__)

// Compatibility macros for single parameter (no context)
#define LOG_INF_S_ASYNC(message) AsyncLogger::getLogger().Log(AsyncLogger::LogLevel::INF, std::string(message), "", __FILE__, __LINE__)
#define LOG_DBG_S_ASYNC(message) AsyncLogger::getLogger().Log(AsyncLogger::LogLevel::DBG, std::string(message), "", __FILE__, __LINE__)
#define LOG_WRN_S_ASYNC(message) AsyncLogger::getLogger().Log(AsyncLogger::LogLevel::WRN, std::string(message), "", __FILE__, __LINE__)
#define LOG_ERR_S_ASYNC(message) AsyncLogger::getLogger().Log(AsyncLogger::LogLevel::ERR, std::string(message), "", __FILE__, __LINE__)

// Additional macros for wxString
#define LOG_INF_WX_ASYNC(message, context) AsyncLogger::getLogger().LogWx(AsyncLogger::LogLevel::INF, message, context, __FILE__, __LINE__)
#define LOG_DBG_WX_ASYNC(message, context) AsyncLogger::getLogger().LogWx(AsyncLogger::LogLevel::DBG, message, context, __FILE__, __LINE__)
#define LOG_WRN_WX_ASYNC(message, context) AsyncLogger::getLogger().LogWx(AsyncLogger::LogLevel::WRN, message, context, __FILE__, __LINE__)
#define LOG_ERR_WX_ASYNC(message, context) AsyncLogger::getLogger().LogWx(AsyncLogger::LogLevel::ERR, message, context, __FILE__, __LINE__)

#endif // ASYNC_LOGGER_H
