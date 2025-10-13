#pragma once

#include <wx/colour.h>

/**
 * @brief Original edges display configuration
 * 
 * Controls visualization of original CAD edges extracted from geometry.
 */
struct OriginalEdgesConfig {
    double samplingDensity = 80.0;      // Sampling points per unit length
    double minLength = 0.01;            // Minimum edge length to display
    bool showLinesOnly = false;         // Show only lines, no curves
    wxColour color{255, 255, 255};      // Edge color (default: white)
    double width = 1.0;                 // Line width
    bool highlightIntersectionNodes = false;
    wxColour intersectionNodeColor{255, 0, 0};  // Node color (default: red)
    double intersectionNodeSize = 3.0;  // Node size in pixels
    
    bool operator==(const OriginalEdgesConfig& other) const {
        return samplingDensity == other.samplingDensity &&
               minLength == other.minLength &&
               showLinesOnly == other.showLinesOnly &&
               width == other.width &&
               highlightIntersectionNodes == other.highlightIntersectionNodes &&
               intersectionNodeSize == other.intersectionNodeSize;
    }
    
    bool operator!=(const OriginalEdgesConfig& other) const {
        return !(*this == other);
    }
};

