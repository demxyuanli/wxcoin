#pragma once

#include "RenderingSettings.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <wx/colour.h>

// Forward declarations
class SoSeparator;
class wxGLCanvas;
class wxGLContext;
class PreviewCanvas;

// Background configuration structure
struct BackgroundSettings {
    int style;                    // 0=Solid, 1=Gradient, 2=Image, 3=Environment, 4=Studio, 5=Outdoor, 6=Industrial, 7=CAD, 8=Dark
    wxColour backgroundColor;
    wxColour gradientTopColor;
    wxColour gradientBottomColor;
    std::string imagePath;
    bool imageEnabled;
    float imageOpacity;
    int imageFit;                 // 0=Stretch, 1=Fit, 2=Center, 3=Tile
    bool imageMaintainAspect;
    std::string name;
    bool isActive;
    
    BackgroundSettings()
        : style(0)
        , backgroundColor(173, 204, 255)
        , gradientTopColor(200, 220, 255)
        , gradientBottomColor(150, 180, 255)
        , imageEnabled(false)
        , imageOpacity(1.0f)
        , imageFit(1)
        , imageMaintainAspect(true)
        , name("Default Background")
        , isActive(false)
    {}
};

// Managed background configuration
struct ManagedBackground {
    BackgroundSettings settings;
    int configId;
    bool isActive;
    
    ManagedBackground() : configId(-1), isActive(false) {}
};

/**
 * @brief Background settings manager for unified background management
 * 
 * Manages background configurations and applies them to the preview viewport
 */
class BackgroundManager {
public:
    BackgroundManager(PreviewCanvas* canvas);
    ~BackgroundManager();
    
    // Configuration management methods
    int addConfiguration(const BackgroundSettings& settings);
    bool removeConfiguration(int configId);
    bool updateConfiguration(int configId, const BackgroundSettings& settings);
    void clearAllConfigurations();
    
    // Configuration query methods
    std::vector<int> getAllConfigurationIds() const;
    std::vector<BackgroundSettings> getAllConfigurations() const;
    BackgroundSettings getConfiguration(int configId) const;
    bool hasConfiguration(int configId) const;
    int getConfigurationCount() const;
    
    // Active configuration management
    bool setActiveConfiguration(int configId);
    int getActiveConfigurationId() const;
    BackgroundSettings getActiveConfiguration() const;
    bool hasActiveConfiguration() const;
    
    // Parameter update methods
    void setStyle(int configId, int style);
    void setBackgroundColor(int configId, const wxColour& color);
    void setGradientTopColor(int configId, const wxColour& color);
    void setGradientBottomColor(int configId, const wxColour& color);
    void setImagePath(int configId, const std::string& path);
    void setImageEnabled(int configId, bool enabled);
    void setImageOpacity(int configId, float opacity);
    void setImageFit(int configId, int fit);
    void setImageMaintainAspect(int configId, bool maintain);
    
    // Preset management
    void applyPreset(const std::string& presetName);
    void saveAsPreset(int configId, const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    
    // Rendering application
    void applyToPreviewViewport();
    void updatePreviewViewport();
    void renderBackground();
    
    // Background style specific methods
    void renderSolidBackground(const wxColour& color);
    void renderGradientBackground(const wxColour& topColor, const wxColour& bottomColor);
    void renderImageBackground(const std::string& imagePath, float opacity, int fit, bool maintainAspect);
    void renderEnvironmentBackground();
    void renderStudioBackground();
    void renderOutdoorBackground();
    void renderIndustrialBackground();
    void renderCADBackground();
    void renderDarkBackground();
    
    // Utility methods
    void loadFromRenderingSettings(const RenderingSettings& settings);
    void saveToRenderingSettings(RenderingSettings& settings) const;
    void resetToDefaults();
    
    // Performance monitoring
    float getPerformanceImpact() const;
    std::string getQualityDescription() const;
    
private:
    PreviewCanvas* m_canvas;
    std::unordered_map<int, std::unique_ptr<ManagedBackground>> m_configurations;
    int m_nextConfigId;
    int m_activeConfigId;
    
    // Preset configurations
    std::unordered_map<std::string, BackgroundSettings> m_presets;
    
    // Helper methods
    void initializePresets();
    void setupOpenGLState(const BackgroundSettings& settings);
};
