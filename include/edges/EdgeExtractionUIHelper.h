#pragma once

#include <wx/wx.h>
#include <wx/cursor.h>
#include <string>
#include <chrono>
#include <functional>

// Forward declarations
class FlatUIStatusBar;
class FlatFrame;

/**
 * @brief UI helper for edge extraction operations
 * 
 * Provides centralized UI feedback during edge extraction:
 * - Status bar progress updates
 * - Waiting cursor management
 * - Performance statistics display
 * - Progress percentage tracking
 */
class EdgeExtractionUIHelper {
public:
    /**
     * @brief Operation statistics
     */
    struct Statistics {
        size_t totalEdges = 0;
        size_t processedEdges = 0;
        size_t intersectionNodes = 0;
        size_t sampledPoints = 0;
        double extractionTime = 0.0;      // seconds
        double intersectionTime = 0.0;    // seconds
        
        std::string toString() const;
    };
    
    /**
     * @brief Construct UI helper from frame
     * @param frame Parent frame (will try to get FlatFrame for status bar access)
     */
    explicit EdgeExtractionUIHelper(wxFrame* frame = nullptr);
    
    /**
     * @brief Destructor - automatically restores cursor
     */
    ~EdgeExtractionUIHelper();
    
    /**
     * @brief Begin operation - set waiting cursor and enable progress bar
     * @param operationName Name of operation for status display
     */
    void beginOperation(const std::string& operationName);
    
    /**
     * @brief End operation - restore cursor, hide progress, show final stats
     */
    void endOperation();
    
    /**
     * @brief Update progress
     * @param progress Progress percentage (0-100)
     * @param message Status message
     */
    void updateProgress(int progress, const std::string& message);
    
    /**
     * @brief Set indeterminate progress (animated blue bar)
     * @param indeterminate Whether to show indeterminate progress
     * @param message Status message
     */
    void setIndeterminateProgress(bool indeterminate, const std::string& message = "");
    
    /**
     * @brief Set statistics
     * @param stats Operation statistics
     */
    void setStatistics(const Statistics& stats);
    
    /**
     * @brief Get progress callback function
     * @return Callback that can be passed to edge extractors
     */
    std::function<void(int, const std::string&)> getProgressCallback();
    
    /**
     * @brief Show final statistics in status bar
     */
    void showFinalStatistics();
    
    /**
     * @brief Check if UI is available
     */
    bool hasUI() const { return m_statusBar != nullptr; }
    
private:
    wxFrame* m_frame;
    FlatUIStatusBar* m_statusBar;
    wxCursor m_originalCursor;
    bool m_cursorChanged;
    bool m_progressEnabled;
    std::string m_operationName;
    Statistics m_stats;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
    
    void setWaitingCursor();
    void restoreCursor();
    void enableProgressBar();
    void disableProgressBar();
    void updateStatusText(const wxString& text);
};

