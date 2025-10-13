#pragma once

/**
 * @brief Mesh smoothing configuration
 * 
 * Controls mesh smoothing algorithms for surface quality improvement.
 */
struct SmoothingConfig {
    bool enabled = false;
    int method = 0;             // 0=Laplacian, 1=Taubin, 2=HC, 3=Mean Curvature
    int iterations = 2;         // Range: 1-10
    double strength = 0.5;      // Range: 0.01-1.0
    double creaseAngle = 30.0;  // Angle in degrees (0-180)
    
    bool operator==(const SmoothingConfig& other) const {
        return enabled == other.enabled &&
               method == other.method &&
               iterations == other.iterations &&
               strength == other.strength &&
               creaseAngle == other.creaseAngle;
    }
    
    bool operator!=(const SmoothingConfig& other) const {
        return !(*this == other);
    }
};

