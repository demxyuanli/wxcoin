#pragma once

#include "AsyncIntersectionTask.h"
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <memory>
#include <mutex>

class FlatUIStatusBar;

/**
 * @brief Asynchronous intersection computation manager
 * 
 * Manages asynchronous intersection computation tasks, handling UI updates,
 * progress display, and message panel output. Automatically connects wxWidgets
 * events and updates UI components.
 * 
 * Features:
 * - Automatic event handler connection
 * - Status bar progress updates
 * - Message panel detailed logging
 * - Task lifecycle management
 * - Thread-safe operations
 * 
 * Usage example:
 * @code
 * auto manager = std::make_shared<AsyncIntersectionManager>(frame, statusBar, messagePanel);
 * 
 * manager->startIntersectionComputation(
 *     shape, tolerance,
 *     [this](const std::vector<gp_Pnt>& points) {
 *         renderIntersectionNodes(points);
 *     }
 * );
 * @endcode
 */
class AsyncIntersectionManager {
public:
    /**
     * @brief Constructor
     * @param frame Main window
     * @param statusBar Status bar for progress display
     * @param messagePanel Message text control for detailed logging
     */
    AsyncIntersectionManager(wxFrame* frame, 
                            FlatUIStatusBar* statusBar = nullptr,
                            wxTextCtrl* messagePanel = nullptr);
    
    ~AsyncIntersectionManager();
    
    /**
     * @brief Start asynchronous intersection computation
     * @param shape CAD shape
     * @param tolerance Tolerance
     * @param onComplete Completion callback
     * @param onPartialResults Partial results callback for progressive display (optional)
     * @param batchSize Batch size for progressive display (default: 50)
     * @return true if started, false if already running
     */
    bool startIntersectionComputation(
        const TopoDS_Shape& shape,
        double tolerance,
        AsyncIntersectionTask::CompletionCallback onComplete,
        AsyncIntersectionTask::PartialResultsCallback onPartialResults = nullptr,
        size_t batchSize = 50);
    
    /**
     * @brief Cancel current computation
     */
    void cancelCurrentComputation();
    
    /**
     * @brief Check if a task is currently running
     */
    bool isComputationRunning() const;
    
    /**
     * @brief Set status bar
     */
    void setStatusBar(FlatUIStatusBar* statusBar) { m_statusBar = statusBar; }
    
    /**
     * @brief Set message panel
     */
    void setMessagePanel(wxTextCtrl* messagePanel) { m_messagePanel = messagePanel; }

private:
    /**
     * @brief Event handler: Progress update
     */
    void onProgressUpdate(IntersectionProgressEvent& event);
    
    /**
     * @brief Event handler: Computation completed
     */
    void onComputationCompleted(IntersectionCompletedEvent& event);
    
    /**
     * @brief Event handler: Computation error
     */
    void onComputationError(IntersectionErrorEvent& event);
    
    /**
     * @brief Event handler: Partial results (for progressive display)
     */
    void onPartialResults(PartialIntersectionResultsEvent& event);
    
    /**
     * @brief Update status bar progress
     */
    void updateStatusBarProgress(int progress, const std::string& message);
    
    /**
     * @brief Append text to message panel
     */
    void appendToMessagePanel(const std::string& text);
    
    /**
     * @brief Clean up current task
     */
    void cleanupCurrentTask();

private:
    wxFrame* m_frame;
    FlatUIStatusBar* m_statusBar;
    wxTextCtrl* m_messagePanel;
    
    std::shared_ptr<AsyncIntersectionTask> m_currentTask;
    mutable std::mutex m_taskMutex;
    
    AsyncIntersectionTask::CompletionCallback m_userCompletionCallback;
    AsyncIntersectionTask::PartialResultsCallback m_userPartialCallback;
};

