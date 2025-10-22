# Progressive Intersection Display Fix Guide

## Problem Analysis

From the logs, we can see that:

1. **Intersection computation is too fast** (0.000055s for 2 points)
2. **Callbacks happen after computation completes**, not during computation
3. **The real bottleneck is edge extraction** (51653 edges in 1.05s)

### Root Cause

The `OriginalEdgeExtractor::findEdgeIntersections` uses `std::for_each(std::execution::par, ...)` which:
- Waits for ALL computations to complete before returning
- Cannot invoke callbacks during computation
- Processes intersections in batches only AFTER all computation is done

## Solution

### Option 1: Real Progressive Computation (Recommended)

Replace the parallel `std::for_each` with a custom loop that:
1. Processes intersection checks in chunks
2. Invokes callback after each chunk
3. Allows UI updates during computation

```cpp
void OriginalEdgeExtractor::findEdgeIntersections(
    const TopoDS_Shape& shape,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance,
    std::function<void(const std::vector<gp_Pnt>&)> progressCallback)
{
    // ... existing setup code ...
    
    // Process in chunks for progressive display
    const size_t CHUNK_SIZE = 100; // Process 100 intersection checks at a time
    std::vector<gp_Pnt> chunkResults;
    
    for (size_t start = 0; start < intersectionChecks.size(); start += CHUNK_SIZE) {
        size_t end = std::min(start + CHUNK_SIZE, intersectionChecks.size());
        
        // Process this chunk in parallel
        std::vector<gp_Pnt> tempResults;
        std::mutex tempMutex;
        
        std::for_each(std::execution::par,
            intersectionChecks.begin() + start,
            intersectionChecks.begin() + end,
            [&](const auto& check) {
                // ... intersection detection logic ...
                if (found_intersection) {
                    std::lock_guard<std::mutex> lock(tempMutex);
                    tempResults.push_back(intersectionPoint);
                }
            });
        
        // Add chunk results and invoke callback
        if (!tempResults.empty()) {
            intersectionPoints.insert(intersectionPoints.end(),
                                    tempResults.begin(), tempResults.end());
            
            if (progressCallback) {
                progressCallback(tempResults); // Report this chunk
            }
        }
    }
}
```

### Option 2: Use TBB with Progress Callback (Better)

Since we now have TBB integrated, use `tbb::parallel_for` with a custom body that reports progress:

```cpp
void OriginalEdgeExtractor::findEdgeIntersections(
    const TopoDS_Shape& shape,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance,
    std::function<void(const std::vector<gp_Pnt>&)> progressCallback)
{
    // ... existing setup code ...
    
    tbb::concurrent_vector<gp_Pnt> concurrentResults;
    std::atomic<size_t> processedCount{0};
    const size_t totalChecks = intersectionChecks.size();
    const size_t reportInterval = 50; // Report every 50 intersections found
    
    tbb::parallel_for(tbb::blocked_range<size_t>(0, intersectionChecks.size()),
        [&](const tbb::blocked_range<size_t>& range) {
            std::vector<gp_Pnt> localResults;
            
            for (size_t i = range.begin(); i != range.end(); ++i) {
                const auto& check = intersectionChecks[i];
                
                // ... intersection detection logic ...
                if (found_intersection) {
                    localResults.push_back(intersectionPoint);
                    
                    // Report progress periodically
                    if (localResults.size() >= reportInterval) {
                        concurrentResults.grow_by(localResults.begin(), localResults.end());
                        
                        if (progressCallback) {
                            progressCallback(localResults);
                        }
                        
                        localResults.clear();
                    }
                }
                
                processedCount++;
            }
            
            // Add remaining local results
            if (!localResults.empty()) {
                concurrentResults.grow_by(localResults.begin(), localResults.end());
                
                if (progressCallback) {
                    progressCallback(localResults);
                }
            }
        });
    
    // Copy results
    intersectionPoints.assign(concurrentResults.begin(), concurrentResults.end());
}
```

### Option 3: Focus on Edge Extraction Optimization (Quick Win)

Since edge extraction (1.05s) is much slower than intersection detection (0.000055s), optimize that first:

1. **Parallelize edge extraction** using TBB
2. **Stream results** to the scene graph as edges are extracted
3. **Defer intersection computation** until user explicitly requests it

This would give immediate visual feedback while edges are being extracted.

## Implementation Priority

1. **High Priority**: Optimize edge extraction with progressive display
2. **Medium Priority**: Add true progressive intersection detection (Option 2)
3. **Low Priority**: Add UI control to toggle progressive vs. batch mode

## Code Files to Modify

1. `src/opencascade/edges/extractors/OriginalEdgeExtractor.cpp`
   - Modify `findEdgeIntersections` to support progress callbacks
   - Add chunked processing or TBB-based progressive computation

2. `include/edges/extractors/OriginalEdgeExtractor.h`
   - Add progress callback parameter to `findEdgeIntersections`

3. `src/opencascade/edges/AsyncIntersectionTask.cpp`
   - Update to pass progress callback to extractor
   - Ensure callback is invoked during computation, not after

## Testing Strategy

1. Test with small model (< 100 edges)
   - Should see immediate intersection display
   
2. Test with medium model (1000-5000 edges)
   - Should see progressive batches appearing
   
3. Test with large model (> 10000 edges)
   - Should see smooth progressive display
   - Should be cancellable mid-computation

## Performance Targets

- **Small models**: Instant display (< 100ms)
- **Medium models**: First batch within 200ms, complete in < 2s
- **Large models**: First batch within 500ms, progressive updates every 100-200ms


