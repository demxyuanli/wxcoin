#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/ImageOutlinePass.h"

#include "SceneManager.h"
#include "Canvas.h"
#include "logger/Logger.h"
#include <string>

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
#ifdef IMAGE_OUTLINE_ENABLE_GL_VALIDATION
#include <GL/gl.h>
#endif

namespace {
#ifdef IMAGE_OUTLINE_ENABLE_GL_VALIDATION
	static void debugOutput(const char* msg) {
#ifdef _WIN32
		OutputDebugStringA(msg);
		OutputDebugStringA("\n");
#else
		(void)msg;
#endif
	}

	static GLuint compileShaderGL(GLenum type, const char* src, std::string& log) {
		GLuint sh = glCreateShader(type);
		glShaderSource(sh, 1, &src, nullptr);
		glCompileShader(sh);
		GLint ok = GL_FALSE;
		glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
		GLint len = 0;
		glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
		if (len > 1) { log.resize(static_cast<size_t>(len)); glGetShaderInfoLog(sh, len, nullptr, log.data()); }
		if (!ok) { glDeleteShader(sh); return 0; }
		return sh;
	}

	static bool validateProgramGL(const char* vs, const char* fs) {
		std::string log;
		GLuint v = compileShaderGL(GL_VERTEX_SHADER, vs, log);
		if (!v) { debugOutput(log.c_str()); return false; }
		GLuint f = compileShaderGL(GL_FRAGMENT_SHADER, fs, log);
		if (!f) { debugOutput(log.c_str()); glDeleteShader(v); return false; }
		GLuint p = glCreateProgram();
		glAttachShader(p, v);
		glAttachShader(p, f);
		glLinkProgram(p);
		GLint ok = GL_FALSE; glGetProgramiv(p, GL_LINK_STATUS, &ok);
		GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
		if (len > 1) { log.resize(static_cast<size_t>(len)); glGetProgramInfoLog(p, len, nullptr, log.data()); debugOutput(log.c_str()); }
		glDeleteShader(v); glDeleteShader(f); glDeleteProgram(p);
		return ok == GL_TRUE;
	}
#endif // IMAGE_OUTLINE_ENABLE_GL_VALIDATION
}

ImageOutlinePass::ImageOutlinePass(SceneManager* sceneManager, SoSeparator* captureRoot)
	: m_sceneManager(sceneManager), m_captureRoot(captureRoot) {
	// Initialize parameters with sensible defaults
	    m_params.edgeIntensity = 1.0f;     // Full intensity (0.0 - 1.0)
    m_params.depthWeight = 1.5f;       // Weight for depth edges (0.0 - 2.0)
    m_params.normalWeight = 1.0f;      // Weight for normal edges (0.0 - 2.0)
    m_params.depthThreshold = 0.001f;  // Depth discontinuity threshold
    m_params.normalThreshold = 0.4f;   // Normal angle threshold (adjusted for better detection)
    m_params.thickness = 1.5f;         // Edge thickness multiplier (0.5 - 3.0)
	LOG_INF("constructed", "ImageOutlinePass");
}

ImageOutlinePass::~ImageOutlinePass() {
	LOG_INF("destructor begin", "ImageOutlinePass");
	setEnabled(false);

	// Clean up shader nodes
	if (m_program) { m_program->unref(); m_program = nullptr; }
	if (m_vs) { m_vs->unref(); m_vs = nullptr; }
	if (m_fs) { m_fs->unref(); m_fs = nullptr; }

	// Clean up texture nodes
	if (m_colorTexture) { m_colorTexture->unref(); m_colorTexture = nullptr; }
	if (m_depthTexture) { m_depthTexture->unref(); m_depthTexture = nullptr; }
	if (m_colorSampler) { m_colorSampler->unref(); m_colorSampler = nullptr; }
	if (m_depthSampler) { m_depthSampler->unref(); m_depthSampler = nullptr; }

	// Clean up geometry
	if (m_quadSeparator) { m_quadSeparator->unref(); m_quadSeparator = nullptr; }

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
	
	// Clean up temporary scene root
	if (m_tempSceneRoot) { m_tempSceneRoot->unref(); m_tempSceneRoot = nullptr; }
	
	LOG_INF("destructor end", "ImageOutlinePass");
}

bool ImageOutlinePass::chooseTextureUnits() {
	GLint maxUnits = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
	LOG_DBG((std::string("GL_MAX_TEXTURE_IMAGE_UNITS=") + std::to_string(maxUnits)).c_str(), "ImageOutlinePass");
	if (maxUnits >= 2) {
		m_colorUnit = maxUnits - 1;
		m_depthUnit = maxUnits - 2;
		if (m_depthUnit < 0) { m_colorUnit = 0; m_depthUnit = 1; }
		LOG_DBG((std::string("selected units color=") + std::to_string(m_colorUnit) + ", depth=" + std::to_string(m_depthUnit)).c_str(), "ImageOutlinePass");
		return true;
	}
	m_colorUnit = 0;
	m_depthUnit = 1;
	LOG_WRN("fallback units color=0 depth=1", "ImageOutlinePass");
	return false;
}

void ImageOutlinePass::setEnabled(bool enabled) {
	if (m_enabled == enabled) return;
	m_enabled = enabled;
	LOG_INF((std::string("setEnabled ") + (enabled ? "true" : "false")).c_str(), "ImageOutlinePass");
	if (m_enabled) attachOverlay(); else detachOverlay();
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh(false);
}

void ImageOutlinePass::setParams(const ImageOutlineParams& p) {
	m_params = p;
	LOG_DBG((std::string("setParams depthWeight=") + std::to_string(p.depthWeight)
		+ ", normalWeight=" + std::to_string(p.normalWeight)
		+ ", depthThreshold=" + std::to_string(p.depthThreshold)
		+ ", normalThreshold=" + std::to_string(p.normalThreshold)
		+ ", edgeIntensity=" + std::to_string(p.edgeIntensity)
		+ ", thickness=" + std::to_string(p.thickness)).c_str(), "ImageOutlinePass");
	refresh();
}

void ImageOutlinePass::refresh() {
	// Update shader parameters if they exist
	if (m_uIntensity) m_uIntensity->value = m_params.edgeIntensity;
	if (m_uDepthWeight) m_uDepthWeight->value = m_params.depthWeight;
	if (m_uNormalWeight) m_uNormalWeight->value = m_params.normalWeight;
	if (m_uDepthThreshold) m_uDepthThreshold->value = m_params.depthThreshold;
	if (m_uNormalThreshold) m_uNormalThreshold->value = m_params.normalThreshold;
	if (m_uThickness) m_uThickness->value = m_params.thickness;

	// Update resolution if viewport has changed
	if (m_uResolution && m_sceneManager && m_sceneManager->getCanvas()) {
		int width, height;
		m_sceneManager->getCanvas()->GetSize(&width, &height);
		if (width > 0 && height > 0) {
			m_uResolution->value = SbVec2f(1.0f / float(width), 1.0f / float(height));
			LOG_DBG((std::string("resolution set from viewport ") + std::to_string(width) + "x" + std::to_string(height)).c_str(), "ImageOutlinePass");
		}
		m_sceneManager->getCanvas()->Refresh(false);
	}

	// Update camera matrices
	updateCameraMatrices();
}

void ImageOutlinePass::setDebugOutput(DebugOutput mode) {
	m_debugOutput = mode;
	if (m_uDebugOutput) m_uDebugOutput->value = static_cast<int>(mode);
	LOG_INF((std::string("setDebugOutput ") + std::to_string(static_cast<int>(mode))).c_str(), "ImageOutlinePass");
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh(false);
}

void ImageOutlinePass::attachOverlay() {
	if (!m_sceneManager) return;
	if (m_overlayRoot) return; // Already attached
	LOG_INF("attachOverlay begin", "ImageOutlinePass");

	SoSeparator* root = m_sceneManager->getObjectRoot();
	if (!root) {
		// Log error or handle gracefully
		return;
	}

	// Select texture units (avoid conflicts with rest of app)
	chooseTextureUnits();
	LOG_DBG("texture units chosen", "ImageOutlinePass");

	m_overlayRoot = new SoSeparator;
	m_overlayRoot->ref();
	m_annotation = new SoAnnotation;
	m_overlayRoot->addChild(m_annotation);
	
	// Add a camera-facing transform for the quad
	auto* transform = new SoTransform;
	m_annotation->addChild(transform);

	// Build shader resources
	buildShaders();
	LOG_DBG("buildShaders done", "ImageOutlinePass");

	// Construct the render graph in proper order:

	// 1. Bind color texture to its unit, then render scene into it
	if (m_colorTexture) {
		auto* texUnit0 = new SoTextureUnit;
		texUnit0->unit = m_colorUnit;
		m_annotation->addChild(texUnit0);
		m_annotation->addChild(m_colorTexture);
		auto* colorBind = new SoShaderParameter1i;
		colorBind->name = "uColorTex";
		colorBind->value = m_colorUnit;
		m_annotation->addChild(colorBind);
		LOG_DBG("color texture bound", "ImageOutlinePass");
	}

	// 2. Bind depth texture to its unit, then render scene into it
	if (m_depthTexture) {
		auto* texUnit1 = new SoTextureUnit;
		texUnit1->unit = m_depthUnit;
		m_annotation->addChild(texUnit1);
		m_annotation->addChild(m_depthTexture);
		auto* depthBind = new SoShaderParameter1i;
		depthBind->name = "uDepthTex";
		depthBind->value = m_depthUnit;
		m_annotation->addChild(depthBind);
		LOG_DBG("depth texture bound", "ImageOutlinePass");
	}

	// 3. Add shader parameters
	if (m_uIntensity) m_annotation->addChild(m_uIntensity);
	if (m_uDepthWeight) m_annotation->addChild(m_uDepthWeight);
	if (m_uNormalWeight) m_annotation->addChild(m_uNormalWeight);
	if (m_uDepthThreshold) m_annotation->addChild(m_uDepthThreshold);
	if (m_uNormalThreshold) m_annotation->addChild(m_uNormalThreshold);
	if (m_uThickness) m_annotation->addChild(m_uThickness);
	if (m_uResolution) {
		// Update resolution based on actual viewport size
		if (m_sceneManager && m_sceneManager->getCanvas()) {
			int width, height;
			m_sceneManager->getCanvas()->GetSize(&width, &height);
			if (width > 0 && height > 0) {
				m_uResolution->value = SbVec2f(1.0f / float(width), 1.0f / float(height));
			}
		}
		m_annotation->addChild(m_uResolution);
	}
	    // Update camera matrices
    updateCameraMatrices();
    
    // Optional matrices (if available)
    if (m_uInvProjection) m_annotation->addChild(m_uInvProjection);
    if (m_uInvView) m_annotation->addChild(m_uInvView);
    if (m_uDebugOutput) m_annotation->addChild(m_uDebugOutput);

	// 4. Apply shader program
	if (m_program) { m_annotation->addChild(m_program); LOG_DBG("shader program applied", "ImageOutlinePass"); }

	// 5. Render fullscreen quad
	if (m_quadSeparator) { m_annotation->addChild(m_quadSeparator); LOG_DBG("fullscreen quad added", "ImageOutlinePass"); }

	// Add to scene
	root->addChild(m_overlayRoot);
	LOG_INF("attachOverlay end", "ImageOutlinePass");
}

void ImageOutlinePass::detachOverlay() {
	if (!m_sceneManager) return;
	if (!m_overlayRoot) return;
	LOG_INF("detachOverlay begin", "ImageOutlinePass");

	SoSeparator* root = m_sceneManager->getObjectRoot();
	if (root) {
		int idx = root->findChild(m_overlayRoot);
		if (idx >= 0) {
			root->removeChild(idx);
		}
	}

	// Clear scene references to avoid circular dependencies
	if (m_colorTexture) {
		m_colorTexture->scene = nullptr;
	}
	if (m_depthTexture) {
		m_depthTexture->scene = nullptr;
	}
	
	// Clean up temporary scene root
	if (m_tempSceneRoot) {
		m_tempSceneRoot->unref();
		m_tempSceneRoot = nullptr;
	}

	// Properly clean up the overlay root
	m_overlayRoot->unref();
	m_overlayRoot = nullptr;
	m_annotation = nullptr; // Owned by m_overlayRoot, no need to unref
	LOG_INF("detachOverlay end", "ImageOutlinePass");
}

void ImageOutlinePass::buildShaders() {
	if (m_program) return;
	LOG_INF("buildShaders begin", "ImageOutlinePass");

	// Create shader nodes and increase reference count
	m_program = new SoShaderProgram;
	m_program->ref();
	m_vs = new SoVertexShader;
	m_vs->ref();
	m_fs = new SoFragmentShader;
	m_fs->ref();

	// Pass-through vertex shader that preserves texture coordinates
	static const char* kVS = R"GLSL(
        varying vec2 vTexCoord;
        void main() {
            vTexCoord = gl_MultiTexCoord0.xy;
            gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
        }
    )GLSL";

	    // Advanced edge detection shader with Three.js-style outline
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
        uniform vec2 uResolution; // Inverse of viewport size (1/width, 1/height)
        uniform mat4 uInvProjection;
        uniform mat4 uInvView;
        uniform int uDebugOutput;    // 0=final, 1=color, 2=edge

        // Sample depth with offset
        float sampleDepth(sampler2D tex, vec2 uv) {
            return texture2D(tex, uv).r;
        }
        
        // Linear depth conversion
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
        
        // Reconstruct normal from depth
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

        // Color luminance-based Sobel
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

        // Roberts Cross edge detection on depth (more suitable for outlines)
        float depthEdge(vec2 uv, vec2 texelSize) {
            vec2 offset = texelSize * uThickness;
            
            float center = linearizeDepth(sampleDepth(uDepthTex, uv));
            float tl = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(-offset.x, -offset.y)));
            float tr = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(offset.x, -offset.y)));
            float bl = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(-offset.x, offset.y)));
            float br = linearizeDepth(sampleDepth(uDepthTex, uv + vec2(offset.x, offset.y)));
            
            // Roberts Cross operators
            float robertsX = abs(center - br) + abs(tr - bl);
            float robertsY = abs(tl - br) + abs(center - tr);
            
            float edge = sqrt(robertsX * robertsX + robertsY * robertsY);
            
            // Adaptive threshold based on depth
            float adaptiveThreshold = uDepthThreshold * (1.0 + center * 10.0);
            return smoothstep(0.0, adaptiveThreshold, edge);
        }

        // Normal-based edge detection
        float normalEdge(vec2 uv, vec2 texelSize) {
            vec3 normal = getNormalFromDepth(uv, texelSize);
            
            vec2 offset = texelSize * uThickness;
            vec3 normalRight = getNormalFromDepth(uv + vec2(offset.x, 0.0), texelSize);
            vec3 normalUp = getNormalFromDepth(uv + vec2(0.0, offset.y), texelSize);
            
            float dotRight = dot(normal, normalRight);
            float dotUp = dot(normal, normalUp);
            
            float edge = max(0.0, 1.0 - min(dotRight, dotUp));
            return smoothstep(uNormalThreshold, uNormalThreshold * 2.0, edge);
        }

        void main() {
            vec2 texelSize = uResolution;

            // Sample color as base
            vec4 color = texture2D(uColorTex, vTexCoord);
            
            // Skip processing for background (far depth)
            float centerDepth = sampleDepth(uDepthTex, vTexCoord);
            if (centerDepth > 0.999) {
                gl_FragColor = color;
                return;
            }

            // Calculate edges
            float depthE = depthEdge(vTexCoord, texelSize) * uDepthWeight;
            float normalE = normalEdge(vTexCoord, texelSize) * uNormalWeight;
            float colorE = colorSobel(vTexCoord, texelSize) * 0.3; // Reduced color edge weight

            // Combine edges with clamping
            float edge = clamp(depthE + normalE + colorE, 0.0, 1.0);
            edge *= uIntensity;
            
            // Apply gaussian-like smoothing for better outline quality
            if (edge > 0.1) {
                // Sample surrounding pixels for smoothing
                float smoothEdge = edge;
                for (int i = -1; i <= 1; i++) {
                    for (int j = -1; j <= 1; j++) {
                        if (i == 0 && j == 0) continue;
                        vec2 sampleUV = vTexCoord + vec2(float(i), float(j)) * texelSize;
                        float sampleDepthE = depthEdge(sampleUV, texelSize) * uDepthWeight;
                        float sampleNormalE = normalEdge(sampleUV, texelSize) * uNormalWeight;
                        smoothEdge += (sampleDepthE + sampleNormalE) * 0.125;
                    }
                }
                edge = smoothEdge;
            }

            // Debug views
            if (uDebugOutput == 1) {
                gl_FragColor = color;
                return;
            } else if (uDebugOutput == 2) {
                gl_FragColor = vec4(edge, edge, edge, 1.0);
                return;
            }

            // Three.js style: overlay black outline on edges
            vec3 outlineColor = vec3(0.0, 0.0, 0.0); // Black outline
            gl_FragColor = vec4(mix(color.rgb, outlineColor, edge), color.a);
        }
    )GLSL";

	// Optional: validate GLSL via native GL path for better error messages
#ifdef IMAGE_OUTLINE_ENABLE_GL_VALIDATION
	validateProgramGL(kVS, kFS);
#endif

	m_vs->sourceType = SoShaderObject::GLSL_PROGRAM;
	m_vs->sourceProgram.setValue(kVS);
	m_fs->sourceType = SoShaderObject::GLSL_PROGRAM;
	m_fs->sourceProgram.setValue(kFS);
	m_program->shaderObject.set1Value(0, m_vs);
	m_program->shaderObject.set1Value(1, m_fs);
	LOG_DBG("shader objects set", "ImageOutlinePass");

	// Create render-to-texture nodes
	m_colorTexture = new SoSceneTexture2;
	m_colorTexture->ref();
	m_colorTexture->size.setValue(SbVec2s(0, 0)); // auto-size to viewport
	m_colorTexture->transparencyFunction = SoSceneTexture2::NONE;
	m_colorTexture->type = SoSceneTexture2::RGBA8;
	m_colorTexture->wrapS = SoSceneTexture2::CLAMP;
	m_colorTexture->wrapT = SoSceneTexture2::CLAMP;
	LOG_DBG("color RTT created", "ImageOutlinePass");

	m_depthTexture = new SoSceneTexture2;
	m_depthTexture->ref();
	m_depthTexture->size.setValue(SbVec2s(0, 0)); // auto-size to viewport
	m_depthTexture->transparencyFunction = SoSceneTexture2::NONE;
	m_depthTexture->type = SoSceneTexture2::DEPTH;
	m_depthTexture->wrapS = SoSceneTexture2::CLAMP;
	m_depthTexture->wrapT = SoSceneTexture2::CLAMP;
	LOG_DBG("depth RTT created", "ImageOutlinePass");

	// CRITICAL: Create a scene graph that includes camera for proper rendering
	// SoSceneTexture2 needs camera to render depth properly
	if (m_sceneManager && m_captureRoot) {
		// Create a temporary scene root that includes camera
		SoSeparator* tempSceneRoot = new SoSeparator;
		tempSceneRoot->ref();
		
		// Add camera from scene manager
		SoCamera* camera = m_sceneManager->getCamera();
		if (camera) {
			tempSceneRoot->addChild(camera);
		}
		
		// Add the capture root (geometry)
		tempSceneRoot->addChild(m_captureRoot);
		
		// Set scenes for render-to-texture
		m_colorTexture->scene = tempSceneRoot;
		m_depthTexture->scene = tempSceneRoot;
		
		// Store reference for cleanup
		m_tempSceneRoot = tempSceneRoot;
		
		LOG_DBG("RTT scenes set with camera and geometry", "ImageOutlinePass");
	} else {
		LOG_WRN("missing scene manager or capture root", "ImageOutlinePass");
	}

	// Create texture samplers
	m_colorSampler = new SoTexture2;
	m_colorSampler->ref();
	m_depthSampler = new SoTexture2;
	m_depthSampler->ref();
	LOG_DBG("samplers created", "ImageOutlinePass");

	// Create fullscreen quad
	m_quadSeparator = new SoSeparator;
	m_quadSeparator->ref();
	auto* coords = new SoCoordinate3;
	SbVec3f vertices[] = {
		SbVec3f(-1.0f, -1.0f, 0.0f),
		SbVec3f(1.0f, -1.0f, 0.0f),
		SbVec3f(1.0f,  1.0f, 0.0f),
		SbVec3f(-1.0f,  1.0f, 0.0f)
	};
	coords->point.setValues(0, 4, vertices);

	auto* texCoords = new SoTextureCoordinate2;
	SbVec2f uvs[] = {
		SbVec2f(0.0f, 0.0f),
		SbVec2f(1.0f, 0.0f),
		SbVec2f(1.0f, 1.0f),
		SbVec2f(0.0f, 1.0f)
	};
	texCoords->point.setValues(0, 4, uvs);

	auto* face = new SoFaceSet;
	face->numVertices.set1Value(0, 4);
	
	// Disable lighting for the quad
	auto* lightModel = new SoLightModel;
	lightModel->model = SoLightModel::BASE_COLOR;
	
	// Set material to ensure visibility
	auto* material = new SoMaterial;
	material->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
	material->transparency = 0.0f;

	m_quadSeparator->addChild(lightModel);
	m_quadSeparator->addChild(material);
	m_quadSeparator->addChild(texCoords);
	m_quadSeparator->addChild(coords);
	m_quadSeparator->addChild(face);
	LOG_DBG("fullscreen quad built", "ImageOutlinePass");

	// Create shader parameters - these control the outline appearance

	// Edge intensity: overall strength of the outline effect (0.0 = no outline, 1.0 = full strength)
	m_uIntensity = new SoShaderParameter1f;
	m_uIntensity->ref();
	m_uIntensity->name = "uIntensity";
	m_uIntensity->value = m_params.edgeIntensity;

	// Depth weight: contribution of depth-based edges (0.0 = ignore depth, 2.0 = strong depth edges)
	m_uDepthWeight = new SoShaderParameter1f;
	m_uDepthWeight->ref();
	m_uDepthWeight->name = "uDepthWeight";
	m_uDepthWeight->value = m_params.depthWeight;

	// Normal weight: contribution of normal-based edges (0.0 = ignore normals, 2.0 = strong normal edges)
	m_uNormalWeight = new SoShaderParameter1f;
	m_uNormalWeight->ref();
	m_uNormalWeight->name = "uNormalWeight";
	m_uNormalWeight->value = m_params.normalWeight;

	// Depth threshold: minimum depth difference to consider as an edge
	m_uDepthThreshold = new SoShaderParameter1f;
	m_uDepthThreshold->ref();
	m_uDepthThreshold->name = "uDepthThreshold";
	m_uDepthThreshold->value = m_params.depthThreshold;

	// Normal threshold: minimum normal angle difference to consider as an edge (in dot product units)
	m_uNormalThreshold = new SoShaderParameter1f;
	m_uNormalThreshold->ref();
	m_uNormalThreshold->name = "uNormalThreshold";
	m_uNormalThreshold->value = m_params.normalThreshold;

	// Thickness: edge thickness multiplier (1.0 = normal, 2.0 = double thickness)
	m_uThickness = new SoShaderParameter1f;
	m_uThickness->ref();
	m_uThickness->name = "uThickness";
	m_uThickness->value = m_params.thickness;

	// Resolution parameter (will be updated when viewport changes)
	m_uResolution = new SoShaderParameter2f;
	m_uResolution->ref();
	m_uResolution->name = "uResolution";
	// Default to 1024x1024, will be updated in attachOverlay
	m_uResolution->value = SbVec2f(1.0f / 1024.0f, 1.0f / 1024.0f);

	// Optional inverse matrices (populate if available)
	m_uInvProjection = new SoShaderParameterMatrix;
	m_uInvProjection->ref();
	m_uInvProjection->name = "uInvProjection";
	m_uInvView = new SoShaderParameterMatrix;
	m_uInvView->ref();
	m_uInvView->name = "uInvView";

	// Debug output selector
	m_uDebugOutput = new SoShaderParameter1i;
	m_uDebugOutput->ref();
	m_uDebugOutput->name = "uDebugOutput";
	m_uDebugOutput->value = static_cast<int>(m_debugOutput);
	    LOG_INF("buildShaders end", "ImageOutlinePass");
}

void ImageOutlinePass::updateCameraMatrices() {
    if (!m_sceneManager) return;
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) return;
    
    // Get viewport dimensions
    SbVec2s vpSize(1920, 1080); // Default size
    if (m_sceneManager->getCanvas()) {
        int width, height;
        m_sceneManager->getCanvas()->GetSize(&width, &height);
        if (width > 0 && height > 0) {
            vpSize = SbVec2s(width, height);
        }
    }
    
    // Get camera view volume
    SbViewVolume viewVol = camera->getViewVolume(float(vpSize[0]) / float(vpSize[1]));
    
    // Get projection matrix and invert it
    SbMatrix projMatrix = viewVol.getMatrix();
    SbMatrix invProjMatrix = projMatrix.inverse();
    
    // Get view matrix (camera transformation) and invert it
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