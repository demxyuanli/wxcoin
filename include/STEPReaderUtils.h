#pragma once

#include <vector>
#include <string>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopoDS_Shell.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>

/**
 * @brief Utility functions for STEP file processing
 * 
 * Provides common helper functions used across STEP processing modules
 */
class STEPReaderUtils {
public:
    // Safe property calculation with error handling
    static double safeCalculateArea(const TopoDS_Shape& shape);
    static double safeCalculateVolume(const TopoDS_Shape& shape);
    static gp_Pnt safeCalculateCentroid(const TopoDS_Shape& shape);
    static Bnd_Box safeCalculateBoundingBox(const TopoDS_Shape& shape);

    // Standardized logging functions
    static void logCount(const std::string& prefix, size_t count, const std::string& suffix = "");
    static void logSuccess(const std::string& operation, size_t count, const std::string& unit = "items");

    // Common shape creation utilities
    static TopoDS_Shape createCompoundFromShapes(const std::vector<TopoDS_Shape>& shapes);
    static TopoDS_Shape tryCreateShellFromFaces(const std::vector<TopoDS_Shape>& faces);

    // Common vector initialization with reserve
    template<typename T>
    static std::vector<T> createReservedVector(size_t capacity) {
        std::vector<T> vec;
        vec.reserve(capacity);
        return vec;
    }
};

