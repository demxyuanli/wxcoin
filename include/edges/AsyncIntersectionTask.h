#pragma once

#include <functional>
#include <thread>
#include <atomic>
#include <future>
#include <string>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <wx/event.h>

class wxFrame;

/**
 * @brief Asynchronous intersection computation task
 * 
 * Computes edge intersections in background thread without blocking UI.
 * Supports progress callbacks, completion callbacks, and cancellation.
 * 
 * Features:
 * - Background thread async computation
 * - Real-time progress updates (thread-safe)
 * - Detailed logging to message panel
 * - Automatic result caching
 * - Completion callback for rendering
 * - Task cancellation support
 * 
 * Usage example:
 * @code
 * auto task = std::make_shared<AsyncIntersectionTask>(
 *     shape, tolerance, frame,
 *     [this](const std::vector<gp_Pnt>& points) {
 *         renderIntersections(points);
 *     }
 * );
 * task->start();
 * @endcode
 */
class AsyncIntersectionTask {
public:
    /**
     * @brief Progress callback function type
     * @param progress Progress percentage (0-100)
     * @param message Current status message
     * @param details Detailed information (for message panel)
     */
    using ProgressCallback = std::function<void(int progress, const std::string& message, const std::string& details)>;
    
    /**
     * @brief Partial results callback function type (for progressive display)
     * @param partialPoints Batch of newly computed intersection points
     * @param totalSoFar Total number of points computed so far
     */
    using PartialResultsCallback = std::function<void(const std::vector<gp_Pnt>& partialPoints, size_t totalSoFar)>;
    
    /**
     * @brief Completion callback function type
     * @param intersectionPoints All computed intersection points
     */
    using CompletionCallback = std::function<void(const std::vector<gp_Pnt>& intersectionPoints)>;
    
    /**
     * @brief Error callback function type
     * @param errorMessage Error message
     */
    using ErrorCallback = std::function<void(const std::string& errorMessage)>;
    
    /**
     * @brief Constructor
     * @param shape CAD shape to analyze
     * @param tolerance Intersection detection tolerance
     * @param frame Main window (for UI updates)
     * @param onComplete Completion callback
     * @param onProgress Progress callback (optional)
     * @param onPartialResults Partial results callback for progressive display (optional)
     * @param onError Error callback (optional)
     * @param batchSize Number of points to accumulate before calling partial results callback (default: 50)
     */
    AsyncIntersectionTask(
        const TopoDS_Shape& shape,
        double tolerance,
        wxFrame* frame,
        CompletionCallback onComplete,
        ProgressCallback onProgress = nullptr,
        PartialResultsCallback onPartialResults = nullptr,
        ErrorCallback onError = nullptr,
        size_t batchSize = 50);
    
    ~AsyncIntersectionTask();
    
    /**
     * @brief Start asynchronous computation
     * @return true if started successfully, false if already running
     */
    bool start();
    
    /**
     * @brief Cancel ongoing computation
     */
    void cancel();
    
    /**
     * @brief Check if task is running
     */
    bool isRunning() const { return m_isRunning.load(); }
    
    /**
     * @brief Check if task is cancelled
     */
    bool isCancelled() const { return m_isCancelled.load(); }
    
    /**
     * @brief Wait for task completion
     * @param timeoutMs Timeout in milliseconds, 0 means wait indefinitely
     * @return true if completed, false if timeout
     */
    bool waitForCompletion(int timeoutMs = 0);
    
    /**
     * @brief Get current progress (0-100)
     */
    int getProgress() const { return m_progress.load(); }
    
    /**
     * @brief Get current status message
     */
    std::string getCurrentMessage() const;

private:
    /**
     * @brief Worker thread function
     */
    void workerThreadFunc();
    
    /**
     * @brief Update progress (thread-safe)
     */
    void updateProgress(int progress, const std::string& message, const std::string& details);
    
    /**
     * @brief Post completion event to main thread
     */
    void postCompletionEvent(const std::vector<gp_Pnt>& points);
    
    /**
     * @brief Post error event to main thread
     */
    void postErrorEvent(const std::string& errorMessage);
    
    /**
     * @brief Compute intersection points (core computation logic)
     */
    std::vector<gp_Pnt> computeIntersections();

private:
    TopoDS_Shape m_shape;
    double m_tolerance;
    wxFrame* m_frame;
    
    CompletionCallback m_onComplete;
    ProgressCallback m_onProgress;
    PartialResultsCallback m_onPartialResults;
    ErrorCallback m_onError;
    
    std::thread m_workerThread;
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_isCancelled{false};
    std::atomic<int> m_progress{0};
    
    mutable std::mutex m_messageMutex;
    std::string m_currentMessage;
    
    std::chrono::steady_clock::time_point m_startTime;
    
    size_t m_batchSize;
    std::vector<gp_Pnt> m_batchBuffer;
    std::mutex m_batchMutex;
    size_t m_totalPointsFound{0};
    
    void flushBatch(bool isFinal = false);
};

/**
 * @brief wxWidgets custom event - Intersection computation completed
 */
class IntersectionCompletedEvent : public wxEvent {
public:
    IntersectionCompletedEvent(wxEventType eventType, int winid, const std::vector<gp_Pnt>& points)
        : wxEvent(winid, eventType), m_points(points) {}
    
    wxEvent* Clone() const override { return new IntersectionCompletedEvent(*this); }
    
    const std::vector<gp_Pnt>& GetPoints() const { return m_points; }

private:
    std::vector<gp_Pnt> m_points;
};

/**
 * @brief wxWidgets custom event - Partial intersection results (for progressive display)
 */
class PartialIntersectionResultsEvent : public wxEvent {
public:
    PartialIntersectionResultsEvent(wxEventType eventType, int winid, 
                                   const std::vector<gp_Pnt>& points, size_t totalSoFar)
        : wxEvent(winid, eventType), m_partialPoints(points), m_totalSoFar(totalSoFar) {}
    
    wxEvent* Clone() const override { return new PartialIntersectionResultsEvent(*this); }
    
    const std::vector<gp_Pnt>& GetPartialPoints() const { return m_partialPoints; }
    size_t GetTotalSoFar() const { return m_totalSoFar; }

private:
    std::vector<gp_Pnt> m_partialPoints;
    size_t m_totalSoFar;
};

/**
 * @brief wxWidgets custom event - Intersection computation error
 */
class IntersectionErrorEvent : public wxEvent {
public:
    IntersectionErrorEvent(wxEventType eventType, int winid, const std::string& error)
        : wxEvent(winid, eventType), m_errorMessage(error) {}
    
    wxEvent* Clone() const override { return new IntersectionErrorEvent(*this); }
    
    const std::string& GetErrorMessage() const { return m_errorMessage; }

private:
    std::string m_errorMessage;
};

/**
 * @brief wxWidgets custom event - Progress update
 */
class IntersectionProgressEvent : public wxEvent {
public:
    IntersectionProgressEvent(wxEventType eventType, int winid, 
                             int progress, const std::string& message, const std::string& details)
        : wxEvent(winid, eventType), 
          m_progress(progress), 
          m_message(message),
          m_details(details) {}
    
    wxEvent* Clone() const override { return new IntersectionProgressEvent(*this); }
    
    int GetProgress() const { return m_progress; }
    const std::string& GetMessage() const { return m_message; }
    const std::string& GetDetails() const { return m_details; }

private:
    int m_progress;
    std::string m_message;
    std::string m_details;
};

// Declare custom event types
wxDECLARE_EVENT(wxEVT_INTERSECTION_COMPLETED, IntersectionCompletedEvent);
wxDECLARE_EVENT(wxEVT_INTERSECTION_ERROR, IntersectionErrorEvent);
wxDECLARE_EVENT(wxEVT_INTERSECTION_PROGRESS, IntersectionProgressEvent);
wxDECLARE_EVENT(wxEVT_INTERSECTION_PARTIAL_RESULTS, PartialIntersectionResultsEvent);

