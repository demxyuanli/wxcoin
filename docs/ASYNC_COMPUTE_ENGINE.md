# Async Compute Engine Architecture

## Overview

The Async Compute Engine provides a unified framework for background processing of computationally intensive tasks with efficient memory sharing between worker threads and the main UI thread.

## Key Features

### 1. Zero-Copy Memory Sharing
- Uses `std::shared_ptr` for efficient data sharing
- Atomic reference counting
- No data copying between threads
- Thread-safe access patterns

### 2. Priority-Based Task Scheduling
- Four priority levels: Low, Normal, High, Critical
- Priority queue ensures critical tasks execute first
- FIFO within same priority level

### 3. Task Lifecycle Management
- States: Pending → Running → Completed/Failed/Cancelled
- Cancellation support for long-running tasks
- Progress callbacks
- Partial result callbacks

### 4. Result Caching
- Automatic result caching based on task ID
- Configurable cache size and expiration
- LRU eviction policy (future enhancement)

### 5. wxWidgets Integration
- Custom events for thread-safe UI updates
- Event-driven result delivery
- Compatible with existing wxWidgets applications

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Main Thread (UI)                          │
│  ┌──────────────┐         ┌─────────────────────────────────┐  │
│  │ Application  │─────────│ AsyncEngineIntegration          │  │
│  │   Code       │         │  - Task submission              │  │
│  └──────────────┘         │  - Event handling               │  │
│         ↑                 │  - Shared data access           │  │
│         │ wxEvents        └─────────────┬───────────────────┘  │
│         │                               │                       │
└─────────┼───────────────────────────────┼───────────────────────┘
          │                               │
          │                               ↓
┌─────────┼───────────────────────────────────────────────────────┐
│         │                    AsyncComputeEngine                 │
│         │                                                        │
│  ┌──────┴──────────┐    ┌────────────────────────────────┐    │
│  │ Event Posting   │    │   Priority Task Queue          │    │
│  │   Thread        │    │   (thread-safe)                │    │
│  └─────────────────┘    └────────────┬───────────────────┘    │
│                                       │                         │
│  ┌───────────────────────────────────┼───────────────────────┐ │
│  │            Shared Data Cache      │                       │ │
│  │       (std::shared_ptr based)     │                       │ │
│  │  ┌─────────┐  ┌─────────┐  ┌────┴────┐  ┌─────────┐   │ │
│  │  │ Mesh    │  │ Points  │  │ BBox    │  │  ...    │   │ │
│  │  │ Data    │  │ Data    │  │ Data    │  │         │   │ │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘   │ │
│  └───────────────────────────────────────────────────────────┘ │
│                                       │                         │
│  ┌────────────────────────────────────┼───────────────────────┐│
│  │             Worker Thread Pool     │                       ││
│  │   ┌──────────┐  ┌──────────┐  ┌───┴──────┐  ┌──────────┐││
│  │   │ Worker 1 │  │ Worker 2 │  │ Worker 3 │  │ Worker N │││
│  │   └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘││
│  │        │             │              │             │       ││
│  └────────┼─────────────┼──────────────┼─────────────┼───────┘│
└───────────┼─────────────┼──────────────┼─────────────┼─────────┘
            │             │              │             │
            ↓             ↓              ↓             ↓
    ┌──────────────────────────────────────────────────────┐
    │          Compute Tasks (OpenCASCADE, etc.)           │
    │  - Intersection computation                          │
    │  - Mesh generation                                   │
    │  - Bounding box calculation                          │
    │  - Custom geometry operations                        │
    └──────────────────────────────────────────────────────┘
```

## Core Components

### 1. AsyncComputeEngine

Main engine class that manages:
- Worker thread pool
- Task queue with priority scheduling
- Shared data cache
- Task lifecycle and statistics

**Key Methods:**
```cpp
template<typename InputType, typename ResultType>
void submitTask(std::shared_ptr<AsyncTask<InputType, ResultType>> task);

void cancelTask(const std::string& taskId);
void cancelAllTasks();
void pause();
void resume();
void shutdown();

TaskStatistics getStatistics() const;

template<typename T>
std::shared_ptr<SharedComputeData<T>> getSharedData(const std::string& key);

template<typename T>
void setSharedData(const std::string& key, std::shared_ptr<T> data);
```

### 2. AsyncTask<InputType, ResultType>

Generic task template supporting:
- Custom input/output types
- Progress callbacks
- Partial results
- Cancellation support

**Configuration Options:**
```cpp
struct Config {
    TaskPriority priority{TaskPriority::Normal};
    bool cacheResult{true};
    bool supportCancellation{true};
    bool enableProgressCallback{false};
    bool enablePartialResults{false};
    size_t partialResultBatchSize{50};
};
```

### 3. SharedComputeData<T>

Thread-safe shared data container:
```cpp
template<typename T>
struct SharedComputeData {
    std::shared_ptr<T> data;              // Actual data
    std::atomic<bool> ready{false};        // Ready flag
    std::atomic<size_t> refCount{0};       // Reference count
    std::chrono::steady_clock::time_point lastAccessTime;
};
```

### 4. GeometryComputeTasks

Predefined geometry computation tasks:
- **Intersection Computation**: Extract edge intersections
- **Mesh Generation**: Tessellate shapes into triangular meshes
- **Bounding Box**: Compute 3D bounding boxes

### 5. AsyncEngineIntegration

wxWidgets integration layer:
- Wraps AsyncComputeEngine
- Handles event posting
- Provides high-level API for UI code

## Memory Sharing Strategy

### Zero-Copy Principle

```cpp
// Worker thread computes result
MeshData meshData = generateMesh(shape);

// Wrap in shared_ptr (no copy)
auto sharedMesh = std::make_shared<MeshData>(std::move(meshData));

// Store in shared cache (no copy)
engine->setSharedData("mesh_key", sharedMesh);

// Main thread accesses (no copy)
auto cachedMesh = engine->getSharedData<MeshData>("mesh_key");
if (cachedMesh && cachedMesh->ready) {
    // Use cachedMesh->data directly
    renderMesh(*cachedMesh->data);
}
```

### Thread Safety Guarantees

1. **Atomic Operations**: State flags use `std::atomic`
2. **Mutex Protection**: Cache access protected by mutex
3. **Immutable Data**: Once computed, shared data should be read-only
4. **Event-Based Updates**: UI updates via wxEvents (thread-safe)

## Usage Examples

### Example 1: Compute Intersections

```cpp
// In your main frame initialization
m_asyncEngine = std::make_unique<AsyncEngineIntegration>(this);

// Bind event handler
Bind(wxEVT_ASYNC_INTERSECTION_RESULT, 
     &MyFrame::OnIntersectionResult, this);

// Submit task
void MyFrame::ComputeIntersections(const TopoDS_Shape& shape) {
    std::string taskId = "intersection_" + generateUniqueId();
    m_asyncEngine->computeIntersectionsAsync(taskId, shape, 1e-6);
}

// Handle result
void MyFrame::OnIntersectionResult(AsyncIntersectionResultEvent& event) {
    const auto& result = event.GetResult();
    
    if (event.HasResult()) {
        // Update UI with intersection points
        updateIntersectionDisplay(result.points);
        
        wxLogMessage("Found %zu intersections in %lld ms",
            result.points.size(), 
            result.computeTime.count());
    } else {
        wxLogError("Intersection computation failed: %s",
            event.GetErrorMessage());
    }
}
```

### Example 2: Generate Mesh with Shared Data

```cpp
// Submit mesh generation task
void MyFrame::GenerateMesh(const TopoDS_Shape& shape) {
    std::string taskId = "mesh_" + generateUniqueId();
    m_asyncEngine->generateMeshAsync(taskId, shape, 0.1, 0.5);
}

// Handle mesh result
void MyFrame::OnMeshResult(AsyncMeshResultEvent& event) {
    // Get shared mesh data (zero-copy)
    auto meshData = event.GetMeshData();
    
    if (meshData) {
        // Upload to GPU for rendering
        uploadMeshToGPU(
            meshData->vertices.data(),
            meshData->normals.data(),
            meshData->indices.data(),
            meshData->vertexCount,
            meshData->triangleCount
        );
        
        // Mesh data remains in shared cache for later use
        wxLogMessage("Mesh: %zu vertices, %zu triangles, %zu KB",
            meshData->vertexCount,
            meshData->triangleCount,
            meshData->getMemoryUsage() / 1024);
    }
}

// Later: Access cached mesh without recomputation
void MyFrame::ReRenderMesh() {
    std::string cacheKey = "mesh_" + lastTaskId;
    auto cachedMesh = m_asyncEngine->getSharedData<MeshData>(cacheKey);
    
    if (cachedMesh && cachedMesh->ready) {
        // Reuse mesh data without recomputation
        renderMesh(*cachedMesh->data);
    }
}
```

### Example 3: Custom Task with Progress

```cpp
// Create custom task
auto task = std::make_shared<AsyncTask<MyInput, MyResult>>(
    "custom_task_1",
    inputData,
    [](const MyInput& input, std::atomic<bool>& cancelled) -> MyResult {
        MyResult result;
        
        for (size_t i = 0; i < input.itemCount; ++i) {
            if (cancelled.load()) {
                break;  // Support cancellation
            }
            
            // Process item
            result.items.push_back(processItem(input.items[i]));
        }
        
        return result;
    },
    [this](const ComputeResult<MyResult>& result) {
        // Completion callback (runs in worker thread)
        postResultToUI(result);
    }
);

// Set progress callback
task->setProgressCallback([this](int progress, const std::string& msg) {
    // Update UI progress bar
    wxQueueEvent(this, new ProgressEvent(progress, msg));
});

// Submit to engine
m_asyncEngine->getEngine()->submitTask(task);
```

### Example 4: Task Cancellation

```cpp
// Store task IDs
std::vector<std::string> m_activeTaskIds;

// Submit multiple tasks
for (const auto& shape : shapes) {
    std::string taskId = "task_" + std::to_string(m_taskCounter++);
    m_activeTaskIds.push_back(taskId);
    m_asyncEngine->computeIntersectionsAsync(taskId, shape, 1e-6);
}

// Cancel specific task
void MyFrame::CancelTask(const std::string& taskId) {
    m_asyncEngine->cancelTask(taskId);
}

// Cancel all tasks (e.g., on window close)
void MyFrame::OnClose(wxCloseEvent& event) {
    m_asyncEngine->cancelAllTasks();
    event.Skip();
}
```

## Performance Considerations

### 1. Worker Thread Count
```cpp
// Default: hardware_concurrency - 1 (leave one core for UI)
AsyncComputeEngine::Config config;
config.numWorkerThreads = 0;  // Auto-detect

// Or specify manually
config.numWorkerThreads = 4;
```

### 2. Task Granularity
- **Too Fine**: Overhead from scheduling exceeds computation time
- **Too Coarse**: Poor load balancing, one thread may dominate
- **Optimal**: Tasks taking 10-1000ms of computation time

### 3. Memory Management
```cpp
// Good: Move large data into shared_ptr
auto data = std::make_shared<LargeData>(std::move(computedData));
engine->setSharedData("key", data);

// Bad: Copy large data
auto data = std::make_shared<LargeData>(computedData);  // Copy!
```

### 4. Cache Management
```cpp
// Configure cache limits
config.maxCacheSize = 100;  // Max entries
config.cacheExpirationTime = std::chrono::minutes(30);

// Manual cleanup
engine->removeSharedData("old_key");
```

## Statistics and Monitoring

```cpp
auto stats = m_asyncEngine->getStatistics();

std::cout << "Queued: " << stats.queuedTasks << "\n"
          << "Running: " << stats.runningTasks << "\n"
          << "Completed: " << stats.completedTasks << "\n"
          << "Failed: " << stats.failedTasks << "\n"
          << "Avg Time: " << stats.avgExecutionTimeMs << "ms\n"
          << "Total: " << stats.totalProcessedTasks << "\n";
```

## Thread Safety Checklist

✅ **DO:**
- Use shared_ptr for large data structures
- Access shared data through getSharedData/setSharedData
- Post UI updates via wxQueueEvent
- Check cancelled flag in long loops
- Use const references when possible

❌ **DON'T:**
- Access wxWidgets UI elements from worker threads
- Modify shared data after storing in cache
- Hold mutexes while doing heavy computation
- Ignore cancellation requests
- Copy large data structures unnecessarily

## Integration with Existing Code

### Replacing AsyncIntersectionTask

**Before:**
```cpp
auto task = std::make_shared<AsyncIntersectionTask>(
    shape, tolerance, frame,
    [this](const std::vector<gp_Pnt>& points) {
        renderIntersections(points);
    }
);
task->start();
```

**After:**
```cpp
m_asyncEngine->computeIntersectionsAsync(
    "intersection_task",
    shape,
    tolerance
);

// Handle in event:
Bind(wxEVT_ASYNC_INTERSECTION_RESULT, 
     &MyClass::OnIntersectionResult, this);
```

## Future Enhancements

1. **Work Stealing**: Allow idle workers to steal tasks from busy workers
2. **Task Dependencies**: Chain tasks with dependencies
3. **Adaptive Thread Pool**: Dynamically adjust thread count based on load
4. **NUMA Awareness**: Pin threads to CPU cores for cache efficiency
5. **GPU Offloading**: Integrate GPU compute for supported operations
6. **Distributed Computing**: Scale across multiple machines

## Troubleshooting

### Issue: Tasks Not Executing
- Check engine is not paused: `engine->isRunning()`
- Verify queue not full: `engine->getQueueSize()`
- Check for exceptions in worker threads (see logs)

### Issue: Memory Leaks
- Ensure shared_ptr usage (no raw pointers)
- Call `removeSharedData()` for large cached items
- Check for circular references in shared data

### Issue: Slow Performance
- Profile task execution time
- Check if worker threads are starved
- Verify no mutex contention (check logs)
- Consider increasing batch sizes for partial results

### Issue: Race Conditions
- Never modify shared data after caching
- Always check `ready` flag before accessing
- Use atomic operations for state changes

## Conclusion

The Async Compute Engine provides a robust, efficient framework for background computation with minimal overhead and maximum flexibility. By leveraging shared pointers and careful thread synchronization, it achieves near-zero-copy performance while maintaining thread safety and ease of use.


