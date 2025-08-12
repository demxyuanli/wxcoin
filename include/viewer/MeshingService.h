#pragma once

#include <memory>
#include <vector>

struct MeshParameters;
class OCCGeometry;

class MeshingService {
public:
    MeshingService() = default;

    void applyAndRemesh(
        const MeshParameters& meshParams,
        const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        bool smoothingEnabled,
        int smoothingIterations,
        double smoothingStrength,
        double smoothingCreaseAngle,
        bool subdivisionEnabled,
        int subdivisionLevel,
        int subdivisionMethod,
        double subdivisionCreaseAngle,
        int tessellationMethod,
        int tessellationQuality,
        double featurePreservation,
        bool adaptiveMeshing,
        bool parallelProcessing
    );
};


