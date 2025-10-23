#include "edges/AsyncEdgeIntersectionComputer.h"
#include "async/AsyncEngineIntegration.h"
#include "async/GeometryComputeTasks.h"
#include "logger/Logger.h"
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <chrono>

namespace async {

AsyncEdgeIntersectionComputer::AsyncEdgeIntersectionComputer(class IAsyncEngine* engine)
    : m_engine(engine)
    , m_computing(false)
{
}

AsyncEdgeIntersectionComputer::~AsyncEdgeIntersectionComputer() {
    cancelComputation();
}

void AsyncEdgeIntersectionComputer::computeIntersectionsAsync(
    const TopoDS_Shape& shape,
    double tolerance,
    ResultCallback onComplete,
    ProgressCallback onProgress)
{
    if (m_computing.load()) {
        LOG_WRN_S("AsyncEdgeIntersectionComputer: Computation already in progress");
        return;
    }

    if (!m_engine) {
        LOG_ERR_S("AsyncEdgeIntersectionComputer: Engine not initialized");
        if (onComplete) {
            onComplete({}, false, "Engine not initialized");
        }
        return;
    }

    m_computing.store(true);
    m_currentTaskId = "edge_intersection_" + 
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

    // Count edges for diagnostic
    size_t edgeCount = 0;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        edgeCount++;
    }

    LOG_INF_S("AsyncEdgeIntersectionComputer: Starting async intersection computation");
    LOG_INF_S("AsyncEdgeIntersectionComputer: Shape has " + std::to_string(edgeCount) + 
             " edges, tolerance: " + std::to_string(tolerance));

    IntersectionComputeInput input(shape, tolerance);

    try {
        m_engine->submitIntersectionTask(
            m_currentTaskId,
            shape,
            tolerance,
            [this, onComplete, edgeCount](bool success, const std::vector<gp_Pnt>& points, const std::string& error) {
                m_computing.store(false);

                if (success) {
                    LOG_INF_S("AsyncEdgeIntersectionComputer: Found " +
                             std::to_string(points.size()) + " intersections from " +
                             std::to_string(edgeCount) + " edges");

                    if (onComplete) {
                        onComplete(points, true, "");
                    }
                } else {
                    LOG_ERR_S("AsyncEdgeIntersectionComputer: Failed: " + error);

                    if (onComplete) {
                        onComplete({}, false, error);
                    }
                }
            }
        );

        if (onProgress) {
            m_engine->setGlobalProgressCallback([onProgress](const std::string& taskId, int progress,
                                                             const std::string& message) {
                onProgress(progress, message);
            });
        }
        
    } catch (const std::exception& e) {
        m_computing.store(false);
        LOG_ERR_S("AsyncEdgeIntersectionComputer: Exception: " + std::string(e.what()));
        
        if (onComplete) {
            onComplete({}, false, std::string(e.what()));
        }
    }
}

void AsyncEdgeIntersectionComputer::cancelComputation() {
    if (m_computing.load() && m_engine && !m_currentTaskId.empty()) {
        LOG_INF_S("AsyncEdgeIntersectionComputer: Cancelling: " + m_currentTaskId);
        m_engine->cancelTask(m_currentTaskId);
        m_computing.store(false);
        m_currentTaskId.clear();
    }
}

} // namespace async
