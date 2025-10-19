#include "viewer/MeshParameterAdvisor.h"
#include "logger/Logger.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <cmath>
#include <algorithm>

ShapeComplexity MeshParameterAdvisor::analyzeShape(const TopoDS_Shape& shape) {
    ShapeComplexity complexity;
    
    if (shape.IsNull()) { 
        LOG_WRN_S("Cannot analyze null shape");
        return complexity;
    }
    
    try {
        // Calculate bounding box
        Bnd_Box bbox;
        BRepBndLib::Add(shape, bbox);
        
        if (bbox.IsVoid()) {
            LOG_WRN_S("Shape has void bounding box");
            return complexity;
        }
        
        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        
        double dx = xmax - xmin;
        double dy = ymax - ymin;
        double dz = zmax - zmin;
        complexity.boundingBoxSize = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        // Count faces and edges
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            complexity.faceCount++;
        }
        
        for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            complexity.edgeCount++;
        }
        
        // Calculate surface area
        GProp_GProps surfaceProps;
        BRepGProp::SurfaceProperties(shape, surfaceProps);
        complexity.surfaceArea = surfaceProps.Mass();
        
        // Calculate volume (for solids)
        if (shape.ShapeType() == TopAbs_SOLID || shape.ShapeType() == TopAbs_COMPOUND) {
            try {
                GProp_GProps volumeProps;
                BRepGProp::VolumeProperties(shape, volumeProps);
                double volume = volumeProps.Mass();
                
                if (complexity.surfaceArea > 0 && volume > 0) {
                    complexity.volumeToSurfaceRatio = volume / complexity.surfaceArea;
                }
            } catch (...) {
                // Not a solid, skip volume calculation
            }
        }
        
        // Check for complex surfaces
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            TopoDS_Face face = TopoDS::Face(exp.Current());
            BRepAdaptor_Surface surf(face);
            GeomAbs_SurfaceType type = surf.GetType();
            
            if (type == GeomAbs_BSplineSurface || type == GeomAbs_BezierSurface) {
                complexity.hasComplexSurfaces = true;
                break;
            }
        }
        
        // Estimate average curvature (simplified metric)
        if (complexity.surfaceArea > 0 && complexity.faceCount > 0) {
            complexity.avgCurvature = complexity.faceCount / complexity.surfaceArea;
        }

        
    } catch (const std::exception& e) {
        LOG_ERR_S("Error analyzing shape: " + std::string(e.what()));
    }
    
    return complexity;
}

MeshParameters MeshParameterAdvisor::recommendParameters(const TopoDS_Shape& shape) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    MeshParameters params;
    params.relative = false;
    params.inParallel = (complexity.faceCount > 100);
    
    // Base deflection on bounding box size
    if (complexity.boundingBoxSize < 10.0) {
        // Small parts - fine detail
        params.deflection = 0.001;
        params.angularDeflection = 0.05;
    } else if (complexity.boundingBoxSize < 100.0) {
        // Medium parts - standard quality
        params.deflection = 0.01;
        params.angularDeflection = 0.1;
    } else if (complexity.boundingBoxSize < 1000.0) {
        // Large parts - balanced
        params.deflection = 0.1;
        params.angularDeflection = 0.2;
    } else {
        // Very large assemblies - coarse
        params.deflection = 1.0;
        params.angularDeflection = 0.5;
    }
    
    // Adjust for complexity
    if (complexity.hasComplexSurfaces) {
        params.deflection *= 0.5;
        params.angularDeflection *= 0.7;
    }
    
    // Adjust for high curvature
    if (complexity.avgCurvature > 0.1) {
        params.deflection *= 0.7;
        params.angularDeflection *= 0.8;
    }
    
    // Adjust for many faces (assemblies)
    if (complexity.faceCount > 1000) {
        params.deflection *= 1.5;  // Coarser for assemblies
    }
    
    // Ensure reasonable limits
    params.deflection = std::max(0.0001, std::min(10.0, params.deflection));
    params.angularDeflection = std::max(0.01, std::min(1.0, params.angularDeflection));

    
    return params;
}

size_t MeshParameterAdvisor::estimateTriangleCount(const TopoDS_Shape& shape, 
                                                    const MeshParameters& params) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    if (complexity.surfaceArea <= 0) {
        return 0;
    }
    
    // Estimate based on surface area and deflection
    // Average triangle area â‰?deflectionÂ²
    double avgTriangleArea = params.deflection * params.deflection;
    
    // Roughly 2 triangles per unit area
    size_t estimate = static_cast<size_t>(complexity.surfaceArea / avgTriangleArea * 2.0);
    
    // Apply complexity factors
    if (complexity.hasComplexSurfaces) {
        estimate = static_cast<size_t>(estimate * 1.5);
    }
    
    if (complexity.avgCurvature > 0.1) {
        estimate = static_cast<size_t>(estimate * 1.3);
    }
    
    // Angular deflection affects density
    double angularFactor = 0.2 / std::max(0.01, params.angularDeflection);
    estimate = static_cast<size_t>(estimate * angularFactor);
    
    
    return estimate;
}

MeshParameters MeshParameterAdvisor::getPresetParameters(const TopoDS_Shape& shape,
                                                         MeshQualityPreset preset) {
    switch (preset) {
        case MeshQualityPreset::Draft:
            return getDraftPreset(shape);
        case MeshQualityPreset::Low:
            return getLowPreset(shape);
        case MeshQualityPreset::Medium:
            return getMediumPreset(shape);
        case MeshQualityPreset::High:
            return getHighPreset(shape);
        case MeshQualityPreset::VeryHigh:
            return getVeryHighPreset(shape);
        default:
            return getMediumPreset(shape);
    }
}

MeshParameters MeshParameterAdvisor::getDraftPreset(const TopoDS_Shape& shape) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    MeshParameters params;
    params.deflection = complexity.boundingBoxSize * 0.05;  // 5% of model size
    params.angularDeflection = 0.5;
    params.relative = false;
    params.inParallel = (complexity.faceCount > 50);
    
    return params;
}

MeshParameters MeshParameterAdvisor::getLowPreset(const TopoDS_Shape& shape) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    MeshParameters params;
    params.deflection = complexity.boundingBoxSize * 0.02;  // 2% of model size
    params.angularDeflection = 0.3;
    params.relative = false;
    params.inParallel = (complexity.faceCount > 100);
    
    return params; 
}

MeshParameters MeshParameterAdvisor::getMediumPreset(const TopoDS_Shape& shape) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    MeshParameters params;
    params.deflection = complexity.boundingBoxSize * 0.01;  // 1% of model size
    params.angularDeflection = 0.1;
    params.relative = false;
    params.inParallel = (complexity.faceCount > 100);
    
    return params;
}

MeshParameters MeshParameterAdvisor::getHighPreset(const TopoDS_Shape& shape) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    MeshParameters params;
    params.deflection = complexity.boundingBoxSize * 0.005;  // 0.5% of model size
    params.angularDeflection = 0.05;
    params.relative = false;
    params.inParallel = true;
    
    return params;
}

MeshParameters MeshParameterAdvisor::getVeryHighPreset(const TopoDS_Shape& shape) {
    ShapeComplexity complexity = analyzeShape(shape);
    
    MeshParameters params;
    params.deflection = complexity.boundingBoxSize * 0.001;  // 0.1% of model size
    params.angularDeflection = 0.02;
    params.relative = false;
    params.inParallel = true;
    
    return params;
}

bool MeshParameterAdvisor::validateParameters(const MeshParameters& params,
                                              const TopoDS_Shape* shape) {
    // Check basic validity
    if (params.deflection <= 0 || params.deflection > 1000.0) {
        LOG_WRN_S("Invalid deflection: " + std::to_string(params.deflection));
        return false;
    }
    
    if (params.angularDeflection <= 0 || params.angularDeflection > M_PI) {
        LOG_WRN_S("Invalid angular deflection: " + std::to_string(params.angularDeflection));
        return false;
    }
    
    // Context-aware validation
    if (shape != nullptr) {
        ShapeComplexity complexity = analyzeShape(*shape);
        
        // Warn if deflection is too coarse
        if (params.deflection > complexity.boundingBoxSize * 0.1) {
            LOG_WRN_S("Deflection may be too coarse for this model");
        }
        
        // Warn if deflection is too fine (performance issue)
        if (params.deflection < complexity.boundingBoxSize * 0.0001) {
            LOG_WRN_S("Deflection may be too fine, could cause performance issues");
        }
    }
    
    return true;
}

double MeshParameterAdvisor::getRecommendedDeflection(double boundingBoxSize, 
                                                      double quality) {
    // Quality: 0.0 (coarse) to 1.0 (fine)
    quality = std::max(0.0, std::min(1.0, quality));
    
    // Map quality to percentage of bbox size
    // Quality 0.0 -> 5% of bbox
    // Quality 0.5 -> 1% of bbox  
    // Quality 1.0 -> 0.1% of bbox
    double percentage = 0.05 * std::pow(0.1, quality * 2.0);
    
    return boundingBoxSize * percentage;
}

double MeshParameterAdvisor::estimateMemoryUsage(size_t triangleCount) {
    // Rough estimate:
    // - 3 vertices per triangle: 3 * 3 * 8 bytes (3 doubles) = 72 bytes
    // - 3 normals per triangle: 3 * 3 * 8 bytes = 72 bytes
    // - 3 indices per triangle: 3 * 4 bytes = 12 bytes
    // - Plus overhead: ~50%
    // Total: ~230 bytes per triangle
    
    const double bytesPerTriangle = 230.0;
    double totalBytes = triangleCount * bytesPerTriangle;
    double megabytes = totalBytes / (1024.0 * 1024.0);
    
    return megabytes;
}

