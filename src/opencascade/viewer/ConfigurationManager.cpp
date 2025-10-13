#include "viewer/ConfigurationManager.h"
#include "logger/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ConfigurationManager::ConfigurationManager()
{
    setupDefaultConfigurations();
}

ConfigurationManager::~ConfigurationManager()
{
}

SubdivisionConfig& ConfigurationManager::getSubdivisionConfig()
{
    return m_subdivisionConfig;
}

const SubdivisionConfig& ConfigurationManager::getSubdivisionConfig() const
{
    return m_subdivisionConfig;
}

SmoothingConfig& ConfigurationManager::getSmoothingConfig()
{
    return m_smoothingConfig;
}

const SmoothingConfig& ConfigurationManager::getSmoothingConfig() const
{
    return m_smoothingConfig;
}

TessellationConfig& ConfigurationManager::getTessellationConfig()
{
    return m_tessellationConfig;
}

const TessellationConfig& ConfigurationManager::getTessellationConfig() const
{
    return m_tessellationConfig;
}

NormalDisplayConfig& ConfigurationManager::getNormalDisplayConfig()
{
    return m_normalDisplayConfig;
}

const NormalDisplayConfig& ConfigurationManager::getNormalDisplayConfig() const
{
    return m_normalDisplayConfig;
}

OriginalEdgesConfig& ConfigurationManager::getOriginalEdgesConfig()
{
    return m_originalEdgesConfig;
}

const OriginalEdgesConfig& ConfigurationManager::getOriginalEdgesConfig() const
{
    return m_originalEdgesConfig;
}

void ConfigurationManager::loadDefaultConfigurations()
{
    setupDefaultConfigurations();
    LOG_INF_S("Loaded default configurations");
}

bool ConfigurationManager::loadConfigurationFromFile(const std::string& filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_ERR_S("Failed to open configuration file: " + filename);
            return false;
        }

        // Simple JSON-like parsing (placeholder implementation)
        std::string line;
        while (std::getline(file, line)) {
            // Parse configuration lines
            // This would be expanded to properly parse JSON or INI format
        }

        LOG_INF_S("Loaded configuration from file: " + filename);
        return validateAllConfigurations();
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Error loading configuration file '" + filename + "': " + std::string(e.what()));
        return false;
    }
}

bool ConfigurationManager::saveConfigurationToFile(const std::string& filename) const
{
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERR_S("Failed to open configuration file for writing: " + filename);
            return false;
        }

        // Export configuration as JSON
        std::string json = exportConfigurationAsJson();
        file << json;

        LOG_INF_S("Saved configuration to file: " + filename);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Error saving configuration file '" + filename + "': " + std::string(e.what()));
        return false;
    }
}

bool ConfigurationManager::validateAllConfigurations() const
{
    m_validationErrors.clear();

    bool isValid = true;
    isValid &= validateSubdivisionConfig();
    isValid &= validateSmoothingConfig();
    isValid &= validateTessellationConfig();
    isValid &= validateNormalDisplayConfig();
    isValid &= validateOriginalEdgesConfig();

    if (!isValid) {
        LOG_WRN_S("Configuration validation failed: " + m_validationErrors);
    }

    return isValid;
}

std::string ConfigurationManager::getValidationErrors() const
{
    return m_validationErrors;
}

void ConfigurationManager::applyQualityPreset(const std::string& presetName)
{
    if (presetName == "draft") {
        applyDraftPreset();
    }
    else if (presetName == "standard") {
        applyStandardPreset();
    }
    else if (presetName == "high_quality") {
        applyHighQualityPreset();
    }
    else {
        LOG_WRN_S("Unknown quality preset: " + presetName);
        return;
    }

    LOG_INF_S("Applied quality preset: " + presetName);
}

void ConfigurationManager::applyPerformancePreset(const std::string& presetName)
{
    if (presetName == "performance") {
        applyPerformancePresetInternal();
    }
    else {
        LOG_WRN_S("Unknown performance preset: " + presetName);
        return;
    }

    LOG_INF_S("Applied performance preset: " + presetName);
}

std::vector<std::string> ConfigurationManager::getAvailablePresets() const
{
    return {"draft", "standard", "high_quality", "performance"};
}

void ConfigurationManager::resetToDefaults()
{
    setupDefaultConfigurations();
    LOG_INF_S("Reset all configurations to defaults");
}

std::string ConfigurationManager::exportConfigurationAsJson() const
{
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"subdivision\": {\n";
    ss << "    \"enabled\": " << (m_subdivisionConfig.enabled ? "true" : "false") << ",\n";
    ss << "    \"level\": " << m_subdivisionConfig.level << ",\n";
    ss << "    \"method\": " << m_subdivisionConfig.method << ",\n";
    ss << "    \"crease_angle\": " << m_subdivisionConfig.creaseAngle << "\n";
    ss << "  },\n";
    ss << "  \"smoothing\": {\n";
    ss << "    \"enabled\": " << (m_smoothingConfig.enabled ? "true" : "false") << ",\n";
    ss << "    \"method\": " << m_smoothingConfig.method << ",\n";
    ss << "    \"iterations\": " << m_smoothingConfig.iterations << ",\n";
    ss << "    \"strength\": " << m_smoothingConfig.strength << ",\n";
    ss << "    \"crease_angle\": " << m_smoothingConfig.creaseAngle << "\n";
    ss << "  },\n";
    ss << "  \"tessellation\": {\n";
    ss << "    \"method\": " << m_tessellationConfig.method << ",\n";
    ss << "    \"quality\": " << m_tessellationConfig.quality << ",\n";
    ss << "    \"feature_preservation\": " << m_tessellationConfig.featurePreservation << ",\n";
    ss << "    \"parallel_processing\": " << (m_tessellationConfig.parallelProcessing ? "true" : "false") << ",\n";
    ss << "    \"adaptive_meshing\": " << (m_tessellationConfig.adaptiveMeshing ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"normal_display\": {\n";
    ss << "    \"show_normals\": " << (m_normalDisplayConfig.showNormals ? "true" : "false") << ",\n";
    ss << "    \"length\": " << m_normalDisplayConfig.length << ",\n";
    ss << "    \"consistency_mode\": " << (m_normalDisplayConfig.consistencyMode ? "true" : "false") << ",\n";
    ss << "    \"debug_mode\": " << (m_normalDisplayConfig.debugMode ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"original_edges\": {\n";
    ss << "    \"sampling_density\": " << m_originalEdgesConfig.samplingDensity << ",\n";
    ss << "    \"min_length\": " << m_originalEdgesConfig.minLength << ",\n";
    ss << "    \"show_lines_only\": " << (m_originalEdgesConfig.showLinesOnly ? "true" : "false") << ",\n";
    ss << "    \"highlight_intersection_nodes\": " << (m_originalEdgesConfig.highlightIntersectionNodes ? "true" : "false") << "\n";
    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}

bool ConfigurationManager::importConfigurationFromJson(const std::string& jsonString)
{
    // Placeholder implementation for JSON import
    // This would parse the JSON string and update configurations
    LOG_INF_S("Configuration import from JSON - placeholder implementation");
    return true;
}

bool ConfigurationManager::validateSubdivisionConfig() const
{
    if (m_subdivisionConfig.level < 1 || m_subdivisionConfig.level > 5) {
        m_validationErrors += "Subdivision level must be between 1 and 5. ";
        return false;
    }
    if (m_subdivisionConfig.method < 0 || m_subdivisionConfig.method > 3) {
        m_validationErrors += "Subdivision method must be between 0 and 3. ";
        return false;
    }
    if (m_subdivisionConfig.creaseAngle < 0.0 || m_subdivisionConfig.creaseAngle > 180.0) {
        m_validationErrors += "Subdivision crease angle must be between 0 and 180 degrees. ";
        return false;
    }
    return true;
}

bool ConfigurationManager::validateSmoothingConfig() const
{
    if (m_smoothingConfig.method < 0 || m_smoothingConfig.method > 3) {
        m_validationErrors += "Smoothing method must be between 0 and 3. ";
        return false;
    }
    if (m_smoothingConfig.iterations < 1 || m_smoothingConfig.iterations > 10) {
        m_validationErrors += "Smoothing iterations must be between 1 and 10. ";
        return false;
    }
    if (m_smoothingConfig.strength < 0.01 || m_smoothingConfig.strength > 1.0) {
        m_validationErrors += "Smoothing strength must be between 0.01 and 1.0. ";
        return false;
    }
    if (m_smoothingConfig.creaseAngle < 0.0 || m_smoothingConfig.creaseAngle > 180.0) {
        m_validationErrors += "Smoothing crease angle must be between 0 and 180 degrees. ";
        return false;
    }
    return true;
}

bool ConfigurationManager::validateTessellationConfig() const
{
    if (m_tessellationConfig.method < 0 || m_tessellationConfig.method > 3) {
        m_validationErrors += "Tessellation method must be between 0 and 3. ";
        return false;
    }
    if (m_tessellationConfig.quality < 1 || m_tessellationConfig.quality > 5) {
        m_validationErrors += "Tessellation quality must be between 1 and 5. ";
        return false;
    }
    if (m_tessellationConfig.featurePreservation < 0.0 || m_tessellationConfig.featurePreservation > 1.0) {
        m_validationErrors += "Feature preservation must be between 0.0 and 1.0. ";
        return false;
    }
    return true;
}

bool ConfigurationManager::validateNormalDisplayConfig() const
{
    if (m_normalDisplayConfig.length <= 0.0) {
        m_validationErrors += "Normal display length must be positive. ";
        return false;
    }
    return true;
}

bool ConfigurationManager::validateOriginalEdgesConfig() const
{
    if (m_originalEdgesConfig.samplingDensity <= 0.0) {
        m_validationErrors += "Original edges sampling density must be positive. ";
        return false;
    }
    if (m_originalEdgesConfig.minLength < 0.0) {
        m_validationErrors += "Original edges minimum length must be non-negative. ";
        return false;
    }
    return true;
}

void ConfigurationManager::setupDefaultConfigurations()
{
    // Subdivision defaults
    m_subdivisionConfig = SubdivisionConfig{};

    // Smoothing defaults
    m_smoothingConfig = SmoothingConfig{};

    // Tessellation defaults
    m_tessellationConfig = TessellationConfig{};

    // Normal display defaults
    m_normalDisplayConfig = NormalDisplayConfig{};

    // Original edges defaults
    m_originalEdgesConfig = OriginalEdgesConfig{};
}

void ConfigurationManager::applyDraftPreset()
{
    m_subdivisionConfig.enabled = false;
    m_smoothingConfig.enabled = false;
    m_tessellationConfig.quality = 1;
    m_tessellationConfig.adaptiveMeshing = false;
}

void ConfigurationManager::applyStandardPreset()
{
    m_subdivisionConfig.enabled = true;
    m_subdivisionConfig.level = 2;
    m_smoothingConfig.enabled = true;
    m_smoothingConfig.iterations = 3;
    m_tessellationConfig.quality = 3;
    m_tessellationConfig.adaptiveMeshing = true;
}

void ConfigurationManager::applyHighQualityPreset()
{
    m_subdivisionConfig.enabled = true;
    m_subdivisionConfig.level = 3;
    m_smoothingConfig.enabled = true;
    m_smoothingConfig.iterations = 5;
    m_smoothingConfig.strength = 0.8;
    m_tessellationConfig.quality = 5;
    m_tessellationConfig.featurePreservation = 0.9;
    m_tessellationConfig.adaptiveMeshing = true;
    m_tessellationConfig.parallelProcessing = true;
}

void ConfigurationManager::applyPerformancePresetInternal()
{
    m_subdivisionConfig.enabled = false;
    m_smoothingConfig.enabled = false;
    m_smoothingConfig.iterations = 1;
    m_tessellationConfig.quality = 1;
    m_tessellationConfig.parallelProcessing = false;
    m_tessellationConfig.adaptiveMeshing = false;
}
