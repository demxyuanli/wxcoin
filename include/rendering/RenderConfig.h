#pragma once

#include <string>
#include <map>
#include <memory>
#include <OpenCASCADE/Quantity_Color.hxx>

/**
 * @brief Edge display settings for rendering toolkit
 */
struct RenderEdgeSettings {
    bool showEdges;                    // Show edges flag
    bool edgeColorEnabled;             // Enable custom edge color
    Quantity_Color edgeColor;          // Edge color
    double featureEdgeAngle;           // Feature edge angle threshold
    
    RenderEdgeSettings() 
        : showEdges(true)
        , edgeColorEnabled(false)
        , edgeColor(Quantity_NOC_BLACK)
        , featureEdgeAngle(45.0) {}
};

/**
 * @brief Smoothing settings
 */
struct SmoothingSettings {
    bool enabled;                      // Enable smoothing
    double creaseAngle;                // Crease angle threshold
    int iterations;                    // Smoothing iterations
    
    SmoothingSettings() 
        : enabled(true)
        , creaseAngle(30.0)
        , iterations(2) {}
};

/**
 * @brief Subdivision settings
 */
struct SubdivisionSettings {
    bool enabled;                      // Enable subdivision
    int levels;                        // Subdivision levels
    
    SubdivisionSettings() 
        : enabled(false)
        , levels(2) {}
};

/**
 * @brief Rendering configuration manager
 */
class RenderConfig {
public:
    /**
     * @brief Get singleton instance
     * @return Configuration instance
     */
    static RenderConfig& getInstance();

    /**
     * @brief Load configuration from file
     * @param filename Configuration file path
     * @return true if loading successful
     */
    bool loadFromFile(const std::string& filename);

    /**
     * @brief Save configuration to file
     * @param filename Configuration file path
     * @return true if saving successful
     */
    bool saveToFile(const std::string& filename);

    /**
     * @brief Get edge settings
     * @return Edge settings reference
     */
    RenderEdgeSettings& getEdgeSettings() { return m_edgeSettings; }
    const RenderEdgeSettings& getEdgeSettings() const { return m_edgeSettings; }

    /**
     * @brief Get smoothing settings
     * @return Smoothing settings reference
     */
    SmoothingSettings& getSmoothingSettings() { return m_smoothingSettings; }
    const SmoothingSettings& getSmoothingSettings() const { return m_smoothingSettings; }

    /**
     * @brief Get subdivision settings
     * @return Subdivision settings reference
     */
    SubdivisionSettings& getSubdivisionSettings() { return m_subdivisionSettings; }
    const SubdivisionSettings& getSubdivisionSettings() const { return m_subdivisionSettings; }

    /**
     * @brief Set custom parameter
     * @param key Parameter key
     * @param value Parameter value
     */
    void setParameter(const std::string& key, const std::string& value);

    /**
     * @brief Get custom parameter
     * @param key Parameter key
     * @param defaultValue Default value if not found
     * @return Parameter value
     */
    std::string getParameter(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * @brief Reset to default settings
     */
    void resetToDefaults();

private:
    RenderConfig() = default;
    RenderConfig(const RenderConfig&) = delete;
    RenderConfig& operator=(const RenderConfig&) = delete;

    RenderEdgeSettings m_edgeSettings;
    SmoothingSettings m_smoothingSettings;
    SubdivisionSettings m_subdivisionSettings;
    std::map<std::string, std::string> m_customParameters;
}; 