#include "async/GeometryComputeTasks.h"
#include "edges/extractors/OriginalEdgeExtractor.h"
#include "logger/Logger.h"
#include <tbb/tbb.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/concurrent_vector.h>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include <OpenCASCADE/BRepBndLib.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/BRepMesh_IncrementalMesh.hxx>
#include <OpenCASCADE/Poly_Triangulation.hxx>
#include <OpenCASCADE/TopLoc_Location.hxx>
#include <OpenCASCADE/gp_Trsf.hxx>

namespace async {

// Task factory registry implementation
std::unordered_map<std::string, GeometryComputeTasks::TaskFactory>&
GeometryComputeTasks::getTaskFactories() {
    static std::unordered_map<std::string, TaskFactory> factories;
    return factories;
}

bool GeometryComputeTasks::registerTaskFactory(const std::string& taskType, TaskFactory factory) {
    auto& factories = getTaskFactories();
    if (factories.find(taskType) != factories.end()) {
        LOG_WRN_S("GeometryComputeTasks: Task factory for type '" + taskType + "' already registered");
        return false;
    }

    factories[taskType] = factory;
    LOG_INF_S("GeometryComputeTasks: Registered task factory for type '" + taskType + "'");
    return true;
}

bool GeometryComputeTasks::unregisterTaskFactory(const std::string& taskType) {
    auto& factories = getTaskFactories();
    auto it = factories.find(taskType);
    if (it == factories.end()) {
        LOG_WRN_S("GeometryComputeTasks: Task factory for type '" + taskType + "' not found");
        return false;
    }

    factories.erase(it);
    LOG_INF_S("GeometryComputeTasks: Unregistered task factory for type '" + taskType + "'");
    return true;
}

std::any GeometryComputeTasks::createTask(const std::string& taskType,
                                         const std::string& taskId,
                                         std::any input,
                                         std::function<void(std::any)> onComplete) {
    auto& factories = getTaskFactories();
    auto it = factories.find(taskType);
    if (it == factories.end()) {
        LOG_ERR_S("GeometryComputeTasks: No factory registered for task type '" + taskType + "'");
        throw std::runtime_error("Task factory not found for type: " + taskType);
    }

    return it->second(taskId, input, onComplete);
}

std::shared_ptr<AsyncTask<IntersectionComputeInput, IntersectionComputeResult>>
GeometryComputeTasks::createIntersectionTask(
    const std::string& taskId,
    const TopoDS_Shape& shape,
    double tolerance,
    std::function<void(const ComputeResult<IntersectionComputeResult>&)> onComplete)
{
    IntersectionComputeInput input(shape, tolerance);
    
    AsyncTask<IntersectionComputeInput, IntersectionComputeResult>::Config config;
    config.priority = TaskPriority::High;
    config.cacheResult = true;
    config.supportCancellation = true;
    config.enableProgressCallback = true;
    
    auto task = std::make_shared<AsyncTask<IntersectionComputeInput, IntersectionComputeResult>>(
        taskId,
        input,
        [onComplete](const IntersectionComputeInput& input, std::atomic<bool>& cancelled, AsyncTask<IntersectionComputeInput, IntersectionComputeResult>::ProgressFunc progress) {
            return GeometryComputeTasks::computeIntersections(input, cancelled, progress);
        },
        onComplete,
        config
    );
    
    return task;
}

std::shared_ptr<AsyncTask<MeshGenerationInput, MeshData>>
GeometryComputeTasks::createMeshGenerationTask(
    const std::string& taskId,
    const TopoDS_Shape& shape,
    double deflection,
    double angle,
    std::function<void(const ComputeResult<MeshData>&)> onComplete)
{
    MeshGenerationInput input(shape, deflection, angle);
    
    AsyncTask<MeshGenerationInput, MeshData>::Config config;
    config.priority = TaskPriority::Normal;
    config.cacheResult = true;
    config.supportCancellation = true;
    
    auto task = std::make_shared<AsyncTask<MeshGenerationInput, MeshData>>(
        taskId,
        input,
        [onComplete](const MeshGenerationInput& input, std::atomic<bool>& cancelled, AsyncTask<MeshGenerationInput, MeshData>::ProgressFunc progress) {
            return GeometryComputeTasks::generateMesh(input, cancelled, progress);
        },
        onComplete,
        config
    );
    
    return task;
}

std::shared_ptr<AsyncTask<BoundingBoxInput, BoundingBoxResult>>
GeometryComputeTasks::createBoundingBoxTask(
    const std::string& taskId,
    const TopoDS_Shape& shape,
    std::function<void(const ComputeResult<BoundingBoxResult>&)> onComplete)
{
    BoundingBoxInput input(shape);
    
    AsyncTask<BoundingBoxInput, BoundingBoxResult>::Config config;
    config.priority = TaskPriority::High;
    config.cacheResult = true;
    config.supportCancellation = false;
    
    auto task = std::make_shared<AsyncTask<BoundingBoxInput, BoundingBoxResult>>(
        taskId,
        input,
        [onComplete](const BoundingBoxInput& input, std::atomic<bool>& cancelled, AsyncTask<BoundingBoxInput, BoundingBoxResult>::ProgressFunc progress) {
            return GeometryComputeTasks::computeBoundingBox(input, cancelled, progress);
        },
        onComplete,
        config
    );
    
    return task;
}

IntersectionComputeResult GeometryComputeTasks::computeIntersections(
    const IntersectionComputeInput& input,
    std::atomic<bool>& cancelled,
    std::function<void(int, const std::string&)>& progressCallback)
{
    (void)cancelled;
    LOG_INF_S("GeometryComputeTasks: Starting intersection computation");
    
    auto startTime = std::chrono::steady_clock::now();
    
    IntersectionComputeResult result;
    
    try {
        // Collect all edges first for diagnostic and processing
        std::vector<TopoDS_Edge> edges;
        for (TopExp_Explorer exp(input.shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            edges.push_back(TopoDS::Edge(exp.Current()));
        }
        result.edgeCount = edges.size();
        
        LOG_INF_S("GeometryComputeTasks: Processing " + std::to_string(result.edgeCount) + 
                 " edges, tolerance: " + std::to_string(input.tolerance));
        
        // Progress: Starting intersection computation
        if (progressCallback) {
            progressCallback(10, "Starting intersection computation for " + 
                            std::to_string(result.edgeCount) + " edges");
        }
        
        // Always use OriginalEdgeExtractor which has optimized spatial grid filtering
        // This reduces O(n^2) edge pair checks to O(n) using spatial partitioning
        LOG_INF_S("GeometryComputeTasks: Using optimized spatial grid algorithm");
        
        if (progressCallback) {
            const size_t estimatedPairs = result.edgeCount * result.edgeCount / 2;
            progressCallback(20, "Using spatial grid optimization for " + 
                           std::to_string(result.edgeCount) + " edges (avoiding " +
                           std::to_string(estimatedPairs) + " brute-force checks)");
        }
        
        // OriginalEdgeExtractor uses optimized spatial grid to filter candidate pairs
        // This is MUCH faster than brute-force O(n^2) checking
        OriginalEdgeExtractor extractor;
        extractor.findEdgeIntersections(input.shape, result.points, input.tolerance);
        
        if (progressCallback) {
            progressCallback(90, "Spatial grid processing completed, found " + 
                           std::to_string(result.points.size()) + " intersections");
        }

        auto endTime = std::chrono::steady_clock::now();
        result.computeTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        // Progress: Computation completed
        if (progressCallback) {
            progressCallback(100, "Intersection computation completed");
        }
        
        LOG_INF_S("GeometryComputeTasks: Found " + std::to_string(result.points.size()) + 
                  " intersections from " + std::to_string(result.edgeCount) + " edges in " +
                  std::to_string(result.computeTime.count()) + "ms");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("GeometryComputeTasks: Intersection computation failed: " + std::string(e.what()));
        throw;
    }
    
    return result;
}

MeshData GeometryComputeTasks::generateMesh(
    const MeshGenerationInput& input,
    std::atomic<bool>& cancelled,
    std::function<void(int, const std::string&)>& progressCallback)
{
    LOG_INF_S("GeometryComputeTasks: Starting mesh generation");
    
    MeshData result;
    
    try {
        // Progress: Starting mesh generation
        if (progressCallback) {
            progressCallback(5, "Starting mesh generation");
        }

        BRepMesh_IncrementalMesh mesh(input.shape, input.deflection, Standard_False, input.angle);

        if (cancelled.load()) {
            LOG_INF_S("GeometryComputeTasks: Mesh generation cancelled");
            return result;
        }

        // Progress: Mesh creation completed
        if (progressCallback) {
            progressCallback(20, "Mesh creation completed");
        }

        // Progress: Collecting faces
        if (progressCallback) {
            progressCallback(30, "Collecting faces for processing");
        }

        // Collect all faces for parallel processing
        std::vector<TopoDS_Face> faces;
        for (TopExp_Explorer exp(input.shape, TopAbs_FACE); exp.More(); exp.Next()) {
            faces.push_back(TopoDS::Face(exp.Current()));
        }

        // Progress: Starting parallel processing
        if (progressCallback) {
            progressCallback(40, "Starting parallel face processing");
        }

        // Use TBB concurrent vectors for thread-safe accumulation
        tbb::concurrent_vector<float> vertices;
        tbb::concurrent_vector<float> normals;
        tbb::concurrent_vector<unsigned int> indices;

        // Progress tracking for parallel processing
        std::atomic<size_t> processedFaces{0};

        // Parallel processing of faces using TBB
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, faces.size()),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t faceIdx = range.begin(); faceIdx < range.end(); ++faceIdx) {
                    if (cancelled.load()) {
                        break;
                    }

                    const TopoDS_Face& face = faces[faceIdx];
                    TopLoc_Location location;
                    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);

                    if (triangulation.IsNull()) {
                        continue;
                    }

                    gp_Trsf transform = location.Transformation();
                    size_t baseVertex = vertices.size() / 3;

                    // Process vertices
                    for (int i = 1; i <= triangulation->NbNodes(); i++) {
                        gp_Pnt pt = triangulation->Node(i).Transformed(transform);
                        vertices.push_back(static_cast<float>(pt.X()));
                        vertices.push_back(static_cast<float>(pt.Y()));
                        vertices.push_back(static_cast<float>(pt.Z()));
                    }

                    // Process normals if available
                    if (triangulation->HasNormals()) {
                        for (int i = 1; i <= triangulation->NbNodes(); i++) {
                            gp_Dir normal = triangulation->Normal(i);
                            normals.push_back(static_cast<float>(normal.X()));
                            normals.push_back(static_cast<float>(normal.Y()));
                            normals.push_back(static_cast<float>(normal.Z()));
                        }
                    }

                    // Process indices
                    for (int i = 1; i <= triangulation->NbTriangles(); i++) {
                        const Poly_Triangle& tri = triangulation->Triangle(i);
                        int n1, n2, n3;
                        tri.Get(n1, n2, n3);

                        indices.push_back(static_cast<unsigned int>(baseVertex + n1 - 1));
                        indices.push_back(static_cast<unsigned int>(baseVertex + n2 - 1));
                        indices.push_back(static_cast<unsigned int>(baseVertex + n3 - 1));
                    }

                    // Update progress counter
                    size_t currentProcessed = processedFaces.fetch_add(1) + 1;
                    if (progressCallback && currentProcessed % 10 == 0) {  // Update every 10 faces
                        int progressPercent = 40 + static_cast<int>((currentProcessed * 50.0) / faces.size());
                        progressCallback(progressPercent, "Processing face " + std::to_string(currentProcessed) + "/" + std::to_string(faces.size()));
                    }
                }
            },
            tbb::auto_partitioner()
        );

        // Copy results to final structure
        result.vertices.assign(vertices.begin(), vertices.end());
        result.normals.assign(normals.begin(), normals.end());
        result.indices.assign(indices.begin(), indices.end());
        
        result.vertexCount = result.vertices.size() / 3;
        result.triangleCount = result.indices.size() / 3;

        // Progress: Mesh generation completed
        if (progressCallback) {
            progressCallback(100, "Mesh generation completed");
        }

        LOG_INF_S("GeometryComputeTasks: Generated mesh with " +
                  std::to_string(result.vertexCount) + " vertices, " +
                  std::to_string(result.triangleCount) + " triangles, " +
                  std::to_string(result.getMemoryUsage() / 1024) + " KB");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("GeometryComputeTasks: Mesh generation failed: " + std::string(e.what()));
        throw;
    }
    
    return result;
}

BoundingBoxResult GeometryComputeTasks::computeBoundingBox(
    const BoundingBoxInput& input,
    std::atomic<bool>& cancelled,
    std::function<void(int, const std::string&)>& progressCallback)
{
    (void)cancelled;
    BoundingBoxResult result;
    
    try {
        // Progress: Starting bounding box computation
        if (progressCallback) {
            progressCallback(20, "Starting bounding box computation");
        }

        Bnd_Box box;
        BRepBndLib::Add(input.shape, box);

        // Progress: Bounding box calculation completed
        if (progressCallback) {
            progressCallback(80, "Bounding box calculation completed");
        }

        if (!box.IsVoid()) {
            box.Get(result.xMin, result.yMin, result.zMin, result.xMax, result.yMax, result.zMax);
        }

        // Progress: Bounding box computation completed
        if (progressCallback) {
            progressCallback(100, "Bounding box computation completed");
        }
        
        LOG_DBG_S("GeometryComputeTasks: Computed bounding box: [" +
                  std::to_string(result.xMin) + ", " + std::to_string(result.yMin) + ", " + std::to_string(result.zMin) + "] to [" +
                  std::to_string(result.xMax) + ", " + std::to_string(result.yMax) + ", " + std::to_string(result.zMax) + "]");
    }
    catch (const std::exception& e) {
        LOG_ERR_S("GeometryComputeTasks: Bounding box computation failed: " + std::string(e.what()));
        throw;
    }
    
    return result;
}

} // namespace async

