#pragma once

#include <memory>

// Forward declarations
class SceneManager;
class SoSeparator;
class SoShaderProgram;
class SoVertexShader;
class SoFragmentShader;
class SoSceneTexture2;
class SoTexture2;
class SoAnnotation;
class SoShaderParameter1f;
class SoShaderParameter2f;
class SoShaderParameter1i;
class SoShaderParameterMatrix;

/**
 * @brief Parameters for image-based outline rendering
 */
struct ImageOutlineParams {
    float edgeIntensity = 1.0f;        // Overall outline intensity (0.0 - 1.0)
    float depthWeight = 2.0f;          // Depth edge contribution (0.0 - 2.0)
    float normalWeight = 1.0f;         // Normal edge contribution (0.0 - 2.0)
    float depthThreshold = 0.0005f;    // Depth discontinuity threshold
    float normalThreshold = 0.1f;      // Normal angle threshold
    float thickness = 1.0f;            // Edge thickness multiplier (0.5 - 3.0)
};

/**
 * @brief Debug output modes for outline rendering
 */
enum class DebugOutput {
    Final = 0,      // Show final result with outlines
    ColorOnly = 1,  // Show color texture only
    EdgeOnly = 2    // Show edge detection only
};

/**
 * @brief Image-space outline rendering using fragment shaders
 * 
 * Implements real-time outline detection using depth and color discontinuities.
 * Uses render-to-texture to capture scene data and applies edge detection
 * in screen space for high-performance outline rendering.
 */
class ImageOutlinePass {
public:
    /**
     * @brief Constructor
     * @param sceneManager Scene manager for viewport access
     * @param captureRoot Scene root to capture (geometry only, no overlays)
     */
    ImageOutlinePass(SceneManager* sceneManager, SoSeparator* captureRoot);
    
    /**
     * @brief Destructor - cleans up all Coin3D resources
     */
    ~ImageOutlinePass();

    /**
     * @brief Enable or disable outline rendering
     * @param enabled True to enable outline rendering
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if outline rendering is enabled
     * @return True if outline rendering is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Set outline rendering parameters
     * @param params New parameters for outline rendering
     */
    void setParams(const ImageOutlineParams& params);
    
    /**
     * @brief Get current outline rendering parameters
     * @return Current outline parameters
     */
    const ImageOutlineParams& getParams() const { return m_params; }

    /**
     * @brief Refresh outline rendering (updates uniforms and viewport)
     */
    void refresh();

    /**
     * @brief Set debug output mode
     * @param mode Debug visualization mode
     */
    void setDebugOutput(DebugOutput mode);

private:
    void attachOverlay();
    void detachOverlay();
    void buildShaders();
    bool chooseTextureUnits();

    SceneManager* m_sceneManager;
    SoSeparator* m_captureRoot;
    SoSeparator* m_overlayRoot = nullptr;
    SoAnnotation* m_annotation = nullptr;

    // Shader resources
    SoShaderProgram* m_program = nullptr;
    SoVertexShader* m_vs = nullptr;
    SoFragmentShader* m_fs = nullptr;

    // Render targets
    SoSceneTexture2* m_colorTexture = nullptr;
    SoSceneTexture2* m_depthTexture = nullptr;
    SoTexture2* m_colorSampler = nullptr;
    SoTexture2* m_depthSampler = nullptr;

    // Fullscreen quad geometry
    SoSeparator* m_quadSeparator = nullptr;

    // Shader parameters
    SoShaderParameter1f* m_uIntensity = nullptr;
    SoShaderParameter1f* m_uDepthWeight = nullptr;
    SoShaderParameter1f* m_uNormalWeight = nullptr;
    SoShaderParameter1f* m_uDepthThreshold = nullptr;
    SoShaderParameter1f* m_uNormalThreshold = nullptr;
    SoShaderParameter1f* m_uThickness = nullptr;
    SoShaderParameter2f* m_uResolution = nullptr;
    SoShaderParameterMatrix* m_uInvProjection = nullptr;
    SoShaderParameterMatrix* m_uInvView = nullptr;
    SoShaderParameter1i* m_uDebugOutput = nullptr;

    // Texture units
    int m_colorUnit = 0;
    int m_depthUnit = 1;

    // State
    bool m_enabled = false;
    ImageOutlineParams m_params;
    DebugOutput m_debugOutput = DebugOutput::Final;
};