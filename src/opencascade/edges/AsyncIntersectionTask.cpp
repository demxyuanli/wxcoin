#include "edges/AsyncIntersectionTask.h"
#include "edges/extractors/OriginalEdgeExtractor.h"
#include "edges/EdgeGeometryCache.h"
#include "logger/Logger.h"
#include <wx/frame.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

wxDEFINE_EVENT(wxEVT_INTERSECTION_COMPLETED, IntersectionCompletedEvent);
wxDEFINE_EVENT(wxEVT_INTERSECTION_ERROR, IntersectionErrorEvent);
wxDEFINE_EVENT(wxEVT_INTERSECTION_PROGRESS, IntersectionProgressEvent);
wxDEFINE_EVENT(wxEVT_INTERSECTION_PARTIAL_RESULTS, PartialIntersectionResultsEvent);

AsyncIntersectionTask::AsyncIntersectionTask(
    const TopoDS_Shape& shape,
    double tolerance,
    wxFrame* frame,
    CompletionCallback onComplete,
    ProgressCallback onProgress,
    PartialResultsCallback onPartialResults,
    ErrorCallback onError,
    size_t batchSize)
    : m_shape(shape)
    , m_tolerance(tolerance)
    , m_frame(frame)
    , m_onComplete(onComplete)
    , m_onProgress(onProgress)
    , m_onPartialResults(onPartialResults)
    , m_onError(onError)
    , m_batchSize(batchSize)
{
    LOG_INF_S("AsyncIntersectionTask created with batch size: " + std::to_string(batchSize));
}

AsyncIntersectionTask::~AsyncIntersectionTask() {
    cancel();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    LOG_INF_S("AsyncIntersectionTask destroyed");
}

bool AsyncIntersectionTask::start() {
    if (m_isRunning.load()) {
        LOG_WRN_S("AsyncIntersectionTask: already running");
        return false;
    }
    
    m_isRunning.store(true);
    m_isCancelled.store(false);
    m_progress.store(0);
    m_startTime = std::chrono::steady_clock::now();
    
    m_workerThread = std::thread(&AsyncIntersectionTask::workerThreadFunc, this);
    
    LOG_INF_S("AsyncIntersectionTask: worker thread started");
    return true;
}

void AsyncIntersectionTask::cancel() {
    if (m_isRunning.load()) {
        LOG_INF_S("AsyncIntersectionTask: cancelling...");
        m_isCancelled.store(true);
    }
}

bool AsyncIntersectionTask::waitForCompletion(int timeoutMs) {
    if (!m_workerThread.joinable()) {
        return !m_isRunning.load();
    }
    
    if (timeoutMs <= 0) {
        m_workerThread.join();
        return true;
    }
    
    auto start = std::chrono::steady_clock::now();
    while (m_isRunning.load()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeoutMs) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return true;
}

std::string AsyncIntersectionTask::getCurrentMessage() const {
    std::lock_guard<std::mutex> lock(m_messageMutex);
    return m_currentMessage;
}

void AsyncIntersectionTask::workerThreadFunc() {
    try {
        LOG_INF_S("AsyncIntersectionTask: computation started");
        
        updateProgress(0, "Starting intersection computation...", "Initializing edge extraction and intersection detection");
        
        auto intersectionPoints = computeIntersections();
        
        if (m_isCancelled.load()) {
            LOG_INF_S("AsyncIntersectionTask: cancelled by user");
            updateProgress(100, "Cancelled", "Computation was cancelled by user");
            m_isRunning.store(false);
            return;
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_startTime).count();
        
        std::string details = "Intersection computation completed successfully";
        details += "\n  - Found " + std::to_string(intersectionPoints.size()) + " intersection points";
        details += "\n  - Computation time: " + std::to_string(elapsed / 1000.0) + " seconds";
        details += "\n  - Result cached for future use";
        
        updateProgress(100, "Completed", details);
        
        LOG_INF_S("AsyncIntersectionTask: computation completed, found " + 
                  std::to_string(intersectionPoints.size()) + " points in " +
                  std::to_string(elapsed) + "ms");
        
        postCompletionEvent(intersectionPoints);
        
    } catch (const std::exception& e) {
        LOG_ERR_S("AsyncIntersectionTask: exception - " + std::string(e.what()));
        postErrorEvent("Intersection computation failed: " + std::string(e.what()));
    } catch (...) {
        LOG_ERR_S("AsyncIntersectionTask: unknown exception");
        postErrorEvent("Intersection computation failed: unknown error");
    }
    
    m_isRunning.store(false);
}

void AsyncIntersectionTask::updateProgress(int progress, const std::string& message, const std::string& details) {
    m_progress.store(progress);
    
    {
        std::lock_guard<std::mutex> lock(m_messageMutex);
        m_currentMessage = message;
    }
    
    if (m_onProgress) {
        m_onProgress(progress, message, details);
    }
    
    if (m_frame) {
        auto* event = new IntersectionProgressEvent(wxEVT_INTERSECTION_PROGRESS, wxID_ANY, progress, message, details);
        wxQueueEvent(m_frame, event);
    }
}

void AsyncIntersectionTask::postCompletionEvent(const std::vector<gp_Pnt>& points) {
    if (m_frame) {
        auto* event = new IntersectionCompletedEvent(wxEVT_INTERSECTION_COMPLETED, wxID_ANY, points);
        wxQueueEvent(m_frame, event);
    }
    
    if (m_onComplete) {
        m_onComplete(points);
    }
}

void AsyncIntersectionTask::postErrorEvent(const std::string& errorMessage) {
    if (m_frame) {
        auto* event = new IntersectionErrorEvent(wxEVT_INTERSECTION_ERROR, wxID_ANY, errorMessage);
        wxQueueEvent(m_frame, event);
    }
    
    if (m_onError) {
        m_onError(errorMessage);
    }
}

std::vector<gp_Pnt> AsyncIntersectionTask::computeIntersections() {
    updateProgress(5, "Extracting edges...", "Phase 1/3: Extracting edges from CAD geometry");
    
    OriginalEdgeExtractor extractor;
    OriginalEdgeParams params;
    
    params.progressCallback = [this](int edgeProgress, const std::string& edgeMsg) {
        if (m_isCancelled.load()) return;
        
        int totalProgress = 5 + (edgeProgress * 15 / 100);
        std::string details = "Phase 1/3: Extracting edges\n  - " + edgeMsg;
        updateProgress(totalProgress, "Extracting edges...", details);
    };
    
    auto edgePoints = extractor.extract(m_shape, &params);
    
    if (m_isCancelled.load()) {
        return {};
    }
    
    updateProgress(20, "Computing adaptive tolerance...", "Phase 2/3: Analyzing geometry bounds and calculating tolerance");
    
    double adaptiveTolerance = m_tolerance;
    if (m_tolerance < 1e-6) {
        Bnd_Box bbox;
        BRepBndLib::Add(m_shape, bbox);
        if (!bbox.IsVoid()) {
            double xMin, yMin, zMin, xMax, yMax, zMax;
            bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            double diagonal = std::sqrt(
                std::pow(xMax - xMin, 2) +
                std::pow(yMax - yMin, 2) +
                std::pow(zMax - zMin, 2)
            );
            adaptiveTolerance = diagonal * 0.001;
            
            std::ostringstream details;
            details << "Phase 2/3: Adaptive tolerance computed";
            details << "\n  - Bounding box diagonal: " << std::fixed << std::setprecision(3) << diagonal << " units";
            details << "\n  - Adaptive tolerance: " << std::setprecision(6) << adaptiveTolerance << " (0.1% of diagonal)";
            updateProgress(25, "Adaptive tolerance computed", details.str());
        }
    }
    
    if (m_isCancelled.load()) {
        return {};
    }
    
    updateProgress(30, "Checking cache...", "Phase 3/3: Checking if intersection result is cached");
    
    size_t shapeHash = reinterpret_cast<size_t>(m_shape.TShape().get());
    std::ostringstream keyStream;
    keyStream << "intersections_" << shapeHash << "_" << std::fixed << std::setprecision(6) << adaptiveTolerance;
    std::string cacheKey = keyStream.str();
    
    auto& cache = EdgeGeometryCache::getInstance();
    
    updateProgress(35, "Computing intersections...", "Phase 3/3: Cache miss, computing edge intersections using BVH acceleration");
    
    updateProgress(35, "Computing intersections...", "Phase 3/3: Computing edge intersections with progressive display");
    
    std::vector<gp_Pnt> result;
    
    // Check cache first
    auto cachedPoints = cache.tryGetCached(cacheKey);
    if (cachedPoints) {
        LOG_INF_S("AsyncIntersectionTask: using cached intersection points");
        result = *cachedPoints;
        
        // For cached results, send in batches for progressive display
        if (!result.empty()) {
            LOG_INF_S("AsyncIntersectionTask: sending cached results in batches (" + 
                     std::to_string(result.size()) + " points, batch size " + 
                     std::to_string(m_batchSize) + ")");
            
            for (size_t i = 0; i < result.size(); i += m_batchSize) {
                size_t end = std::min(i + m_batchSize, result.size());
                std::vector<gp_Pnt> batch(result.begin() + i, result.begin() + end);
                
                if (m_isCancelled.load()) return {};
                
                // Send batch via event (always send event if frame exists)
                if (m_frame) {
                    wxQueueEvent(m_frame, new PartialIntersectionResultsEvent(
                        wxEVT_INTERSECTION_PARTIAL_RESULTS, wxID_ANY, batch, end));
                }
                
                // Also call callback if provided
                if (m_onPartialResults) {
                    m_onPartialResults(batch, end);
                }
                
                // Update progress during batch sending
                int progress = 40 + static_cast<int>((60.0 * end) / result.size());
                std::string progressMsg = "Sending cached results: " + std::to_string(end) + "/" + std::to_string(result.size());
                updateProgress(progress, progressMsg, "Batch " + std::to_string((i / m_batchSize) + 1));
                
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    } else {
        // Compute with progressive display
        LOG_INF_S("AsyncIntersectionTask: computing intersections with progressive display");
        
        // Custom computation with batch callbacks
        OriginalEdgeExtractor tempExtractor;
        std::vector<gp_Pnt> allPoints;
        
        // We need to modify findEdgeIntersections to support batch callbacks
        // For now, compute all and then send in batches
        tempExtractor.findEdgeIntersections(m_shape, allPoints, adaptiveTolerance);
        
        if (m_isCancelled.load()) return {};
        
        // Send results in batches
        for (size_t i = 0; i < allPoints.size(); i += m_batchSize) {
            size_t end = std::min(i + m_batchSize, allPoints.size());
            std::vector<gp_Pnt> batch(allPoints.begin() + i, allPoints.begin() + end);
            
            if (m_isCancelled.load()) return {};
            
            // Send batch
            m_totalPointsFound = end;
            if (m_frame) {
                wxQueueEvent(m_frame, new PartialIntersectionResultsEvent(
                    wxEVT_INTERSECTION_PARTIAL_RESULTS, wxID_ANY, batch, end));
            }
            if (m_onPartialResults) {
                m_onPartialResults(batch, end);
            }
            
            int progress = 35 + (int)(60.0 * end / allPoints.size());
            std::string msg = "Found " + std::to_string(end) + "/" + std::to_string(allPoints.size()) + " intersections";
            updateProgress(progress, "Computing intersections...", msg);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        result = allPoints;
        
        // Cache the result
        cache.storeCached(cacheKey, result, shapeHash, adaptiveTolerance);
    }
    
    if (m_isCancelled.load()) {
        return {};
    }
    
    updateProgress(95, "Finalizing...", "Phase 3/3: All intersections computed");
    
    return result;
}
