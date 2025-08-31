#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/ImageOutlinePass2.h"
#include "viewer/IOutlineRenderer.h"

#include <wx/glcanvas.h>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoShaderObject.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoTextureCoordinateBinding.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoSceneTexture2.h>
#include <Inventor/nodes/SoShaderParameter.h>
#include <Inventor/nodes/SoTextureUnit.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SbMatrix.h>

#include <GL/gl.h>

namespace {
    // Very simple test shaders
    static const char* kVS = R"GLSL(
        void main() {
            gl_TexCoord[0] = gl_MultiTexCoord0;
            gl_Position = ftransform();
        }
    )GLSL";

    // Fragment shader - simple test
    static const char* kFS = R"GLSL(
        void main() {
            // Always output red for testing
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    )GLSL";
}

namespace old_shaders {
    static const char* kFS_OLD = R"GLSL(
        #version 120
        varying vec2 vTexCoord;
        uniform sampler2D uColorTex;
        uniform sampler2D uDepthTex;

        uniform float uIntensity;
        uniform float uDepthWeight;
        uniform float uNormalWeight;
        uniform float uDepthThreshold;
        uniform float uNormalThreshold;
        uniform float uThickness;
        uniform vec2 uResolution;
        uniform mat4 uInvProjection;
        uniform mat4 uInvView;
        uniform int uDebugOutput;

        float sampleDepth(sampler2D tex, vec2 uv) {
            return texture2D(tex, uv).r;
        }
        
        float linearizeDepth(float depth) {
            float near = 0.1;
            float far = 1000.0;
            return (2.0 * near) / (far + near - depth * (far - near));
        }
        
        vec3 getWorldPos(vec2 uv, float depth) {
            vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
            vec4 viewPos = uInvProjection * clipPos;
            viewPos /= viewPos.w;
            vec4 worldPos = uInvView * viewPos;
            return worldPos.xyz;
        }
        
        vec3 getNormalFromDepth(vec2 uv, vec2 texelSize) {
            float depth = sampleDepth(uDepthTex, uv);
            vec3 pos = getWorldPos(uv, depth);
            
            vec2 offsetX = vec2(texelSize.x, 0.0);
            vec2 offsetY = vec2(0.0, texelSize.y);
            
            float depthX = sampleDepth(uDepthTex, uv + offsetX);
            float depthY = sampleDepth(uDepthTex, uv + offsetY);
            
            vec3 posX = getWorldPos(uv + offsetX, depthX);
            vec3 posY = getWorldPos(uv + offsetY, depthY);
            
            vec3 dx = posX - pos;
            vec3 dy = posY - pos;
            
            return normalize(cross(dy, dx));
        }

        float colorSobel(vec2 uv, vec2 texelSize) {
            vec2 o = texelSize * uThickness;
            vec3 tl = texture2D(uColorTex, uv + vec2(-o.x, -o.y)).rgb;
            vec3 tm = texture2D(uColorTex, uv + vec2( 0.0, -o.y)).rgb;
            vec3 tr = texture2D(uColorTex, uv + vec2( o.x, -o.y)).rgb;
            vec3 ml = texture2D(uColorTex, uv + vec2(-o.x,  0.0)).rgb;
            vec3 mr = texture2D(uColorTex, uv + vec2( o.x,  0.0)).rgb;
            vec3 bl = texture2D(uColorTex, uv + vec2(-o.x,  o.y)).rgb;
            vec3 bm = texture2D(uColorTex, uv + vec2( 0.0,  o.y)).rgb;
            vec3 br = texture2D(uColorTex, uv + vec2( o.x,  o.y)).rgb;
            float luma(vec3 c) { return dot(c, vec3(0.299,0.587,0.114)); }
            float gx = luma(tr) + 2.0*luma(mr) + luma(br) - (luma(tl) + 2.0*luma(ml) + luma(bl));
            float gy = luma(bl) + 2.0*luma(bm) + luma(br) - (luma(tl) + 2.0*luma(tm) + luma(tr));
            return length(vec2(gx, gy));
        }

        float depthEdge(vec2 uv, vec2 texelSize) {
            vec2 offset = texelSize * uThickness;
            
            float center = linearizeDepth(sampleDepth(uDepthTex, uv));
            float tl = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(-offset.x, -offset.y)));
            float tr = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(offset.x, -offset.y)));
            float bl = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(-offset.x, offset.y)));
            float br = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(offset.x, offset.y)));
            
            float robertsX = abs(center - br) + abs(tr - bl);
            float robertsY = abs(tl - br) + abs(center - tr);
            
            float edge = sqrt(robertsX * robertsX + robertsY * robertsY);
            
            float adaptiveThreshold = uDepthThreshold * (1.0 + center * 10.0);
            return smoothstep(0.0, adaptiveThreshold, edge);
        }

        float normalEdge(vec2 uv, vec2 texelSize) {
            vec3 normal = getNormalFromDepth(uv, texelSize);
            
            vec2 offset = texelSize * uThickness;
            vec3 normalRight = getNormalFromDepth(uv + vec2(offset.x, 0.0), texelSize);
            vec3 normalUp = getNormalFromDepth(uv + vec2(0.0, offset.y), texelSize);
            
            float dotRight = dot(normal, normalRight);
            float dotUp = dot(normal, normalUp);
            
            float edge = 1.0 - min(dotRight, dotUp);
            return smoothstep(0.0, uNormalThreshold, edge);
        }

        void main() {
            vec4 color = texture2D(uColorTex, vTexCoord);
            
            // Simple debug: just show the texture
            if (uDebugOutput == 1) {
                gl_FragColor = color;
            } else if (uDebugOutput == 2) {
                // Show texture coordinates as colors for debugging
                gl_FragColor = vec4(vTexCoord.x, vTexCoord.y, 0.0, 1.0);
            } else {
                // Normal outline processing
                vec2 texelSize = uResolution;
                
                float cEdge = colorSobel(vTexCoord, texelSize);
                float dEdge = depthEdge(vTexCoord, texelSize) * uDepthWeight;
                float nEdge = normalEdge(vTexCoord, texelSize) * uNormalWeight;
                
                float edge = clamp((cEdge + dEdge + nEdge) * uIntensity, 0.0, 1.0);
                
                vec3 outlineColor = vec3(0.0); // Black outline
                gl_FragColor = vec4(mix(color.rgb, outlineColor, edge), color.a);
            }
        }
    )GLSL";
}

ImageOutlinePass2::ImageOutlinePass2(IOutlineRenderer* renderer, SoSeparator* captureRoot)
    : m_renderer(renderer)
    , m_captureRoot(captureRoot)
    , m_overlayRoot(nullptr)
    , m_quadSeparator(nullptr)
    , m_tempSceneRoot(nullptr)
    , m_program(nullptr)
    , m_vs(nullptr)
    , m_fs(nullptr)
    , m_colorTexture(nullptr)
    , m_depthTexture(nullptr)
    , m_uIntensity(nullptr)
    , m_uDepthWeight(nullptr)
    , m_uNormalWeight(nullptr)
    , m_uDepthThreshold(nullptr)
    , m_uNormalThreshold(nullptr)
    , m_uThickness(nullptr)
    , m_uResolution(nullptr)
    , m_uInvProjection(nullptr)
    , m_uInvView(nullptr)
    , m_uDebugOutput(nullptr)
    , m_enabled(false)
    , m_debugOutput(DebugOutput::Final)
    , m_colorUnit(0)
    , m_depthUnit(1) {
    
    if (!m_renderer) {
        return;
    }
    
    // Select texture units
    chooseTextureUnits();
}

ImageOutlinePass2::~ImageOutlinePass2() {
    detachOverlay();
    
    if (m_quadSeparator) { m_quadSeparator->unref(); m_quadSeparator = nullptr; }
    if (m_overlayRoot) { m_overlayRoot->unref(); m_overlayRoot = nullptr; }
    if (m_program) { m_program->unref(); m_program = nullptr; }
    if (m_vs) { m_vs->unref(); m_vs = nullptr; }
    if (m_fs) { m_fs->unref(); m_fs = nullptr; }
    if (m_colorTexture) { m_colorTexture->unref(); m_colorTexture = nullptr; }
    if (m_depthTexture) { m_depthTexture->unref(); m_depthTexture = nullptr; }
    
    // Clean up shader parameters
    if (m_uIntensity) { m_uIntensity->unref(); m_uIntensity = nullptr; }
    if (m_uDepthWeight) { m_uDepthWeight->unref(); m_uDepthWeight = nullptr; }
    if (m_uNormalWeight) { m_uNormalWeight->unref(); m_uNormalWeight = nullptr; }
    if (m_uDepthThreshold) { m_uDepthThreshold->unref(); m_uDepthThreshold = nullptr; }
    if (m_uNormalThreshold) { m_uNormalThreshold->unref(); m_uNormalThreshold = nullptr; }
    if (m_uThickness) { m_uThickness->unref(); m_uThickness = nullptr; }
    if (m_uResolution) { m_uResolution->unref(); m_uResolution = nullptr; }
    if (m_uInvProjection) { m_uInvProjection->unref(); m_uInvProjection = nullptr; }
    if (m_uInvView) { m_uInvView->unref(); m_uInvView = nullptr; }
    if (m_uDebugOutput) { m_uDebugOutput->unref(); m_uDebugOutput = nullptr; }
    
    if (m_tempSceneRoot) { m_tempSceneRoot->unref(); m_tempSceneRoot = nullptr; }
}

bool ImageOutlinePass2::chooseTextureUnits() {
    // Use simple fixed texture units for now
    m_colorUnit = 0;
    m_depthUnit = 1;
    return true;
}

void ImageOutlinePass2::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (m_enabled) attachOverlay(); else detachOverlay();
    if (m_renderer) m_renderer->requestRedraw();
}

void ImageOutlinePass2::setParams(const ImageOutlineParams& p) {
    m_params = p;
    refresh();
}

void ImageOutlinePass2::refresh() {
    // Update shader parameters if they exist
    if (m_uIntensity) m_uIntensity->value = m_params.edgeIntensity;
    if (m_uDepthWeight) m_uDepthWeight->value = m_params.depthWeight;
    if (m_uNormalWeight) m_uNormalWeight->value = m_params.normalWeight;
    if (m_uDepthThreshold) m_uDepthThreshold->value = m_params.depthThreshold;
    if (m_uNormalThreshold) m_uNormalThreshold->value = m_params.normalThreshold;
    if (m_uThickness) m_uThickness->value = m_params.thickness;

    // Update resolution if viewport has changed
    if (m_uResolution && m_renderer && m_renderer->getGLCanvas()) {
        int width, height;
        m_renderer->getGLCanvas()->GetSize(&width, &height);
        if (width > 0 && height > 0) {
            m_uResolution->value = SbVec2f(1.0f / float(width), 1.0f / float(height));
        }
        m_renderer->requestRedraw();
    }

    // Update camera matrices
    updateCameraMatrices();
}

void ImageOutlinePass2::setDebugOutput(DebugOutput mode) {
    m_debugOutput = mode;
    if (m_uDebugOutput) m_uDebugOutput->value = static_cast<int>(mode);
    if (m_renderer) m_renderer->requestRedraw();
}

void ImageOutlinePass2::attachOverlay() {
    if (!m_renderer) return;
    if (m_overlayRoot) return; // Already attached
    
    SoSeparator* root = m_renderer->getSceneRoot();
    if (!root) return;

    m_overlayRoot = new SoSeparator;
    m_overlayRoot->ref();
    SoAnnotation* annotation = new SoAnnotation;
    m_overlayRoot->addChild(annotation);
    
    // Add a camera-facing transform for the quad
    auto* transform = new SoTransform;
    annotation->addChild(transform);
    
    // Build shader resources
    buildShaders();
    
    // Add scene capture textures
    if (m_colorTexture) {
        // Color texture
        auto* texUnit0 = new SoTextureUnit;
        texUnit0->unit = m_colorUnit;
        annotation->addChild(texUnit0);
        annotation->addChild(m_colorTexture);
        
        auto* colorBind = new SoShaderParameter1i;
        colorBind->name = "uColorTex";
        colorBind->value = m_colorUnit;
        annotation->addChild(colorBind);
    }
    
    if (m_depthTexture) {
        // Depth texture
        auto* texUnit1 = new SoTextureUnit;
        texUnit1->unit = m_depthUnit;
        annotation->addChild(texUnit1);
        annotation->addChild(m_depthTexture);
        
        auto* depthBind = new SoShaderParameter1i;
        depthBind->name = "uDepthTex";
        depthBind->value = m_depthUnit;
        annotation->addChild(depthBind);
    }
    
    // Add shader parameters
    if (m_uIntensity) annotation->addChild(m_uIntensity);
    if (m_uDepthWeight) annotation->addChild(m_uDepthWeight);
    if (m_uNormalWeight) annotation->addChild(m_uNormalWeight);
    if (m_uDepthThreshold) annotation->addChild(m_uDepthThreshold);
    if (m_uNormalThreshold) annotation->addChild(m_uNormalThreshold);
    if (m_uThickness) annotation->addChild(m_uThickness);
    if (m_uResolution) annotation->addChild(m_uResolution);
    if (m_uInvProjection) annotation->addChild(m_uInvProjection);
    if (m_uInvView) annotation->addChild(m_uInvView);
    if (m_uDebugOutput) annotation->addChild(m_uDebugOutput);
    
    // Add shader program and quad
    if (m_program) annotation->addChild(m_program);
    if (m_quadSeparator) annotation->addChild(m_quadSeparator);
    
    // Attach to scene
    root->addChild(m_overlayRoot);
}

void ImageOutlinePass2::detachOverlay() {
    if (!m_overlayRoot || !m_renderer) return;
    
    SoSeparator* root = m_renderer->getSceneRoot();
    if (root) {
        root->removeChild(m_overlayRoot);
    }
    
    m_overlayRoot->unref();
    m_overlayRoot = nullptr;
}

void ImageOutlinePass2::buildShaders() {
    // Create shader program
    m_program = new SoShaderProgram;
    m_program->ref();
    
    // Enable shader program
    m_program->isActive = TRUE;
    
    // Create vertex shader
    m_vs = new SoVertexShader;
    m_vs->ref();
    m_vs->sourceProgram = kVS;
    m_vs->sourceType = SoShaderObject::GLSL_PROGRAM;
    m_program->shaderObject.set1Value(0, m_vs);
    
    // Create fragment shader
    m_fs = new SoFragmentShader;
    m_fs->ref();
    m_fs->sourceProgram = kFS;
    m_fs->sourceType = SoShaderObject::GLSL_PROGRAM;
    m_program->shaderObject.set1Value(1, m_fs);
    
    // Create color texture capture node
    m_colorTexture = new SoSceneTexture2;
    m_colorTexture->ref();
    m_colorTexture->transparencyFunction = SoSceneTexture2::NONE;
    m_colorTexture->size = SbVec2s(0, 0); // auto-size to viewport
    m_colorTexture->type = SoSceneTexture2::RGBA8;
    m_colorTexture->wrapS = SoSceneTexture2::CLAMP_TO_BORDER;
    m_colorTexture->wrapT = SoSceneTexture2::CLAMP_TO_BORDER;
    m_colorTexture->backgroundColor = SbVec4f(0.5f, 0.5f, 0.5f, 1.0f); // Gray background for debugging
    
    // Create depth texture capture node
    m_depthTexture = new SoSceneTexture2;
    m_depthTexture->ref();
    m_depthTexture->transparencyFunction = SoSceneTexture2::NONE;
    m_depthTexture->size = SbVec2s(0, 0); // auto-size to viewport
    m_depthTexture->type = SoSceneTexture2::DEPTH;
    m_depthTexture->wrapS = SoSceneTexture2::CLAMP;
    m_depthTexture->wrapT = SoSceneTexture2::CLAMP;
    
    // Create temporary scene root that includes camera and capture root
    m_tempSceneRoot = new SoSeparator;
    m_tempSceneRoot->ref();
    if (m_renderer->getCamera()) {
        m_tempSceneRoot->addChild(m_renderer->getCamera());
    }
    m_tempSceneRoot->addChild(m_captureRoot);
    
    // Set the same scene for both textures
    m_colorTexture->scene = m_tempSceneRoot;
    m_depthTexture->scene = m_tempSceneRoot;
    
    // Skip shader parameters for now - just test basic shader execution
    
    m_uDepthWeight = new SoShaderParameter1f;
    m_uDepthWeight->ref();
    m_uDepthWeight->name = "uDepthWeight";
    m_uDepthWeight->value = m_params.depthWeight;
    
    m_uNormalWeight = new SoShaderParameter1f;
    m_uNormalWeight->ref();
    m_uNormalWeight->name = "uNormalWeight";
    m_uNormalWeight->value = m_params.normalWeight;
    
    m_uDepthThreshold = new SoShaderParameter1f;
    m_uDepthThreshold->ref();
    m_uDepthThreshold->name = "uDepthThreshold";
    m_uDepthThreshold->value = m_params.depthThreshold;
    
    m_uNormalThreshold = new SoShaderParameter1f;
    m_uNormalThreshold->ref();
    m_uNormalThreshold->name = "uNormalThreshold";
    m_uNormalThreshold->value = m_params.normalThreshold;
    
    m_uThickness = new SoShaderParameter1f;
    m_uThickness->ref();
    m_uThickness->name = "uThickness";
    m_uThickness->value = m_params.thickness;
    
    m_uResolution = new SoShaderParameter2f;
    m_uResolution->ref();
    m_uResolution->name = "uResolution";
    m_uResolution->value = SbVec2f(1.0f/512.0f, 1.0f/512.0f);
    
    m_uInvProjection = new SoShaderParameterMatrix;
    m_uInvProjection->ref();
    m_uInvProjection->name = "uInvProjection";
    
    m_uInvView = new SoShaderParameterMatrix;
    m_uInvView->ref();
    m_uInvView->name = "uInvView";
    
    m_uDebugOutput = new SoShaderParameter1i;
    m_uDebugOutput->ref();
    m_uDebugOutput->name = "uDebugOutput";
    m_uDebugOutput->value = static_cast<int>(m_debugOutput);
    
    // Parameters will be added to the annotation node in attachOverlay()
    
    // Create fullscreen quad
    m_quadSeparator = new SoSeparator;
    m_quadSeparator->ref();
    
    // Disable lighting
    SoLightModel* lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    m_quadSeparator->addChild(lightModel);
    
    // No material - let shader handle color
    
    // Texture coordinate binding
    SoTextureCoordinateBinding* texBinding = new SoTextureCoordinateBinding;
    texBinding->value = SoTextureCoordinateBinding::PER_VERTEX;
    m_quadSeparator->addChild(texBinding);
    
    // Texture coordinates
    SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
    texCoords->point.set1Value(0, SbVec2f(0, 0));
    texCoords->point.set1Value(1, SbVec2f(1, 0));
    texCoords->point.set1Value(2, SbVec2f(1, 1));
    texCoords->point.set1Value(3, SbVec2f(0, 1));
    m_quadSeparator->addChild(texCoords);
    
    // Quad vertices
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.set1Value(0, SbVec3f(-1, -1, 0));
    coords->point.set1Value(1, SbVec3f( 1, -1, 0));
    coords->point.set1Value(2, SbVec3f( 1,  1, 0));
    coords->point.set1Value(3, SbVec3f(-1,  1, 0));
    m_quadSeparator->addChild(coords);
    
    // Face set
    SoFaceSet* faceSet = new SoFaceSet;
    faceSet->numVertices.set1Value(0, 4);
    m_quadSeparator->addChild(faceSet);
    
    // Update camera matrices
    updateCameraMatrices();
}

void ImageOutlinePass2::updateCameraMatrices() {
    if (!m_renderer || !m_renderer->getCamera()) return;
    
    SoCamera* camera = m_renderer->getCamera();
    SbViewVolume viewVolume = camera->getViewVolume();
    
    // Get projection matrix
    SbMatrix projMatrix = viewVolume.getMatrix();
    SbMatrix invProjMatrix = projMatrix.inverse();
    
    // Get view matrix (camera transformation)
    SbMatrix viewMatrix;
    viewMatrix.setTransform(camera->position.getValue(),
                           camera->orientation.getValue(),
                           SbVec3f(1, 1, 1));
    SbMatrix invViewMatrix = viewMatrix.inverse();
    
    // Update shader parameters
    if (m_uInvProjection) m_uInvProjection->value = invProjMatrix;
    if (m_uInvView) m_uInvView->value = invViewMatrix;
}