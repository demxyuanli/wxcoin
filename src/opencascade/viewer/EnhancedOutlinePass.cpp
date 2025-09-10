#include "viewer/EnhancedOutlinePass.h"

#include "SceneManager.h"
#include "Canvas.h"
#include "logger/Logger.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <Inventor/SoPath.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoSceneTexture2.h>
#include <Inventor/nodes/SoShaderParameter.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SbViewportRegion.h>

#include <GL/gl.h>
#include <cmath>
#include <algorithm>

// Shader source code
static const char* kEnhancedVS = R"(
#version 330 core

in vec3 position;
in vec2 texCoord;

out vec2 vTexCoord;

void main() {
    gl_Position = vec4(position, 1.0);
    vTexCoord = texCoord;
}
)";

static const char* kEnhancedFS = R"(
#version 330 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uColorTex;
uniform sampler2D uDepthTex;
uniform sampler2D uNormalTex;
uniform sampler2D uSelectionTex;

uniform float uDepthWeight;
uniform float uNormalWeight;
uniform float uColorWeight;
uniform float uEdgeIntensity;
uniform float uThickness;
uniform vec3 uOutlineColor;
uniform vec3 uSelectedColor;
uniform vec3 uHoverColor;
uniform vec3 uGlowColor;
uniform float uGlowStrength;
uniform float uGlowRadius;
uniform int uDownsampleFactor;
uniform int uEnableEarlyCulling;

vec2 texelSize = 1.0 / textureSize(uDepthTex, 0);

// Roberts Cross edge detection on depth
float depthEdge(vec2 uv, vec2 texelSize) {
    float tl = texture(uDepthTex, uv + vec2(-texelSize.x, -texelSize.y)).r;
    float tr = texture(uDepthTex, uv + vec2(texelSize.x, -texelSize.y)).r;
    float bl = texture(uDepthTex, uv + vec2(-texelSize.x, texelSize.y)).r;
    float br = texture(uDepthTex, uv + vec2(texelSize.x, texelSize.y)).r;
    
    float gx = tl - br;
    float gy = tr - bl;
    
    return sqrt(gx * gx + gy * gy);
}

// Normal-based edge detection
float normalEdge(vec2 uv, vec2 texelSize) {
    vec3 n1 = texture(uNormalTex, uv + vec2(-texelSize.x, 0)).rgb;
    vec3 n2 = texture(uNormalTex, uv + vec2(texelSize.x, 0)).rgb;
    vec3 n3 = texture(uNormalTex, uv + vec2(0, -texelSize.y)).rgb;
    vec3 n4 = texture(uNormalTex, uv + vec2(0, texelSize.y)).rgb;
    
    float edgeX = length(n1 - n2);
    float edgeY = length(n3 - n4);
    
    return max(edgeX, edgeY);
}

// Color luminance-based Sobel
float colorSobel(vec2 uv, vec2 texelSize) {
    vec3 tl = texture(uColorTex, uv + vec2(-texelSize.x, -texelSize.y)).rgb;
    vec3 tm = texture(uColorTex, uv + vec2(0, -texelSize.y)).rgb;
    vec3 tr = texture(uColorTex, uv + vec2(texelSize.x, -texelSize.y)).rgb;
    vec3 ml = texture(uColorTex, uv + vec2(-texelSize.x, 0)).rgb;
    vec3 mm = texture(uColorTex, uv).rgb;
    vec3 mr = texture(uColorTex, uv + vec2(texelSize.x, 0)).rgb;
    vec3 bl = texture(uColorTex, uv + vec2(-texelSize.x, texelSize.y)).rgb;
    vec3 bm = texture(uColorTex, uv + vec2(0, texelSize.y)).rgb;
    vec3 br = texture(uColorTex, uv + vec2(texelSize.x, texelSize.y)).rgb;
    
    // Convert to luminance
    float tlL = dot(tl, vec3(0.299, 0.587, 0.114));
    float tmL = dot(tm, vec3(0.299, 0.587, 0.114));
    float trL = dot(tr, vec3(0.299, 0.587, 0.114));
    float mlL = dot(ml, vec3(0.299, 0.587, 0.114));
    float mmL = dot(mm, vec3(0.299, 0.587, 0.114));
    float mrL = dot(mr, vec3(0.299, 0.587, 0.114));
    float blL = dot(bl, vec3(0.299, 0.587, 0.114));
    float bmL = dot(bm, vec3(0.299, 0.587, 0.114));
    float brL = dot(br, vec3(0.299, 0.587, 0.114));
    
    float gx = (trL + 2.0 * mrL + brL) - (tlL + 2.0 * mlL + blL);
    float gy = (blL + 2.0 * bmL + brL) - (tlL + 2.0 * tmL + trL);
    
    return sqrt(gx * gx + gy * gy);
}

void main() {
    vec4 color = texture(uColorTex, vTexCoord);
    vec4 selection = texture(uSelectionTex, vTexCoord);
    
    // Calculate edges
    float depthE = depthEdge(vTexCoord, texelSize) * uDepthWeight;
    float normalE = normalEdge(vTexCoord, texelSize) * uNormalWeight;
    float colorE = colorSobel(vTexCoord, texelSize) * uColorWeight;
    
    float edge = clamp(depthE + normalE + colorE, 0.0, 1.0);
    
    // Determine outline color based on selection
    vec3 outlineColor = uOutlineColor;
    if (selection.r > 0.5) {
        outlineColor = uSelectedColor;
    } else if (selection.g > 0.5) {
        outlineColor = uHoverColor;
    }
    
    // Apply glow effect
    if (uGlowStrength > 0.0) {
        vec3 glow = uGlowColor * uGlowStrength;
        outlineColor = mix(outlineColor, glow, 0.5);
    }
    
    // Mix outline with original color
    vec3 finalColor = mix(color.rgb, outlineColor, edge * uEdgeIntensity);
    
    FragColor = vec4(finalColor, color.a);
}
)";

EnhancedOutlinePass::EnhancedOutlinePass(SceneManager* sceneManager, SoSeparator* captureRoot)
    : m_sceneManager(sceneManager), m_captureRoot(captureRoot) {
    
    LOG_INF("EnhancedOutlinePass constructed", "EnhancedOutlinePass");
    
    // Initialize with enhanced default parameters
    m_params = EnhancedOutlineParams();
    m_selectionConfig = SelectionOutlineConfig();
}

EnhancedOutlinePass::~EnhancedOutlinePass() {
    LOG_INF("EnhancedOutlinePass destructor begin", "EnhancedOutlinePass");
    
    setEnabled(false);
    cleanupFBO();
    
    // Clean up all nodes
    if (m_program) { m_program->unref(); m_program = nullptr; }
    if (m_vs) { m_vs->unref(); m_vs = nullptr; }
    if (m_fs) { m_fs->unref(); m_fs = nullptr; }
    if (m_quadSeparator) { m_quadSeparator->unref(); m_quadSeparator = nullptr; }
    if (m_quadCoords) { m_quadCoords->unref(); m_quadCoords = nullptr; }
    if (m_quadTexCoords) { m_quadTexCoords->unref(); m_quadTexCoords = nullptr; }
    if (m_quadFaces) { m_quadFaces->unref(); m_quadFaces = nullptr; }
    if (m_colorTexture) { m_colorTexture->unref(); m_colorTexture = nullptr; }
    if (m_depthTexture) { m_depthTexture->unref(); m_depthTexture = nullptr; }
    if (m_normalTexture) { m_normalTexture->unref(); m_normalTexture = nullptr; }
    if (m_selectionTexture) { m_selectionTexture->unref(); m_selectionTexture = nullptr; }
    
    // Clean up shader parameters
    if (m_uDepthWeight) { m_uDepthWeight->unref(); m_uDepthWeight = nullptr; }
    if (m_uNormalWeight) { m_uNormalWeight->unref(); m_uNormalWeight = nullptr; }
    if (m_uColorWeight) { m_uColorWeight->unref(); m_uColorWeight = nullptr; }
    if (m_uEdgeIntensity) { m_uEdgeIntensity->unref(); m_uEdgeIntensity = nullptr; }
    if (m_uThickness) { m_uThickness->unref(); m_uThickness = nullptr; }
    if (m_uOutlineColor) { m_uOutlineColor->unref(); m_uOutlineColor = nullptr; }
    if (m_uSelectedColor) { m_uSelectedColor->unref(); m_uSelectedColor = nullptr; }
    if (m_uHoverColor) { m_uHoverColor->unref(); m_uHoverColor = nullptr; }
    if (m_uGlowColor) { m_uGlowColor->unref(); m_uGlowColor = nullptr; }
    if (m_uGlowStrength) { m_uGlowStrength->unref(); m_uGlowStrength = nullptr; }
    if (m_uGlowRadius) { m_uGlowRadius->unref(); m_uGlowRadius = nullptr; }
    if (m_uDownsampleFactor) { m_uDownsampleFactor->unref(); m_uDownsampleFactor = nullptr; }
    if (m_uEnableEarlyCulling) { m_uEnableEarlyCulling->unref(); m_uEnableEarlyCulling = nullptr; }
    
    if (m_tempSceneRoot) { m_tempSceneRoot->unref(); m_tempSceneRoot = nullptr; }
    
    LOG_INF("EnhancedOutlinePass destructor end", "EnhancedOutlinePass");
}

void EnhancedOutlinePass::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    
    m_enabled = enabled;
    LOG_INF((std::string("setEnabled ") + (enabled ? "true" : "false")).c_str(), "EnhancedOutlinePass");
    
    if (m_enabled) {
        attachOverlay();
    } else {
        detachOverlay();
    }
}

void EnhancedOutlinePass::setParams(const EnhancedOutlineParams& params) {
    m_params = params;
    
    LOG_INF((std::string("setParams - depthWeight: ") + std::to_string(params.depthWeight) +
            ", normalWeight: " + std::to_string(params.normalWeight) +
            ", colorWeight: " + std::to_string(params.colorWeight) +
            ", edgeIntensity: " + std::to_string(params.edgeIntensity) +
            ", thickness: " + std::to_string(params.thickness)).c_str(), "EnhancedOutlinePass");
    
    refresh();
}

void EnhancedOutlinePass::setSelectionConfig(const SelectionOutlineConfig& config) {
    m_selectionConfig = config;
    refresh();
}

void EnhancedOutlinePass::setSelectedObjects(const std::vector<int>& objectIds) {
    m_selectedObjects = objectIds;
    updateSelectionState();
}

void EnhancedOutlinePass::setHoveredObject(int objectId) {
    m_hoveredObject = objectId;
    refresh();
}

void EnhancedOutlinePass::clearHover() {
    m_hoveredObject = -1;
    refresh();
}

void EnhancedOutlinePass::setDebugMode(OutlineDebugMode mode) {
    m_debugMode = mode;
    refresh();
}

void EnhancedOutlinePass::setDownsampleFactor(int factor) {
    m_params.downsampleFactor = factor;
    refresh();
}

void EnhancedOutlinePass::setEarlyCullingEnabled(bool enabled) {
    m_params.enableEarlyCulling = enabled;
    refresh();
}

void EnhancedOutlinePass::setMultiSampleEnabled(bool enabled) {
    m_params.enableMultiSample = enabled;
    refresh();
}

void EnhancedOutlinePass::refresh() {
    if (m_enabled) {
        updateShaderParameters();
    }
}

void EnhancedOutlinePass::forceUpdate() {
    refresh();
}

void EnhancedOutlinePass::setCustomOutlineCallback(OutlineCallback callback) {
    m_customCallback = callback;
}

int EnhancedOutlinePass::extractObjectIdFromPath(SoPath* path) {
    if (!path) return -1;
    return path->getLength() % 1000; // Simple implementation
}

void EnhancedOutlinePass::attachOverlay() {
    if (!m_sceneManager || m_overlayRoot) return;
    
    LOG_INF("attachOverlay begin", "EnhancedOutlinePass");
    
    SoSeparator* root = m_sceneManager->getObjectRoot();
    if (!root) {
        LOG_ERR("No object root found", "EnhancedOutlinePass");
        return;
    }
    
    // Choose texture units
    chooseTextureUnits();
    
    // Build components
    buildShaders();
    buildGeometry();
    setupTextures();
    updateShaderParameters();
    
    // Create overlay root
    m_overlayRoot = new SoSeparator;
    m_overlayRoot->ref();
    
    // Add annotation for post-processing
    SoAnnotation* annotation = new SoAnnotation;
    annotation->addChild(m_program);
    annotation->addChild(m_quadSeparator);
    m_overlayRoot->addChild(annotation);
    
    // Attach to scene
    root->addChild(m_overlayRoot);
    
    LOG_INF("attachOverlay end", "EnhancedOutlinePass");
}

void EnhancedOutlinePass::detachOverlay() {
    if (m_overlayRoot && m_sceneManager) {
        SoSeparator* root = m_sceneManager->getObjectRoot();
        if (root) {
            root->removeChild(m_overlayRoot);
        }
        m_overlayRoot->unref();
        m_overlayRoot = nullptr;
    }
}

void EnhancedOutlinePass::buildShaders() {
    if (m_program) return;
    
    LOG_INF("buildShaders begin", "EnhancedOutlinePass");
    
    // Create shader nodes
    m_program = new SoShaderProgram;
    m_program->ref();
    m_vs = new SoVertexShader;
    m_vs->ref();
    m_fs = new SoFragmentShader;
    m_fs->ref();
    
    // Set shader source
    m_vs->sourceProgram.setValue(kEnhancedVS);
    m_fs->sourceProgram.setValue(kEnhancedFS);
    
    m_program->shaderObject.set1Value(0, m_vs);
    m_program->shaderObject.set1Value(1, m_fs);
    
    LOG_INF("buildShaders end", "EnhancedOutlinePass");
}

void EnhancedOutlinePass::buildGeometry() {
    if (m_quadSeparator) return;
    
    LOG_INF("buildGeometry begin", "EnhancedOutlinePass");
    
    // Create fullscreen quad
    m_quadSeparator = new SoSeparator;
    m_quadSeparator->ref();
    
    // Coordinates
    m_quadCoords = new SoCoordinate3;
    m_quadCoords->ref();
    m_quadCoords->point.set1Value(0, -1.0f, -1.0f, 0.0f);
    m_quadCoords->point.set1Value(1, 1.0f, -1.0f, 0.0f);
    m_quadCoords->point.set1Value(2, 1.0f, 1.0f, 0.0f);
    m_quadCoords->point.set1Value(3, -1.0f, 1.0f, 0.0f);
    m_quadSeparator->addChild(m_quadCoords);
    
    // Texture coordinates
    m_quadTexCoords = new SoTextureCoordinate2;
    m_quadTexCoords->ref();
    m_quadTexCoords->point.set1Value(0, 0.0f, 0.0f);
    m_quadTexCoords->point.set1Value(1, 1.0f, 0.0f);
    m_quadTexCoords->point.set1Value(2, 1.0f, 1.0f);
    m_quadTexCoords->point.set1Value(3, 0.0f, 1.0f);
    m_quadSeparator->addChild(m_quadTexCoords);
    
    // Face set
    m_quadFaces = new SoFaceSet;
    m_quadFaces->ref();
    m_quadFaces->numVertices.set1Value(0, 4);
    m_quadSeparator->addChild(m_quadFaces);
    
    LOG_INF("buildGeometry end", "EnhancedOutlinePass");
}

void EnhancedOutlinePass::setupTextures() {
    LOG_INF("setupTextures begin", "EnhancedOutlinePass");
    
    // Create render-to-texture nodes
    m_colorTexture = new SoSceneTexture2;
    m_colorTexture->ref();
    m_colorTexture->size.setValue(SbVec2s(0, 0)); // auto-size
    m_colorTexture->wrapS = SoSceneTexture2::CLAMP_TO_EDGE;
    m_colorTexture->wrapT = SoSceneTexture2::CLAMP_TO_EDGE;
    m_colorTexture->type = SoSceneTexture2::RGBA8;
    m_colorTexture->scene = m_captureRoot;
    
    m_depthTexture = new SoSceneTexture2;
    m_depthTexture->ref();
    m_depthTexture->size.setValue(SbVec2s(0, 0)); // auto-size
    m_depthTexture->wrapS = SoSceneTexture2::CLAMP_TO_EDGE;
    m_depthTexture->wrapT = SoSceneTexture2::CLAMP_TO_EDGE;
    m_depthTexture->type = SoSceneTexture2::DEPTH;
    m_depthTexture->scene = m_captureRoot;
    
    LOG_INF("setupTextures end", "EnhancedOutlinePass");
}

void EnhancedOutlinePass::updateShaderParameters() {
    if (!m_program) return;
    
    // Create shader parameters if they don't exist
    if (!m_uDepthWeight) {
        m_uDepthWeight = new SoShaderParameter1f;
        m_uDepthWeight->ref();
        m_uDepthWeight->name = "uDepthWeight";
        m_program->parameter.set1Value(0, m_uDepthWeight);
    }
    
    if (!m_uNormalWeight) {
        m_uNormalWeight = new SoShaderParameter1f;
        m_uNormalWeight->ref();
        m_uNormalWeight->name = "uNormalWeight";
        m_program->parameter.set1Value(1, m_uNormalWeight);
    }
    
    if (!m_uColorWeight) {
        m_uColorWeight = new SoShaderParameter1f;
        m_uColorWeight->ref();
        m_uColorWeight->name = "uColorWeight";
        m_program->parameter.set1Value(2, m_uColorWeight);
    }
    
    if (!m_uEdgeIntensity) {
        m_uEdgeIntensity = new SoShaderParameter1f;
        m_uEdgeIntensity->ref();
        m_uEdgeIntensity->name = "uEdgeIntensity";
        m_program->parameter.set1Value(3, m_uEdgeIntensity);
    }
    
    if (!m_uThickness) {
        m_uThickness = new SoShaderParameter1f;
        m_uThickness->ref();
        m_uThickness->name = "uThickness";
        m_program->parameter.set1Value(4, m_uThickness);
    }
    
    if (!m_uOutlineColor) {
        m_uOutlineColor = new SoShaderParameter3f;
        m_uOutlineColor->ref();
        m_uOutlineColor->name = "uOutlineColor";
        m_program->parameter.set1Value(5, m_uOutlineColor);
    }
    
    // Update parameter values
    m_uDepthWeight->value = m_params.depthWeight;
    m_uNormalWeight->value = m_params.normalWeight;
    m_uColorWeight->value = m_params.colorWeight;
    m_uEdgeIntensity->value = m_params.edgeIntensity;
    m_uThickness->value = m_params.thickness;
    m_uOutlineColor->value = m_params.outlineColor;
}

void EnhancedOutlinePass::updateSelectionState() {
    // Update selection state in outline pass
    // This would need to be implemented based on your object ID system
    LOG_INF((std::string("updateSelectionState - ") + std::to_string(m_selectedObjects.size()) + " objects selected").c_str(), "EnhancedOutlinePass");
}

bool EnhancedOutlinePass::chooseTextureUnits() {
    // Choose available texture units
    m_colorUnit = 0;
    m_depthUnit = 1;
    m_normalUnit = 2;
    m_selectionUnit = 3;
    
    LOG_WRN("Using fallback texture units", "EnhancedOutlinePass");
    return false;
}

void EnhancedOutlinePass::cleanupFBO() {
    // Cleanup is handled by Coin3D's reference counting
    // No explicit OpenGL cleanup needed
}