#pragma once

#include <memory>
#include <functional>
#include <atomic>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
class IAsyncEngine;
#include "edges/extractors/OriginalEdgeExtractor.h"

namespace async {

class AsyncEdgeIntersectionComputer {
public:
    using ResultCallback = std::function<void(const std::vector<gp_Pnt>&, bool success, const std::string& error)>;
    using ProgressCallback = std::function<void(int progress, const std::string& message)>;

    explicit AsyncEdgeIntersectionComputer(class IAsyncEngine* engine);
    ~AsyncEdgeIntersectionComputer();

    void computeIntersectionsAsync(
        const TopoDS_Shape& shape,
        double tolerance,
        ResultCallback onComplete,
        ProgressCallback onProgress = nullptr
    );

    void cancelComputation();
    bool isComputing() const { return m_computing.load(); }

private:
    class IAsyncEngine* m_engine;
    std::atomic<bool> m_computing{false};
    std::string m_currentTaskId;
};

} // namespace async


