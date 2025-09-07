#pragma once

#include <wx/string.h>
#include <vector>
#include "renderpreview/RenderLightSettings.h"
#include "renderpreview/RenderingSettings.h"

// Configuration validation result
struct ValidationResult {
    bool isValid;
    wxString errorMessage;
    
    ValidationResult(bool valid = true, const wxString& message = wxEmptyString)
        : isValid(valid), errorMessage(message) {}
};

class ConfigValidator
{
public:
    // Validate lighting settings
    static ValidationResult validateLightSettings(const RenderLightSettings& light);
    static ValidationResult validateLightSettings(const std::vector<RenderLightSettings>& lights);
    
    // Validate anti-aliasing settings
    static ValidationResult validateAntiAliasingSettings(int method, int msaaSamples, bool fxaaEnabled);
    
    // Validate rendering mode
    static ValidationResult validateRenderingMode(int mode);
    
    // Validate material settings
    static ValidationResult validateMaterialSettings(float ambient, float diffuse, float specular, float shininess, float transparency);
    
    // Validate texture settings
    static ValidationResult validateTextureSettings(bool enabled, int mode, float scale);
    
    // General validation helpers
    static bool isValidColor(const wxColour& color);
    static bool isValidIntensity(double intensity);
    static bool isValidPosition(double x, double y, double z);
    static bool isValidDirection(double x, double y, double z);
    static bool isValidRange(double value, double min, double max, const wxString& name);
    
    // Validate complete rendering settings
    static ValidationResult validateRenderingSettings(const RenderingSettings& settings);
    
    // Validate rendering configuration consistency
    static ValidationResult validateRenderingConfiguration(const RenderingSettings& settings);
    
    // Validate preset compatibility
    static ValidationResult validatePresetCompatibility(const std::string& presetName, const RenderingSettings& settings);
    
    // Performance impact validation
    static ValidationResult validatePerformanceImpact(const RenderingSettings& settings, float maxImpactThreshold = 1.0f);
};