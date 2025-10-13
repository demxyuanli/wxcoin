#pragma once

/**
 * @brief Subdivision surface configuration
 * 
 * Controls mesh subdivision parameters for smoother surface approximation.
 */
struct SubdivisionConfig {
    bool enabled = false;
    int level = 2;              // Range: 1-5
    int method = 0;             // 0=Catmull-Clark, 1=Loop, 2=Butterfly, 3=Linear
    double creaseAngle = 30.0;  // Angle in degrees (0-180)
    
    bool operator==(const SubdivisionConfig& other) const {
        return enabled == other.enabled &&
               level == other.level &&
               method == other.method &&
               creaseAngle == other.creaseAngle;
    }
    
    bool operator!=(const SubdivisionConfig& other) const {
        return !(*this == other);
    }
};

