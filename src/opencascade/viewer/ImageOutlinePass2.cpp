#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/ImageOutlinePass2.h"
#include "viewer/IOutlineRenderer.h"

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
#include <Inventor/nodes/SoTextureCoordinate2.h>
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
    // Vertex shader - same as original
    static const char* kVS = R"GLSL(
        varying vec2 vTexCoord;
        void main() {
            vTexCoord = gl_MultiTexCoord0.xy;
            gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
        }
    )GLSL";

    // Fragment shader - same as original
    static const char* kFS = R"GLSL(
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
            vec2 texelSize = uResolution;
            vec4 color = texture2D(uColorTex, vTexCoord);
            
            float cEdge = colorSobel(vTexCoord, texelSize);
            float dEdge = depthEdge(vTexCoord, texelSize) * uDepthWeight;
            float nEdge = normalEdge(vTexCoord, texelSize) * uNormalWeight;
            
            float edge = clamp((cEdge + dEdge + nEdge) * uIntensity, 0.0, 1.0);
            
            vec3 outlineColor = vec3(0.0); // Black outline
            
            if (uDebugOutput == 1) {
                gl_FragColor = color;
            } else if (uDebugOutput == 2) {
                gl_FragColor = vec4(vec3(edge), 1.0);
            } else {
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
    , m_captureNode(nullptr)
    , m_colorSampler(nullptr)
    , m_depthSampler(nullptr)
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
    if (m_captureNode) { m_captureNode->unref(); m_captureNode = nullptr; }
    if (m_colorSampler) { m_colorSampler->unref(); m_colorSampler = nullptr; }
    if (m_depthSampler) { m_depthSampler->unref(); m_depthSampler = nullptr; }
    
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
    GLint maxUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
    if (maxUnits >= 2) {
        m_colorUnit = maxUnits - 1;
        m_depthUnit = maxUnits - 2;
        if (m_depthUnit < 0) { m_colorUnit = 0; m_depthUnit = 1; }
        return true;
    }
    m_colorUnit = 0;
    m_depthUnit = 1;
    return false;
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
    
    // Build shader resources
    buildShaders();
    
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
    
    // Create vertex shader
    m_vs = new SoVertexShader;
    m_vs->ref();
    m_vs->sourceProgram = kVS;
    m_program->shaderObject.set1Value(0, m_vs);
    
    // Create fragment shader
    m_fs = new SoFragmentShader;
    m_fs->ref();
    m_fs->sourceProgram = kFS;
    m_program->shaderObject.set1Value(1, m_fs);
    
    // Create scene capture node
    m_captureNode = new SoSceneTexture2;
    m_captureNode->ref();
    m_captureNode->transparencyFunction = SoSceneTexture2::NONE;
    m_captureNode->size = SbVec2s(512, 512); // Will be updated dynamically
    m_captureNode->wrapS = SoSceneTexture2::CLAMP_TO_BORDER;
    m_captureNode->wrapT = SoSceneTexture2::CLAMP_TO_BORDER;
    
    // Create temporary scene root that includes camera and capture root
    m_tempSceneRoot = new SoSeparator;
    m_tempSceneRoot->ref();
    if (m_renderer->getCamera()) {
        m_tempSceneRoot->addChild(m_renderer->getCamera());
    }
    m_tempSceneRoot->addChild(m_captureRoot);
    m_captureNode->scene = m_tempSceneRoot;
    
    // Create shader parameters
    m_uIntensity = new SoShaderParameter1f;
    m_uIntensity->ref();
    m_uIntensity->name = "uIntensity";
    m_uIntensity->value = m_params.edgeIntensity;
    
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
    
    // Add all parameters to program
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uIntensity);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uDepthWeight);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uNormalWeight);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uDepthThreshold);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uNormalThreshold);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uThickness);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uResolution);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uInvProjection);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uInvView);
    m_program->parameter.set1Value(m_program->parameter.getNum(), m_uDebugOutput);
    
    // Create fullscreen quad
    m_quadSeparator = new SoSeparator;
    m_quadSeparator->ref();
    
    // Disable lighting
    SoLightModel* lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    m_quadSeparator->addChild(lightModel);
    
    // White material
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    m_quadSeparator->addChild(material);
    
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