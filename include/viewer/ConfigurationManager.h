#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "viewer/config/SubdivisionConfig.h"
#include "viewer/config/SmoothingConfig.h"
#include "viewer/config/TessellationConfig.h"
#include "viewer/config/NormalDisplayConfig.h"
#include "viewer/config/OriginalEdgesConfig.h"

/**
 * @brief Unified configuration management service
 *
 * This service provides centralized management for all application configurations,
 * including loading, saving, validation, and access to configuration objects.
 */
class ConfigurationManager {
public:
    ConfigurationManager();
    ~ConfigurationManager();

    // Configuration object access
    SubdivisionConfig& getSubdivisionConfig();
    const SubdivisionConfig& getSubdivisionConfig() const;

    SmoothingConfig& getSmoothingConfig();
    const SmoothingConfig& getSmoothingConfig() const;

    TessellationConfig& getTessellationConfig();
    const TessellationConfig& getTessellationConfig() const;

    NormalDisplayConfig& getNormalDisplayConfig();
    const NormalDisplayConfig& getNormalDisplayConfig() const;

    OriginalEdgesConfig& getOriginalEdgesConfig();
    const OriginalEdgesConfig& getOriginalEdgesConfig() const;

    // Configuration operations
    void loadDefaultConfigurations();
    bool loadConfigurationFromFile(const std::string& filename);
    bool saveConfigurationToFile(const std::string& filename) const;

    // Validation
    bool validateAllConfigurations() const;
    std::string getValidationErrors() const;

    // Configuration presets
    void applyQualityPreset(const std::string& presetName);
    void applyPerformancePreset(const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;

    // Utility methods
    void resetToDefaults();
    std::string exportConfigurationAsJson() const;
    bool importConfigurationFromJson(const std::string& jsonString);

private:
    // Configuration objects
    SubdivisionConfig m_subdivisionConfig;
    SmoothingConfig m_smoothingConfig;
    TessellationConfig m_tessellationConfig;
    NormalDisplayConfig m_normalDisplayConfig;
    OriginalEdgesConfig m_originalEdgesConfig;

    // Validation state
    mutable std::string m_validationErrors;

    // Helper methods
    bool validateSubdivisionConfig() const;
    bool validateSmoothingConfig() const;
    bool validateTessellationConfig() const;
    bool validateNormalDisplayConfig() const;
    bool validateOriginalEdgesConfig() const;

    void setupDefaultConfigurations();
    void applyDraftPreset();
    void applyStandardPreset();
    void applyHighQualityPreset();
    void applyPerformancePresetInternal();
};
