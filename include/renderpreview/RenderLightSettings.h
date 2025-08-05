#pragma once

#include <wx/colour.h>
#include <string>

// Light settings structure for render preview
struct RenderLightSettings {
    bool enabled;
    std::string name;
    std::string type;  // "directional", "point", "spot"
    
    // Position and direction
    double positionX, positionY, positionZ;
    double directionX, directionY, directionZ;
    
    // Color and intensity
    wxColour color;
    double intensity;
    
    // Spot light specific
    double spotAngle;
    double spotExponent;
    
    // Animation parameters
    bool animated;
    double animationSpeed;  // rotations per second
    double animationRadius; // for orbital animation
    double animationHeight; // for orbital animation
    
    // Attenuation parameters (for point and spot lights)
    double constantAttenuation;
    double linearAttenuation;
    double quadraticAttenuation;
    
    // Shadow and advanced parameters
    bool castShadows;
    double shadowIntensity;
    
    // Performance parameters
    int priority;  // higher priority lights are kept when limit is reached
    
    RenderLightSettings()
        : enabled(true)
        , name("Light 1")
        , type("directional")
        , positionX(0.0), positionY(0.0), positionZ(10.0)
        , directionX(0.0), directionY(0.0), directionZ(-1.0)
        , color(255, 255, 255)
        , intensity(1.0)
        , spotAngle(30.0)
        , spotExponent(1.0)
        , animated(false)
        , animationSpeed(1.0)
        , animationRadius(5.0)
        , animationHeight(0.0)
        , constantAttenuation(1.0)
        , linearAttenuation(0.0)
        , quadraticAttenuation(0.0)
        , castShadows(false)
        , shadowIntensity(0.5)
        , priority(1)
    {}
}; 