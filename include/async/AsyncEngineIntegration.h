#pragma once

#include <wx/event.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>

class wxFrame;

// Forward declarations
struct IntersectionComputeResult;
class AsyncComputeEngine;
class GeometryComputeTasks;

// Abstract interface for async engine operations
class IAsyncEngine {
public:
    virtual ~IAsyncEngine() = default;

    // Submit intersection task
    virtual void submitIntersectionTask(
        const std::string& taskId,
        const TopoDS_Shape& shape,
        double tolerance,
        std::function<void(bool, const std::vector<gp_Pnt>&, const std::string&)> onComplete
    ) = 0;

    // Set global progress callback
    virtual void setGlobalProgressCallback(
        std::function<void(const std::string&, int, const std::string&)> callback
    ) = 0;

    // Cancel task by ID
    virtual void cancelTask(const std::string& taskId) = 0;

    // Cancel all tasks
    virtual void cancelAllTasks() = 0;
};

// Include implementation headers after interface
#include "async/AsyncComputeEngine.h"
#include "async/GeometryComputeTasks.h"

namespace async {

class AsyncEngineResultEvent : public wxEvent {
public:
    AsyncEngineResultEvent(wxEventType eventType, int winid, const std::string& taskId)
        : wxEvent(winid, eventType)
        , m_taskId(taskId)
    {}
    
    wxEvent* Clone() const override { return new AsyncEngineResultEvent(*this); }
    
    const std::string& GetTaskId() const { return m_taskId; }
    
    template<typename T>
    void SetResult(const ComputeResult<T>& result) {
        m_hasResult = result.success;
        m_errorMessage = result.errorMessage;
    }
    
    bool HasResult() const { return m_hasResult; }
    const std::string& GetErrorMessage() const { return m_errorMessage; }

private:
    std::string m_taskId;
    bool m_hasResult{false};
    std::string m_errorMessage;
};

class AsyncIntersectionResultEvent : public AsyncEngineResultEvent {
public:
    AsyncIntersectionResultEvent(wxEventType eventType, int winid, 
                                 const std::string& taskId,
                                 const IntersectionComputeResult& result)
        : AsyncEngineResultEvent(eventType, winid, taskId)
        , m_result(result)
    {}
    
    wxEvent* Clone() const override { return new AsyncIntersectionResultEvent(*this); }
    
    const IntersectionComputeResult& GetResult() const { return m_result; }

private:
    IntersectionComputeResult m_result;
};

class AsyncMeshResultEvent : public AsyncEngineResultEvent {
public:
    AsyncMeshResultEvent(wxEventType eventType, int winid,
                        const std::string& taskId,
                        std::shared_ptr<MeshData> meshData)
        : AsyncEngineResultEvent(eventType, winid, taskId)
        , m_meshData(meshData)
    {}
    
    wxEvent* Clone() const override { return new AsyncMeshResultEvent(*this); }
    
    std::shared_ptr<MeshData> GetMeshData() const { return m_meshData; }

private:
    std::shared_ptr<MeshData> m_meshData;
};

wxDECLARE_EVENT(wxEVT_ASYNC_INTERSECTION_RESULT, AsyncIntersectionResultEvent);
wxDECLARE_EVENT(wxEVT_ASYNC_MESH_RESULT, AsyncMeshResultEvent);
wxDECLARE_EVENT(wxEVT_ASYNC_TASK_PROGRESS, AsyncEngineResultEvent);

class AsyncEngineIntegration : public IAsyncEngine {
public:
    // GUI mode constructor
    explicit AsyncEngineIntegration(wxFrame* mainFrame);

    // Headless mode constructor (no GUI dependencies)
    explicit AsyncEngineIntegration(bool headless = false);

    ~AsyncEngineIntegration();
    
    AsyncEngineIntegration(const AsyncEngineIntegration&) = delete;
    AsyncEngineIntegration& operator=(const AsyncEngineIntegration&) = delete;
    
    void computeIntersectionsAsync(
        const std::string& taskId,
        const TopoDS_Shape& shape,
        double tolerance);
    
    void generateMeshAsync(
        const std::string& taskId,
        const TopoDS_Shape& shape,
        double deflection,
        double angle);
    
    void computeBoundingBoxAsync(
        const std::string& taskId,
        const TopoDS_Shape& shape);

    // Submit intersection task with callback
    void submitIntersectionTask(
        const std::string& taskId,
        const TopoDS_Shape& shape,
        double tolerance,
        std::function<void(bool, const std::vector<gp_Pnt>&, const std::string&)> onComplete) override;

    // Set global progress callback
    void setGlobalProgressCallback(
        std::function<void(const std::string&, int, const std::string&)> callback
    ) override;

    // Set progress callback
    void setProgressCallback(std::function<void(const std::string&, int, const std::string&)> callback);

    template<typename InputType, typename OutputType>
    void submitGenericTask(
        std::shared_ptr<GenericAsyncTask<InputType, OutputType>> task,
        std::function<void(const OutputType&)> onComplete = nullptr);
    
    template<typename T>
    std::shared_ptr<SharedComputeData<T>> getSharedData(const std::string& key) {
        return m_engine->getSharedData<T>(key);
    }
    
    template<typename T>
    void setSharedData(const std::string& key, std::shared_ptr<T> data) {
        m_engine->setSharedData(key, data);
    }
    
    void cancelTask(const std::string& taskId) override;
    void cancelAllTasks() override;
    
    TaskStatistics getStatistics() const;
    
    AsyncComputeEngine* getEngine() { return m_engine.get(); }

private:
    void postIntersectionResult(const std::string& taskId,
                              const ComputeResult<IntersectionComputeResult>& result);
    
    void postSimpleIntersectionResult(const std::string& taskId,
                                     const ComputeResult<IntersectionComputeResult>& result);
    void postIntersectionResultWithCallback(const std::string& taskId,
                              const ComputeResult<IntersectionComputeResult>& result);
    
    void onIntersectionResultEvent(AsyncIntersectionResultEvent& evt);

    void postMeshResult(const std::string& taskId,
                       const ComputeResult<MeshData>& result);

    void postBoundingBoxResult(const std::string& taskId,
                              const ComputeResult<BoundingBoxResult>& result);

    void safePostEvent(wxEvent* event);


private:
    bool m_headless{false};
    wxFrame* m_mainFrame{nullptr};
    std::unique_ptr<AsyncComputeEngine> m_engine;
    
    // Callback storage for main thread execution
    std::mutex m_callbackMutex;
    std::unordered_map<std::string, std::function<void(const ComputeResult<IntersectionComputeResult>&)>> m_intersectionCallbacks;
    std::unordered_map<std::string, std::function<void(bool, const std::vector<gp_Pnt>&, const std::string&)>> m_simpleIntersectionCallbacks;
    std::unordered_map<std::string, ComputeResult<IntersectionComputeResult>> m_pendingResults;
};

} // namespace async

