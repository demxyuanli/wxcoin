#ifndef MESH_PARAM_NAMES_H
#define MESH_PARAM_NAMES_H

/**
 * MeshParamNames - Name constants for mesh parameters
 * Used by MeshParameterManager to identify parameters
 */
namespace MeshParamNames {
    
    namespace BasicMesh {
        static const char* DEFLECTION = "deflection";
        static const char* ANGULAR_DEFLECTION = "angular_deflection";
        static const char* RELATIVE = "relative_precision";
        static const char* IN_PARALLEL = "in_parallel";
    }
    
    namespace LOD {
        static const char* ENABLED = "lod_enabled";
        static const char* ROUGH_DEFLECTION = "lod_rough_deflection";
        static const char* FINE_DEFLECTION = "lod_fine_deflection";
        static const char* TRANSITION_TIME = "lod_transition_time";
    }
    
    namespace Subdivision {
        static const char* ENABLED = "subdivision_enabled";
        static const char* LEVEL = "subdivision_level";
        static const char* METHOD = "subdivision_method";
        static const char* CREASE_ANGLE = "subdivision_crease_angle";
    }
    
    namespace Smoothing {
        static const char* ENABLED = "smoothing_enabled";
        static const char* METHOD = "smoothing_method";
        static const char* ITERATIONS = "smoothing_iterations";
        static const char* STRENGTH = "smoothing_strength";
        static const char* CREASE_ANGLE = "smoothing_crease_angle";
    }
    
    namespace Tessellation {
        static const char* METHOD = "tessellation_method";
        static const char* QUALITY = "tessellation_quality";
        static const char* FEATURE_PRESERVATION = "feature_preservation";
    }
    
    namespace Performance {
        static const char* PARALLEL_PROCESSING = "parallel_processing";
        static const char* ADAPTIVE_MESHING = "adaptive_meshing";
    }
}

#endif // MESH_PARAM_NAMES_H
