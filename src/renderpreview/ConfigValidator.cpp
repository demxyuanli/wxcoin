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
    if (mode < 0 || mode > 20) {
        return ValidationResult(false, "Rendering mode must be between 0 and 20");
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

ValidationResult ConfigValidator::validateRenderingSettings(const RenderingSettings& settings)
{
    // Validate rendering mode
    auto modeResult = validateRenderingMode(settings.mode);
    if (!modeResult.isValid) {
        return modeResult;
    }
    
    // Validate quality settings
    if (settings.quality < 0 || settings.quality > 3) {
        return ValidationResult(false, "Quality must be between 0 (Low) and 3 (Ultra)");
    }
    
    // Validate polygon mode
    if (settings.polygonMode < 0 || settings.polygonMode > 2) {
        return ValidationResult(false, "Polygon mode must be 0 (Fill), 1 (Line), or 2 (Point)");
    }
    
    // Validate transparency type
    if (settings.transparencyType < 0 || settings.transparencyType > 3) {
        return ValidationResult(false, "Transparency type must be between 0 and 3");
    }
    
    // Validate line width
    if (!isValidRange(settings.lineWidth, 0.1, 10.0, "Line width")) {
        return ValidationResult(false, "Line width must be between 0.1 and 10.0");
    }
    
    // Validate point size
    if (!isValidRange(settings.pointSize, 1.0, 20.0, "Point size")) {
        return ValidationResult(false, "Point size must be between 1.0 and 20.0");
    }
    
    // Validate background color
    if (!isValidColor(settings.backgroundColor)) {
        return ValidationResult(false, "Invalid background color");
    }
    
    return ValidationResult(true, "Rendering settings are valid");
}

ValidationResult ConfigValidator::validateRenderingConfiguration(const RenderingSettings& settings)
{
    // First validate basic settings
    auto basicResult = validateRenderingSettings(settings);
    if (!basicResult.isValid) {
        return basicResult;
    }
    
    // Check for conflicting settings
    if (settings.mode == 1 && settings.polygonMode != 1) { // Wireframe mode
        return ValidationResult(false, "Wireframe mode requires polygon mode to be Line (1)");
    }
    
    if (settings.mode == 2 && settings.polygonMode != 2) { // Points mode
        return ValidationResult(false, "Points mode requires polygon mode to be Point (2)");
    }
    
    // Check shading consistency
    if (settings.phongShading && settings.gouraudShading) {
        return ValidationResult(false, "Cannot enable both Phong and Gouraud shading simultaneously");
    }
    
    // Check performance-related conflicts
    if (settings.fastMode && settings.quality > 1) {
        return ValidationResult(false, "Fast mode is incompatible with High or Ultra quality settings");
    }
    
    return ValidationResult(true, "Rendering configuration is consistent");
}

ValidationResult ConfigValidator::validatePresetCompatibility(const std::string& presetName, const RenderingSettings& settings)
{
    // Validate that custom settings are compatible with preset requirements
    if (presetName.find("Mobile") != std::string::npos) {
        if (settings.quality > 1) {
            return ValidationResult(false, "Mobile presets should not use High or Ultra quality");
        }
        if (settings.occlusionCulling) {
            return ValidationResult(false, "Mobile presets should not use occlusion culling");
        }
    }
    
    if (presetName.find("Gaming") != std::string::npos) {
        if (!settings.frustumCulling) {
            return ValidationResult(false, "Gaming presets should enable frustum culling for performance");
        }
    }
    
    if (presetName.find("CAD") != std::string::npos) {
        if (settings.fastMode) {
            return ValidationResult(false, "CAD presets should not use fast mode for accuracy");
        }
    }
    
    return ValidationResult(true, "Settings are compatible with preset " + presetName);
}

ValidationResult ConfigValidator::validatePerformanceImpact(const RenderingSettings& settings, float maxImpactThreshold)
{
    float impactFactor = 0.0f;
    
    // Calculate performance impact based on settings
    impactFactor += settings.quality * 0.3f;  // Quality has major impact
    
    if (settings.phongShading) impactFactor += 0.2f;
    if (settings.smoothShading) impactFactor += 0.1f;
    if (settings.occlusionCulling) impactFactor += 0.15f;
    if (settings.transparencyType > 1) impactFactor += 0.2f;
    if (!settings.frustumCulling) impactFactor += 0.1f;
    if (settings.adaptiveQuality) impactFactor -= 0.1f;  // Reduces impact
    if (settings.fastMode) impactFactor -= 0.2f;  // Reduces impact
    
    if (impactFactor > maxImpactThreshold) {
        return ValidationResult(false, 
            "Performance impact (" + std::to_string(impactFactor) + 
            ") exceeds threshold (" + std::to_string(maxImpactThreshold) + ")");
    }
    
    return ValidationResult(true, 
        "Performance impact (" + std::to_string(impactFactor) + ") is acceptable");
}