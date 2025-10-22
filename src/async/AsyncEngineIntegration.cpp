#include "async/AsyncEngineIntegration.h"
#include "logger/Logger.h"
#include <wx/frame.h>

namespace async {

wxDEFINE_EVENT(wxEVT_ASYNC_INTERSECTION_RESULT, AsyncIntersectionResultEvent);
wxDEFINE_EVENT(wxEVT_ASYNC_MESH_RESULT, AsyncMeshResultEvent);
wxDEFINE_EVENT(wxEVT_ASYNC_TASK_PROGRESS, AsyncEngineResultEvent);

AsyncEngineIntegration::AsyncEngineIntegration(wxFrame* mainFrame)
    : m_headless(false), m_mainFrame(mainFrame)
{
    AsyncComputeEngine::Config config;
    config.numWorkerThreads = 0;
    config.maxQueueSize = 1000;
    config.enableResultCache = true;
    config.maxCacheSize = 100;

    m_engine = std::make_unique<AsyncComputeEngine>(config);
    
    // Bind event handler for intersection results
    if (m_mainFrame) {
        m_mainFrame->Bind(wxEVT_ASYNC_INTERSECTION_RESULT, 
            [this](AsyncIntersectionResultEvent& evt) {
                LOG_INF_S("AsyncEngineIntegration: EVENT HANDLER TRIGGERED!");
                onIntersectionResultEvent(evt);
            });
        LOG_INF_S("AsyncEngineIntegration: Event handler bound to main frame at address: " + 
                 std::to_string(reinterpret_cast<uintptr_t>(m_mainFrame)));
    } else {
        LOG_ERR_S("AsyncEngineIntegration: No main frame to bind event handler!");
    }

    LOG_INF_S("AsyncEngineIntegration: Initialized with GUI mode");
}

AsyncEngineIntegration::AsyncEngineIntegration(bool headless)
    : m_headless(headless), m_mainFrame(nullptr)
{
    AsyncComputeEngine::Config config;
    config.numWorkerThreads = 0;
    config.maxQueueSize = 1000;
    config.enableResultCache = true;
    config.maxCacheSize = 100;

    m_engine = std::make_unique<AsyncComputeEngine>(config);

    LOG_INF_S("AsyncEngineIntegration: Initialized in headless mode");
}

AsyncEngineIntegration::~AsyncEngineIntegration() {
    if (m_engine) {
        m_engine->shutdown();
    }
    LOG_INF_S("AsyncEngineIntegration: Destroyed");
}

void AsyncEngineIntegration::computeIntersectionsAsync(
    const std::string& taskId,
    const TopoDS_Shape& shape,
    double tolerance)
{
    LOG_INF_S("AsyncEngineIntegration: Submitting intersection task " + taskId);
    
    auto task = GeometryComputeTasks::createIntersectionTask(
        taskId,
        shape,
        tolerance,
        [this, taskId](const ComputeResult<IntersectionComputeResult>& result) {
            postIntersectionResult(taskId, result);
        }
    );
    
    task->setProgressCallback([this, taskId](int progress, const std::string& message) {
        (void)progress;
        (void)message;
        auto* event = new AsyncEngineResultEvent(wxEVT_ASYNC_TASK_PROGRESS,
                                                 m_mainFrame ? m_mainFrame->GetId() : 0, taskId);
        safePostEvent(event);
    });
    
    m_engine->submitTask(task);
}

void AsyncEngineIntegration::generateMeshAsync(
    const std::string& taskId,
    const TopoDS_Shape& shape,
    double deflection,
    double angle)
{
    LOG_INF_S("AsyncEngineIntegration: Submitting mesh generation task " + taskId);
    
    auto task = GeometryComputeTasks::createMeshGenerationTask(
        taskId,
        shape,
        deflection,
        angle,
        [this, taskId](const ComputeResult<MeshData>& result) {
            postMeshResult(taskId, result);
        }
    );
    
    m_engine->submitTask(task);
}

void AsyncEngineIntegration::computeBoundingBoxAsync(
    const std::string& taskId,
    const TopoDS_Shape& shape)
{
    LOG_INF_S("AsyncEngineIntegration: Submitting bounding box task " + taskId);
    
    auto task = GeometryComputeTasks::createBoundingBoxTask(
        taskId,
        shape,
        [this, taskId](const ComputeResult<BoundingBoxResult>& result) {
            postBoundingBoxResult(taskId, result);
        }
    );
    
    m_engine->submitTask(task);
}

void AsyncEngineIntegration::cancelTask(const std::string& taskId) {
    m_engine->cancelTask(taskId);
}

void AsyncEngineIntegration::cancelAllTasks() {
    m_engine->cancelAllTasks();
}

TaskStatistics AsyncEngineIntegration::getStatistics() const {
    return m_engine->getStatistics();
}

void AsyncEngineIntegration::postIntersectionResult(
    const std::string& taskId,
    const ComputeResult<IntersectionComputeResult>& result)
{
    if (result.success) {
        LOG_INF_S("AsyncEngineIntegration: Intersection task " + taskId +
                  " completed with " + std::to_string(result.data.points.size()) + " points");

        auto* event = new AsyncIntersectionResultEvent(
            wxEVT_ASYNC_INTERSECTION_RESULT,
            m_mainFrame ? m_mainFrame->GetId() : 0,
            taskId,
            result.data
        );

        safePostEvent(event);
    } else {
        LOG_ERR_S("AsyncEngineIntegration: Intersection task " + taskId +
                  " failed: " + result.errorMessage);
    }
}

void AsyncEngineIntegration::postIntersectionResultWithCallback(
    const std::string& taskId,
    const ComputeResult<IntersectionComputeResult>& result)
{
    LOG_INF_S("AsyncEngineIntegration: Posting intersection result for " + taskId);
    
    // Store result for callback execution
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_pendingResults[taskId] = result;
    }
    
    // Create and post event to main thread
    auto* event = new AsyncIntersectionResultEvent(
        wxEVT_ASYNC_INTERSECTION_RESULT,
        m_mainFrame ? m_mainFrame->GetId() : 0,
        taskId,
        result.data
    );
    
    safePostEvent(event);
}

void AsyncEngineIntegration::safePostEvent(wxEvent* event)
{
    if (!m_headless && m_mainFrame) {
        LOG_INF_S("AsyncEngineIntegration: Posting event to main frame");
        wxQueueEvent(m_mainFrame, event);
    } else {
        LOG_WRN_S("AsyncEngineIntegration: Cannot post event (headless=" + 
                 std::to_string(m_headless) + ", mainFrame=" + 
                 std::to_string(m_mainFrame != nullptr) + ")");
        delete event;
    }
}

void AsyncEngineIntegration::onIntersectionResultEvent(AsyncIntersectionResultEvent& evt)
{
    std::string taskId = evt.GetTaskId();
    LOG_INF_S("AsyncEngineIntegration: Event received for " + taskId + " on main thread");
    
    // Execute user callback on main thread (safe for UI operations)
    std::function<void(const ComputeResult<IntersectionComputeResult>&)> callback;
    ComputeResult<IntersectionComputeResult> result;
    
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        auto callbackIt = m_intersectionCallbacks.find(taskId);
        auto resultIt = m_pendingResults.find(taskId);
        
        if (callbackIt != m_intersectionCallbacks.end() && 
            resultIt != m_pendingResults.end()) {
            callback = callbackIt->second;
            result = resultIt->second;
            
            // Cleanup
            m_intersectionCallbacks.erase(callbackIt);
            m_pendingResults.erase(resultIt);
        }
    }
    
    if (callback) {
        LOG_INF_S("AsyncEngineIntegration: Executing callback for " + taskId + " on main thread");
        callback(result);
    } else {
        LOG_WRN_S("AsyncEngineIntegration: No callback found for task " + taskId);
    }
}

void AsyncEngineIntegration::postMeshResult(
    const std::string& taskId,
    const ComputeResult<MeshData>& result)
{
    if (result.success) {
        LOG_INF_S("AsyncEngineIntegration: Mesh generation task " + taskId +
                  " completed with " + std::to_string(result.data.vertexCount) + " vertices");

        auto meshData = std::make_shared<MeshData>(result.data);

        m_engine->setSharedData(taskId + "_mesh", meshData);

        auto* event = new AsyncMeshResultEvent(
            wxEVT_ASYNC_MESH_RESULT,
            m_mainFrame ? m_mainFrame->GetId() : 0,
            taskId,
            meshData
        );

        safePostEvent(event);
    } else {
        LOG_ERR_S("AsyncEngineIntegration: Mesh generation task " + taskId + 
                  " failed: " + result.errorMessage);
    }
}

void AsyncEngineIntegration::postBoundingBoxResult(
    const std::string& taskId,
    const ComputeResult<BoundingBoxResult>& result)
{
    if (!m_mainFrame) {
        return;
    }
    
    if (result.success) {
        LOG_INF_S("AsyncEngineIntegration: Bounding box task " + taskId + " completed");
    } else {
        LOG_ERR_S("AsyncEngineIntegration: Bounding box task " + taskId + 
                  " failed: " + result.errorMessage);
    }
}

// Template method implementation for generic tasks
template<typename InputType, typename OutputType>
void AsyncEngineIntegration::submitGenericTask(
    std::shared_ptr<GenericAsyncTask<InputType, OutputType>> task,
    std::function<void(const OutputType&)> onComplete)
{
    if (!m_engine) {
        LOG_ERR_S("AsyncEngineIntegration: Engine not initialized");
        return;
    }

    m_engine->submitGenericTask(task, [this, task, onComplete](const OutputType& result) {
        LOG_INF_S("AsyncEngineIntegration: Generic task '" + task->getTaskId() + "' completed");

        // Call user-provided completion callback
        if (onComplete) {
            onComplete(result);
        }
    });

    LOG_INF_S("AsyncEngineIntegration: Generic task '" + task->getTaskId() + "' submitted");
}

void AsyncEngineIntegration::submitIntersectionTask(
    const std::string& taskId,
    const IntersectionComputeInput& input,
    std::function<void(const ComputeResult<IntersectionComputeResult>&)> onComplete)
{
    LOG_INF_S("AsyncEngineIntegration: Submitting intersection task " + taskId);
    
    // Store user callback for later execution on main thread
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_intersectionCallbacks[taskId] = onComplete;
    }
    
    auto task = GeometryComputeTasks::createIntersectionTask(
        taskId,
        input.shape,
        input.tolerance,
        [this, taskId](const ComputeResult<IntersectionComputeResult>& result) {
            // This runs in worker thread - must post to main thread
            postIntersectionResultWithCallback(taskId, result);
        }
    );
    
    m_engine->submitTask(task);
}

void AsyncEngineIntegration::setProgressCallback(
    std::function<void(const std::string&, int, const std::string&)> callback)
{
    m_engine->setGlobalProgressCallback(callback);
}

} // namespace async

