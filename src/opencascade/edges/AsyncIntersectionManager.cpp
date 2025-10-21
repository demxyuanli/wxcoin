#include "edges/AsyncIntersectionManager.h"
#include "flatui/FlatUIStatusBar.h"
#include "logger/Logger.h"
#include <wx/datetime.h>

AsyncIntersectionManager::AsyncIntersectionManager(
    wxFrame* frame,
    FlatUIStatusBar* statusBar,
    wxTextCtrl* messagePanel)
    : m_frame(frame)
    , m_statusBar(statusBar)
    , m_messagePanel(messagePanel)
{
    if (m_frame) {
        m_frame->Bind(wxEVT_INTERSECTION_PROGRESS, 
                     &AsyncIntersectionManager::onProgressUpdate, this);
        m_frame->Bind(wxEVT_INTERSECTION_COMPLETED, 
                     &AsyncIntersectionManager::onComputationCompleted, this);
        m_frame->Bind(wxEVT_INTERSECTION_ERROR, 
                     &AsyncIntersectionManager::onComputationError, this);
        m_frame->Bind(wxEVT_INTERSECTION_PARTIAL_RESULTS,
                     &AsyncIntersectionManager::onPartialResults, this);
    }
    
    LOG_INF_S("AsyncIntersectionManager created");
}

AsyncIntersectionManager::~AsyncIntersectionManager() {
    cancelCurrentComputation();
    
    if (m_frame) {
        m_frame->Unbind(wxEVT_INTERSECTION_PROGRESS, 
                       &AsyncIntersectionManager::onProgressUpdate, this);
        m_frame->Unbind(wxEVT_INTERSECTION_COMPLETED, 
                       &AsyncIntersectionManager::onComputationCompleted, this);
        m_frame->Unbind(wxEVT_INTERSECTION_ERROR, 
                       &AsyncIntersectionManager::onComputationError, this);
        m_frame->Unbind(wxEVT_INTERSECTION_PARTIAL_RESULTS,
                       &AsyncIntersectionManager::onPartialResults, this);
    }
    
    LOG_INF_S("AsyncIntersectionManager destroyed");
}

bool AsyncIntersectionManager::startIntersectionComputation(
    const TopoDS_Shape& shape,
    double tolerance,
    AsyncIntersectionTask::CompletionCallback onComplete,
    AsyncIntersectionTask::PartialResultsCallback onPartialResults,
    size_t batchSize)
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    if (m_currentTask && m_currentTask->isRunning()) {
        LOG_WRN_S("AsyncIntersectionManager: computation already running");
        appendToMessagePanel("[Warning] Intersection computation is already running. "
                           "Please wait or cancel the current task.\n");
        return false;
    }
    
    m_userCompletionCallback = onComplete;
    m_userPartialCallback = onPartialResults;
    
    m_currentTask = std::make_shared<AsyncIntersectionTask>(
        shape,
        tolerance,
        m_frame,
        nullptr,  // Completion handled via event
        nullptr,  // Progress handled via event
        nullptr,  // Partial results handled via event
        nullptr,  // Error handled via event
        batchSize
    );
    
    // Output start message to message panel
    wxDateTime now = wxDateTime::Now();
    std::string timestamp = now.Format("%H:%M:%S").ToStdString();
    
    std::ostringstream startMsg;
    startMsg << "\n[" << timestamp << "] ========================================\n"
             << "[" << timestamp << "] Starting Asynchronous Intersection Computation\n"
             << "[" << timestamp << "] ========================================\n"
             << "[" << timestamp << "] Tolerance: " << tolerance << "\n"
             << "[" << timestamp << "] Status: Initializing...\n";
    appendToMessagePanel(startMsg.str());
    
    // Start task
    bool started = m_currentTask->start();
    
    if (started) {
        LOG_INF_S("AsyncIntersectionManager: computation started");
        updateStatusBarProgress(0, "Starting intersection computation...");
    } else {
        LOG_ERR_S("AsyncIntersectionManager: failed to start computation");
        appendToMessagePanel("[Error] Failed to start intersection computation\n");
    }
    
    return started;
}

void AsyncIntersectionManager::cancelCurrentComputation() {
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    if (m_currentTask && m_currentTask->isRunning()) {
        LOG_INF_S("AsyncIntersectionManager: cancelling computation");
        
        appendToMessagePanel("\n[Info] Cancelling intersection computation...\n");
        
        m_currentTask->cancel();
        m_currentTask->waitForCompletion(5000);  // Wait up to 5 seconds
        
        cleanupCurrentTask();
        
        updateStatusBarProgress(0, "Cancelled");
        appendToMessagePanel("[Info] Computation cancelled successfully\n");
    }
}

bool AsyncIntersectionManager::isComputationRunning() const {
    std::lock_guard<std::mutex> lock(m_taskMutex);
    return m_currentTask && m_currentTask->isRunning();
}

void AsyncIntersectionManager::onProgressUpdate(IntersectionProgressEvent& event) {
    int progress = event.GetProgress();
    std::string message = event.GetMessage();
    std::string details = event.GetDetails();
    
    // Update status bar
    updateStatusBarProgress(progress, message);
    
    // Output details to message panel
    if (!details.empty()) {
        wxDateTime now = wxDateTime::Now();
        std::string timestamp = now.Format("%H:%M:%S").ToStdString();
        
        std::ostringstream output;
        output << "[" << timestamp << "] Progress: " << progress << "%\n";
        
        // Handle multi-line details
        std::istringstream detailsStream(details);
        std::string line;
        while (std::getline(detailsStream, line)) {
            output << "[" << timestamp << "]   " << line << "\n";
        }
        
        appendToMessagePanel(output.str());
    }
}

void AsyncIntersectionManager::onComputationCompleted(IntersectionCompletedEvent& event) {
    LOG_INF_S("AsyncIntersectionManager: computation completed event received");
    
    const auto& points = event.GetPoints();
    
    // Output completion message to message panel
    wxDateTime now = wxDateTime::Now();
    std::string timestamp = now.Format("%H:%M:%S").ToStdString();
    
    std::ostringstream completeMsg;
    completeMsg << "[" << timestamp << "] ========================================\n"
                << "[" << timestamp << "] Intersection Computation COMPLETED\n"
                << "[" << timestamp << "] ========================================\n"
                << "[" << timestamp << "] Result: " << points.size() << " intersection points found\n"
                << "[" << timestamp << "] Status: Success\n"
                << "[" << timestamp << "] Cache: Result cached for future use\n"
                << "[" << timestamp << "] ========================================\n\n";
    appendToMessagePanel(completeMsg.str());
    
    // Update status bar and disable progress gauge
    updateStatusBarProgress(100, "Intersection computation completed");
    
    // Disable progress gauge after completion
    if (m_statusBar) {
        m_statusBar->EnableProgressGauge(false);
    }
    
    // Call user completion callback
    if (m_userCompletionCallback) {
        try {
            m_userCompletionCallback(points);
        } catch (const std::exception& e) {
            LOG_ERR_S("AsyncIntersectionManager: exception in user completion callback - " + 
                     std::string(e.what()));
            appendToMessagePanel("[Error] Failed to render results: " + 
                               std::string(e.what()) + "\n");
        }
    }
    
    // Clean up task
    cleanupCurrentTask();
}

void AsyncIntersectionManager::onComputationError(IntersectionErrorEvent& event) {
    LOG_ERR_S("AsyncIntersectionManager: computation error event received");
    
    std::string errorMessage = event.GetErrorMessage();
    
    // Output error message to message panel
    wxDateTime now = wxDateTime::Now();
    std::string timestamp = now.Format("%H:%M:%S").ToStdString();
    
    std::ostringstream errorMsg;
    errorMsg << "[" << timestamp << "] ========================================\n"
             << "[" << timestamp << "] Intersection Computation FAILED\n"
             << "[" << timestamp << "] ========================================\n"
             << "[" << timestamp << "] Error: " << errorMessage << "\n"
             << "[" << timestamp << "] ========================================\n\n";
    appendToMessagePanel(errorMsg.str());
    
    // Update status bar and disable progress gauge
    updateStatusBarProgress(0, "Intersection computation failed");
    
    // Disable progress gauge after error
    if (m_statusBar) {
        m_statusBar->EnableProgressGauge(false);
    }
    
    cleanupCurrentTask();
}

void AsyncIntersectionManager::onPartialResults(PartialIntersectionResultsEvent& event) {
    const auto& batch = event.GetPartialPoints();
    size_t totalSoFar = event.GetTotalSoFar();
    
    wxDateTime now = wxDateTime::Now();
    std::string timestamp = now.Format("%H:%M:%S").ToStdString();
    
    std::ostringstream batchMsg;
    batchMsg << "[" << timestamp << "] Partial Results: Displayed " 
             << batch.size() << " intersection nodes (" << totalSoFar << " total so far)\n";
    appendToMessagePanel(batchMsg.str());
    
    if (m_userPartialCallback) {
        try {
            m_userPartialCallback(batch, totalSoFar);
        } catch (const std::exception& e) {
            LOG_ERR_S("AsyncIntersectionManager: exception in partial results callback - " + 
                     std::string(e.what()));
        }
    }
}

void AsyncIntersectionManager::updateStatusBarProgress(int progress, const std::string& message) {
    if (m_statusBar) {
        try {
            m_statusBar->EnableProgressGauge(true);
            m_statusBar->SetGaugeRange(100);
            m_statusBar->SetGaugeValue(progress);
            m_statusBar->SetStatusText(wxString::FromUTF8(message), 0);
        } catch (...) {
            LOG_WRN_S("AsyncIntersectionManager: failed to update status bar");
        }
    }
}

void AsyncIntersectionManager::appendToMessagePanel(const std::string& text) {
    if (m_messagePanel) {
        try {
            // Append text in main thread
            wxString wxText = wxString::FromUTF8(text);
            m_messagePanel->AppendText(wxText);
            
            // Scroll to end
            m_messagePanel->ShowPosition(m_messagePanel->GetLastPosition());
        } catch (...) {
            LOG_WRN_S("AsyncIntersectionManager: failed to append to message panel");
        }
    }
}

void AsyncIntersectionManager::cleanupCurrentTask() {
    if (m_currentTask) {
        m_currentTask->waitForCompletion(1000);
        m_currentTask.reset();
    }
    
    m_userCompletionCallback = nullptr;
    m_userPartialCallback = nullptr;
}

