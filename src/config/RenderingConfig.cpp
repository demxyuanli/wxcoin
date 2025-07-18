#include "config/RenderingConfig.h"
#include "logger/Logger.h"
#include <fstream>
#include <sstream>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

// Initialize static member
std::map<RenderingConfig::MaterialPreset, RenderingConfig::MaterialSettings> RenderingConfig::s_materialPresets;

RenderingConfig& RenderingConfig::getInstance()
{
    static RenderingConfig instance;
    return instance;
}

RenderingConfig::RenderingConfig()
    : m_autoSave(true)
{
    // Initialize material presets
    initializeMaterialPresets();
    
    // Load configuration from file on startup
    loadFromFile();
}

void RenderingConfig::initializeMaterialPresets()
{
    if (!s_materialPresets.empty()) {
        return; // Already initialized
    }
    
    // Glass material
    s_materialPresets[MaterialPreset::Glass] = MaterialSettings(
        Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB),    // ambient
        Quantity_Color(0.8, 0.9, 1.0, Quantity_TOC_RGB),    // diffuse (light blue)
        Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),    // specular (white)
        128.0,  // shininess (very shiny)
        0.7     // transparency (70% transparent)
    );
    
    // Metal material
    s_materialPresets[MaterialPreset::Metal] = MaterialSettings(
        Quantity_Color(0.2, 0.2, 0.2, Quantity_TOC_RGB),    // ambient
        Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB),    // diffuse (gray)
        Quantity_Color(0.9, 0.9, 0.9, Quantity_TOC_RGB),    // specular
        80.0,   // shininess
        0.0     // transparency (opaque)
    );
    
    // Plastic material
    s_materialPresets[MaterialPreset::Plastic] = MaterialSettings(
        Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB),    // ambient
        Quantity_Color(0.8, 0.2, 0.2, Quantity_TOC_RGB),    // diffuse (red)
        Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB),    // specular
        32.0,   // shininess
        0.0     // transparency
    );
    
    // Wood material
    s_materialPresets[MaterialPreset::Wood] = MaterialSettings(
        Quantity_Color(0.2, 0.1, 0.05, Quantity_TOC_RGB),   // ambient (dark brown)
        Quantity_Color(0.6, 0.4, 0.2, Quantity_TOC_RGB),    // diffuse (brown)
        Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB),    // specular
        16.0,   // shininess (low)
        0.0     // transparency
    );
    
    // Ceramic material
    s_materialPresets[MaterialPreset::Ceramic] = MaterialSettings(
        Quantity_Color(0.15, 0.15, 0.15, Quantity_TOC_RGB), // ambient
        Quantity_Color(0.9, 0.9, 0.85, Quantity_TOC_RGB),   // diffuse (off-white)
        Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB),    // specular
        64.0,   // shininess
        0.0     // transparency
    );
    
    // Rubber material
    s_materialPresets[MaterialPreset::Rubber] = MaterialSettings(
        Quantity_Color(0.05, 0.05, 0.05, Quantity_TOC_RGB), // ambient (very dark)
        Quantity_Color(0.2, 0.2, 0.2, Quantity_TOC_RGB),    // diffuse (dark gray)
        Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB),    // specular (very low)
        4.0,    // shininess (very low)
        0.0     // transparency
    );
    
    // Chrome material
    s_materialPresets[MaterialPreset::Chrome] = MaterialSettings(
        Quantity_Color(0.3, 0.3, 0.3, Quantity_TOC_RGB),    // ambient
        Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB),    // diffuse
        Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),    // specular (bright white)
        128.0,  // shininess (very shiny)
        0.0     // transparency
    );
    
    // Gold material
    s_materialPresets[MaterialPreset::Gold] = MaterialSettings(
        Quantity_Color(0.24, 0.20, 0.07, Quantity_TOC_RGB), // ambient (dark gold)
        Quantity_Color(0.75, 0.61, 0.23, Quantity_TOC_RGB), // diffuse (gold)
        Quantity_Color(0.63, 0.56, 0.37, Quantity_TOC_RGB), // specular (golden)
        64.0,   // shininess
        0.0     // transparency
    );
    
    // Silver material
    s_materialPresets[MaterialPreset::Silver] = MaterialSettings(
        Quantity_Color(0.19, 0.19, 0.19, Quantity_TOC_RGB), // ambient
        Quantity_Color(0.51, 0.51, 0.51, Quantity_TOC_RGB), // diffuse
        Quantity_Color(0.51, 0.51, 0.51, Quantity_TOC_RGB), // specular
        64.0,   // shininess
        0.0     // transparency
    );
    
    // Copper material
    s_materialPresets[MaterialPreset::Copper] = MaterialSettings(
        Quantity_Color(0.19, 0.07, 0.02, Quantity_TOC_RGB), // ambient (dark copper)
        Quantity_Color(0.70, 0.27, 0.08, Quantity_TOC_RGB), // diffuse (copper)
        Quantity_Color(0.26, 0.14, 0.09, Quantity_TOC_RGB), // specular (copper)
        64.0,   // shininess
        0.0     // transparency
    );
    
    // Aluminum material
    s_materialPresets[MaterialPreset::Aluminum] = MaterialSettings(
        Quantity_Color(0.2, 0.2, 0.2, Quantity_TOC_RGB),    // ambient
        Quantity_Color(0.7, 0.7, 0.7, Quantity_TOC_RGB),    // diffuse
        Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB),    // specular
        96.0,   // shininess
        0.0     // transparency
    );
}

std::vector<std::string> RenderingConfig::getAvailablePresets()
{
    return {
        "Custom",
        "Glass",
        "Metal", 
        "Plastic",
        "Wood",
        "Ceramic",
        "Rubber",
        "Chrome",
        "Gold",
        "Silver",
        "Copper",
        "Aluminum"
    };
}

std::string RenderingConfig::getPresetName(MaterialPreset preset)
{
    switch (preset) {
        case MaterialPreset::Custom: return "Custom";
        case MaterialPreset::Glass: return "Glass";
        case MaterialPreset::Metal: return "Metal";
        case MaterialPreset::Plastic: return "Plastic";
        case MaterialPreset::Wood: return "Wood";
        case MaterialPreset::Ceramic: return "Ceramic";
        case MaterialPreset::Rubber: return "Rubber";
        case MaterialPreset::Chrome: return "Chrome";
        case MaterialPreset::Gold: return "Gold";
        case MaterialPreset::Silver: return "Silver";
        case MaterialPreset::Copper: return "Copper";
        case MaterialPreset::Aluminum: return "Aluminum";
        default: return "Custom";
    }
}

RenderingConfig::MaterialPreset RenderingConfig::getPresetFromName(const std::string& name)
{
    if (name == "Glass") return MaterialPreset::Glass;
    if (name == "Metal") return MaterialPreset::Metal;
    if (name == "Plastic") return MaterialPreset::Plastic;
    if (name == "Wood") return MaterialPreset::Wood;
    if (name == "Ceramic") return MaterialPreset::Ceramic;
    if (name == "Rubber") return MaterialPreset::Rubber;
    if (name == "Chrome") return MaterialPreset::Chrome;
    if (name == "Gold") return MaterialPreset::Gold;
    if (name == "Silver") return MaterialPreset::Silver;
    if (name == "Copper") return MaterialPreset::Copper;
    if (name == "Aluminum") return MaterialPreset::Aluminum;
    return MaterialPreset::Custom;
}

RenderingConfig::MaterialSettings RenderingConfig::getPresetMaterial(MaterialPreset preset) const
{
    auto it = s_materialPresets.find(preset);
    if (it != s_materialPresets.end()) {
        return it->second;
    }
    return MaterialSettings(); // Return default if not found
}

std::vector<std::string> RenderingConfig::getAvailableBlendModes()
{
    return {
        "None",
        "Alpha",
        "Additive",
        "Multiply",
        "Screen",
        "Overlay"
    };
}

std::string RenderingConfig::getBlendModeName(BlendMode mode)
{
    switch (mode) {
        case BlendMode::None: return "None";
        case BlendMode::Alpha: return "Alpha";
        case BlendMode::Additive: return "Additive";
        case BlendMode::Multiply: return "Multiply";
        case BlendMode::Screen: return "Screen";
        case BlendMode::Overlay: return "Overlay";
        default: return "Alpha";
    }
}

RenderingConfig::BlendMode RenderingConfig::getBlendModeFromName(const std::string& name)
{
    if (name == "None") return BlendMode::None;
    if (name == "Alpha") return BlendMode::Alpha;
    if (name == "Additive") return BlendMode::Additive;
    if (name == "Multiply") return BlendMode::Multiply;
    if (name == "Screen") return BlendMode::Screen;
    if (name == "Overlay") return BlendMode::Overlay;
    return BlendMode::Alpha;
}

std::vector<std::string> RenderingConfig::getAvailableTextureModes()
{
    return {
        "Replace",
        "Modulate",
        "Decal",
        "Blend"
    };
}

std::string RenderingConfig::getTextureModeName(TextureMode mode)
{
    switch (mode) {
        case TextureMode::Replace: return "Replace";
        case TextureMode::Modulate: return "Modulate";
        case TextureMode::Decal: return "Decal";
        case TextureMode::Blend: return "Blend";
        default: return "Modulate";
    }
}

RenderingConfig::TextureMode RenderingConfig::getTextureModeFromName(const std::string& name)
{
    if (name == "Replace") return TextureMode::Replace;
    if (name == "Modulate") return TextureMode::Modulate;
    if (name == "Decal") return TextureMode::Decal;
    if (name == "Blend") return TextureMode::Blend;
    return TextureMode::Modulate;
}

// Shading mode utility methods
std::vector<std::string> RenderingConfig::getAvailableShadingModes()
{
    return {
        "Flat",
        "Gouraud", 
        "Phong",
        "Smooth",
        "Wireframe",
        "Points"
    };
}

std::string RenderingConfig::getShadingModeName(ShadingMode mode)
{
    switch (mode) {
        case ShadingMode::Flat: return "Flat";
        case ShadingMode::Gouraud: return "Gouraud";
        case ShadingMode::Phong: return "Phong";
        case ShadingMode::Smooth: return "Smooth";
        case ShadingMode::Wireframe: return "Wireframe";
        case ShadingMode::Points: return "Points";
        default: return "Smooth";
    }
}

RenderingConfig::ShadingMode RenderingConfig::getShadingModeFromName(const std::string& name)
{
    if (name == "Flat") return ShadingMode::Flat;
    if (name == "Gouraud") return ShadingMode::Gouraud;
    if (name == "Phong") return ShadingMode::Phong;
    if (name == "Smooth") return ShadingMode::Smooth;
    if (name == "Wireframe") return ShadingMode::Wireframe;
    if (name == "Points") return ShadingMode::Points;
    return ShadingMode::Smooth;
}

// Display mode utility methods
std::vector<std::string> RenderingConfig::getAvailableDisplayModes()
{
    return {
        "Solid",
        "Wireframe",
        "HiddenLine",
        "SolidWireframe",
        "Points",
        "Transparent"
    };
}

std::string RenderingConfig::getDisplayModeName(DisplayMode mode)
{
    switch (mode) {
        case DisplayMode::Solid: return "Solid";
        case DisplayMode::Wireframe: return "Wireframe";
        case DisplayMode::HiddenLine: return "HiddenLine";
        case DisplayMode::SolidWireframe: return "SolidWireframe";
        case DisplayMode::Points: return "Points";
        case DisplayMode::Transparent: return "Transparent";
        default: return "Solid";
    }
}

RenderingConfig::DisplayMode RenderingConfig::getDisplayModeFromName(const std::string& name)
{
    if (name == "Solid") return DisplayMode::Solid;
    if (name == "Wireframe") return DisplayMode::Wireframe;
    if (name == "HiddenLine") return DisplayMode::HiddenLine;
    if (name == "SolidWireframe") return DisplayMode::SolidWireframe;
    if (name == "Points") return DisplayMode::Points;
    if (name == "Transparent") return DisplayMode::Transparent;
    return DisplayMode::Solid;
}

// Quality utility methods
std::vector<std::string> RenderingConfig::getAvailableQualityModes()
{
    return {
        "Draft",
        "Normal",
        "High",
        "Ultra",
        "Realtime"
    };
}

std::string RenderingConfig::getQualityModeName(RenderingQuality quality)
{
    switch (quality) {
        case RenderingQuality::Draft: return "Draft";
        case RenderingQuality::Normal: return "Normal";
        case RenderingQuality::High: return "High";
        case RenderingQuality::Ultra: return "Ultra";
        case RenderingQuality::Realtime: return "Realtime";
        default: return "Normal";
    }
}

RenderingConfig::RenderingQuality RenderingConfig::getQualityModeFromName(const std::string& name)
{
    if (name == "Draft") return RenderingQuality::Draft;
    if (name == "Normal") return RenderingQuality::Normal;
    if (name == "High") return RenderingQuality::High;
    if (name == "Ultra") return RenderingQuality::Ultra;
    if (name == "Realtime") return RenderingQuality::Realtime;
    return RenderingQuality::Normal;
}

// Shadow mode utility methods
std::vector<std::string> RenderingConfig::getAvailableShadowModes()
{
    return {
        "None",
        "Hard",
        "Soft",
        "Volumetric",
        "Contact",
        "Cascade"
    };
}

std::string RenderingConfig::getShadowModeName(ShadowMode mode)
{
    switch (mode) {
        case ShadowMode::None: return "None";
        case ShadowMode::Hard: return "Hard";
        case ShadowMode::Soft: return "Soft";
        case ShadowMode::Volumetric: return "Volumetric";
        case ShadowMode::Contact: return "Contact";
        case ShadowMode::Cascade: return "Cascade";
        default: return "Soft";
    }
}

RenderingConfig::ShadowMode RenderingConfig::getShadowModeFromName(const std::string& name)
{
    if (name == "None") return ShadowMode::None;
    if (name == "Hard") return ShadowMode::Hard;
    if (name == "Soft") return ShadowMode::Soft;
    if (name == "Volumetric") return ShadowMode::Volumetric;
    if (name == "Contact") return ShadowMode::Contact;
    if (name == "Cascade") return ShadowMode::Cascade;
    return ShadowMode::Soft;
}

// Lighting model utility methods
std::vector<std::string> RenderingConfig::getAvailableLightingModels()
{
    return {
        "Lambert",
        "BlinnPhong",
        "CookTorrance",
        "OrenNayar",
        "Minnaert",
        "Fresnel"
    };
}

std::string RenderingConfig::getLightingModelName(LightingModel model)
{
    switch (model) {
        case LightingModel::Lambert: return "Lambert";
        case LightingModel::BlinnPhong: return "BlinnPhong";
        case LightingModel::CookTorrance: return "CookTorrance";
        case LightingModel::OrenNayar: return "OrenNayar";
        case LightingModel::Minnaert: return "Minnaert";
        case LightingModel::Fresnel: return "Fresnel";
        default: return "BlinnPhong";
    }
}

RenderingConfig::LightingModel RenderingConfig::getLightingModelFromName(const std::string& name)
{
    if (name == "Lambert") return LightingModel::Lambert;
    if (name == "BlinnPhong") return LightingModel::BlinnPhong;
    if (name == "CookTorrance") return LightingModel::CookTorrance;
    if (name == "OrenNayar") return LightingModel::OrenNayar;
    if (name == "Minnaert") return LightingModel::Minnaert;
    if (name == "Fresnel") return LightingModel::Fresnel;
    return LightingModel::BlinnPhong;
}

void RenderingConfig::applyMaterialPreset(MaterialPreset preset)
{
    if (preset == MaterialPreset::Custom) {
        return; // Don't change anything for custom
    }
    
    MaterialSettings presetMaterial = getPresetMaterial(preset);
    setMaterialSettings(presetMaterial);
}

std::string RenderingConfig::getConfigFilePath() const
{
    // Save to local root directory instead of user config directory
    wxString currentDir = wxGetCwd();
    wxFileName configFile(currentDir, "rendering_settings.ini");
    
    // Create directory if it doesn't exist (should always exist for current directory)
    if (!configFile.DirExists()) {
        configFile.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    
    return configFile.GetFullPath().ToStdString();
}

bool RenderingConfig::loadFromFile(const std::string& filename)
{
    std::string configPath = filename.empty() ? getConfigFilePath() : filename;
    std::ifstream file(configPath);
    
    if (!file.is_open()) {
        LOG_INF_S("RenderingConfig: No config file found, using defaults: " + configPath);
        return false;
    }
    
    LOG_INF_S("RenderingConfig: Loading configuration from: " + configPath);
    
    std::string line;
    std::string section;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Check for section headers
        if (line[0] == '[' && line.back() == ']') {
            section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Parse key=value pairs
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        // Remove whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Parse values based on section
        if (section == "Material") {
            if (key == "AmbientColor") {
                m_materialSettings.ambientColor = parseColor(value, m_materialSettings.ambientColor);
            } else if (key == "DiffuseColor") {
                m_materialSettings.diffuseColor = parseColor(value, m_materialSettings.diffuseColor);
            } else if (key == "SpecularColor") {
                m_materialSettings.specularColor = parseColor(value, m_materialSettings.specularColor);
            } else if (key == "Shininess") {
                m_materialSettings.shininess = std::stod(value);
            } else if (key == "Transparency") {
                m_materialSettings.transparency = std::stod(value);
            }
        } else if (section == "Lighting") {
            if (key == "AmbientColor") {
                m_lightingSettings.ambientColor = parseColor(value, m_lightingSettings.ambientColor);
            } else if (key == "DiffuseColor") {
                m_lightingSettings.diffuseColor = parseColor(value, m_lightingSettings.diffuseColor);
            } else if (key == "SpecularColor") {
                m_lightingSettings.specularColor = parseColor(value, m_lightingSettings.specularColor);
            } else if (key == "Intensity") {
                m_lightingSettings.intensity = std::stod(value);
            } else if (key == "AmbientIntensity") {
                m_lightingSettings.ambientIntensity = std::stod(value);
            }
        } else if (section == "Texture") {
            if (key == "Color") {
                m_textureSettings.color = parseColor(value, m_textureSettings.color);
            } else if (key == "Intensity") {
                m_textureSettings.intensity = std::stod(value);
            } else if (key == "Enabled") {
                m_textureSettings.enabled = (value == "true" || value == "1");
            } else if (key == "ImagePath") {
                m_textureSettings.imagePath = value;
            } else if (key == "TextureMode") {
                m_textureSettings.textureMode = getTextureModeFromName(value);
            }
        } else if (section == "Blend") {
            if (key == "BlendMode") {
                m_blendSettings.blendMode = getBlendModeFromName(value);
            } else if (key == "DepthTest") {
                m_blendSettings.depthTest = (value == "true" || value == "1");
            } else if (key == "DepthWrite") {
                m_blendSettings.depthWrite = (value == "true" || value == "1");
            } else if (key == "CullFace") {
                m_blendSettings.cullFace = (value == "true" || value == "1");
            } else if (key == "AlphaThreshold") {
                m_blendSettings.alphaThreshold = std::stod(value);
            }
        } else if (section == "Shading") {
            if (key == "ShadingMode") {
                m_shadingSettings.shadingMode = getShadingModeFromName(value);
            } else if (key == "SmoothNormals") {
                m_shadingSettings.smoothNormals = (value == "true" || value == "1");
            } else if (key == "WireframeWidth") {
                m_shadingSettings.wireframeWidth = std::stod(value);
            } else if (key == "PointSize") {
                m_shadingSettings.pointSize = std::stod(value);
            }
        } else if (section == "Display") {
            if (key == "DisplayMode") {
                m_displaySettings.displayMode = getDisplayModeFromName(value);
            } else if (key == "ShowEdges") {
                m_displaySettings.showEdges = (value == "true" || value == "1");
            } else if (key == "ShowVertices") {
                m_displaySettings.showVertices = (value == "true" || value == "1");
            } else if (key == "EdgeWidth") {
                m_displaySettings.edgeWidth = std::stod(value);
            } else if (key == "VertexSize") {
                m_displaySettings.vertexSize = std::stod(value);
            } else if (key == "EdgeColor") {
                m_displaySettings.edgeColor = parseColor(value, m_displaySettings.edgeColor);
            } else if (key == "VertexColor") {
                m_displaySettings.vertexColor = parseColor(value, m_displaySettings.vertexColor);
            }
        } else if (section == "Quality") {
            if (key == "Quality") {
                m_qualitySettings.quality = getQualityModeFromName(value);
            } else if (key == "TessellationLevel") {
                m_qualitySettings.tessellationLevel = std::stoi(value);
            } else if (key == "AntiAliasingSamples") {
                m_qualitySettings.antiAliasingSamples = std::stoi(value);
            } else if (key == "EnableLOD") {
                m_qualitySettings.enableLOD = (value == "true" || value == "1");
            } else if (key == "LODDistance") {
                m_qualitySettings.lodDistance = std::stod(value);
            }
        } else if (section == "Shadow") {
            if (key == "ShadowMode") {
                m_shadowSettings.shadowMode = getShadowModeFromName(value);
            } else if (key == "ShadowIntensity") {
                m_shadowSettings.shadowIntensity = std::stod(value);
            } else if (key == "ShadowSoftness") {
                m_shadowSettings.shadowSoftness = std::stod(value);
            } else if (key == "ShadowMapSize") {
                m_shadowSettings.shadowMapSize = std::stoi(value);
            } else if (key == "ShadowBias") {
                m_shadowSettings.shadowBias = std::stod(value);
            }
        } else if (section == "LightingModel") {
            if (key == "LightingModel") {
                m_lightingModelSettings.lightingModel = getLightingModelFromName(value);
            } else if (key == "Roughness") {
                m_lightingModelSettings.roughness = std::stod(value);
            } else if (key == "Metallic") {
                m_lightingModelSettings.metallic = std::stod(value);
            } else if (key == "Fresnel") {
                m_lightingModelSettings.fresnel = std::stod(value);
            } else if (key == "SubsurfaceScattering") {
                m_lightingModelSettings.subsurfaceScattering = std::stod(value);
            }
        }
    }
    
    file.close();
    LOG_INF_S("RenderingConfig: Configuration loaded successfully");
    
    // Notify listeners of settings change
    notifySettingsChanged();
    
    return true;
}

bool RenderingConfig::saveToFile(const std::string& filename) const
{
    std::string configPath = filename.empty() ? getConfigFilePath() : filename;
    std::ofstream file(configPath);
    
    if (!file.is_open()) {
        LOG_ERR_S("RenderingConfig: Failed to save configuration to: " + configPath);
        return false;
    }
    
    LOG_INF_S("RenderingConfig: Saving configuration to: " + configPath);
    
    // Write header
    file << "# Rendering Settings Configuration\n";
    file << "# This file is automatically generated\n\n";
    
    // Write Material section
    file << "[Material]\n";
    file << "AmbientColor=" << colorToString(m_materialSettings.ambientColor) << "\n";
    file << "DiffuseColor=" << colorToString(m_materialSettings.diffuseColor) << "\n";
    file << "SpecularColor=" << colorToString(m_materialSettings.specularColor) << "\n";
    file << "Shininess=" << m_materialSettings.shininess << "\n";
    file << "Transparency=" << m_materialSettings.transparency << "\n\n";
    
    // Write Lighting section
    file << "[Lighting]\n";
    file << "AmbientColor=" << colorToString(m_lightingSettings.ambientColor) << "\n";
    file << "DiffuseColor=" << colorToString(m_lightingSettings.diffuseColor) << "\n";
    file << "SpecularColor=" << colorToString(m_lightingSettings.specularColor) << "\n";
    file << "Intensity=" << m_lightingSettings.intensity << "\n";
    file << "AmbientIntensity=" << m_lightingSettings.ambientIntensity << "\n\n";
    
    // Write Texture section
    file << "[Texture]\n";
    file << "Color=" << colorToString(m_textureSettings.color) << "\n";
    file << "Intensity=" << m_textureSettings.intensity << "\n";
    file << "Enabled=" << (m_textureSettings.enabled ? "true" : "false") << "\n";
    file << "ImagePath=" << m_textureSettings.imagePath << "\n";
    file << "TextureMode=" << getTextureModeName(m_textureSettings.textureMode) << "\n\n";
    
    // Write Blend section
    file << "[Blend]\n";
    file << "BlendMode=" << getBlendModeName(m_blendSettings.blendMode) << "\n";
    file << "DepthTest=" << (m_blendSettings.depthTest ? "true" : "false") << "\n";
    file << "DepthWrite=" << (m_blendSettings.depthWrite ? "true" : "false") << "\n";
    file << "CullFace=" << (m_blendSettings.cullFace ? "true" : "false") << "\n";
    file << "AlphaThreshold=" << m_blendSettings.alphaThreshold << "\n";
    
    // Write Shading section
    file << "\n[Shading]\n";
    file << "ShadingMode=" << getShadingModeName(m_shadingSettings.shadingMode) << "\n";
    file << "SmoothNormals=" << (m_shadingSettings.smoothNormals ? "true" : "false") << "\n";
    file << "WireframeWidth=" << m_shadingSettings.wireframeWidth << "\n";
    file << "PointSize=" << m_shadingSettings.pointSize << "\n";
    
    // Write Display section
    file << "\n[Display]\n";
    file << "DisplayMode=" << getDisplayModeName(m_displaySettings.displayMode) << "\n";
    file << "ShowEdges=" << (m_displaySettings.showEdges ? "true" : "false") << "\n";
    file << "ShowVertices=" << (m_displaySettings.showVertices ? "true" : "false") << "\n";
    file << "EdgeWidth=" << m_displaySettings.edgeWidth << "\n";
    file << "VertexSize=" << m_displaySettings.vertexSize << "\n";
    file << "EdgeColor=" << colorToString(m_displaySettings.edgeColor) << "\n";
    file << "VertexColor=" << colorToString(m_displaySettings.vertexColor) << "\n";
    
    // Write Quality section
    file << "\n[Quality]\n";
    file << "Quality=" << getQualityModeName(m_qualitySettings.quality) << "\n";
    file << "TessellationLevel=" << m_qualitySettings.tessellationLevel << "\n";
    file << "AntiAliasingSamples=" << m_qualitySettings.antiAliasingSamples << "\n";
    file << "EnableLOD=" << (m_qualitySettings.enableLOD ? "true" : "false") << "\n";
    file << "LODDistance=" << m_qualitySettings.lodDistance << "\n";
    
    // Write Shadow section
    file << "\n[Shadow]\n";
    file << "ShadowMode=" << getShadowModeName(m_shadowSettings.shadowMode) << "\n";
    file << "ShadowIntensity=" << m_shadowSettings.shadowIntensity << "\n";
    file << "ShadowSoftness=" << m_shadowSettings.shadowSoftness << "\n";
    file << "ShadowMapSize=" << m_shadowSettings.shadowMapSize << "\n";
    file << "ShadowBias=" << m_shadowSettings.shadowBias << "\n";
    
    // Write LightingModel section
    file << "\n[LightingModel]\n";
    file << "LightingModel=" << getLightingModelName(m_lightingModelSettings.lightingModel) << "\n";
    file << "Roughness=" << m_lightingModelSettings.roughness << "\n";
    file << "Metallic=" << m_lightingModelSettings.metallic << "\n";
    file << "Fresnel=" << m_lightingModelSettings.fresnel << "\n";
    file << "SubsurfaceScattering=" << m_lightingModelSettings.subsurfaceScattering << "\n";
    
    file.close();
    LOG_INF_S("RenderingConfig: Configuration saved successfully");
    
    // Notify listeners of settings change
    notifySettingsChanged();
    
    return true;
}

Quantity_Color RenderingConfig::parseColor(const std::string& value, const Quantity_Color& defaultValue) const
{
    std::istringstream iss(value);
    std::string token;
    std::vector<double> components;
    
    while (std::getline(iss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        try {
            components.push_back(std::stod(token));
        } catch (const std::exception& e) {
            LOG_ERR_S("RenderingConfig: Failed to parse color component: " + token);
            return defaultValue;
        }
    }
    
    if (components.size() >= 3) {
        return Quantity_Color(components[0], components[1], components[2], Quantity_TOC_RGB);
    }
    
    return defaultValue;
}

std::string RenderingConfig::colorToString(const Quantity_Color& color) const
{
    std::ostringstream oss;
    oss << color.Red() << "," << color.Green() << "," << color.Blue();
    return oss.str();
}

// Setters with auto-save
void RenderingConfig::setMaterialSettings(const MaterialSettings& settings)
{
    m_materialSettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setLightingSettings(const LightingSettings& settings)
{
    m_lightingSettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setTextureSettings(const TextureSettings& settings)
{
    m_textureSettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setMaterialAmbientColor(const Quantity_Color& color)
{
    m_materialSettings.ambientColor = color;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setMaterialDiffuseColor(const Quantity_Color& color)
{
    m_materialSettings.diffuseColor = color;
    if (m_autoSave) {
        saveToFile();
    }
    
    // Notify listeners of settings change
    notifySettingsChanged();
}

void RenderingConfig::setMaterialSpecularColor(const Quantity_Color& color)
{
    m_materialSettings.specularColor = color;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setMaterialShininess(double shininess)
{
    m_materialSettings.shininess = shininess;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setMaterialTransparency(double transparency)
{
    m_materialSettings.transparency = transparency;
    if (m_autoSave) {
        saveToFile();
    }
    
    // Notify listeners of settings change
    notifySettingsChanged();
}

void RenderingConfig::setLightAmbientColor(const Quantity_Color& color)
{
    m_lightingSettings.ambientColor = color;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setLightDiffuseColor(const Quantity_Color& color)
{
    m_lightingSettings.diffuseColor = color;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setLightSpecularColor(const Quantity_Color& color)
{
    m_lightingSettings.specularColor = color;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setLightIntensity(double intensity)
{
    m_lightingSettings.intensity = intensity;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setLightAmbientIntensity(double intensity)
{
    m_lightingSettings.ambientIntensity = intensity;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setTextureColor(const Quantity_Color& color)
{
    m_textureSettings.color = color;
    if (m_autoSave) {
        saveToFile();
    }
    
    // Notify listeners of settings change
    notifySettingsChanged();
}

void RenderingConfig::setTextureIntensity(double intensity)
{
    m_textureSettings.intensity = intensity;
    if (m_autoSave) {
        saveToFile();
    }
    
    // Notify listeners of settings change
    notifySettingsChanged();
}

void RenderingConfig::setTextureEnabled(bool enabled)
{
    m_textureSettings.enabled = enabled;
    if (m_autoSave) {
        saveToFile();
    }
    
    // Notify listeners of settings change
    notifySettingsChanged();
}

void RenderingConfig::setTextureImagePath(const std::string& path)
{
    m_textureSettings.imagePath = path;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setTextureMode(TextureMode mode)
{
    m_textureSettings.textureMode = mode;
    if (m_autoSave) {
        saveToFile();
    }
    
    // Notify listeners of settings change
    notifySettingsChanged();
}

void RenderingConfig::setBlendSettings(const BlendSettings& settings)
{
    m_blendSettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setBlendMode(BlendMode mode)
{
    m_blendSettings.blendMode = mode;
    if (m_autoSave) {
        saveToFile();
    }
    
    // Notify listeners of settings change
    notifySettingsChanged();
}

void RenderingConfig::setDepthTest(bool enabled)
{
    m_blendSettings.depthTest = enabled;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setDepthWrite(bool enabled)
{
    m_blendSettings.depthWrite = enabled;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setCullFace(bool enabled)
{
    m_blendSettings.cullFace = enabled;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setAlphaThreshold(double threshold)
{
    m_blendSettings.alphaThreshold = threshold;
    if (m_autoSave) {
        saveToFile();
    }
}

// New setter methods for rendering modes
void RenderingConfig::setShadingSettings(const ShadingSettings& settings)
{
    m_shadingSettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setDisplaySettings(const DisplaySettings& settings)
{
    m_displaySettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setQualitySettings(const QualitySettings& settings)
{
    m_qualitySettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setShadowSettings(const ShadowSettings& settings)
{
    m_shadowSettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setLightingModelSettings(const LightingModelSettings& settings)
{
    m_lightingModelSettings = settings;
    if (m_autoSave) {
        saveToFile();
    }
}

// Shading mode individual setters
void RenderingConfig::setShadingMode(ShadingMode mode)
{
    m_shadingSettings.shadingMode = mode;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setSmoothNormals(bool enabled)
{
    m_shadingSettings.smoothNormals = enabled;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setWireframeWidth(double width)
{
    m_shadingSettings.wireframeWidth = width;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setPointSize(double size)
{
    m_shadingSettings.pointSize = size;
    if (m_autoSave) {
        saveToFile();
    }
}

// Display mode individual setters
void RenderingConfig::setDisplayMode(DisplayMode mode)
{
    m_displaySettings.displayMode = mode;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setShowEdges(bool enabled)
{
    m_displaySettings.showEdges = enabled;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setShowVertices(bool enabled)
{
    m_displaySettings.showVertices = enabled;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setEdgeWidth(double width)
{
    m_displaySettings.edgeWidth = width;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setVertexSize(double size)
{
    m_displaySettings.vertexSize = size;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setEdgeColor(const Quantity_Color& color)
{
    m_displaySettings.edgeColor = color;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setVertexColor(const Quantity_Color& color)
{
    m_displaySettings.vertexColor = color;
    if (m_autoSave) {
        saveToFile();
    }
}

// Quality individual setters
void RenderingConfig::setRenderingQuality(RenderingQuality quality)
{
    m_qualitySettings.quality = quality;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setTessellationLevel(int level)
{
    m_qualitySettings.tessellationLevel = level;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setAntiAliasingSamples(int samples)
{
    m_qualitySettings.antiAliasingSamples = samples;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setEnableLOD(bool enabled)
{
    m_qualitySettings.enableLOD = enabled;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setLODDistance(double distance)
{
    m_qualitySettings.lodDistance = distance;
    if (m_autoSave) {
        saveToFile();
    }
}

// Shadow individual setters
void RenderingConfig::setShadowMode(ShadowMode mode)
{
    m_shadowSettings.shadowMode = mode;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setShadowIntensity(double intensity)
{
    m_shadowSettings.shadowIntensity = intensity;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setShadowSoftness(double softness)
{
    m_shadowSettings.shadowSoftness = softness;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setShadowMapSize(int size)
{
    m_shadowSettings.shadowMapSize = size;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setShadowBias(double bias)
{
    m_shadowSettings.shadowBias = bias;
    if (m_autoSave) {
        saveToFile();
    }
}

// Lighting model individual setters
void RenderingConfig::setLightingModel(LightingModel model)
{
    m_lightingModelSettings.lightingModel = model;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setRoughness(double roughness)
{
    m_lightingModelSettings.roughness = roughness;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setMetallic(double metallic)
{
    m_lightingModelSettings.metallic = metallic;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setFresnel(double fresnel)
{
    m_lightingModelSettings.fresnel = fresnel;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::setSubsurfaceScattering(double scattering)
{
    m_lightingModelSettings.subsurfaceScattering = scattering;
    if (m_autoSave) {
        saveToFile();
    }
}

void RenderingConfig::resetToDefaults()
{
    m_materialSettings = MaterialSettings();
    m_lightingSettings = LightingSettings();
    m_textureSettings = TextureSettings();
    m_blendSettings = BlendSettings();
    m_shadingSettings = ShadingSettings();
    m_displaySettings = DisplaySettings();
    m_qualitySettings = QualitySettings();
    m_shadowSettings = ShadowSettings();
    m_lightingModelSettings = LightingModelSettings();
    
    if (m_autoSave) {
        saveToFile();
    }
    
    // Notify listeners of settings change
    notifySettingsChanged();
}

// Notification system implementation
void RenderingConfig::registerSettingsChangedCallback(SettingsChangedCallback callback)
{
    m_settingsChangedCallback = callback;
    LOG_INF_S("RenderingConfig: Settings changed callback registered");
}

void RenderingConfig::unregisterSettingsChangedCallback()
{
    m_settingsChangedCallback = nullptr;
    LOG_INF_S("RenderingConfig: Settings changed callback unregistered");
}

void RenderingConfig::notifySettingsChanged() const
{
    if (m_settingsChangedCallback) {
        LOG_INF_S("RenderingConfig: Notifying settings changed - callback is registered");
        m_settingsChangedCallback();
        LOG_INF_S("RenderingConfig: Settings change notification completed");
    } else {
        LOG_WRN_S("RenderingConfig: Settings changed but no callback is registered");
    }
}

// Selected objects rendering settings implementation
// These methods apply settings only to selected geometries

void RenderingConfig::applyMaterialSettingsToSelected(const MaterialSettings& settings)
{
    // Store the settings for selected objects
    // The actual application to selected objects is handled by SceneManager callback
    m_materialSettings = settings;
    notifySettingsChanged();
}

void RenderingConfig::applyTextureSettingsToSelected(const TextureSettings& settings)
{
    m_textureSettings = settings;
    notifySettingsChanged();
}

void RenderingConfig::applyBlendSettingsToSelected(const BlendSettings& settings)
{
    m_blendSettings = settings;
    notifySettingsChanged();
}

void RenderingConfig::applyShadingSettingsToSelected(const ShadingSettings& settings)
{
    m_shadingSettings = settings;
    notifySettingsChanged();
}

void RenderingConfig::applyDisplaySettingsToSelected(const DisplaySettings& settings)
{
    m_displaySettings = settings;
    notifySettingsChanged();
}

// Individual property setters for selected objects
void RenderingConfig::setSelectedMaterialAmbientColor(const Quantity_Color& color)
{
    m_materialSettings.ambientColor = color;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedMaterialDiffuseColor(const Quantity_Color& color)
{
    m_materialSettings.diffuseColor = color;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedMaterialSpecularColor(const Quantity_Color& color)
{
    m_materialSettings.specularColor = color;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedMaterialShininess(double shininess)
{
    m_materialSettings.shininess = shininess;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedMaterialTransparency(double transparency)
{
    m_materialSettings.transparency = transparency;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureColor(const Quantity_Color& color)
{
    m_textureSettings.color = color;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureIntensity(double intensity)
{
    m_textureSettings.intensity = intensity;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureEnabled(bool enabled)
{
    m_textureSettings.enabled = enabled;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureImagePath(const std::string& path)
{
    m_textureSettings.imagePath = path;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedTextureMode(TextureMode mode)
{
    m_textureSettings.textureMode = mode;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedBlendMode(BlendMode mode)
{
    m_blendSettings.blendMode = mode;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedDepthTest(bool enabled)
{
    m_blendSettings.depthTest = enabled;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedDepthWrite(bool enabled)
{
    m_blendSettings.depthWrite = enabled;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedCullFace(bool enabled)
{
    m_blendSettings.cullFace = enabled;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedAlphaThreshold(double threshold)
{
    m_blendSettings.alphaThreshold = threshold;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedShadingMode(ShadingMode mode)
{
    m_shadingSettings.shadingMode = mode;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedSmoothNormals(bool enabled)
{
    m_shadingSettings.smoothNormals = enabled;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedWireframeWidth(double width)
{
    m_shadingSettings.wireframeWidth = width;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedPointSize(double size)
{
    m_shadingSettings.pointSize = size;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedDisplayMode(DisplayMode mode)
{
    m_displaySettings.displayMode = mode;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedShowEdges(bool enabled)
{
    m_displaySettings.showEdges = enabled;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedShowVertices(bool enabled)
{
    m_displaySettings.showVertices = enabled;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedEdgeWidth(double width)
{
    m_displaySettings.edgeWidth = width;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedVertexSize(double size)
{
    m_displaySettings.vertexSize = size;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedEdgeColor(const Quantity_Color& color)
{
    m_displaySettings.edgeColor = color;
    notifySettingsChanged();
}

void RenderingConfig::setSelectedVertexColor(const Quantity_Color& color)
{
    m_displaySettings.vertexColor = color;
    notifySettingsChanged();
}

bool RenderingConfig::hasSelectedObjects() const
{
    // This method should check with SceneManager or OCCViewer
    // For now, we'll return true to indicate that selection-based rendering is available
    // The actual selection check is done in SceneManager callback
    return true;
}

// Static method to get OCCViewer instance for selection checking
OCCViewer* RenderingConfig::getOCCViewerInstance()
{
    // This is a temporary solution - in a real implementation, we would have a proper
    // way to access the OCCViewer instance from RenderingConfig
    // For now, we'll return nullptr and handle the selection check in SceneManager callback
    return nullptr;
}

void RenderingConfig::applyMaterialPresetToSelected(MaterialPreset preset)
{
    MaterialSettings presetSettings = getPresetMaterial(preset);
    applyMaterialSettingsToSelected(presetSettings);
}

std::string RenderingConfig::getCurrentSelectionStatus() const
{
    // This would ideally check with OCCViewer, but for now return a placeholder
    return "Selection status: Available (check OCCViewer for actual selection)";
}

std::string RenderingConfig::getCurrentRenderingSettings() const
{
    std::stringstream ss;
    ss << "Current Rendering Settings:\n";
    ss << "- Material Diffuse: R=" << m_materialSettings.diffuseColor.Red() 
       << " G=" << m_materialSettings.diffuseColor.Green() 
       << " B=" << m_materialSettings.diffuseColor.Blue() << "\n";
    ss << "- Material Transparency: " << m_materialSettings.transparency << "\n";
    ss << "- Texture Enabled: " << (m_textureSettings.enabled ? "Yes" : "No") << "\n";
    ss << "- Texture Mode: " << getTextureModeName(m_textureSettings.textureMode) << "\n";
    ss << "- Texture Color: R=" << m_textureSettings.color.Red() 
       << " G=" << m_textureSettings.color.Green() 
       << " B=" << m_textureSettings.color.Blue() << "\n";
    ss << "- Blend Mode: " << getBlendModeName(m_blendSettings.blendMode) << "\n";
    ss << "- Display Mode: " << getDisplayModeName(m_displaySettings.displayMode) << "\n";
    ss << "- Shading Mode: " << getShadingModeName(m_shadingSettings.shadingMode);
    
    return ss.str();
}

void RenderingConfig::showTestFeedback() const
{
    std::string status = getCurrentSelectionStatus();
    std::string settings = getCurrentRenderingSettings();
    
    LOG_INF_S("=== RenderingConfig Test Feedback ===");
    LOG_INF_S(status);
    LOG_INF_S(settings);
    LOG_INF_S("=== End Test Feedback ===");
} 