#ifndef MESH_PARAMETER_VALIDATOR_H
#define MESH_PARAMETER_VALIDATOR_H

#include <memory>
#include <string>
#include "rendering/GeometryProcessor.h"

class OCCGeometry;

/**
 * MeshParameterValidator - Validates mesh parameters and ensures
 * consistency between dialog settings and actual geometry representation
 */
class MeshParameterValidator {
public:
    static MeshParameterValidator& getInstance();
    
    // Core validation functions
    void validateMeshCoherence(std::shared_ptr<OCCGeometry> geometry, 
                              const MeshParameters& params);
    
    void validateParameterRanges(const MeshParameters& params);
    void validateGeometryShape(std::shared_ptr<OCCGeometry> geometry);
    void validateMeshConsistency(std::shared_ptr<OCCGeometry> geometry, 
                                const MeshParameters& params);
    void validateCoinRepresentation(std::shared_ptr<OCCGeometry> geometry);
    
    // Report generation
    std::string generateValidationReport(std::shared_ptr<OCCGeometry> geometry,
                                         const MeshParameters& params);
    
    void validateAndSaveReport(const std::string& filename, 
                              std::shared_ptr<OCCGeometry> geometry,
                              const MeshParameters& params);

private:
    MeshParameterValidator();
    ~MeshParameterValidator();
    
    MeshParameterValidator(const MeshParameterValidator&) = delete;
    MeshParameterValidator& operator=(const MeshParameterValidator&) = delete;
};

#endif // MESH_PARAMETER_VALIDATOR_H
