#pragma once

#include "RenderingSettings.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

// Forward declarations
class SoSeparator;
class SoGLRenderAction;
class wxGLCanvas;
class wxGLContext;

// Managed rendering configuration
struct ManagedRendering {
    RenderingSettings settings;
    int configId;
    bool isActive;
    
    ManagedRendering() : configId(-1), isActive(false) {}
};

/**
 * @brief Rendering parameter manager for unified rendering management
 * 
 * Manages rendering configurations and applies them to the rendering pipeline
 */
class RenderingManager {
public:
    RenderingManager(SoSeparator* sceneRoot, wxGLCanvas* canvas, wxGLContext* glContext);
    ~RenderingManager();
    
    // Configuration management methods
    int addConfiguration(const RenderingSettings& settings);
    bool removeConfiguration(int configId);
    bool updateConfiguration(int configId, const RenderingSettings& settings);
    void clearAllConfigurations();
    
    // Configuration query methods
    std::vector<int> getAllConfigurationIds() const;
    std::vector<RenderingSettings> getAllConfigurations() const;
    RenderingSettings getConfiguration(int configId) const;
    bool hasConfiguration(int configId) const;
    int getConfigurationCount() const;
    
    // Active configuration management
    bool setActiveConfiguration(int configId);
    int getActiveConfigurationId() const;
    RenderingSettings getActiveConfiguration() const;
    bool hasActiveConfiguration() const;
    
    // Parameter update methods
    void setRenderingMode(int configId, int mode);
    void setQuality(int configId, int quality);
    void setFastMode(int configId, bool enabled);
    void setTransparencyType(int configId, int type);
    void setShadingMode(int configId, bool smooth, bool phong);
    void setCullingMode(int configId, int mode);
    void setDepthSettings(int configId, bool test, bool write);
    void setPolygonMode(int configId, int mode);
    void setBackgroundColor(int configId, const wxColour& color);
    
    // Preset management
    void applyPreset(const std::string& presetName);
    void saveAsPreset(int configId, const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    
    // Rendering application
    void applyToRenderAction(SoGLRenderAction* renderAction);
    void setupRenderingState();
    void restoreRenderingState();
    
    // Rendering mode application methods
    void applySolidMode(const RenderingSettings& settings);
    void applyWireframeMode(const RenderingSettings& settings);
    void applyPointsMode(const RenderingSettings& settings);
    void applyHiddenLineMode(const RenderingSettings& settings);
    void applyShadedMode(const RenderingSettings& settings);
    void applyNoShadingMode(const RenderingSettings& settings);
    
    // Shading and material methods
    void enablePhongShading();
    void enableGouraudShading();
    void configureMaterialProperties(const RenderingSettings& settings);
    
    // Performance monitoring
    float getPerformanceImpact() const;
    std::string getQualityDescription() const;
    int getEstimatedFPS() const;
    
private:
    SoSeparator* m_sceneRoot;
    wxGLCanvas* m_canvas;
    wxGLContext* m_glContext;
    std::unordered_map<int, std::unique_ptr<ManagedRendering>> m_configurations;
    int m_nextConfigId;
    int m_activeConfigId;
    
    // Preset configurations
    std::unordered_map<std::string, RenderingSettings> m_presets;
    
    // Helper methods
    void initializePresets();
    void applyRenderingMode(const RenderingSettings& settings);
    void applyQualitySettings(const RenderingSettings& settings);
    void applyTransparencySettings(const RenderingSettings& settings);
    void applyShadingSettings(const RenderingSettings& settings);
    void applyCullingSettings(const RenderingSettings& settings);
    void applyDepthSettings(const RenderingSettings& settings);
    void applyPolygonSettings(const RenderingSettings& settings);
    void applyBackgroundSettings(const RenderingSettings& settings);
    
    // OpenGL state management
    void setupOpenGLState(const RenderingSettings& settings);
    void restoreOpenGLState();
    
    // Performance optimization
    void optimizeForPerformance(const RenderingSettings& settings);
    void optimizeForQuality(const RenderingSettings& settings);
}; 