#pragma once

#include "async/AsyncComputeEngine.h"
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <any>

namespace async {

struct IntersectionComputeInput {
    TopoDS_Shape shape;
    double tolerance;
    
    IntersectionComputeInput() = default;
    IntersectionComputeInput(const TopoDS_Shape& s, double t) 
        : shape(s), tolerance(t) {}
};

struct IntersectionComputeResult {
    std::vector<gp_Pnt> points;
    size_t edgeCount{0};
    std::chrono::milliseconds computeTime{0};
    
    IntersectionComputeResult() = default;
};

struct MeshGenerationInput {
    TopoDS_Shape shape;
    double deflection;
    double angle;
    
    MeshGenerationInput() = default;
    MeshGenerationInput(const TopoDS_Shape& s, double d, double a)
        : shape(s), deflection(d), angle(a) {}
};

struct MeshData {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> indices;
    size_t vertexCount{0};
    size_t triangleCount{0};
    
    MeshData() = default;
    
    size_t getMemoryUsage() const {
        return vertices.size() * sizeof(float) +
               normals.size() * sizeof(float) +
               indices.size() * sizeof(unsigned int);
    }
};

struct BoundingBoxInput {
    TopoDS_Shape shape;
    
    explicit BoundingBoxInput(const TopoDS_Shape& s) : shape(s) {}
};

struct BoundingBoxResult {
    double xMin, yMin, zMin;
    double xMax, yMax, zMax;
    
    BoundingBoxResult() 
        : xMin(0), yMin(0), zMin(0)
        , xMax(0), yMax(0), zMax(0) {}
};

class GeometryComputeTasks {
public:
    // Task factory registration system
    using TaskFactory = std::function<std::any(const std::string&,
                                              std::any,
                                              std::function<void(std::any)>)>;

    static bool registerTaskFactory(const std::string& taskType, TaskFactory factory);
    static bool unregisterTaskFactory(const std::string& taskType);
    static std::any createTask(const std::string& taskType,
                               const std::string& taskId,
                               std::any input,
                               std::function<void(std::any)> onComplete);
    static std::shared_ptr<AsyncTask<IntersectionComputeInput, IntersectionComputeResult>>
    createIntersectionTask(
        const std::string& taskId,
        const TopoDS_Shape& shape,
        double tolerance,
        std::function<void(const ComputeResult<IntersectionComputeResult>&)> onComplete);
    
    static std::shared_ptr<AsyncTask<MeshGenerationInput, MeshData>>
    createMeshGenerationTask(
        const std::string& taskId,
        const TopoDS_Shape& shape,
        double deflection,
        double angle,
        std::function<void(const ComputeResult<MeshData>&)> onComplete);
    
    static std::shared_ptr<AsyncTask<BoundingBoxInput, BoundingBoxResult>>
    createBoundingBoxTask(
        const std::string& taskId,
        const TopoDS_Shape& shape,
        std::function<void(const ComputeResult<BoundingBoxResult>&)> onComplete);

private:
    // Task factory registry
    static std::unordered_map<std::string, TaskFactory>& getTaskFactories();

    static IntersectionComputeResult computeIntersections(
        const IntersectionComputeInput& input,
        std::atomic<bool>& cancelled,
        std::function<void(int, const std::string&)>& progressCallback);

    static MeshData generateMesh(
        const MeshGenerationInput& input,
        std::atomic<bool>& cancelled,
        std::function<void(int, const std::string&)>& progressCallback);

    static BoundingBoxResult computeBoundingBox(
        const BoundingBoxInput& input,
        std::atomic<bool>& cancelled,
        std::function<void(int, const std::string&)>& progressCallback);
};

} // namespace async

