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
    {}
}; 