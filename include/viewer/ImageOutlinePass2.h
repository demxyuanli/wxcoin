#pragma once

#include <memory>
#include "viewer/IOutlineRenderer.h"
#include "viewer/ImageOutlinePass.h" // For ImageOutlineParams

class SoSeparator;
class SoCamera;
class SoAnnotation;
class SoShaderProgram;
class SoVertexShader;
class SoFragmentShader;
class SoSceneTexture2;
class SoShaderParameter1f;
class SoShaderParameter2f;
class SoShaderParameter1i;
class SoShaderParameterMatrix;
class SoTextureUnit;
class wxGLCanvas;

// Generic ImageOutlinePass that works with any IOutlineRenderer
class ImageOutlinePass2 {
public:
    ImageOutlinePass2(IOutlineRenderer* renderer, SoSeparator* captureRoot);
    ~ImageOutlinePass2();

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void setParams(const ImageOutlineParams& p);
    ImageOutlineParams getParams() const { return m_params; }

    enum class DebugOutput : int { Final = 0, ShowColor = 1, ShowEdge = 2 };
    void setDebugOutput(DebugOutput mode);

    void refresh();

private:
    void attachOverlay();
    void detachOverlay();
    void buildShaders();
    void updateCameraMatrices();
    bool chooseTextureUnits();
    void renderFallback();

    IOutlineRenderer* m_renderer;
    SoSeparator* m_captureRoot;
    SoSeparator* m_overlayRoot;
    SoSeparator* m_quadSeparator;
    SoSeparator* m_tempSceneRoot;

    // Shader components
    SoShaderProgram* m_program;
    SoVertexShader* m_vs;
    SoFragmentShader* m_fs;

    // Textures
    SoSceneTexture2* m_colorTexture;
    SoSceneTexture2* m_depthTexture;

    // Shader parameters
    SoShaderParameter1f* m_uIntensity;
    SoShaderParameter1f* m_uDepthWeight;
    SoShaderParameter1f* m_uNormalWeight;
    SoShaderParameter1f* m_uDepthThreshold;
    SoShaderParameter1f* m_uNormalThreshold;
    SoShaderParameter1f* m_uThickness;
    SoShaderParameter2f* m_uResolution;
    SoShaderParameterMatrix* m_uInvProjection;
    SoShaderParameterMatrix* m_uInvView;
    SoShaderParameter1i* m_uDebugOutput;

    ImageOutlineParams m_params;
    bool m_enabled;
    DebugOutput m_debugOutput;
    
    int m_colorUnit;
    int m_depthUnit;
};