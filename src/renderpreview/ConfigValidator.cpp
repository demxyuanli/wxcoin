#include "renderpreview/ConfigValidator.h"
#include <wx/colour.h>
#include <cmath>

ValidationResult ConfigValidator::validateLightSettings(const RenderLightSettings& light)
{
    // Validate name
    if (light.name.empty()) {
        return ValidationResult(false, "Light name cannot be empty");
    }
    
    // Validate type
    if (light.type != "directional" && light.type != "point" && light.type != "spot") {
        return ValidationResult(false, "Invalid light type. Must be 'directional', 'point', or 'spot'");
    }
    
    // Validate position
    if (!isValidPosition(light.positionX, light.positionY, light.positionZ)) {
        return ValidationResult(false, "Invalid light position");
    }
    
    // Validate direction
    if (!isValidDirection(light.directionX, light.directionY, light.directionZ)) {
        return ValidationResult(false, "Invalid light direction");
    }
    
    // Validate color
    if (!isValidColor(light.color)) {
        return ValidationResult(false, "Invalid light color");
    }
    
    // Validate intensity
    if (!isValidIntensity(light.intensity)) {
        return ValidationResult(false, "Light intensity must be between 0.0 and 10.0");
    }
    
    // Validate spot light specific settings
    if (light.type == "spot") {
        if (!isValidRange(light.spotAngle, 0.0, 180.0, "Spot angle")) {
            return ValidationResult(false, "Spot angle must be between 0.0 and 180.0 degrees");
        }
        if (!isValidRange(light.spotExponent, 0.0, 128.0, "Spot exponent")) {
            return ValidationResult(false, "Spot exponent must be between 0.0 and 128.0");
        }
    }
    
    return ValidationResult(true);
}

ValidationResult ConfigValidator::validateLightSettings(const std::vector<RenderLightSettings>& lights)
{
    if (lights.empty()) {
        return ValidationResult(false, "At least one light must be defined");
    }
    
    for (size_t i = 0; i < lights.size(); ++i) {
        auto result = validateLightSettings(lights[i]);
        if (!result.isValid) {
            return ValidationResult(false, wxString::Format("Light %zu: %s", i + 1, result.errorMessage));
        }
    }
    
    return ValidationResult(true);
}

ValidationResult ConfigValidator::validateAntiAliasingSettings(int method, int msaaSamples, bool fxaaEnabled)
{
    // Validate method
    if (method < 0 || method > 25) {
        return ValidationResult(false, "Anti-aliasing method must be between 0 and 25");
    }
    
    // Validate MSAA samples
    if (method == 1) { // MSAA method
        if (msaaSamples < 2 || msaaSamples > 16) {
            return ValidationResult(false, "MSAA samples must be between 2 and 16");
        }
        // Check if samples is a power of 2
        if ((msaaSamples & (msaaSamples - 1)) != 0) {
            return ValidationResult(false, "MSAA samples must be a power of 2");
        }
    }
    
    return ValidationResult(true);
}

ValidationResult ConfigValidator::validateRenderingMode(int mode)
{
    if (mode < 0 || mode > 4) {
        return ValidationResult(false, "Rendering mode must be between 0 and 4");
    }
    
    return ValidationResult(true);
}

ValidationResult ConfigValidator::validateMaterialSettings(float ambient, float diffuse, float specular, float shininess, float transparency)
{
    if (!isValidRange(ambient, 0.0f, 1.0f, "Ambient")) {
        return ValidationResult(false, "Ambient value must be between 0.0 and 1.0");
    }
    
    if (!isValidRange(diffuse, 0.0f, 1.0f, "Diffuse")) {
        return ValidationResult(false, "Diffuse value must be between 0.0 and 1.0");
    }
    
    if (!isValidRange(specular, 0.0f, 1.0f, "Specular")) {
        return ValidationResult(false, "Specular value must be between 0.0 and 1.0");
    }
    
    if (!isValidRange(shininess, 0.0f, 128.0f, "Shininess")) {
        return ValidationResult(false, "Shininess value must be between 0.0 and 128.0");
    }
    
    if (!isValidRange(transparency, 0.0f, 1.0f, "Transparency")) {
        return ValidationResult(false, "Transparency value must be between 0.0 and 1.0");
    }
    
    return ValidationResult(true);
}

ValidationResult ConfigValidator::validateTextureSettings(bool enabled, int mode, float scale)
{
    if (enabled) {
        if (mode < 0 || mode > 2) {
            return ValidationResult(false, "Texture mode must be between 0 and 2");
        }
        
        if (!isValidRange(scale, 0.1f, 10.0f, "Texture scale")) {
            return ValidationResult(false, "Texture scale must be between 0.1 and 10.0");
        }
    }
    
    return ValidationResult(true);
}

bool ConfigValidator::isValidColor(const wxColour& color)
{
    return color.Red() >= 0 && color.Red() <= 255 &&
           color.Green() >= 0 && color.Green() <= 255 &&
           color.Blue() >= 0 && color.Blue() <= 255;
}

bool ConfigValidator::isValidIntensity(double intensity)
{
    return intensity >= 0.0 && intensity <= 10.0;
}

bool ConfigValidator::isValidPosition(double x, double y, double z)
{
    // Allow reasonable position values
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
           x >= -1000.0 && x <= 1000.0 &&
           y >= -1000.0 && y <= 1000.0 &&
           z >= -1000.0 && z <= 1000.0;
}

bool ConfigValidator::isValidDirection(double x, double y, double z)
{
    // Direction should be a normalized vector (length close to 1.0)
    double length = std::sqrt(x*x + y*y + z*z);
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
           length > 0.1; // Allow some tolerance for non-normalized vectors
}

bool ConfigValidator::isValidRange(double value, double min, double max, const wxString& name)
{
    return std::isfinite(value) && value >= min && value <= max;
} 