#pragma once

#include <OpenCASCADE/Quantity_Color.hxx>

/**
 * @brief Normal display configuration
 * 
 * Controls visualization of surface normals for debugging and quality checking.
 */
struct NormalDisplayConfig {
    bool showNormals = false;
    double length = 0.5;
    Quantity_Color correctColor{1.0, 0.0, 0.0, Quantity_TOC_RGB};    // Red for correct
    Quantity_Color incorrectColor{0.0, 1.0, 0.0, Quantity_TOC_RGB};  // Green for incorrect
    bool consistencyMode = true;
    bool debugMode = false;
    
    bool operator==(const NormalDisplayConfig& other) const {
        return showNormals == other.showNormals &&
               length == other.length &&
               consistencyMode == other.consistencyMode &&
               debugMode == other.debugMode;
    }
    
    bool operator!=(const NormalDisplayConfig& other) const {
        return !(*this == other);
    }
};

