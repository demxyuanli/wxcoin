#pragma once

#include <string>

// Anti-aliasing settings structure for render preview
struct AntiAliasingSettings {
    bool enabled;
    std::string name;
    int method;        // 0=None, 1=MSAA, 2=FXAA, 3=SSAA, 4=TAA
    int msaaSamples;   // 2, 4, 8, 16
    bool fxaaEnabled;
    float fxaaQuality; // 0.0-1.0
    bool ssaaEnabled;
    int ssaaFactor;    // 2, 4
    bool taaEnabled;
    float taaStrength; // 0.0-1.0
    
    // Advanced settings
    bool adaptiveEnabled;     // Adaptive anti-aliasing
    float edgeThreshold;      // Edge detection threshold
    bool temporalFiltering;   // Temporal filtering
    float jitterStrength;     // Jitter strength for TAA
    
    AntiAliasingSettings()
        : enabled(true)
        , name("Default AA")
        , method(1)  // MSAA
        , msaaSamples(4)
        , fxaaEnabled(false)
        , fxaaQuality(0.75f)
        , ssaaEnabled(false)
        , ssaaFactor(2)
        , taaEnabled(false)
        , taaStrength(0.8f)
        , adaptiveEnabled(false)
        , edgeThreshold(0.1f)
        , temporalFiltering(false)
        , jitterStrength(0.5f)
    {}
}; 