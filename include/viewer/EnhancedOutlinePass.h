#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <map>
#include <chrono>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/SoPath.h>
#include "viewer/ImageOutlinePass.h"

// Forward declarations
class SceneManager;
class SoSeparator;
class SoAnnotation;
class SoShaderProgram;
class SoVertexShader;
class SoFragmentShader;
class SoSceneTexture2;
class SoShaderParameter1f;
class SoShaderParameter2f;
class SoShaderParameter3f;
class SoShaderParameter4f;
class SoShaderParameter1i;
class SoCoordinate3;
class SoTextureCoordinate2;
class SoFaceSet;
class SoTexture2;
class SoCamera;
class SoSelection;

// Extended outline parameters
struct EnhancedOutlineParams : public ImageOutlineParams {
    // Colors
    SbColor outlineColor{0.0f, 0.0f, 0.0f}; // Default black
    SbColor selectedColor{0.0f, 0.5f, 1.0f}; // Blue for selected
    SbColor hoverColor{1.0f, 0.5f, 0.0f};    // Orange for hover
    SbColor glowColor{1.0f, 1.0f, 0.0f};     // Yellow glow
    SbColor backgroundColor{1.0f, 1.0f, 1.0f}; // Background color
    
    // Glow effects
    float glowStrength = 0.0f; // 0.0 - 1.0
    float glowRadius = 2.0f;   // Pixel radius for blur
    float glowIntensity = 0.0f; // Glow intensity
    
    // Selection and hover
    bool enableSelectionOutline = true;
    bool enableHoverOutline = true;
    
    // Anti-aliasing
    bool enableAntiAliasing = true; // MSAA or FXAA
    
    // Performance settings
    int downsampleFactor = 1; // 1 = full resolution, 2 = half, etc.
    bool enableEarlyCulling = true;
    bool enableMultiSample = false;
    
    // Additional parameters
    float colorWeight = 0.3f; // Color edge detection weight
    float colorThreshold = 0.1f; // Color threshold
    bool adaptiveThreshold = true; // Adaptive thresholding
    float smoothingFactor = 1.0f; // Smoothing factor
    float backgroundFade = 0.0f; // Background fade
    
    // Debug settings
    bool showDebugInfo = false;
    bool showPerformanceStats = false;
};

// Selection outline configuration
struct SelectionOutlineConfig {
    bool enableSelectionOutline = true;
    bool enableHoverOutline = true;
    float selectionIntensity = 1.0f;
    float hoverIntensity = 0.7f;
    float selectionThickness = 2.0f;
    float hoverThickness = 1.5f;
};

// Debug output modes
enum class OutlineDebugMode {
    None,
    ShowEdges,
    ShowNormals,
    ShowDepth,
    ShowObjectIds,
    ShowPerformance
};

// Enhanced outline pass class
class EnhancedOutlinePass {
public:
    EnhancedOutlinePass(SceneManager* sceneManager, SoSeparator* captureRoot);
    ~EnhancedOutlinePass();
    
    // Basic control
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
    // Parameters
    void setParams(const EnhancedOutlineParams& params);
    EnhancedOutlineParams getParams() const { return m_params; }
    
    void setSelectionConfig(const SelectionOutlineConfig& config);
    SelectionOutlineConfig getSelectionConfig() const { return m_selectionConfig; }
    
    // Selection and hover
    void setSelectedObjects(const std::vector<int>& objectIds);
    void setHoveredObject(int objectId);
    void clearHover();
    
    // Debug
    void setDebugMode(OutlineDebugMode mode);
    OutlineDebugMode getDebugMode() const { return m_debugMode; }
    
    // Performance settings
    void setDownsampleFactor(int factor);
    void setEarlyCullingEnabled(bool enabled);
    void setMultiSampleEnabled(bool enabled);
    
    // Refresh and update
    void refresh();
    void forceUpdate();
    
    // Callback system for custom outline logic
    using OutlineCallback = std::function<float(const SbVec3f& worldPos, const SbVec3f& normal, int objectId)>;
    void setCustomOutlineCallback(OutlineCallback callback);
    
    // Helper method for extracting object ID from path
    int extractObjectIdFromPath(SoPath* path);
    
    // Selection root management
    void setSelectionRoot(SoSelection* selectionRoot);

private:
    // Core rendering pipeline
    void attachOverlay();
    void detachOverlay();
    void buildShaders();
    void buildGeometry();
    void setupTextures();
    void updateShaderParameters();
    
    // FBO management
    void cleanupFBO();
    bool chooseTextureUnits();
    
    // Selection state management
    void updateSelectionState();
    
    // Internal helpers
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    
    // Core members
    SceneManager* m_sceneManager{nullptr};
    SoSeparator* m_captureRoot{nullptr};
    SoSeparator* m_overlayRoot{nullptr};
    
    // Shader components
    SoShaderProgram* m_program{nullptr};
    SoVertexShader* m_vs{nullptr};
    SoFragmentShader* m_fs{nullptr};
    
    // Geometry
    SoSeparator* m_quadSeparator{nullptr};
    SoCoordinate3* m_quadCoords{nullptr};
    SoTextureCoordinate2* m_quadTexCoords{nullptr};
    SoFaceSet* m_quadFaces{nullptr};
    
    // Textures
    SoSceneTexture2* m_colorTexture{nullptr};
    SoSceneTexture2* m_depthTexture{nullptr};
    SoSceneTexture2* m_normalTexture{nullptr};
    SoSceneTexture2* m_selectionTexture{nullptr};
    
    // Shader parameters
    SoShaderParameter1f* m_uDepthWeight{nullptr};
    SoShaderParameter1f* m_uNormalWeight{nullptr};
    SoShaderParameter1f* m_uColorWeight{nullptr};
    SoShaderParameter1f* m_uEdgeIntensity{nullptr};
    SoShaderParameter1f* m_uThickness{nullptr};
    SoShaderParameter3f* m_uOutlineColor{nullptr};
    SoShaderParameter3f* m_uSelectedColor{nullptr};
    SoShaderParameter3f* m_uHoverColor{nullptr};
    SoShaderParameter3f* m_uGlowColor{nullptr};
    SoShaderParameter1f* m_uGlowStrength{nullptr};
    SoShaderParameter1f* m_uGlowRadius{nullptr};
    SoShaderParameter1i* m_uDownsampleFactor{nullptr};
    SoShaderParameter1i* m_uEnableEarlyCulling{nullptr};
    
    // Temporary scene for selection rendering
    SoSeparator* m_tempSceneRoot{nullptr};
    
    // State
    bool m_enabled{false};
    EnhancedOutlineParams m_params;
    SelectionOutlineConfig m_selectionConfig;
    OutlineDebugMode m_debugMode{OutlineDebugMode::None};
    
    // Selection state
    std::vector<int> m_selectedObjects;
    int m_hoveredObject{-1};
    
    // Texture units
    int m_colorUnit{0};
    int m_depthUnit{1};
    int m_normalUnit{2};
    int m_selectionUnit{3};
    
    // Callback
    OutlineCallback m_customCallback;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point m_lastUpdateTime;
    int m_frameCount{0};
    float m_averageFrameTime{0.0f};
};