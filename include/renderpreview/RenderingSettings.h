#pragma once

#include <string>
#include <wx/colour.h>

// Rendering settings structure for render preview
struct RenderingSettings {
    bool enabled;
    std::string name;
    int mode;              // 0=Solid, 1=Wireframe, 2=Points, 3=HiddenLine, 4=Shaded, 5=ShadedWireframe
    
    // Quality settings
    int quality;           // 0=Low, 1=Medium, 2=High, 3=Ultra
    bool fastMode;
    int lodLevel;          // Level of detail: 0=Off, 1-5
    
    // Transparency settings
    int transparencyType;  // 0=None, 1=Blend, 2=SortedBlend, 3=DelayedBlend
    bool depthSorting;
    float alphaThreshold;
    
    // Shading settings
    bool smoothShading;
    bool flatShading;
    bool phongShading;
    bool gouraudShading;
    
    // Culling settings
    bool backfaceCulling;
    bool frontfaceCulling;
    int cullMode;          // 0=None, 1=Back, 2=Front, 3=Both
    
    // Depth settings
    bool depthTest;
    bool depthWrite;
    int depthFunc;         // OpenGL depth function
    
    // Polygon settings
    int polygonMode;       // 0=Fill, 1=Line, 2=Point
    float lineWidth;
    float pointSize;
    
    // Background settings
    wxColour backgroundColor;
    bool gradientBackground;
    wxColour gradientTopColor;
    wxColour gradientBottomColor;
    
    // Background style settings
    int backgroundStyle;   // 0=Solid, 1=Gradient, 2=Image, 3=Environment, 4=Studio, 5=Outdoor, 6=Industrial
    std::string backgroundImagePath;
    bool backgroundImageEnabled;
    float backgroundImageOpacity;
    int backgroundImageFit;  // 0=Stretch, 1=Fit, 2=Center, 3=Tile
    bool backgroundImageMaintainAspect;
    
    // Performance settings
    bool frustumCulling;
    bool occlusionCulling;
    int maxRenderDistance;
    bool adaptiveQuality;
    
    RenderingSettings()
        : enabled(true)
        , name("Default Rendering")
        , mode(4)  // Shaded
        , quality(2)  // High
        , fastMode(false)
        , lodLevel(0)
        , transparencyType(1)  // Blend
        , depthSorting(true)
        , alphaThreshold(0.5f)
        , smoothShading(true)
        , flatShading(false)
        , phongShading(true)
        , gouraudShading(false)
        , backfaceCulling(true)
        , frontfaceCulling(false)
        , cullMode(1)  // Back
        , depthTest(true)
        , depthWrite(true)
        , depthFunc(4)  // GL_LEQUAL
        , polygonMode(0)  // Fill
        , lineWidth(1.0f)
        , pointSize(1.0f)
        , backgroundColor(173, 204, 255)  // Light blue
        , gradientBackground(false)
        , gradientTopColor(200, 220, 255)
        , gradientBottomColor(150, 180, 255)
        , backgroundStyle(0)  // Solid
        , backgroundImageEnabled(false)
        , backgroundImageOpacity(1.0f)
        , backgroundImageFit(1)  // Fit
        , backgroundImageMaintainAspect(true)
        , frustumCulling(true)
        , occlusionCulling(false)
        , maxRenderDistance(1000)
        , adaptiveQuality(false)
    {}
}; 