#pragma once

/**
 * @brief Advanced tessellation configuration
 * 
 * Controls tessellation method, quality and optimization strategies.
 */
struct TessellationConfig {
    int method = 0;                     // 0=Standard, 1=Adaptive, 2=Quality, 3=Performance
    int quality = 2;                    // Range: 1-5 (1=fast, 5=best)
    double featurePreservation = 0.5;   // Range: 0.0-1.0
    bool parallelProcessing = true;     // Enable multi-threading
    bool adaptiveMeshing = false;       // Use adaptive refinement
    
    bool operator==(const TessellationConfig& other) const {
        return method == other.method &&
               quality == other.quality &&
               featurePreservation == other.featurePreservation &&
               parallelProcessing == other.parallelProcessing &&
               adaptiveMeshing == other.adaptiveMeshing;
    }
    
    bool operator!=(const TessellationConfig& other) const {
        return !(*this == other);
    }
};

