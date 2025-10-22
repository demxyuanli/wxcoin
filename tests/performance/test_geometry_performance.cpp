/**
 * @file test_geometry_performance.cpp
 * @brief Performance benchmark tests for geometry data structure optimizations
 * 
 * Tests the P0 optimizations:
 * 1. FaceIndexMapping reverse mapping
 * 2. ThreadSafeCollector lock-free design
 * 3. EdgeIntersectionAccelerator BVH acceleration
 */

#include "geometry/OCCGeometryMesh.h"
#include "core/ThreadSafeCollector.h"
#include "edges/EdgeIntersectionAccelerator.h"
#include "logger/Logger.h"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <random>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

// Benchmark utilities
class PerformanceBenchmark {
public:
    struct BenchmarkResult {
        std::string name;
        double timeSeconds;
        size_t operations;
        std::string unit;
        
        double opsPerSecond() const {
            return operations / timeSeconds;
        }
        
        double timePerOp() const {
            return (timeSeconds * 1000000.0) / operations; // microseconds
        }
        
        void print() const {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "=== " << name << " ===" << std::endl;
            std::cout << "  Total Time: " << timeSeconds << " seconds" << std::endl;
            std::cout << "  Operations: " << operations << " " << unit << std::endl;
            std::cout << "  Throughput: " << opsPerSecond() << " ops/sec" << std::endl;
            std::cout << "  Time/Op: " << timePerOp() << " μs" << std::endl;
            std::cout << std::endl;
        }
    };
    
    template<typename Func>
    static BenchmarkResult run(const std::string& name, Func func, size_t operations, const std::string& unit = "ops") {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        
        double duration = std::chrono::duration<double>(end - start).count();
        
        return {name, duration, operations, unit};
    }
};

// Test 1: FaceIndexMapping Performance
void testFaceMappingPerformance() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 1: FaceIndexMapping Performance" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    OCCGeometryMesh mesh;
    
    // Simulate large model: 1000 faces, 100 triangles each
    const int numFaces = 1000;
    const int trisPerFace = 100;
    const int totalTris = numFaces * trisPerFace;
    
    std::vector<FaceIndexMapping> mappings;
    mappings.reserve(numFaces);
    
    for (int faceId = 0; faceId < numFaces; ++faceId) {
        FaceIndexMapping mapping(faceId);
        mapping.triangleIndices.reserve(trisPerFace);
        for (int i = 0; i < trisPerFace; ++i) {
            mapping.triangleIndices.push_back(faceId * trisPerFace + i);
        }
        mappings.push_back(std::move(mapping));
    }
    
    mesh.setFaceIndexMappings(mappings);
    
    // Prepare random queries
    const int numQueries = 10000;
    std::vector<int> queryTriangles;
    queryTriangles.reserve(numQueries);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, totalTris - 1);
    
    for (int i = 0; i < numQueries; ++i) {
        queryTriangles.push_back(dis(gen));
    }
    
    // Benchmark
    auto result = PerformanceBenchmark::run("Face Mapping Lookup", [&]() {
        for (int triIdx : queryTriangles) {
            int faceId = mesh.getGeometryFaceIdForTriangle(triIdx);
            (void)faceId; // Prevent optimization
        }
    }, numQueries, "queries");
    
    result.print();
    
    // Performance requirement: at least 1M queries/second
    if (result.opsPerSecond() >= 1000000.0) {
        std::cout << "✅ PASS: Performance exceeds 1M queries/sec" << std::endl;
    } else {
        std::cout << "❌ FAIL: Performance below 1M queries/sec threshold" << std::endl;
    }
    
    // Target: <1 microsecond per query
    if (result.timePerOp() <= 1.0) {
        std::cout << "✅ PASS: Query time < 1 microsecond" << std::endl;
    } else {
        std::cout << "⚠️  WARN: Query time > 1 microsecond" << std::endl;
    }
}

// Test 2: ThreadSafeCollector Performance
void testThreadSafeCollectorPerformance() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 2: ThreadSafeCollector Performance" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    const size_t numThreads = 8;
    const size_t itemsPerThread = 10000;
    const size_t totalItems = numThreads * itemsPerThread;
    
    ThreadSafeCollector<int> collector(numThreads);
    
    auto result = PerformanceBenchmark::run("ThreadSafe Collection", [&]() {
        std::vector<std::future<void>> futures;
        
        for (size_t t = 0; t < numThreads; ++t) {
            futures.push_back(std::async(std::launch::async, [&collector, t, itemsPerThread]() {
                for (size_t i = 0; i < itemsPerThread; ++i) {
                    collector.add(static_cast<int>(t * itemsPerThread + i), t);
                }
            }));
        }
        
        for (auto& future : futures) {
            future.get();
        }
        
        auto all = collector.collect();
        if (all.size() != totalItems) {
            std::cerr << "ERROR: Expected " << totalItems << " items, got " << all.size() << std::endl;
        }
    }, totalItems, "items");
    
    result.print();
    
    // Performance requirement: at least 10M additions/second
    if (result.opsPerSecond() >= 10000000.0) {
        std::cout << "✅ PASS: Performance exceeds 10M additions/sec" << std::endl;
    } else {
        std::cout << "⚠️  WARN: Performance below 10M additions/sec" << std::endl;
    }
    
    // Check buffer distribution
    auto sizes = collector.getBufferSizes();
    std::cout << "\nBuffer Distribution:" << std::endl;
    for (size_t i = 0; i < sizes.size(); ++i) {
        std::cout << "  Thread " << i << ": " << sizes[i] << " items" << std::endl;
    }
}

// Test 3: EdgeIntersectionAccelerator Performance
void testEdgeIntersectionPerformance() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 3: EdgeIntersection Accelerator" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Create a complex shape with many edges
    std::cout << "Creating test geometry..." << std::endl;
    
    // Create a filleted box (generates many edges)
    gp_Pnt corner(0, 0, 0);
    TopoDS_Shape box = BRepPrimAPI_MakeBox(corner, 100, 100, 100).Shape();
    
    // Add fillets to create curved edges
    BRepFilletAPI_MakeFillet fillet(box);
    
    // Add fillets to some edges
    int edgeCount = 0;
    for (TopExp_Explorer exp(box, TopAbs_EDGE); exp.More(); exp.Next()) {
        if (edgeCount++ < 6) { // Fillet first 6 edges
            fillet.Add(5.0, TopoDS::Edge(exp.Current()));
        }
    }
    
    TopoDS_Shape filletedBox;
    try {
        filletedBox = fillet.Shape();
    } catch (...) {
        filletedBox = box; // Fallback to box if fillet fails
    }
    
    // Extract edges
    std::vector<TopoDS_Edge> edges;
    for (TopExp_Explorer exp(filletedBox, TopAbs_EDGE); exp.More(); exp.Next()) {
        edges.push_back(TopoDS::Edge(exp.Current()));
    }
    
    std::cout << "Test geometry has " << edges.size() << " edges" << std::endl;
    
    if (edges.size() < 10) {
        std::cout << "⚠️  SKIP: Not enough edges for meaningful test" << std::endl;
        return;
    }
    
    // Test BVH build performance
    EdgeIntersectionAccelerator accelerator;
    
    auto buildResult = PerformanceBenchmark::run("BVH Build", [&]() {
        accelerator.buildFromEdges(edges, 4);
    }, edges.size(), "edges");
    
    buildResult.print();
    
    // Test query performance
    auto queryResult = PerformanceBenchmark::run("Find Potential Intersections", [&]() {
        auto pairs = accelerator.findPotentialIntersections();
        std::cout << "  Found " << pairs.size() << " potential pairs" << std::endl;
    }, edges.size(), "edges");
    
    queryResult.print();
    
    // Print statistics
    auto stats = accelerator.getStatistics();
    std::cout << "BVH Statistics:" << std::endl;
    std::cout << "  Total Edges: " << stats.totalEdges << std::endl;
    std::cout << "  Potential Pairs: " << stats.potentialPairs << std::endl;
    std::cout << "  Pruning Ratio: " << (stats.pruningRatio * 100.0) << "%" << std::endl;
    std::cout << "  Build Time: " << stats.buildTime << "s" << std::endl;
    std::cout << "  Query Time: " << stats.queryTime << "s" << std::endl;
    
    if (stats.pruningRatio >= 0.80) {
        std::cout << "\n✅ PASS: Pruning ratio >= 80%" << std::endl;
    } else {
        std::cout << "\n⚠️  WARN: Pruning ratio < 80%" << std::endl;
    }
    
    // Test actual intersection extraction
    const double tolerance = 0.01;
    auto extractResult = PerformanceBenchmark::run("Extract Intersections (Sequential)", [&]() {
        auto intersections = accelerator.extractIntersections(tolerance);
        std::cout << "  Found " << intersections.size() << " actual intersections" << std::endl;
    }, stats.potentialPairs, "edge pairs");
    
    extractResult.print();
    
    // Test parallel extraction
    auto parallelResult = PerformanceBenchmark::run("Extract Intersections (Parallel)", [&]() {
        auto intersections = accelerator.extractIntersectionsParallel(tolerance, 0);
        std::cout << "  Found " << intersections.size() << " actual intersections" << std::endl;
    }, stats.potentialPairs, "edge pairs");
    
    parallelResult.print();
    
    // Calculate speedup
    if (extractResult.timeSeconds > 0) {
        double speedup = extractResult.timeSeconds / parallelResult.timeSeconds;
        std::cout << "Parallel Speedup: " << speedup << "x" << std::endl;
        
        if (speedup >= 2.0) {
            std::cout << "✅ PASS: Parallel speedup >= 2x" << std::endl;
        } else {
            std::cout << "⚠️  INFO: Parallel speedup < 2x (may be expected for small datasets)" << std::endl;
        }
    }
}

// Main benchmark runner
int main(int argc, char** argv) {
    std::cout << "╔═══════════════════════════════════════╗" << std::endl;
    std::cout << "║  Geometry Performance Benchmark Suite ║" << std::endl;
    std::cout << "║  P0 Optimizations Validation          ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════╝" << std::endl;
    
    try {
        // Test 1: FaceIndexMapping
        testFaceMappingPerformance();
        
        // Test 2: ThreadSafeCollector
        testThreadSafeCollectorPerformance();
        
        // Test 3: EdgeIntersectionAccelerator
        testEdgeIntersectionPerformance();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "All performance tests completed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}


