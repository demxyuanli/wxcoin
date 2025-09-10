#pragma once

#include <memory>
#include <vector>
#include <functional>

class SceneManager;
class SoSeparator;
class SoAnnotation;
class SoShaderProgram;
class SoFragmentShader;
class SoVertexShader;
class SoTexture2;
class SoSceneTexture2;
class SoCamera;
class SoShaderParameter1f;
class SoShaderParameter2f;
class SoShaderParameterMatrix;
class SoShaderParameter1i;
class SoShaderParameter3f;
class SoSelection;

// Enhanced outline parameters with more control options
struct EnhancedOutlineParams {
    // Core edge detection parameters
    float depthWeight = 1.5f;           // Weight for depth-based edges (0.0 - 3.0)
    float normalWeight = 1.0f;          // Weight for normal-based edges (0.0 - 3.0)
    float colorWeight = 0.3f;          // Weight for color-based edges (0.0 - 1.0)
    
    // Threshold parameters
    float depthThreshold = 0.001f;     // Depth discontinuity threshold
    float normalThreshold = 0.4f;      // Normal angle threshold
    float colorThreshold = 0.1f;       // Color difference threshold
    
    // Visual parameters
    float edgeIntensity = 1.0f;        // Overall outline strength (0.0 - 2.0)
    float thickness = 1.5f;             // Edge thickness multiplier (0.1 - 5.0)
    float glowIntensity = 0.0f;        // Glow effect intensity (0.0 - 1.0)
    float glowRadius = 2.0f;           // Glow effect radius (0.5 - 10.0)
    
    // Advanced parameters
    float adaptiveThreshold = 1.0f;    // Enable adaptive thresholding (0.0 - 1.0)
    float smoothingFactor = 0.5f;      // Edge smoothing factor (0.0 - 1.0)
    float backgroundFade = 0.8f;      // Background fade distance (0.0 - 1.0)
    
    // Color parameters
    float outlineColor[3] = {0.0f, 0.0f, 0.0f};  // RGB outline color
    float glowColor[3] = {1.0f, 1.0f, 0.0f};     // RGB glow color
    float backgroundColor[3] = {0.2f, 0.2f, 0.2f}; // RGB background color
    
    // Performance parameters
    int downsampleFactor = 1;          // Downsample factor for performance (1, 2, 4)
    bool enableEarlyCulling = true;    // Enable early depth culling
    bool enableMultiSample = false;    // Enable multi-sample anti-aliasing
};

// Selection-based outline configuration
struct SelectionOutlineConfig {
    bool enableSelectionOutline = true;    // Enable outline for selected objects
    bool enableHoverOutline = true;         // Enable outline for hovered objects
    bool enableAllObjectsOutline = false;   // Enable outline for all objects
    
    float selectionIntensity = 1.5f;       // Intensity for selected objects
    float hoverIntensity = 1.0f;           // Intensity for hovered objects
    float defaultIntensity = 0.8f;         // Intensity for other objects
    
    float selectionColor[3] = {1.0f, 0.0f, 0.0f};  // Red for selected
    float hoverColor[3] = {0.0f, 1.0f, 0.0f};       // Green for hovered
    float defaultColor[3] = {0.0f, 0.0f, 0.0f};     // Black for default
};

// Debug output modes
enum class OutlineDebugMode : int {
    Final = 0,              // Final composite image
    ShowColor = 1,          // Show color buffer
    ShowDepth = 2,          // Show depth buffer
    ShowNormals = 3,        // Show reconstructed normals
    ShowDepthEdges = 4,    // Show depth edges only
    ShowNormalEdges = 5,   // Show normal edges only
    ShowColorEdges = 6,    // Show color edges only
    ShowEdgeMask = 7,      // Show combined edge mask
    ShowGlow = 8,          // Show glow effect only
    ShowSelection = 9      // Show selection mask
};

/**
 * Enhanced Outline Pass for Coin3D-based applications
 * 
 * This class provides advanced outline rendering capabilities using FBO-based
 * post-processing techniques. It combines multiple edge detection algorithms
 * and provides extensive customization options.
 * 
 * Key Features:
 * - Multi-pass FBO rendering with depth, normal, and color buffers
 * - Advanced edge detection using Roberts Cross, Sobel, and normal-based methods
 * - Selection-aware outline rendering
 * - Glow effects and customizable colors
 * - Performance optimizations including downsampling and early culling
 * - Comprehensive debug visualization modes
 * 
 * Usage:
 * 1. Create instance with SceneManager and capture root
 * 2. Configure parameters using setParams()
 * 3. Enable/disable as needed
 * 4. Update selection state for selection-aware rendering
 */
class EnhancedOutlinePass {
public:
    EnhancedOutlinePass(SceneManager* sceneManager, SoSeparator* captureRoot);
    ~EnhancedOutlinePass();

    // Core functionality
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // Parameter management
    void setParams(const EnhancedOutlineParams& params);
    EnhancedOutlineParams getParams() const { return m_params; }
    
    void setSelectionConfig(const SelectionOutlineConfig& config);
    SelectionOutlineConfig getSelectionConfig() const { return m_selectionConfig; }

    // Selection management
    void setSelectionRoot(SoSelection* selectionRoot);
    void updateSelectionState();
    void setHoveredObject(int objectId);
    void clearHover();

    // Debug and visualization
    void setDebugMode(OutlineDebugMode mode);
    OutlineDebugMode getDebugMode() const { return m_debugMode; }

    // Performance and quality
    void setDownsampleFactor(int factor);
    void setMultiSampleEnabled(bool enabled);
    void setEarlyCullingEnabled(bool enabled);

    // Refresh and update
    void refresh();
    void forceUpdate();

    // Callback system for custom outline logic
    using OutlineCallback = std::function<float(const SbVec3f& worldPos, const SbVec3f& normal, int objectId)>;
    void setCustomOutlineCallback(OutlineCallback callback);

private:
    // Core rendering pipeline
    void attachOverlay();
    void detachOverlay();
    void buildShaders();
    void buildGeometry();
    void setupTextures();
    void updateShaderParameters();

    // FBO and texture management
    void initializeFBO();
    void cleanupFBO();
    void updateTextureSizes();
    bool chooseTextureUnits();

    // Camera and matrix management
    void updateCameraMatrices();
    void updateProjectionMatrix();
    void updateViewMatrix();

    // Selection and object management
    void updateSelectionMask();
    void updateObjectMasks();
    int getObjectIdAtPosition(const SbVec2f& screenPos);

    // Shader compilation and validation
    bool compileShaders();
    bool validateShaders();
    void logShaderErrors();

    // Scene manager and root references
    SceneManager* m_sceneManager;
    SoSeparator* m_captureRoot;
    SoSelection* m_selectionRoot;

    // State management
    bool m_enabled{ false };
    bool m_initialized{ false };
    bool m_needsUpdate{ true };

    // Parameters
    EnhancedOutlineParams m_params;
    SelectionOutlineConfig m_selectionConfig;
    OutlineDebugMode m_debugMode{ OutlineDebugMode::Final };

    // Scene graph nodes
    SoSeparator* m_overlayRoot{ nullptr };
    SoAnnotation* m_annotation{ nullptr };
    SoSeparator* m_tempSceneRoot{ nullptr };

    // Shader nodes
    SoShaderProgram* m_program{ nullptr };
    SoVertexShader* m_vs{ nullptr };
    SoFragmentShader* m_fs{ nullptr };

    // Render-to-texture nodes
    SoSceneTexture2* m_colorTexture{ nullptr };
    SoSceneTexture2* m_depthTexture{ nullptr };
    SoSceneTexture2* m_normalTexture{ nullptr };
    SoSceneTexture2* m_selectionTexture{ nullptr };

    // Geometry
    SoSeparator* m_quadSeparator{ nullptr };
    SoSeparator* m_blurQuadSeparator{ nullptr };

    // Shader parameters
    SoShaderParameter1f* m_uDepthWeight{ nullptr };
    SoShaderParameter1f* m_uNormalWeight{ nullptr };
    SoShaderParameter1f* m_uColorWeight{ nullptr };
    SoShaderParameter1f* m_uDepthThreshold{ nullptr };
    SoShaderParameter1f* m_uNormalThreshold{ nullptr };
    SoShaderParameter1f* m_uColorThreshold{ nullptr };
    SoShaderParameter1f* m_uEdgeIntensity{ nullptr };
    SoShaderParameter1f* m_uThickness{ nullptr };
    SoShaderParameter1f* m_uGlowIntensity{ nullptr };
    SoShaderParameter1f* m_uGlowRadius{ nullptr };
    SoShaderParameter1f* m_uAdaptiveThreshold{ nullptr };
    SoShaderParameter1f* m_uSmoothingFactor{ nullptr };
    SoShaderParameter1f* m_uBackgroundFade{ nullptr };
    SoShaderParameter3f* m_uOutlineColor{ nullptr };
    SoShaderParameter3f* m_uGlowColor{ nullptr };
    SoShaderParameter3f* m_uBackgroundColor{ nullptr };
    SoShaderParameter2f* m_uResolution{ nullptr };
    SoShaderParameterMatrix* m_uInvProjection{ nullptr };
    SoShaderParameterMatrix* m_uInvView{ nullptr };
    SoShaderParameter1i* m_uDebugMode{ nullptr };
    SoShaderParameter1i* m_uDownsampleFactor{ nullptr };
    SoShaderParameter1i* m_uEnableEarlyCulling{ nullptr };

    // Texture unit management
    int m_colorUnit{ 0 };
    int m_depthUnit{ 1 };
    int m_normalUnit{ 2 };
    int m_selectionUnit{ 3 };

    // Performance tracking
    int m_downsampleFactor{ 1 };
    bool m_multiSampleEnabled{ false };
    bool m_earlyCullingEnabled{ true };

    // Selection state
    std::vector<int> m_selectedObjects;
    int m_hoveredObject{ -1 };
    OutlineCallback m_customCallback;

    // Internal helpers
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
};