# Async Compute Engine - Quick Start Guide

## What is it?

A unified asynchronous computation engine for background processing with efficient memory sharing between worker threads and the main UI thread.

## Key Benefits

✅ **Zero-Copy Memory Sharing** - Share data via `std::shared_ptr` without copying  
✅ **Priority Scheduling** - Critical tasks execute first  
✅ **Thread-Safe** - Built-in synchronization and wxWidgets integration  
✅ **Cancellable** - Support for task cancellation  
✅ **Caching** - Automatic result caching to avoid recomputation  

## Quick Example

### Step 1: Initialize Engine

```cpp
// In your main frame constructor
#include "async/AsyncEngineIntegration.h"

MyFrame::MyFrame() {
    m_asyncEngine = std::make_unique<async::AsyncEngineIntegration>(this);
    
    // Bind event handlers
    Bind(wxEVT_ASYNC_INTERSECTION_RESULT, 
         &MyFrame::OnIntersectionResult, this);
    Bind(wxEVT_ASYNC_MESH_RESULT, 
         &MyFrame::OnMeshResult, this);
}
```

### Step 2: Submit Tasks

```cpp
// Compute intersections asynchronously
void MyFrame::ComputeIntersections(const TopoDS_Shape& shape) {
    m_asyncEngine->computeIntersectionsAsync(
        "intersection_" + generateId(),
        shape,
        1e-6  // tolerance
    );
}

// Generate mesh asynchronously
void MyFrame::GenerateMesh(const TopoDS_Shape& shape) {
    m_asyncEngine->generateMeshAsync(
        "mesh_" + generateId(),
        shape,
        0.1,  // deflection
        0.5   // angle
    );
}
```

### Step 3: Handle Results

```cpp
// Handle intersection results
void MyFrame::OnIntersectionResult(async::AsyncIntersectionResultEvent& event) {
    const auto& result = event.GetResult();
    
    wxLogMessage("Found %zu intersections in %lld ms",
        result.points.size(),
        result.computeTime.count());
    
    // Render intersection points
    renderIntersections(result.points);
}

// Handle mesh results (zero-copy)
void MyFrame::OnMeshResult(async::AsyncMeshResultEvent& event) {
    auto meshData = event.GetMeshData();  // std::shared_ptr
    
    if (meshData) {
        wxLogMessage("Mesh: %zu vertices, %zu triangles",
            meshData->vertexCount,
            meshData->triangleCount);
        
        // Upload to GPU (data remains in shared cache)
        uploadMeshToGPU(meshData);
    }
}
```

### Step 4: Access Shared Data (Optional)

```cpp
// Access cached mesh without recomputation
void MyFrame::RenderCachedMesh(const std::string& taskId) {
    auto cachedMesh = m_asyncEngine->getSharedData<async::MeshData>(
        taskId + "_mesh"
    );
    
    if (cachedMesh && cachedMesh->ready) {
        // Reuse mesh data (no copy, no recomputation)
        renderMesh(*cachedMesh->data);
    }
}
```

## Architecture Overview

```
Main Thread                Worker Threads
     │                            │
     ├─→ Submit Task ────────────→│
     │                            ├─→ Execute
     │                            ├─→ Store in Cache (shared_ptr)
     │                            └─→ Post Event
     ←──── wxEvent ───────────────┘
     │
     └─→ Handle Result (access shared data)
```

## Memory Sharing Example

```cpp
// Worker thread: Compute mesh
MeshData mesh = generateMesh(shape);  // Heavy computation

// Wrap in shared_ptr (no copy)
auto sharedMesh = std::make_shared<MeshData>(std::move(mesh));

// Store in cache (no copy)
engine->setSharedData("mesh_key", sharedMesh);

// Main thread: Access mesh (no copy)
auto cachedMesh = engine->getSharedData<MeshData>("mesh_key");
renderMesh(*cachedMesh->data);  // Direct access, zero-copy!
```

## Task Priority

```cpp
AsyncTask<Input, Result>::Config config;
config.priority = TaskPriority::Critical;  // Low, Normal, High, Critical

auto task = std::make_shared<AsyncTask<Input, Result>>(..., config);
engine->submitTask(task);
```

## Task Cancellation

```cpp
// Cancel specific task
m_asyncEngine->cancelTask("task_id_123");

// Cancel all tasks (e.g., on window close)
void MyFrame::OnClose(wxCloseEvent& event) {
    m_asyncEngine->cancelAllTasks();
    event.Skip();
}
```

## Monitoring

```cpp
auto stats = m_asyncEngine->getStatistics();

wxLogMessage("Active: %zu, Completed: %zu, Avg Time: %.1fms",
    stats.runningTasks,
    stats.completedTasks,
    stats.avgExecutionTimeMs);
```

## Predefined Geometry Tasks

1. **Intersection Computation**
   - Input: TopoDS_Shape + tolerance
   - Output: std::vector<gp_Pnt>
   
2. **Mesh Generation**
   - Input: TopoDS_Shape + deflection + angle
   - Output: MeshData (vertices, normals, indices)
   
3. **Bounding Box**
   - Input: TopoDS_Shape
   - Output: BoundingBoxResult (min/max xyz)

## Creating Custom Tasks

```cpp
auto task = std::make_shared<AsyncTask<MyInput, MyResult>>(
    "task_id",
    inputData,
    // Compute function (runs in worker thread)
    [](const MyInput& input, std::atomic<bool>& cancelled) -> MyResult {
        MyResult result;
        
        // Your computation here
        for (auto& item : input.items) {
            if (cancelled.load()) break;  // Support cancellation
            result.process(item);
        }
        
        return result;
    },
    // Completion callback (runs in worker thread)
    [this](const ComputeResult<MyResult>& result) {
        if (result.success) {
            // Post result to UI thread
            wxQueueEvent(this, new MyResultEvent(result.data));
        }
    }
);

m_asyncEngine->getEngine()->submitTask(task);
```

## Best Practices

✅ **DO:**
- Use `std::shared_ptr` for large data structures
- Access shared data through `getSharedData`/`setSharedData`
- Post UI updates via `wxQueueEvent`
- Check `cancelled` flag in long loops
- Move data into shared_ptr to avoid copies

❌ **DON'T:**
- Access UI elements from worker threads directly
- Modify shared data after storing in cache
- Copy large data structures unnecessarily
- Ignore cancellation requests
- Hold mutexes during heavy computation

## Performance Tips

1. **Optimal task granularity**: 10-1000ms per task
2. **Thread count**: Defaults to `hardware_concurrency - 1`
3. **Cache management**: Remove old cached data manually if memory-constrained
4. **Batch partial results**: Use `partialResultBatchSize` to control update frequency

## Common Use Cases

### Use Case 1: Real-time Intersection Display
```cpp
// User imports large model
void onModelLoaded(const TopoDS_Shape& shape) {
    // Start background intersection computation
    m_asyncEngine->computeIntersectionsAsync("intersections", shape, 1e-6);
    
    // UI remains responsive
    showProgressDialog("Computing intersections...");
}

// Progressive display as results arrive
void OnIntersectionResult(AsyncIntersectionResultEvent& event) {
    updateIntersectionDisplay(event.GetResult().points);
    hideProgressDialog();
}
```

### Use Case 2: Mesh Caching for Multiple Renders
```cpp
// Generate mesh once
m_asyncEngine->generateMeshAsync("model_mesh", shape, 0.1, 0.5);

// Reuse mesh for multiple rendering passes (zero-copy)
void render() {
    auto mesh = m_asyncEngine->getSharedData<MeshData>("model_mesh");
    if (mesh && mesh->ready) {
        renderSolidPass(*mesh->data);
        renderWireframePass(*mesh->data);
        renderShadowPass(*mesh->data);
    }
}
```

### Use Case 3: Batch Processing
```cpp
// Process multiple shapes in parallel
for (size_t i = 0; i < shapes.size(); ++i) {
    m_asyncEngine->computeIntersectionsAsync(
        "batch_" + std::to_string(i),
        shapes[i],
        1e-6
    );
}

// Track completion
size_t m_completedTasks = 0;

void OnIntersectionResult(AsyncIntersectionResultEvent& event) {
    m_completedTasks++;
    
    if (m_completedTasks == shapes.size()) {
        wxLogMessage("Batch processing complete!");
    }
}
```

## Troubleshooting

### Tasks not executing?
- Check `engine->isRunning()` returns true
- Verify queue not full: `engine->getQueueSize() < maxQueueSize`

### Memory growing?
- Call `removeSharedData()` for large cached items
- Check for circular shared_ptr references

### Slow performance?
- Profile task execution time
- Check worker thread count
- Reduce mutex contention (minimize lock duration)

## Next Steps

1. Read full documentation: `docs/ASYNC_COMPUTE_ENGINE.md`
2. See example application: `include/async/AsyncEngineExample.h`
3. Create custom tasks for your specific needs

## Summary

The Async Compute Engine provides:
- **Efficient**: Zero-copy memory sharing
- **Safe**: Thread-safe by design
- **Flexible**: Support custom tasks
- **Integrated**: wxWidgets event-based results
- **Performant**: Priority scheduling and caching

Start using it today to move heavy computations off the UI thread!


