#pragma once

#include "AntiAliasingSettings.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

// Forward declarations
class SoSeparator;
class wxGLCanvas;
class wxGLContext;

// Managed anti-aliasing configuration
struct ManagedAntiAliasing {
    AntiAliasingSettings settings;
    int configId;
    bool isActive;
    
    ManagedAntiAliasing() : configId(-1), isActive(false) {}
};

/**
 * @brief Anti-aliasing parameter manager for unified AA management
 * 
 * Manages anti-aliasing configurations and applies them to the rendering pipeline
 */
class AntiAliasingManager {
public:
    AntiAliasingManager(wxGLCanvas* canvas, wxGLContext* glContext);
    ~AntiAliasingManager();
    
    // Configuration management methods
    int addConfiguration(const AntiAliasingSettings& settings);
    bool removeConfiguration(int configId);
    bool updateConfiguration(int configId, const AntiAliasingSettings& settings);
    void clearAllConfigurations();
    
    // Configuration query methods
    std::vector<int> getAllConfigurationIds() const;
    std::vector<AntiAliasingSettings> getAllConfigurations() const;
    AntiAliasingSettings getConfiguration(int configId) const;
    bool hasConfiguration(int configId) const;
    int getConfigurationCount() const;
    
    // Active configuration management
    bool setActiveConfiguration(int configId);
    int getActiveConfigurationId() const;
    AntiAliasingSettings getActiveConfiguration() const;
    bool hasActiveConfiguration() const;
    
    // Parameter update methods
    void setMethod(int configId, int method);
    void setMSAASamples(int configId, int samples);
    void setFXAAEnabled(int configId, bool enabled);
    void setFXAAQuality(int configId, float quality);
    void setSSAAEnabled(int configId, bool enabled);
    void setSSAAFactor(int configId, int factor);
    void setTAAEnabled(int configId, bool enabled);
    void setTAAStrength(int configId, float strength);
    
    // Preset management
    void applyPreset(const std::string& presetName);
    void saveAsPreset(int configId, const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    
    // Rendering application
    void applyToRenderPipeline();
    void updateRenderingState();
    
    // Performance monitoring
    float getPerformanceImpact() const;
    std::string getQualityDescription() const;
    
private:
    wxGLCanvas* m_canvas;
    wxGLContext* m_glContext;
    std::unordered_map<int, std::unique_ptr<ManagedAntiAliasing>> m_configurations;
    int m_nextConfigId;
    int m_activeConfigId;
    
    // Preset configurations
    std::unordered_map<std::string, AntiAliasingSettings> m_presets;
    
    // Helper methods
    void initializePresets();
    void applyMSAA(const AntiAliasingSettings& settings);
    void applyFXAA(const AntiAliasingSettings& settings);
    void applySSAA(const AntiAliasingSettings& settings);
    void applyTAA(const AntiAliasingSettings& settings);
    void disableAllAntiAliasing();
    
    // OpenGL state management
    void setupOpenGLState(const AntiAliasingSettings& settings);
    void restoreOpenGLState();
}; 