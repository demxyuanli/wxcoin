#include "viewer/EnhancedOutlinePass.h"

#include "SceneManager.h"
#include "Canvas.h"
#include "logger/Logger.h"
#include <Inventor/SoPath.h>

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
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>

#include <GL/gl.h>
#include <string>
#include <algorithm>
#include <cmath>

namespace {
    // Enhanced vertex shader with support for multiple texture coordinates
    static const char* kEnhancedVS = R"GLSL(
        varying vec2 vTexCoord;
        varying vec2 vScreenCoord;
        varying vec3 vWorldPos;
        varying vec3 vNormal;
        
        void main() {
            vTexCoord = gl_MultiTexCoord0.xy;
            vScreenCoord = gl_MultiTexCoord1.xy;
            
            // Transform to world space
            vec4 worldPos = gl_ModelViewMatrix * gl_Vertex;
            vWorldPos = worldPos.xyz;
            
            // Transform normal to world space
            vNormal = normalize(gl_NormalMatrix * gl_Normal);
            
            gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
        }
    )GLSL";

    // Advanced fragment shader with multiple edge detection algorithms
    static const char* kEnhancedFS = R"GLSL(
        varying vec2 vTexCoord;
        varying vec2 vScreenCoord;
        varying vec3 vWorldPos;
        varying vec3 vNormal;
        
        uniform sampler2D uColorTex;
        uniform sampler2D uDepthTex;
        uniform sampler2D uNormalTex;
        uniform sampler2D uSelectionTex;
        
        uniform float uDepthWeight;
        uniform float uNormalWeight;
        uniform float uColorWeight;
        uniform float uDepthThreshold;
        uniform float uNormalThreshold;
        uniform float uColorThreshold;
        uniform float uEdgeIntensity;
        uniform float uThickness;
        uniform float uGlowIntensity;
        uniform float uGlowRadius;
        uniform float uAdaptiveThreshold;
        uniform float uSmoothingFactor;
        uniform float uBackgroundFade;
        uniform vec3 uOutlineColor;
        uniform vec3 uGlowColor;
        uniform vec3 uBackgroundColor;
        uniform vec2 uResolution;
        uniform mat4 uInvProjection;
        uniform mat4 uInvView;
        uniform int uDebugMode;
        uniform int uDownsampleFactor;
        uniform int uEnableEarlyCulling;
        
        // Sample depth with linearization
        float sampleDepth(sampler2D tex, vec2 uv) {
            return texture2D(tex, uv).r;
        }
        
        // Convert depth to linear space
        float linearizeDepth(float depth) {
            float near = 0.1;
            float far = 1000.0;
            return (2.0 * near) / (far + near - depth * (far - near));
        }
        
        // Reconstruct world position from depth
        vec3 getWorldPos(vec2 uv, float depth) {
            vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
            vec4 viewPos = uInvProjection * clipPos;
            viewPos /= viewPos.w;
            vec4 worldPos = uInvView * viewPos;
            return worldPos.xyz;
        }
        
        // Enhanced Roberts Cross edge detection for depth
        float depthEdgeRoberts(vec2 uv, vec2 texelSize) {
            vec2 offset = texelSize * uThickness;
            
            float center = linearizeDepth(sampleDepth(uDepthTex, uv));
            float tl = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(-offset.x, -offset.y)));
            float tr = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(offset.x, -offset.y)));
            float bl = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(-offset.x, offset.y)));
            float br = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(offset.x, offset.y)));
            
            float robertsX = abs(center - br) + abs(tr - bl);
            float robertsY = abs(tl - br) + abs(center - tr);
            
            float edge = sqrt(robertsX * robertsX + robertsY * robertsY);
            
            // Adaptive threshold based on depth and distance
            float adaptiveThreshold = uDepthThreshold;
            if (uAdaptiveThreshold > 0.5) {
                adaptiveThreshold *= (1.0 + center * 10.0);
            }
            
            return smoothstep(0.0, adaptiveThreshold, edge);
        }
        
        // Enhanced Sobel edge detection for normals
        float normalEdgeSobel(vec2 uv, vec2 texelSize) {
            vec2 offset = texelSize * uThickness;
            
            vec3 center = normalize(texture2D(uNormalTex, uv).xyz * 2.0 - 1.0);
            vec3 tl = normalize(texture2D(uNormalTex, uv + vec2(-offset.x, -offset.y)).xyz * 2.0 - 1.0);
            vec3 tm = normalize(texture2D(uNormalTex, uv + vec2(0.0, -offset.y)).xyz * 2.0 - 1.0);
            vec3 tr = normalize(texture2D(uNormalTex, uv + vec2(offset.x, -offset.y)).xyz * 2.0 - 1.0);
            vec3 ml = normalize(texture2D(uNormalTex, uv + vec2(-offset.x, 0.0)).xyz * 2.0 - 1.0);
            vec3 mr = normalize(texture2D(uNormalTex, uv + vec2(offset.x, 0.0)).xyz * 2.0 - 1.0);
            vec3 bl = normalize(texture2D(uNormalTex, uv + vec2(-offset.x, offset.y)).xyz * 2.0 - 1.0);
            vec3 bm = normalize(texture2D(uNormalTex, uv + vec2(0.0, offset.y)).xyz * 2.0 - 1.0);
            vec3 br = normalize(texture2D(uNormalTex, uv + vec2(offset.x, offset.y)).xyz * 2.0 - 1.0);
            
            // Sobel operators for normals
            float gx = dot(tl, center) + 2.0 * dot(ml, center) + dot(bl, center) - 
                      (dot(tr, center) + 2.0 * dot(mr, center) + dot(br, center));
            float gy = dot(bl, center) + 2.0 * dot(bm, center) + dot(br, center) - 
                      (dot(tl, center) + 2.0 * dot(tm, center) + dot(tr, center));
            
            float edge = sqrt(gx * gx + gy * gy);
            return smoothstep(uNormalThreshold, uNormalThreshold * 2.0, edge);
        }
        
        // Enhanced color edge detection with luminance
        float colorEdgeSobel(vec2 uv, vec2 texelSize) {
            vec2 offset = texelSize * uThickness;
            
            vec3 tl = texture2D(uColorTex, uv + vec2(-offset.x, -offset.y)).rgb;
            vec3 tm = texture2D(uColorTex, uv + vec2(0.0, -offset.y)).rgb;
            vec3 tr = texture2D(uColorTex, uv + vec2(offset.x, -offset.y)).rgb;
            vec3 ml = texture2D(uColorTex, uv + vec2(-offset.x, 0.0)).rgb;
            vec3 mr = texture2D(uColorTex, uv + vec2(offset.x, 0.0)).rgb;
            vec3 bl = texture2D(uColorTex, uv + vec2(-offset.x, offset.y)).rgb;
            vec3 bm = texture2D(uColorTex, uv + vec2(0.0, offset.y)).rgb;
            vec3 br = texture2D(uColorTex, uv + vec2(offset.x, offset.y)).rgb;
            
            // Luminance calculation
            float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }
            
            float gx = luma(tr) + 2.0 * luma(mr) + luma(br) - 
                      (luma(tl) + 2.0 * luma(ml) + luma(bl));
            float gy = luma(bl) + 2.0 * luma(bm) + luma(br) - 
                      (luma(tl) + 2.0 * luma(tm) + luma(tr));
            
            float edge = sqrt(gx * gx + gy * gy);
            return smoothstep(uColorThreshold, uColorThreshold * 2.0, edge);
        }
        
        // Gaussian blur for glow effect
        float gaussianBlur(vec2 uv, vec2 texelSize, float radius) {
            float result = 0.0;
            float totalWeight = 0.0;
            
            int samples = int(radius * 2.0);
            for (int x = -samples; x <= samples; x++) {
                for (int y = -samples; y <= samples; y++) {
                    vec2 offset = vec2(float(x), float(y)) * texelSize;
                    float distance = length(offset);
                    float weight = exp(-(distance * distance) / (2.0 * radius * radius));
                    
                    result += texture2D(uColorTex, uv + offset).r * weight;
                    totalWeight += weight;
                }
            }
            
            return result / totalWeight;
        }
        
        void main() {
            vec2 texelSize = uResolution;
            
            // Sample base color
            vec4 color = texture2D(uColorTex, vTexCoord);
            
            // Early culling for background
            float centerDepth = sampleDepth(uDepthTex, vTexCoord);
            if (uEnableEarlyCulling > 0 && centerDepth > uBackgroundFade) {
                gl_FragColor = color;
                return;
            }
            
            // Calculate different types of edges
            float depthEdge = depthEdgeRoberts(vTexCoord, texelSize) * uDepthWeight;
            float normalEdge = normalEdgeSobel(vTexCoord, texelSize) * uNormalWeight;
            float colorEdge = colorEdgeSobel(vTexCoord, texelSize) * uColorWeight;
            
            // Combine edges with smoothing
            float combinedEdge = clamp(depthEdge + normalEdge + colorEdge, 0.0, 1.0);
            
            // Apply smoothing if enabled
            if (uSmoothingFactor > 0.0) {
                float smoothedEdge = combinedEdge;
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (i == 0 && j == 0) continue;
                        vec2 sampleUV = vTexCoord + vec2(float(i), float(j)) * texelSize;
                        float sampleDepthEdge = depthEdgeRoberts(sampleUV, texelSize) * uDepthWeight;
                        float sampleNormalEdge = normalEdgeSobel(sampleUV, texelSize) * uNormalWeight;
                        smoothedEdge += (sampleDepthEdge + sampleNormalEdge) * uSmoothingFactor * 0.125;
                    }
                }
                combinedEdge = mix(combinedEdge, smoothedEdge, uSmoothingFactor);
            }
            
            // Apply intensity
            combinedEdge *= uEdgeIntensity;
            
            // Debug output modes
            if (uDebugMode == 1) {
                gl_FragColor = color;
                return;
            } else if (uDebugMode == 2) {
                gl_FragColor = vec4(vec3(centerDepth), 1.0);
                return;
            } else if (uDebugMode == 3) {
                gl_FragColor = vec4(texture2D(uNormalTex, vTexCoord).rgb, 1.0);
                return;
            } else if (uDebugMode == 4) {
                gl_FragColor = vec4(vec3(depthEdge), 1.0);
                return;
            } else if (uDebugMode == 5) {
                gl_FragColor = vec4(vec3(normalEdge), 1.0);
                return;
            } else if (uDebugMode == 6) {
                gl_FragColor = vec4(vec3(colorEdge), 1.0);
                return;
            } else if (uDebugMode == 7) {
                gl_FragColor = vec4(vec3(combinedEdge), 1.0);
                return;
            }
            
            // Apply glow effect if enabled
            vec3 finalColor = color.rgb;
            if (uGlowIntensity > 0.0 && combinedEdge > 0.1) {
                float glow = gaussianBlur(vTexCoord, texelSize, uGlowRadius);
                finalColor = mix(finalColor, uGlowColor, glow * uGlowIntensity);
            }
            
            // Apply outline
            finalColor = mix(finalColor, uOutlineColor, combinedEdge);
            
            gl_FragColor = vec4(finalColor, color.a);
        }
    )GLSL";
}

EnhancedOutlinePass::EnhancedOutlinePass(SceneManager* sceneManager, SoSeparator* captureRoot)
    : m_sceneManager(sceneManager), m_captureRoot(captureRoot) {
    
    // Initialize with enhanced default parameters
    m_params = EnhancedOutlineParams();
    m_selectionConfig = SelectionOutlineConfig();
    
    LOG_INF("EnhancedOutlinePass constructed", "EnhancedOutlinePass");
}

EnhancedOutlinePass::~EnhancedOutlinePass() {
    LOG_INF("EnhancedOutlinePass destructor begin", "EnhancedOutlinePass");
    
    setEnabled(false);
    cleanupFBO();
    
    // Clean up all nodes
    if (m_program) { m_program->unref(); m_program = nullptr; }
    if (m_vs) { m_vs->unref(); m_vs = nullptr; }
    if (m_fs) { m_fs->unref(); m_fs = nullptr; }
    
    if (m_colorTexture) { m_colorTexture->unref(); m_colorTexture = nullptr; }
    if (m_depthTexture) { m_depthTexture->unref(); m_depthTexture = nullptr; }
    if (m_normalTexture) { m_normalTexture->unref(); m_normalTexture = nullptr; }
    if (m_selectionTexture) { m_selectionTexture->unref(); m_selectionTexture = nullptr; }
    
    if (m_quadSeparator) { m_quadSeparator->unref(); m_quadSeparator = nullptr; }
    if (m_blurQuadSeparator) { m_blurQuadSeparator->unref(); m_blurQuadSeparator = nullptr; }
    
    // Clean up shader parameters
    if (m_uDepthWeight) { m_uDepthWeight->unref(); m_uDepthWeight = nullptr; }
    if (m_uNormalWeight) { m_uNormalWeight->unref(); m_uNormalWeight = nullptr; }
    if (m_uColorWeight) { m_uColorWeight->unref(); m_uColorWeight = nullptr; }
    if (m_uDepthThreshold) { m_uDepthThreshold->unref(); m_uDepthThreshold = nullptr; }
    if (m_uNormalThreshold) { m_uNormalThreshold->unref(); m_uNormalThreshold = nullptr; }
    if (m_uColorThreshold) { m_uColorThreshold->unref(); m_uColorThreshold = nullptr; }
    if (m_uEdgeIntensity) { m_uEdgeIntensity->unref(); m_uEdgeIntensity = nullptr; }
    if (m_uThickness) { m_uThickness->unref(); m_uThickness = nullptr; }
    if (m_uGlowIntensity) { m_uGlowIntensity->unref(); m_uGlowIntensity = nullptr; }
    if (m_uGlowRadius) { m_uGlowRadius->unref(); m_uGlowRadius = nullptr; }
    if (m_uAdaptiveThreshold) { m_uAdaptiveThreshold->unref(); m_uAdaptiveThreshold = nullptr; }
    if (m_uSmoothingFactor) { m_uSmoothingFactor->unref(); m_uSmoothingFactor = nullptr; }
    if (m_uBackgroundFade) { m_uBackgroundFade->unref(); m_uBackgroundFade = nullptr; }
    if (m_uOutlineColor) { m_uOutlineColor->unref(); m_uOutlineColor = nullptr; }
    if (m_uGlowColor) { m_uGlowColor->unref(); m_uGlowColor = nullptr; }
    if (m_uBackgroundColor) { m_uBackgroundColor->unref(); m_uBackgroundColor = nullptr; }
    if (m_uResolution) { m_uResolution->unref(); m_uResolution = nullptr; }
    if (m_uInvProjection) { m_uInvProjection->unref(); m_uInvProjection = nullptr; }
    if (m_uInvView) { m_uInvView->unref(); m_uInvView = nullptr; }
    if (m_uDebugMode) { m_uDebugMode->unref(); m_uDebugMode = nullptr; }
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
    
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        m_sceneManager->getCanvas()->Refresh(false);
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
    updateSelectionState();
    refresh();
}

void EnhancedOutlinePass::setSelectionRoot(SoSelection* selectionRoot) {
    m_selectionRoot = selectionRoot;
    updateSelectionState();
}

void EnhancedOutlinePass::updateSelectionState() {
    if (!m_selectionRoot) return;
    
    m_selectedObjects.clear();
    
    // Get selected objects from SoSelection
    for (int i = 0; i < m_selectionRoot->getNumSelected(); i++) {
        SoPath* path = m_selectionRoot->getPath(i);
        if (path) {
            // Extract object ID from path (implementation depends on your object ID system)
            int objectId = extractObjectIdFromPath(path);
            if (objectId >= 0) {
                m_selectedObjects.push_back(objectId);
            }
        }
    }
    
    LOG_INF((std::string("updateSelectionState - ") + std::to_string(m_selectedObjects.size()) + " objects selected").c_str(), "EnhancedOutlinePass");
}

int EnhancedOutlinePass::extractObjectIdFromPath(SoPath* path) {
    // This is a placeholder implementation
    // You need to implement this based on your object ID system
    if (!path) return -1;
    
    // For now, return a simple hash of the path length
    return path->getLength() % 1000;
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
    if (m_uDebugMode) {
        m_uDebugMode->value = static_cast<int>(mode);
    }
    refresh();
}

void EnhancedOutlinePass::setDownsampleFactor(int factor) {
    m_downsampleFactor = std::max(1, std::min(4, factor));
    if (m_uDownsampleFactor) {
        m_uDownsampleFactor->value = m_downsampleFactor;
    }
    updateTextureSizes();
    refresh();
}

void EnhancedOutlinePass::setMultiSampleEnabled(bool enabled) {
    m_multiSampleEnabled = enabled;
    refresh();
}

void EnhancedOutlinePass::setEarlyCullingEnabled(bool enabled) {
    m_earlyCullingEnabled = enabled;
    if (m_uEnableEarlyCulling) {
        m_uEnableEarlyCulling->value = enabled ? 1 : 0;
    }
    refresh();
}

void EnhancedOutlinePass::refresh() {
    updateShaderParameters();
    updateCameraMatrices();
    updateTextureSizes();
    
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        m_sceneManager->getCanvas()->Refresh(false);
    }
}

void EnhancedOutlinePass::forceUpdate() {
    m_needsUpdate = true;
    refresh();
}

void EnhancedOutlinePass::setCustomOutlineCallback(OutlineCallback callback) {
    m_customCallback = callback;
    refresh();
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
    
    // Create overlay structure
    m_overlayRoot = new SoSeparator;
    m_overlayRoot->ref();
    m_annotation = new SoAnnotation;
    m_overlayRoot->addChild(m_annotation);
    
    // Build shader resources
    buildShaders();
    buildGeometry();
    setupTextures();
    
    // Add camera-facing transform
    auto* transform = new SoTransform;
    m_annotation->addChild(transform);
    
    // Bind textures and set parameters
    if (m_colorTexture) {
        auto* texUnit = new SoTextureUnit;
        texUnit->unit = m_colorUnit;
        m_annotation->addChild(texUnit);
        m_annotation->addChild(m_colorTexture);
        
        auto* colorBind = new SoShaderParameter1i;
        colorBind->name = "uColorTex";
        colorBind->value = m_colorUnit;
        m_annotation->addChild(colorBind);
    }
    
    if (m_depthTexture) {
        auto* texUnit = new SoTextureUnit;
        texUnit->unit = m_depthUnit;
        m_annotation->addChild(texUnit);
        m_annotation->addChild(m_depthTexture);
        
        auto* depthBind = new SoShaderParameter1i;
        depthBind->name = "uDepthTex";
        depthBind->value = m_depthUnit;
        m_annotation->addChild(depthBind);
    }
    
    if (m_normalTexture) {
        auto* texUnit = new SoTextureUnit;
        texUnit->unit = m_normalUnit;
        m_annotation->addChild(texUnit);
        m_annotation->addChild(m_normalTexture);
        
        auto* normalBind = new SoShaderParameter1i;
        normalBind->name = "uNormalTex";
        normalBind->value = m_normalUnit;
        m_annotation->addChild(normalBind);
    }
    
    if (m_selectionTexture) {
        auto* texUnit = new SoTextureUnit;
        texUnit->unit = m_selectionUnit;
        m_annotation->addChild(texUnit);
        m_annotation->addChild(m_selectionTexture);
        
        auto* selectionBind = new SoShaderParameter1i;
        selectionBind->name = "uSelectionTex";
        selectionBind->value = m_selectionUnit;
        m_annotation->addChild(selectionBind);
    }
    
    // Add shader parameters
    updateShaderParameters();
    
    // Add shader program
    if (m_program) {
        m_annotation->addChild(m_program);
    }
    
    // Add geometry
    if (m_quadSeparator) {
        m_annotation->addChild(m_quadSeparator);
    }
    
    // Add to scene
    root->addChild(m_overlayRoot);
    
    logInfo("attachOverlay end");
}

void EnhancedOutlinePass::detachOverlay() {
    if (!m_sceneManager || !m_overlayRoot) return;
    
    logInfo("detachOverlay begin");
    
    SoSeparator* root = m_sceneManager->getObjectRoot();
    if (root) {
        int idx = root->findChild(m_overlayRoot);
        if (idx >= 0) {
            root->removeChild(idx);
        }
    }
    
    // Clear scene references
    if (m_colorTexture) m_colorTexture->scene = nullptr;
    if (m_depthTexture) m_depthTexture->scene = nullptr;
    if (m_normalTexture) m_normalTexture->scene = nullptr;
    if (m_selectionTexture) m_selectionTexture->scene = nullptr;
    
    // Clean up temporary scene root
    if (m_tempSceneRoot) {
        m_tempSceneRoot->unref();
        m_tempSceneRoot = nullptr;
    }
    
    // Clean up overlay root
    m_overlayRoot->unref();
    m_overlayRoot = nullptr;
    m_annotation = nullptr;
    
    logInfo("detachOverlay end");
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
    
    // Set shader sources
    m_vs->sourceType = SoShaderObject::GLSL_PROGRAM;
    m_vs->sourceProgram.setValue(kEnhancedVS);
    m_fs->sourceType = SoShaderObject::GLSL_PROGRAM;
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
    
    // Disable lighting
    auto* lightModel = new SoLightModel;
    lightModel->model = SoLightModel::BASE_COLOR;
    m_quadSeparator->addChild(lightModel);
    
    // Set material
    auto* material = new SoMaterial;
    material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    material->transparency = 0.0f;
    m_quadSeparator->addChild(material);
    
    // Texture coordinates
    auto* texCoords = new SoTextureCoordinate2;
    SbVec2f uvs[] = {
        SbVec2f(0.0f, 0.0f),
        SbVec2f(1.0f, 0.0f),
        SbVec2f(1.0f, 1.0f),
        SbVec2f(0.0f, 1.0f)
    };
    texCoords->point.setValues(0, 4, uvs);
    m_quadSeparator->addChild(texCoords);
    
    // Additional texture coordinates for screen space
    auto* screenTexCoords = new SoTextureCoordinate2;
    SbVec2f screenUVs[] = {
        SbVec2f(0.0f, 0.0f),
        SbVec2f(1.0f, 0.0f),
        SbVec2f(1.0f, 1.0f),
        SbVec2f(0.0f, 1.0f)
    };
    screenTexCoords->point.setValues(0, 4, screenUVs);
    m_quadSeparator->addChild(screenTexCoords);
    
    // Vertex coordinates
    auto* coords = new SoCoordinate3;
    SbVec3f vertices[] = {
        SbVec3f(-1.0f, -1.0f, 0.0f),
        SbVec3f(1.0f, -1.0f, 0.0f),
        SbVec3f(1.0f,  1.0f, 0.0f),
        SbVec3f(-1.0f,  1.0f, 0.0f)
    };
    coords->point.setValues(0, 4, vertices);
    m_quadSeparator->addChild(coords);
    
    // Face set
    auto* face = new SoFaceSet;
    face->numVertices.set1Value(0, 4);
    m_quadSeparator->addChild(face);
    
    LOG_INF("buildGeometry end", "EnhancedOutlinePass");
}

void EnhancedOutlinePass::setupTextures() {
    LOG_INF("setupTextures begin", "EnhancedOutlinePass");
    
    // Create render-to-texture nodes
    m_colorTexture = new SoSceneTexture2;
    m_colorTexture->ref();
    m_colorTexture->size.setValue(SbVec2s(0, 0)); // auto-size
    m_colorTexture->transparencyFunction = SoSceneTexture2::NONE;
    m_colorTexture->type = SoSceneTexture2::RGBA8;
    m_colorTexture->wrapS = SoSceneTexture2::CLAMP;
    m_colorTexture->wrapT = SoSceneTexture2::CLAMP;
    
    m_depthTexture = new SoSceneTexture2;
    m_depthTexture->ref();
    m_depthTexture->size.setValue(SbVec2s(0, 0)); // auto-size
    m_depthTexture->transparencyFunction = SoSceneTexture2::NONE;
    m_depthTexture->type = SoSceneTexture2::DEPTH;
    m_depthTexture->wrapS = SoSceneTexture2::CLAMP;
    m_depthTexture->wrapT = SoSceneTexture2::CLAMP;
    
    m_normalTexture = new SoSceneTexture2;
    m_normalTexture->ref();
    m_normalTexture->size.setValue(SbVec2s(0, 0)); // auto-size
    m_normalTexture->transparencyFunction = SoSceneTexture2::NONE;
    m_normalTexture->type = SoSceneTexture2::RGBA8;
    m_normalTexture->wrapS = SoSceneTexture2::CLAMP;
    m_normalTexture->wrapT = SoSceneTexture2::CLAMP;
    
    m_selectionTexture = new SoSceneTexture2;
    m_selectionTexture->ref();
    m_selectionTexture->size.setValue(SbVec2s(0, 0)); // auto-size
    m_selectionTexture->transparencyFunction = SoSceneTexture2::NONE;
    m_selectionTexture->type = SoSceneTexture2::RGBA8;
    m_selectionTexture->wrapS = SoSceneTexture2::CLAMP;
    m_selectionTexture->wrapT = SoSceneTexture2::CLAMP;
    
    // Create temporary scene root for RTT
    if (m_sceneManager && m_captureRoot) {
        SoSeparator* tempSceneRoot = new SoSeparator;
        tempSceneRoot->ref();
        
        // Add camera
        SoCamera* camera = m_sceneManager->getCamera();
        if (camera) {
            tempSceneRoot->addChild(camera);
        }
        
        // Add capture root
        tempSceneRoot->addChild(m_captureRoot);
        
        // Set scenes for RTT
        m_colorTexture->scene = tempSceneRoot;
        m_depthTexture->scene = tempSceneRoot;
        m_normalTexture->scene = tempSceneRoot;
        m_selectionTexture->scene = tempSceneRoot;
        
        m_tempSceneRoot = tempSceneRoot;
    }
    
    LOG_INF("setupTextures end", "EnhancedOutlinePass");
}

void EnhancedOutlinePass::updateShaderParameters() {
    if (!m_program) return;
    
    // Create shader parameters if they don't exist
    if (!m_uDepthWeight) {
        m_uDepthWeight = new SoShaderParameter1f;
        m_uDepthWeight->ref();
        m_uDepthWeight->name = "uDepthWeight";
    }
    m_uDepthWeight->value = m_params.depthWeight;
    
    if (!m_uNormalWeight) {
        m_uNormalWeight = new SoShaderParameter1f;
        m_uNormalWeight->ref();
        m_uNormalWeight->name = "uNormalWeight";
    }
    m_uNormalWeight->value = m_params.normalWeight;
    
    if (!m_uColorWeight) {
        m_uColorWeight = new SoShaderParameter1f;
        m_uColorWeight->ref();
        m_uColorWeight->name = "uColorWeight";
    }
    m_uColorWeight->value = m_params.colorWeight;
    
    if (!m_uDepthThreshold) {
        m_uDepthThreshold = new SoShaderParameter1f;
        m_uDepthThreshold->ref();
        m_uDepthThreshold->name = "uDepthThreshold";
    }
    m_uDepthThreshold->value = m_params.depthThreshold;
    
    if (!m_uNormalThreshold) {
        m_uNormalThreshold = new SoShaderParameter1f;
        m_uNormalThreshold->ref();
        m_uNormalThreshold->name = "uNormalThreshold";
    }
    m_uNormalThreshold->value = m_params.normalThreshold;
    
    if (!m_uColorThreshold) {
        m_uColorThreshold = new SoShaderParameter1f;
        m_uColorThreshold->ref();
        m_uColorThreshold->name = "uColorThreshold";
    }
    m_uColorThreshold->value = m_params.colorThreshold;
    
    if (!m_uEdgeIntensity) {
        m_uEdgeIntensity = new SoShaderParameter1f;
        m_uEdgeIntensity->ref();
        m_uEdgeIntensity->name = "uEdgeIntensity";
    }
    m_uEdgeIntensity->value = m_params.edgeIntensity;
    
    if (!m_uThickness) {
        m_uThickness = new SoShaderParameter1f;
        m_uThickness->ref();
        m_uThickness->name = "uThickness";
    }
    m_uThickness->value = m_params.thickness;
    
    if (!m_uGlowIntensity) {
        m_uGlowIntensity = new SoShaderParameter1f;
        m_uGlowIntensity->ref();
        m_uGlowIntensity->name = "uGlowIntensity";
    }
    m_uGlowIntensity->value = m_params.glowIntensity;
    
    if (!m_uGlowRadius) {
        m_uGlowRadius = new SoShaderParameter1f;
        m_uGlowRadius->ref();
        m_uGlowRadius->name = "uGlowRadius";
    }
    m_uGlowRadius->value = m_params.glowRadius;
    
    if (!m_uAdaptiveThreshold) {
        m_uAdaptiveThreshold = new SoShaderParameter1f;
        m_uAdaptiveThreshold->ref();
        m_uAdaptiveThreshold->name = "uAdaptiveThreshold";
    }
    m_uAdaptiveThreshold->value = m_params.adaptiveThreshold;
    
    if (!m_uSmoothingFactor) {
        m_uSmoothingFactor = new SoShaderParameter1f;
        m_uSmoothingFactor->ref();
        m_uSmoothingFactor->name = "uSmoothingFactor";
    }
    m_uSmoothingFactor->value = m_params.smoothingFactor;
    
    if (!m_uBackgroundFade) {
        m_uBackgroundFade = new SoShaderParameter1f;
        m_uBackgroundFade->ref();
        m_uBackgroundFade->name = "uBackgroundFade";
    }
    m_uBackgroundFade->value = m_params.backgroundFade;
    
    if (!m_uOutlineColor) {
        m_uOutlineColor = new SoShaderParameter3f;
        m_uOutlineColor->ref();
        m_uOutlineColor->name = "uOutlineColor";
    }
    m_uOutlineColor->value = SbVec3f(m_params.outlineColor[0], m_params.outlineColor[1], m_params.outlineColor[2]);
    
    if (!m_uGlowColor) {
        m_uGlowColor = new SoShaderParameter3f;
        m_uGlowColor->ref();
        m_uGlowColor->name = "uGlowColor";
    }
    m_uGlowColor->value = SbVec3f(m_params.glowColor[0], m_params.glowColor[1], m_params.glowColor[2]);
    
    if (!m_uBackgroundColor) {
        m_uBackgroundColor = new SoShaderParameter3f;
        m_uBackgroundColor->ref();
        m_uBackgroundColor->name = "uBackgroundColor";
    }
    m_uBackgroundColor->value = SbVec3f(m_params.backgroundColor[0], m_params.backgroundColor[1], m_params.backgroundColor[2]);
    
    if (!m_uResolution) {
        m_uResolution = new SoShaderParameter2f;
        m_uResolution->ref();
        m_uResolution->name = "uResolution";
    }
    
    if (!m_uInvProjection) {
        m_uInvProjection = new SoShaderParameterMatrix;
        m_uInvProjection->ref();
        m_uInvProjection->name = "uInvProjection";
    }
    
    if (!m_uInvView) {
        m_uInvView = new SoShaderParameterMatrix;
        m_uInvView->ref();
        m_uInvView->name = "uInvView";
    }
    
    if (!m_uDebugMode) {
        m_uDebugMode = new SoShaderParameter1i;
        m_uDebugMode->ref();
        m_uDebugMode->name = "uDebugMode";
    }
    m_uDebugMode->value = static_cast<int>(m_debugMode);
    
    if (!m_uDownsampleFactor) {
        m_uDownsampleFactor = new SoShaderParameter1i;
        m_uDownsampleFactor->ref();
        m_uDownsampleFactor->name = "uDownsampleFactor";
    }
    m_uDownsampleFactor->value = m_downsampleFactor;
    
    if (!m_uEnableEarlyCulling) {
        m_uEnableEarlyCulling = new SoShaderParameter1i;
        m_uEnableEarlyCulling->ref();
        m_uEnableEarlyCulling->name = "uEnableEarlyCulling";
    }
    m_uEnableEarlyCulling->value = m_earlyCullingEnabled ? 1 : 0;
    
    // Update resolution
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        int width, height;
        m_sceneManager->getCanvas()->GetSize(&width, &height);
        if (width > 0 && height > 0) {
            m_uResolution->value = SbVec2f(1.0f / float(width), 1.0f / float(height));
        }
    }
}

void EnhancedOutlinePass::updateCameraMatrices() {
    if (!m_sceneManager) return;
    
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) return;
    
    // Get viewport dimensions
    SbVec2s vpSize(1920, 1080);
    if (m_sceneManager->getCanvas()) {
        int width, height;
        m_sceneManager->getCanvas()->GetSize(&width, &height);
        if (width > 0 && height > 0) {
            vpSize = SbVec2s(width, height);
        }
    }
    
    // Get projection matrix and invert it
    SbViewVolume viewVol = camera->getViewVolume(float(vpSize[0]) / float(vpSize[1]));
    SbMatrix projMatrix = viewVol.getMatrix();
    SbMatrix invProjMatrix = projMatrix.inverse();
    
    // Get view matrix and invert it
    SbMatrix viewMatrix;
    viewMatrix.setTranslate(-camera->position.getValue());
    SbMatrix rotMatrix;
    camera->orientation.getValue().getValue(rotMatrix);
    viewMatrix.multRight(rotMatrix);
    SbMatrix invViewMatrix = viewMatrix.inverse();
    
    // Update shader parameters
    if (m_uInvProjection) {
        m_uInvProjection->value.setValue(invProjMatrix);
    }
    if (m_uInvView) {
        m_uInvView->value.setValue(invViewMatrix);
    }
}

void EnhancedOutlinePass::updateTextureSizes() {
    if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
    
    int width, height;
    m_sceneManager->getCanvas()->GetSize(&width, &height);
    
    if (width > 0 && height > 0) {
        // Apply downsampling
        width /= m_downsampleFactor;
        height /= m_downsampleFactor;
        
        SbVec2s size(width, height);
        
        if (m_colorTexture) m_colorTexture->size.setValue(size);
        if (m_depthTexture) m_depthTexture->size.setValue(size);
        if (m_normalTexture) m_normalTexture->size.setValue(size);
        if (m_selectionTexture) m_selectionTexture->size.setValue(size);
    }
}

bool EnhancedOutlinePass::chooseTextureUnits() {
    GLint maxUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
    
    if (maxUnits >= 4) {
        m_colorUnit = maxUnits - 1;
        m_depthUnit = maxUnits - 2;
        m_normalUnit = maxUnits - 3;
        m_selectionUnit = maxUnits - 4;
        return true;
    }
    
    // Fallback
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