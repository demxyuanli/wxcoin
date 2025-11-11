#include "CuteNavCube.h"
#include "NavigationCubeTextureGenerator.h"
#include "NavigationCubeTypes.h"
#include "Canvas.h"
#include "NavigationCubeConfigDialog.h"
#include "DPIManager.h"
#include "DPIAwareRendering.h"
#include "config/ConfigManager.h"
#include <algorithm>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLinear.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/font.h>
#include <wx/time.h>
#include <cmath>
#include <vector>
#include "logger/Logger.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include <GL/gl.h>
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif



// Get face label for a given PickId
std::string CuteNavCube::getFaceLabel(PickId pickId) {
    switch (pickId) {
        case PickId::Front: return "FRONT";
        case PickId::Top: return "TOP";
        case PickId::Right: return "RIGHT";
        case PickId::Rear: return "REAR";
        case PickId::Bottom: return "BOTTOM";
        case PickId::Left: return "LEFT";
        default: return "";
    }
}


CuteNavCube::CuteNavCube(std::function<void(const std::string&)> viewChangeCallback, float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config)
	: m_root(new SoSeparator)
	, m_orthoCamera(new SoOrthographicCamera)
	, m_textureGenerator(std::make_unique<NavigationCubeTextureGenerator>())
	, m_enabled(true)
	, m_dpiScale(dpiScale)
	, m_viewChangeCallback(viewChangeCallback)
	, m_isDragging(false)
	, m_lastMousePos(0, 0)
	, m_rotationX(0.0f)
	, m_rotationY(0.0f)
	, m_lastDragTime(0)
	, m_windowWidth(windowWidth)
	, m_windowHeight(windowHeight)
	, m_positionX(config.x >= 0 ? config.x : 20)  // Use config or default
	, m_positionY(config.y >= 0 ? config.y : 20)  // Use config or default
	, m_cubeSize(config.size > 0 ? config.size : 140)  // Use config or default
	, m_currentX(0.0f)
	, m_currentY(0.0f)
	, m_geometrySize(config.cubeSize > 0.0f ? config.cubeSize : 0.55f)  // Adjusted to 0.55f for better proportion
	, m_chamferSize(config.chamferSize > 0.0f ? config.chamferSize : 0.12f)
	, m_cameraDistance(config.cameraDistance > 0.0f ? config.cameraDistance : 3.5f)
	, m_needsGeometryRebuild(false)
	, m_showEdges(config.showEdges)
	, m_showCorners(config.showCorners)
	, m_showTextures(config.showTextures)
	, m_enableAnimation(config.enableAnimation)
	, m_transparency(config.transparency >= 0.0f ? config.transparency : 0.0f)
	, m_shininess(config.shininess >= 0.0f ? config.shininess : 0.5f)
	, m_ambientIntensity(config.ambientIntensity >= 0.0f ? config.ambientIntensity : 0.8f)
	, m_circleRadius(config.circleRadius > 0 ? config.circleRadius : 150)
	, m_circleMarginX(config.circleMarginX >= 0 ? config.circleMarginX : 50)
	, m_circleMarginY(config.circleMarginY >= 0 ? config.circleMarginY : 50)
	, m_hoveredFace("")
	, m_normalFaceColor(0.7f, 0.7f, 0.7f)
	, m_hoverFaceColor(1.0f, 0.2f, 0.2f)
	, m_fontZoom(0.3f)
	, m_canvas(nullptr)
	, m_cameraAnimator(nullptr)
	, m_animationDuration(0.2f)
	, m_animationType(CameraAnimation::SMOOTH)
	, m_pendingViewName("")
{
	m_root->ref();
	m_orthoCamera->ref(); // Add reference to camera to prevent premature deletion
	initialize();
}

// New constructor with camera move callback
CuteNavCube::CuteNavCube(std::function<void(const std::string&)> viewChangeCallback,
						std::function<void(const SbVec3f&, const SbRotation&)> cameraMoveCallback,
						float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config)
	: m_root(new SoSeparator)
	, m_orthoCamera(new SoOrthographicCamera)
	, m_textureGenerator(std::make_unique<NavigationCubeTextureGenerator>())
	, m_enabled(true)
	, m_dpiScale(dpiScale)
	, m_viewChangeCallback(viewChangeCallback)
	, m_cameraMoveCallback(cameraMoveCallback)
	, m_rotationChangedCallback(nullptr)
	, m_refreshCallback(nullptr)
	, m_isDragging(false)
	, m_lastMousePos(0, 0)
	, m_rotationX(0.0f)
	, m_rotationY(0.0f)
	, m_lastDragTime(0)
	, m_windowWidth(windowWidth)
	, m_windowHeight(windowHeight)
	, m_positionX(config.x >= 0 ? config.x : 20)  // Use config or default
	, m_positionY(config.y >= 0 ? config.y : 20)  // Use config or default
	, m_cubeSize(config.size > 0 ? config.size : 140)  // Use config or default
	, m_currentX(0.0f)
	, m_currentY(0.0f)
	, m_geometrySize(config.cubeSize > 0.0f ? config.cubeSize : 0.55f)  // Adjusted to 0.55f for better proportion
	, m_chamferSize(config.chamferSize > 0.0f ? config.chamferSize : 0.12f)
	, m_cameraDistance(config.cameraDistance > 0.0f ? config.cameraDistance : 3.5f)
	, m_needsGeometryRebuild(false)
	, m_showEdges(config.showEdges)
	, m_showCorners(config.showCorners)
	, m_showTextures(config.showTextures)
	, m_enableAnimation(config.enableAnimation)
	, m_transparency(config.transparency >= 0.0f ? config.transparency : 0.0f)
	, m_shininess(config.shininess >= 0.0f ? config.shininess : 0.5f)
	, m_ambientIntensity(config.ambientIntensity >= 0.0f ? config.ambientIntensity : 0.8f)
	, m_circleRadius(config.circleRadius > 0 ? config.circleRadius : 150)
	, m_circleMarginX(config.circleMarginX >= 0 ? config.circleMarginX : 50)
	, m_circleMarginY(config.circleMarginY >= 0 ? config.circleMarginY : 50)
	, m_hoveredFace("")
	, m_normalFaceColor(0.7f, 0.7f, 0.7f)
	, m_hoverFaceColor(1.0f, 0.2f, 0.2f)
	, m_fontZoom(0.3f)
	, m_canvas(nullptr)
	, m_cameraAnimator(nullptr)
	, m_animationDuration(0.2f)
	, m_animationType(CameraAnimation::SMOOTH)
	, m_pendingViewName("")
{
	m_root->ref();
	m_orthoCamera->ref(); // Add reference to camera to prevent premature deletion
	initialize();
}

// New constructor with refresh callback
CuteNavCube::CuteNavCube(std::function<void(const std::string&)> viewChangeCallback,
						std::function<void(const SbVec3f&, const SbRotation&)> cameraMoveCallback,
						std::function<void()> refreshCallback,
						float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config)
	: m_root(new SoSeparator)
	, m_orthoCamera(new SoOrthographicCamera)
	, m_textureGenerator(std::make_unique<NavigationCubeTextureGenerator>())
	, m_enabled(true)
	, m_dpiScale(dpiScale)
	, m_viewChangeCallback(viewChangeCallback)
	, m_cameraMoveCallback(cameraMoveCallback)
	, m_rotationChangedCallback(nullptr)
	, m_refreshCallback(refreshCallback)
	, m_isDragging(false)
	, m_lastMousePos(0, 0)
	, m_rotationX(0.0f)
	, m_rotationY(0.0f)
	, m_lastDragTime(0)
	, m_windowWidth(windowWidth)
	, m_windowHeight(windowHeight)
	, m_positionX(config.x >= 0 ? config.x : 20)  // Use config or default
	, m_positionY(config.y >= 0 ? config.y : 20)  // Use config or default
	, m_cubeSize(config.size > 0 ? config.size : 140)  // Use config or default
	, m_currentX(0.0f)
	, m_currentY(0.0f)
	, m_geometrySize(config.cubeSize > 0.0f ? config.cubeSize : 0.55f)  // Adjusted to 0.55f for better proportion
	, m_chamferSize(config.chamferSize > 0.0f ? config.chamferSize : 0.12f)
	, m_cameraDistance(config.cameraDistance > 0.0f ? config.cameraDistance : 3.5f)
	, m_needsGeometryRebuild(false)
	, m_showEdges(config.showEdges)
	, m_showCorners(config.showCorners)
	, m_showTextures(config.showTextures)
	, m_enableAnimation(config.enableAnimation)
	, m_transparency(config.transparency >= 0.0f ? config.transparency : 0.0f)
	, m_shininess(config.shininess >= 0.0f ? config.shininess : 0.5f)
	, m_ambientIntensity(config.ambientIntensity >= 0.0f ? config.ambientIntensity : 0.8f)
	, m_circleRadius(config.circleRadius > 0 ? config.circleRadius : 150)
	, m_circleMarginX(config.circleMarginX >= 0 ? config.circleMarginX : 50)
	, m_circleMarginY(config.circleMarginY >= 0 ? config.circleMarginY : 50)
	, m_hoveredFace("")
	, m_normalFaceColor(0.7f, 0.7f, 0.7f)
	, m_hoverFaceColor(1.0f, 0.2f, 0.2f)
	, m_fontZoom(0.3f)
	, m_canvas(nullptr)
	, m_cameraAnimator(nullptr)
	, m_animationDuration(0.2f)
	, m_animationType(CameraAnimation::SMOOTH)
	, m_pendingViewName("")
{
	m_root->ref();
	m_orthoCamera->ref(); // Add reference to camera to prevent premature deletion
	initialize();
}

CuteNavCube::~CuteNavCube() {
	stopCameraAnimation();
	m_orthoCamera->unref(); // Release camera reference
	m_root->unref();
}

void CuteNavCube::initialize() {
	setupGeometry();

	// Initialize texture generator font sizes
	if (m_textureGenerator) {
		m_textureGenerator->initializeFontSizes();
	}

	m_faceToView = {
		// 6 Main faces - Click face -> View direction
		{ "FRONT",  "FRONT" },
		{ "REAR",  "REAR" },
		{ "LEFT",   "LEFT" },
		{ "RIGHT",  "RIGHT" },
		{ "TOP",    "TOP" },
		{ "BOTTOM", "BOTTOM" },

		// 8 Corner faces (triangular)
		{ "Corner0", "Top" },        // Front-Top-Left corner -> Top view
		{ "Corner1", "Top" },        // Front-Top-Right corner -> Top view
		{ "Corner2", "Top" },        // Back-Top-Right corner -> Top view
		{ "Corner3", "Top" },        // Back-Top-Left corner -> Top view
		{ "Corner4", "Bottom" },     // Front-Bottom-Left corner -> Bottom view
		{ "Corner5", "Bottom" },     // Front-Bottom-Right corner -> Bottom view
		{ "Corner6", "Bottom" },     // Back-Bottom-Right corner -> Bottom view
		{ "Corner7", "Bottom" },     // Back-Bottom-Left corner -> Bottom view

		// 12 Edge faces
		{ "EdgeTF", "Top" },         // Top-Front edge -> Top view
		{ "EdgeTB", "Top" },         // Top-Back edge -> Top view
		{ "EdgeTL", "Top" },         // Top-Left edge -> Top view
		{ "EdgeTR", "Top" },         // Top-Right edge -> Top view
		{ "EdgeBF", "Bottom" },      // Bottom-Front edge -> Bottom view
		{ "EdgeBB", "Bottom" },      // Bottom-Back edge -> Bottom view
		{ "EdgeBL", "Bottom" },      // Bottom-Left edge -> Bottom view
		{ "EdgeBR", "Bottom" },      // Bottom-Right edge -> Bottom view
		{ "EdgeFR", "Front" },       // Front-Right edge -> Front view
		{ "EdgeFL", "Front" },       // Front-Left edge -> Front view
		{ "EdgeBL2", "Back" },       // Back-Left edge -> Back view
		{ "EdgeBR2", "Back" }        // Back-Right edge -> Back view
	};

	// Face normal vectors and center points for camera positioning
	m_faceNormals = {
		// 6 Main faces
		{ "FRONT",  std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0, 0, 1)) },      // +Z axis
		{ "REAR",  std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0, 0, -1)) },    // -Z axis
		{ "LEFT",   std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-1, 0, 0)) },   // -X axis
		{ "RIGHT",  std::make_pair(SbVec3f(1, 0, 0), SbVec3f(1, 0, 0)) },    // +X axis
		{ "TOP",    std::make_pair(SbVec3f(0, 1, 0), SbVec3f(0, 1, 0)) },    // +Y axis
		{ "BOTTOM", std::make_pair(SbVec3f(0, -1, 0), SbVec3f(0, -1, 0)) },   // -Y axis

		// 8 Corner faces (using closest main face normal)
		{ "Corner0", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(-0.707, 0.707, 0.707)) },  // Front-Top-Left
		{ "Corner1", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0.707, 0.707, 0.707)) },   // Front-Top-Right
		{ "Corner2", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0.707, 0.707, -0.707)) },  // Back-Top-Right
		{ "Corner3", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(-0.707, 0.707, -0.707)) }, // Back-Top-Left
		{ "Corner4", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(-0.707, -0.707, 0.707)) }, // Front-Bottom-Left
		{ "Corner5", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0.707, -0.707, 0.707)) },  // Front-Bottom-Right
		{ "Corner6", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0.707, -0.707, -0.707)) }, // Back-Bottom-Right
		{ "Corner7", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(-0.707, -0.707, -0.707)) }, // Back-Bottom-Left

		// 12 Edge faces (using average of adjacent faces)
		{ "EdgeTF", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0, 0.707, 0.707)) },       // Top-Front
		{ "EdgeTB", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0, 0.707, -0.707)) },      // Top-Back
		{ "EdgeTL", std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-0.707, 0.707, 0)) },     // Top-Left
		{ "EdgeTR", std::make_pair(SbVec3f(1, 0, 0), SbVec3f(0.707, 0.707, 0)) },      // Top-Right
		{ "EdgeBF", std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0, -0.707, 0.707)) },      // Bottom-Front
		{ "EdgeBB", std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0, -0.707, -0.707)) },     // Bottom-Back
		{ "EdgeBL", std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-0.707, -0.707, 0)) },    // Bottom-Left
		{ "EdgeBR", std::make_pair(SbVec3f(1, 0, 0), SbVec3f(0.707, -0.707, 0)) },     // Bottom-Right
		{ "EdgeFR", std::make_pair(SbVec3f(1, 0, 0), SbVec3f(0.707, 0, 0.707)) },      // Front-Right
		{ "EdgeFL", std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-0.707, 0, 0.707)) },    // Front-Left
		{ "EdgeBL2", std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-0.707, 0, -0.707)) },   // Back-Left
		{ "EdgeBR2", std::make_pair(SbVec3f(1, 0, 0), SbVec3f(0.707, 0, -0.707)) }      // Back-Right
	};

	if (!m_cameraAnimator) {
		m_cameraAnimator = std::make_unique<CameraAnimation>();
		m_cameraAnimator->setCamera(m_orthoCamera);
		m_cameraAnimator->setAnimationType(m_animationType);
		m_cameraAnimator->setViewRefreshCallback([this]() {
			if (m_refreshCallback) {
				m_refreshCallback();
			}
		});
		m_cameraAnimator->setProgressCallback([this](float) {
			if (m_cameraMoveCallback) {
				m_cameraMoveCallback(m_orthoCamera->position.getValue(), m_orthoCamera->orientation.getValue());
			}
		});
		m_cameraAnimator->setCompletionCallback([this]() {
			if (m_cameraMoveCallback) {
				m_cameraMoveCallback(m_orthoCamera->position.getValue(), m_orthoCamera->orientation.getValue());
			} else if (!m_pendingViewName.empty() && m_viewChangeCallback) {
				m_viewChangeCallback(m_pendingViewName);
			}
			m_pendingViewName.clear();
		});
	} else {
		m_cameraAnimator->setCamera(m_orthoCamera);
		m_cameraAnimator->setAnimationType(m_animationType);
	}
}


void CuteNavCube::setupGeometry() {
	// Create cube face textures using the texture generator
	if (m_textureGenerator) {
		m_textureGenerator->createCubeFaceTextures();
	}

	// Clear previous face mappings
	m_faceMaterials.clear();
	m_faceBaseColors.clear();
	m_faceHoverColors.clear();
	m_faceTextureMaterials.clear();
	m_faceSeparators.clear();
	m_Faces.clear();
	m_LabelTextures.clear();

	// Safely clear previous geometry
	if (m_root->getNumChildren() > 0) {
		m_root->removeAllChildren();
	}

	// Setup camera properties
	m_orthoCamera->viewportMapping = SoOrthographicCamera::ADJUST_CAMERA;
	m_orthoCamera->nearDistance = 0.05f;
	m_orthoCamera->farDistance = 15.0f;
	m_orthoCamera->position.setValue(0.0f, 0.0f, 5.0f);
	m_orthoCamera->orientation.setValue(SbRotation(SbVec3f(1, 0, 0), -M_PI / 2));

	// Always add camera back to the scene
	m_root->addChild(m_orthoCamera);

	// Lighting setup
	SoEnvironment* env = new SoEnvironment;
	env->ambientColor.setValue(0.8f, 0.8f, 0.85f);
	env->ambientIntensity.setValue(m_ambientIntensity);
	m_root->addChild(env);

	m_mainLight = new SoDirectionalLight;
	m_mainLight->direction.setValue(0.5f, 0.5f, -0.5f);
	m_mainLight->intensity.setValue(0.4f);
	m_mainLight->color.setValue(1.0f, 1.0f, 1.0f);
	m_root->addChild(m_mainLight);

	m_fillLight = new SoDirectionalLight;
	m_fillLight->direction.setValue(-0.5f, -0.5f, 0.5f);
	m_fillLight->intensity.setValue(0.4f);
	m_fillLight->color.setValue(0.95f, 0.95f, 1.0f);
	m_root->addChild(m_fillLight);

	m_sideLight = new SoDirectionalLight;
	m_sideLight->direction.setValue(-0.8f, 0.2f, -0.3f);
	m_sideLight->intensity.setValue(0.3f);
	m_sideLight->color.setValue(1.0f, 1.0f, 0.95f);
	m_root->addChild(m_sideLight);

	SoDirectionalLight* backLight = new SoDirectionalLight;
	backLight->direction.setValue(0.0f, 0.0f, 1.0f);
	backLight->intensity.setValue(0.3f);
	backLight->color.setValue(0.9f, 0.9f, 1.0f);
	m_root->addChild(backLight);

	SoDirectionalLight* bottomLight = new SoDirectionalLight;
	bottomLight->direction.setValue(0.4f, -0.8f, 0.2f);
	bottomLight->intensity.setValue(0.2f);
	bottomLight->color.setValue(1.0f, 0.95f, 0.95f);
	m_root->addChild(bottomLight);

	SoDirectionalLight* topSideLight = new SoDirectionalLight;
	topSideLight->direction.setValue(0.8f, 0.3f, 0.3f);
	topSideLight->intensity.setValue(0.2f);
	topSideLight->color.setValue(0.95f, 1.0f, 0.95f);
	m_root->addChild(topSideLight);

	updateCameraRotation();

	// Delegate geometry creation to the builder
	NavigationCubeGeometryBuilder builder;
	NavigationCubeGeometryBuilder::BuildParams params;
	params.chamferSize = m_chamferSize;
	params.geometrySize = m_geometrySize;
	params.showEdges = m_showEdges;
	params.showCorners = m_showCorners;

	auto geometryResult = builder.build(params);

	m_Faces = std::move(geometryResult.faces);
	m_LabelTextures = std::move(geometryResult.labelTextures);
	m_faceMaterials = std::move(geometryResult.faceMaterials);
	m_faceSeparators = std::move(geometryResult.faceSeparators);
	m_faceBaseColors = std::move(geometryResult.faceBaseColors);
	m_faceHoverColors = std::move(geometryResult.faceHoverColors);
	m_faceTextureMaterials = std::move(geometryResult.faceTextureMaterials);
	m_geometryTransform = geometryResult.geometryTransform;

	if (geometryResult.geometryRoot) {
		m_root->addChild(geometryResult.geometryRoot);
	}

	// Note: Outlines are now drawn per-face in the builder

	// Generate and cache all textures after geometry setup
	LOG_INF_S("=== TEXTURE SYSTEM CHECK ===");
	LOG_INF_S("m_showTextures: " + std::string(m_showTextures ? "true" : "false"));
	LOG_INF_S("m_Faces.size(): " + std::to_string(m_Faces.size()));

	if (m_showTextures) {
		LOG_INF_S("Starting texture generation...");
		if (m_textureGenerator) {
			m_textureGenerator->initializeFontSizes();
			m_textureGenerator->generateAndCacheTextures();
			applyInitialTextures();
		}
		LOG_INF_S("Texture generation completed");
	} else {
		LOG_INF_S("Texture generation SKIPPED - m_showTextures is false");
	}

	// Summary and validation log
	LOG_INF_S("=== RHOMBICUBOCTAHEDRON INDIVIDUAL FACES CREATED ===");
	LOG_INF_S("Geometry: 26 independent faces (FreeCAD-style), each with its own separator and material");

	int mainFaces = 0, cornerFaces = 0, edgeFaces = 0;
	std::map<ShapeId, int> vertexCounts;

	int recalculatedTotalVertices = 0;
	for (const auto& pair : m_Faces) {
		recalculatedTotalVertices += static_cast<int>(pair.second.vertexArray.size());
		vertexCounts[pair.second.type] += static_cast<int>(pair.second.vertexArray.size());
		switch (pair.second.type) {
		case ShapeId::Main: mainFaces++; break;
		case ShapeId::Corner: cornerFaces++; break;
		case ShapeId::Edge: edgeFaces++; break;
		}
	}

	int totalTextureVertices = 0;
	for (const auto& pair : m_LabelTextures) {
		totalTextureVertices += static_cast<int>(pair.second.vertexArray.size());
	}

	LOG_INF_S("Face counts - Main: " + std::to_string(mainFaces) + ", Corner: " + std::to_string(cornerFaces) + ", Edge: " + std::to_string(edgeFaces));
	LOG_INF_S("Vertex counts - Main: " + std::to_string(vertexCounts[ShapeId::Main]) +
		", Corner: " + std::to_string(vertexCounts[ShapeId::Corner]) +
		", Edge: " + std::to_string(vertexCounts[ShapeId::Edge]) + " (total: " + std::to_string(recalculatedTotalVertices) + ")");
	LOG_INF_S("Individual faces: " + std::to_string(recalculatedTotalVertices) + " vertices, 26 independent faces (FreeCAD-style)");
	LOG_INF_S("Texture quads: " + std::to_string(totalTextureVertices) + " texture vertices for " + std::to_string(m_showTextures ? 6 : 0) + " main face overlays");
	LOG_INF_S("Total geometry: " + std::to_string(recalculatedTotalVertices + totalTextureVertices) + " vertices, 26 independent faces + 6 texture quads");

	bool valid = true;
	LOG_INF_S("=== VALIDATION CHECKS ===");
	LOG_INF_S("Face counts - Main: " + std::to_string(mainFaces) + "/6, Corner: " + std::to_string(cornerFaces) + "/8, Edge: " + std::to_string(edgeFaces) + "/12");
	LOG_INF_S("Vertex counts - Main: " + std::to_string(vertexCounts[ShapeId::Main]) + "/48, Corner: " + std::to_string(vertexCounts[ShapeId::Corner]) + "/48, Edge: " + std::to_string(vertexCounts[ShapeId::Edge]) + "/48");

	std::vector<PickId> debugFaceIds = {
		PickId::Top, PickId::Front, PickId::Left, PickId::Rear, PickId::Right, PickId::Bottom,
		PickId::FrontTopRight, PickId::FrontTopLeft, PickId::FrontBottomRight, PickId::FrontBottomLeft,
		PickId::RearTopRight, PickId::RearTopLeft, PickId::RearBottomRight, PickId::RearBottomLeft,
		PickId::FrontTop, PickId::RearTop, PickId::TopLeft, PickId::TopRight,
		PickId::FrontBottom, PickId::RearBottom, PickId::BottomLeft, PickId::BottomRight,
		PickId::FrontRight, PickId::FrontLeft, PickId::RearLeft, PickId::RearRight
	};

	for (PickId faceId : debugFaceIds) {
		if (m_Faces.count(faceId) > 0) {
			int vertexCount = static_cast<int>(m_Faces[faceId].vertexArray.size());
			std::string shapeStr = (m_Faces[faceId].type == ShapeId::Main) ? "Main" : (m_Faces[faceId].type == ShapeId::Corner) ? "Corner" : "Edge";
			LOG_INF_S("Face " + std::to_string(static_cast<int>(faceId)) + " (" + shapeStr + "): " + std::to_string(vertexCount) + " vertices");
		}
	}

	if (mainFaces != 6) { LOG_WRN_S("ERROR: Expected 6 main faces, got " + std::to_string(mainFaces)); valid = false; }
	if (cornerFaces != 8) { LOG_WRN_S("ERROR: Expected 8 corner faces, got " + std::to_string(cornerFaces)); valid = false; }
	if (edgeFaces != 12) { LOG_WRN_S("ERROR: Expected 12 edge faces, got " + std::to_string(edgeFaces)); valid = false; }
	if (vertexCounts[ShapeId::Main] != 48) { LOG_WRN_S("ERROR: Expected 48 main face vertices (6x8), got " + std::to_string(vertexCounts[ShapeId::Main])); valid = false; }
	if (vertexCounts[ShapeId::Corner] != 48) { LOG_WRN_S("ERROR: Expected 48 corner face vertices (8x6), got " + std::to_string(vertexCounts[ShapeId::Corner])); valid = false; }
	if (vertexCounts[ShapeId::Edge] != 48) { LOG_WRN_S("ERROR: Expected 48 edge face vertices (12x4), got " + std::to_string(vertexCounts[ShapeId::Edge])); valid = false; }

	if (valid) {
		LOG_INF_S("[PASS] Rhombicuboctahedron individual faces validation PASSED - all 26 faces properly formed");
	} else {
		LOG_ERR_S("[FAIL] Rhombicuboctahedron individual faces validation FAILED - geometry errors detected");
	}
}

void CuteNavCube::updateCameraRotation() {
	// Rotates camera around the cube
	float distance = m_cameraDistance; // Configurable camera distance
	float radX = m_rotationX * M_PI / 180.0f;
	float radY = m_rotationY * M_PI / 180.0f;

	// Calculate position in spherical coordinates
	float x = distance * sin(radY) * cos(radX);
	float y = distance * sin(radX);
	float z = distance * cos(radY) * cos(radX);

	m_orthoCamera->position.setValue(x, y, z);
	m_orthoCamera->pointAt(SbVec3f(0, 0, 0)); // Always look at the origin (cube center)
	
	// Note: Orthographic camera will automatically adjust to viewport

	//LOG_DBG("CuteNavCube::updateCameraRotation: Camera position x=" + std::to_string(x) +
	//    ", y=" + std::to_string(y) + ", z=" + std::to_string(z));
}

std::string CuteNavCube::pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize) {
	// Validate viewport size
	if (viewportSize.x <= 0 || viewportSize.y <= 0) {
		LOG_INF_S("CuteNavCube::pickRegion: Invalid viewport size - " + 
			std::to_string(viewportSize.x) + "x" + std::to_string(viewportSize.y));
		return "";
	}

	// Add picking debug log - disabled for performance
	// LOG_INF_S("CuteNavCube::pickRegion: Picking at position (" +
	//	std::to_string(mousePos[0]) + ", " + std::to_string(mousePos[1]) +
	//	") in viewport " + std::to_string(viewportSize.x) + "x" + std::to_string(viewportSize.y));

	// Create viewport region for picking - use cube's local coordinate system
	SbViewportRegion pickViewport;
	pickViewport.setWindowSize(SbVec2s(static_cast<short>(viewportSize.x), static_cast<short>(viewportSize.y)));

	// Set viewport pixels to match the cube's local viewport (0,0 to cubeSize,cubeSize)
	pickViewport.setViewportPixels(0, 0, viewportSize.x, viewportSize.y);

	// Debug viewport settings
	static int debugCount = 0;
	// Debug counter disabled for performance
	// if (++debugCount % 10 == 0) {
	//	LOG_INF_S("CuteNavCube::pickRegion: Viewport settings - local:" +
	//		std::to_string(viewportSize.x) + "x" + std::to_string(viewportSize.y) +
	//		", mouse:" + std::to_string(mousePos[0]) + "," + std::to_string(mousePos[1]));
	// }

	SoRayPickAction pickAction(pickViewport);
	pickAction.setPoint(mousePos);
	pickAction.apply(m_root);

	SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
	if (!pickedPoint) {
	return "";
	}

	SoPath* pickedPath = pickedPoint->getPath();
	if (!pickedPath || pickedPath->getLength() == 0) {
		return "";
	}

	// Find the named separator for the face
	for (int i = pickedPath->getLength() - 1; i >= 0; --i) {
		SoNode* node = pickedPath->getNode(i);
		if (node && node->isOfType(SoSeparator::getClassTypeId()) && node->getName().getLength() > 0) {
			std::string nameStr = node->getName().getString();
			
			// Check if this is a valid face name
			auto viewIt = m_faceToView.find(nameStr);
			auto normalIt = m_faceNormals.find(nameStr);
			if (viewIt != m_faceToView.end() && normalIt != m_faceNormals.end()) {
				return nameStr; // Return the actual face name, not the mapped view
			}
		}
	}

	return "";
}

// Calculate camera position based on clicked face
void CuteNavCube::calculateCameraPositionForFace(const std::string& faceName, SbVec3f& position, SbRotation& orientation) const {
	// Handle main faces (6 faces) - use standard positioning
	static const std::set<std::string> mainFaces = {"FRONT", "REAR", "LEFT", "RIGHT", "TOP", "BOTTOM"};
	if (mainFaces.find(faceName) != mainFaces.end()) {
		auto it = m_faceNormals.find(faceName);
		if (it != m_faceNormals.end()) {
			const SbVec3f& normal = it->second.first;
			const SbVec3f& center = it->second.second;

			// Camera position: move back along the normal direction by camera distance
			float distance = m_cameraDistance * 1.5f;
			position = center - normal * distance;

			// Camera orientation: look at the center point
			SbVec3f upVector = SbVec3f(0, 1, 0); // Default up vector
			if (std::abs(normal[1]) > 0.5f) {
				// If looking at top/bottom, use X axis as up
				upVector = SbVec3f(1, 0, 0);
			}

			SbVec3f direction = center - position;
			orientation.setValue(SbVec3f(0, 0, -1), direction);
			return;
		}
	}

	// Handle edge faces (12 faces) - calculate position to view the edge
	static const std::map<std::string, std::pair<SbVec3f, SbVec3f>> edgeFacePositions = {
		{ "EdgeTF", std::make_pair(SbVec3f(0, 0.5, 1.2), SbVec3f(0, -1, 0)) },   // Top-Front edge: look down at front-top
		{ "EdgeTB", std::make_pair(SbVec3f(0, 0.5, -1.2), SbVec3f(0, -1, 0)) },  // Top-Back edge: look down at back-top
		{ "EdgeTL", std::make_pair(SbVec3f(-1.2, 0.5, 0), SbVec3f(1, 0, 0)) },  // Top-Left edge: look right at top-left
		{ "EdgeTR", std::make_pair(SbVec3f(1.2, 0.5, 0), SbVec3f(-1, 0, 0)) },  // Top-Right edge: look left at top-right
		{ "EdgeBF", std::make_pair(SbVec3f(0, -0.5, 1.2), SbVec3f(0, 1, 0)) },   // Bottom-Front edge: look up at bottom-front
		{ "EdgeBB", std::make_pair(SbVec3f(0, -0.5, -1.2), SbVec3f(0, 1, 0)) },  // Bottom-Back edge: look up at bottom-back
		{ "EdgeBL", std::make_pair(SbVec3f(-1.2, -0.5, 0), SbVec3f(1, 0, 0)) },  // Bottom-Left edge: look right at bottom-left
		{ "EdgeBR", std::make_pair(SbVec3f(1.2, -0.5, 0), SbVec3f(-1, 0, 0)) },  // Bottom-Right edge: look left at bottom-right
		{ "EdgeFR", std::make_pair(SbVec3f(1.2, 0, 1), SbVec3f(-1, 0, 0)) },    // Front-Right edge: look left at front-right
		{ "EdgeFL", std::make_pair(SbVec3f(-1.2, 0, 1), SbVec3f(1, 0, 0)) },    // Front-Left edge: look right at front-left
		{ "EdgeBL2", std::make_pair(SbVec3f(-1.2, 0, -1), SbVec3f(1, 0, 0)) },   // Back-Left edge: look right at back-left
		{ "EdgeBR2", std::make_pair(SbVec3f(1.2, 0, -1), SbVec3f(-1, 0, 0)) }    // Back-Right edge: look left at back-right
	};

	auto edgeIt = edgeFacePositions.find(faceName);
	if (edgeIt != edgeFacePositions.end()) {
		position = edgeIt->second.first;
		SbVec3f upVector = edgeIt->second.second;
		SbVec3f direction = -position; // Look at origin
		orientation.setValue(upVector, direction);
		return;
	}

	// Handle corner faces (8 faces) - calculate position to view the corner
	static const std::map<std::string, std::pair<SbVec3f, SbVec3f>> cornerFacePositions = {
		{ "Corner0", std::make_pair(SbVec3f(-1.2, 1.2, 1.2), SbVec3f(0, 0, -1)) },  // Front-Top-Left: look towards origin
		{ "Corner1", std::make_pair(SbVec3f(1.2, 1.2, 1.2), SbVec3f(0, 0, -1)) },   // Front-Top-Right: look towards origin
		{ "Corner2", std::make_pair(SbVec3f(1.2, 1.2, -1.2), SbVec3f(0, 0, 1)) },   // Back-Top-Right: look towards origin
		{ "Corner3", std::make_pair(SbVec3f(-1.2, 1.2, -1.2), SbVec3f(0, 0, 1)) },  // Back-Top-Left: look towards origin
		{ "Corner4", std::make_pair(SbVec3f(-1.2, -1.2, 1.2), SbVec3f(0, 0, -1)) }, // Front-Bottom-Left: look towards origin
		{ "Corner5", std::make_pair(SbVec3f(1.2, -1.2, 1.2), SbVec3f(0, 0, -1)) },  // Front-Bottom-Right: look towards origin
		{ "Corner6", std::make_pair(SbVec3f(1.2, -1.2, -1.2), SbVec3f(0, 0, 1)) },  // Back-Bottom-Right: look towards origin
		{ "Corner7", std::make_pair(SbVec3f(-1.2, -1.2, -1.2), SbVec3f(0, 0, 1)) }  // Back-Bottom-Left: look towards origin
	};

	auto cornerIt = cornerFacePositions.find(faceName);
	if (cornerIt != cornerFacePositions.end()) {
		position = cornerIt->second.first;
		SbVec3f upVector = cornerIt->second.second;
		SbVec3f direction = -position; // Look at origin
		orientation.setValue(upVector, direction);
		return;
	}

	// Default fallback
	LOG_WRN_S("CuteNavCube::calculateCameraPositionForFace: Unknown face name: " + faceName);
	position = SbVec3f(0, 0, 5);
	orientation = SbRotation::identity();
}

// FreeCAD-style direct material color update (no texture switching)
void CuteNavCube::updateFaceMaterialColor(const std::string& faceName, bool isHover) {
	auto materialIt = m_faceMaterials.find(faceName);
	if (materialIt == m_faceMaterials.end()) {
		return; // Face material not found
	}
	
	SoMaterial* material = materialIt->second;
	if (!material) {
		return;
	}
	
	// Get the appropriate color (base or hover)
	SbColor color;
	if (isHover) {
		auto hoverIt = m_faceHoverColors.find(faceName);
		if (hoverIt != m_faceHoverColors.end()) {
			color = hoverIt->second;
		} else {
			return; // Hover color not found
		}
	} else {
		auto baseIt = m_faceBaseColors.find(faceName);
		if (baseIt != m_faceBaseColors.end()) {
			color = baseIt->second;
		} else {
			return; // Base color not found
		}
	}
	
	// Update material diffuse color directly (FreeCAD-style)
	material->diffuseColor.setValue(color);

	auto textureMatIt = m_faceTextureMaterials.find(faceName);
	if (textureMatIt != m_faceTextureMaterials.end() && textureMatIt->second) {
		SoMaterial* textureMaterial = textureMatIt->second;
		textureMaterial->diffuseColor.setValue(color);
		textureMaterial->ambientColor.setValue(color);
		textureMaterial->specularColor.setValue(color);
	}
	
	// Trigger refresh if callback is set
	if (m_refreshCallback) {
		m_refreshCallback();
	}
}

bool CuteNavCube::handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize) {
	std::string eventType = event.Moving() ? "MOVING" : event.Leaving() ? "LEAVING" : event.LeftDown() ? "LEFT_DOWN" : event.LeftUp() ? "LEFT_UP" : "OTHER";

	if (!m_enabled) {
		return false;
	}

	static SbVec2s dragStartPos(0, 0);

	SbVec2s currentPos(
		static_cast<short>(event.GetX()),
		static_cast<short>(event.GetY())
	);

	// Handle mouse movement (hover detection) - FreeCAD-style direct color switching
	// Check for motion events (both Moving() and Dragging())
	if (event.GetEventType() == wxEVT_MOTION) {
		// Convert coordinates for picking - NavigationCubeManager already converted to cube-local coordinates
		// We need to flip Y for OpenGL picking (OpenGL uses bottom-left origin)
		SbVec2s pickPos(currentPos[0], static_cast<short>(viewportSize.y - currentPos[1]));
		std::string hoveredFace = pickRegion(pickPos, viewportSize);

		// Update hover state using direct material color switching (FreeCAD-style)
		if (hoveredFace != m_hoveredFace) {
			// Restore previous face color
			if (!m_hoveredFace.empty()) {
				updateFaceMaterialColor(m_hoveredFace, false);
			}

			// Set new hovered face color
			if (!hoveredFace.empty()) {
				updateFaceMaterialColor(hoveredFace, true);
			}

			m_hoveredFace = hoveredFace;
		}

		// Don't return here - allow click/drag events to be processed
		if (!event.LeftIsDown()) {
			return true; // Hover events are always handled
		}
	}

	// When mouse leaves window, restore all face colors
	if (event.Leaving()) {
		if (!m_hoveredFace.empty()) {
			updateFaceMaterialColor(m_hoveredFace, false);
			m_hoveredFace = "";
		}
		return true; // Mouse leaving is always handled
	}

	if (event.LeftDown()) {
		stopCameraAnimation();
		m_isDragging = true;
		m_lastMousePos = currentPos;
		dragStartPos = currentPos; // Capture position at the start of a potential drag/click
	}
	else if (event.LeftUp()) {
		if (m_isDragging) {
			m_isDragging = false;

			// Calculate distance between mouse down and mouse up to distinguish a click from a drag
			SbVec2s delta = currentPos - dragStartPos;
			float distance = std::sqrt(static_cast<float>(delta[0] * delta[0] + delta[1] * delta[1]));
			static const float clickThreshold = 5.0f; // Max distance for a click

			if (distance < clickThreshold) {
				// It's a click.
				// Invert Y-coordinate for picking, as OpenGL's origin is bottom-left.
				SbVec2s pickPos(currentPos[0], static_cast<short>(viewportSize.y - currentPos[1]));

				std::string region = pickRegion(pickPos, viewportSize);
				if (!region.empty()) {
					// Calculate camera position for clicked face
					SbVec3f cameraPos;
					SbRotation cameraOrient;
					calculateCameraPositionForFace(region, cameraPos, cameraOrient);

					// Get the mapped view name for logging
					auto viewIt = m_faceToView.find(region);
					std::string viewName = (viewIt != m_faceToView.end()) ? viewIt->second : region;

					// Call camera move callback if available, otherwise use view change callback
					if (m_enableAnimation) {
						startCameraAnimation(cameraPos, cameraOrient, region);
					} else {
						if (m_cameraMoveCallback) {
							m_cameraMoveCallback(cameraPos, cameraOrient);
						} else if (m_viewChangeCallback) {
							m_viewChangeCallback(region);
						}
					}
					return true;
				} else {
					return false; // Transparent area, allow ray penetration to outline viewport
				}
			}
		}
	}
	else if (event.Dragging() && m_isDragging) {
		if (m_cameraAnimator && m_cameraAnimator->isAnimating()) {
			stopCameraAnimation();
		}
		SbVec2s delta = currentPos - m_lastMousePos;
		if (delta[0] == 0 && delta[1] == 0) return true; // Ignore no-movement events

		float sensitivity = 1.0f;
		m_rotationY += delta[0] * sensitivity;
		m_rotationX -= delta[1] * sensitivity; // Inverted Y-axis for natural feel

		m_rotationX = (std::max)(-89.0f, (std::min)(89.0f, m_rotationX));

		updateCameraRotation();
		m_lastMousePos = currentPos;

		if (m_rotationChangedCallback) {
			m_rotationChangedCallback();
		}

		return true; // Drag events are always handled
	}

	// Default: event not handled (for transparent areas)
	return false;
}

void CuteNavCube::render(int x, int y, const wxSize& size) {
	if (!m_enabled || !m_root) return;

	// Check if geometry rebuild is needed
	if (m_needsGeometryRebuild) {
		setupGeometry();
		m_needsGeometryRebuild = false;
	}

	// Setup viewport for navigation cube at specified position and size
	SbViewportRegion viewport;
	// Use physical window dimensions for correct coordinate mapping
	// Note: m_windowWidth and m_windowHeight are already in physical pixels from NavigationCubeManager
	viewport.setWindowSize(SbVec2s(static_cast<short>(m_windowWidth), static_cast<short>(m_windowHeight)));

	// Save current position for picking coordinate conversion
	m_currentX = x;
	m_currentY = y;

	// Convert logical coordinates to physical pixels
	// Convert logical coordinates to physical pixels
	// size parameter is the layout size (viewport position and size)
	int xPx = static_cast<int>(x * m_dpiScale);
	int yPx = static_cast<int>(y * m_dpiScale);
	int widthPx = static_cast<int>(size.x * m_dpiScale);
	int heightPx = static_cast<int>(size.y * m_dpiScale);

	// Convert top-left origin (x,y) to bottom-left origin for viewport
	int yBottomPx = m_windowHeight - yPx - heightPx;

	// Set the viewport rectangle where the cube will be rendered (origin bottom-left)
	viewport.setViewportPixels(xPx, yBottomPx, widthPx, heightPx);

	// Clear the viewport area to prevent ghosting/trailing effects during rotation
	glPushAttrib(GL_SCISSOR_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_SCISSOR_TEST);
	glScissor(xPx, yBottomPx, widthPx, heightPx);
	// NOTE: When texture alpha=0 in DECAL mode:
	//   - DECAL mode: When texture alpha=0, FinalColor = MaterialColor (shows body material color)
	//   - When texture alpha=1, FinalColor = TextureColor (shows texture color)
	//   - This ensures alpha=0 displays body material color instead of background color
	// Don't clear color buffer to maintain transparent background, only clear depth buffer
	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	glPopAttrib();

	SoGLRenderAction renderAction(viewport);
	renderAction.setSmoothing(true);
	renderAction.setNumPasses(1);

	// Isolate minimal GL state to avoid interference from main scene render
	GLboolean wasTex2D = glIsEnabled(GL_TEXTURE_2D);
	GLboolean wasBlend = glIsEnabled(GL_BLEND);
	GLboolean wasMSAA = glIsEnabled(GL_MULTISAMPLE);
	GLboolean wasDepthTest = glIsEnabled(GL_DEPTH_TEST);
	GLint prevDepthFunc = GL_LEQUAL;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);
	GLint prevSrc = 0, prevDst = 0;
	glGetIntegerv(GL_BLEND_SRC, &prevSrc);
	glGetIntegerv(GL_BLEND_DST, &prevDst);

	glEnable(GL_TEXTURE_2D);
	// Enable MSAA if available
	glEnable(GL_MULTISAMPLE);
	// Since navigation cube textures are fully opaque (alpha=255), we can use standard blending
	// This ensures correct rendering without showing background through transparent areas
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Ensure depth testing is enabled to prevent rendering through opaque surfaces
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	renderAction.apply(m_root);

	// Restore previous state
	glBlendFunc(prevSrc, prevDst);
	if (!wasBlend) glDisable(GL_BLEND);
	if (!wasTex2D) glDisable(GL_TEXTURE_2D);
	if (!wasMSAA) glDisable(GL_MULTISAMPLE);
	glDepthFunc(prevDepthFunc);
	if (!wasDepthTest) glDisable(GL_DEPTH_TEST);
}

void CuteNavCube::setEnabled(bool enabled) {
	m_enabled = enabled;
}

void CuteNavCube::updateMaterialProperties(const CubeConfig& config) {
	// Find and update material nodes in the scene graph
	if (!m_root) return;
	
	// Update environment lighting
	for (int i = 0; i < m_root->getNumChildren(); i++) {
		SoNode* child = m_root->getChild(i);
		
		// Update environment settings
		if (child->isOfType(SoEnvironment::getClassTypeId())) {
			SoEnvironment* env = static_cast<SoEnvironment*>(child);
			env->ambientIntensity.setValue(m_ambientIntensity);
		}
		
		// Update materials in separators
		if (child->isOfType(SoSeparator::getClassTypeId())) {
			updateSeparatorMaterials(static_cast<SoSeparator*>(child));
		}
	}
	
}

void CuteNavCube::updateSeparatorMaterials(SoSeparator* sep) {
	if (!sep) return;

	// Get the separator name to determine material type
	std::string sepName = sep->getName().getString();

	for (int i = 0; i < sep->getNumChildren(); i++) {
		SoNode* child = sep->getChild(i);

		if (child->isOfType(SoMaterial::getClassTypeId())) {
			SoMaterial* material = static_cast<SoMaterial*>(child);

			// Force opaque and no shading mode
			material->transparency.setValue(0.0f); // Always opaque
			material->shininess.setValue(0.0f); // Always no shading

			// Use unified body color for all materials
			float r = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeBodyDiffuseR", 0.9));
			float g = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeBodyDiffuseG", 0.95));
			float b = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeBodyDiffuseB", 1.0));
			material->diffuseColor.setValue(r, g, b);

			// Update ambient color as well
			float ar = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeBodyAmbientR", 0.7));
			float ag = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeBodyAmbientG", 0.8));
			float ab = static_cast<float>(ConfigManager::getInstance().getDouble("NavigationCube", "CubeBodyAmbientB", 0.9));
			material->ambientColor.setValue(ar, ag, ab);
		}

		// Recursively update nested separators
		if (child->isOfType(SoSeparator::getClassTypeId())) {
			updateSeparatorMaterials(static_cast<SoSeparator*>(child));
		}
	}
}

void CuteNavCube::applyConfig(const CubeConfig& config) {
	// Store previous values to check what changed
	bool geometryChanged = (m_geometrySize != config.cubeSize ||
	                       m_chamferSize != config.chamferSize);
	bool cameraChanged = (m_cameraDistance != config.cameraDistance);
	bool displayChanged = (m_showEdges != config.showEdges ||
	                      m_showCorners != config.showCorners ||
	                      m_showTextures != config.showTextures);
	bool colorChanged = false; // Colors are now read from ConfigManager, not from CubeConfig
	bool materialChanged = (m_transparency != config.transparency ||
	                       m_shininess != config.shininess ||
	                       m_ambientIntensity != config.ambientIntensity);
	bool circleChanged = (m_circleRadius != config.circleRadius ||
	                     m_circleMarginX != config.circleMarginX ||
	                     m_circleMarginY != config.circleMarginY);

	// Update all parameters
	m_geometrySize = config.cubeSize;
	m_chamferSize = config.chamferSize;
	m_cameraDistance = config.cameraDistance;
	m_showEdges = config.showEdges;
	m_showCorners = config.showCorners;
	m_showTextures = config.showTextures;
	m_enableAnimation = config.enableAnimation;
	if (!m_enableAnimation) {
		stopCameraAnimation();
	}

	// Update material properties
	m_transparency = config.transparency;
	m_shininess = config.shininess;
	m_ambientIntensity = config.ambientIntensity;

	// Update circle navigation area
	m_circleRadius = config.circleRadius;
	m_circleMarginX = config.circleMarginX;
	m_circleMarginY = config.circleMarginY;

	// Apply camera distance changes immediately
	if (cameraChanged) {
		updateCameraRotation();
	}

	// Apply material and color changes to existing geometry
	updateMaterialProperties(config);

	// Apply geometry changes if needed (requires rebuild)
	if (geometryChanged || displayChanged || colorChanged || materialChanged || circleChanged) {
		// Update geometry transform immediately if it exists
		if (m_geometryTransform) {
			m_geometryTransform->scaleFactor.setValue(m_geometrySize, m_geometrySize, m_geometrySize);
		}
		// Mark for geometry rebuild on next render
		m_needsGeometryRebuild = true;
	}

}

void CuteNavCube::setCameraPosition(const SbVec3f& position) {
	if (m_orthoCamera) {
		m_orthoCamera->position.setValue(position);
		//LOG_DBG("CuteNavCube::setCameraPosition: Set camera position to x=" + std::to_string(position[0]) +
		// ", y=" + std::to_string(position[1]) + ", z=" + std::to_string(position[2]));
	}
	else {
		LOG_WRN_S("CuteNavCube::setCameraPosition: Camera not initialized");
	}
}

void CuteNavCube::setCameraOrientation(const SbRotation& orientation) {
	if (m_orthoCamera) {
		m_orthoCamera->orientation.setValue(orientation);
		//LOG_DBG("CuteNavCube::setCameraOrientation: Set camera orientation");
	}
	else {
		LOG_WRN_S("CuteNavCube::setCameraOrientation: Camera not initialized");
	}
}

void CuteNavCube::applyInitialTextures() {
	if (!m_textureGenerator) {
		LOG_WRN_S("applyInitialTextures skipped: texture generator is null");
		return;
	}

	std::vector<std::string> mainFaceNames = {"FRONT", "REAR", "LEFT", "RIGHT", "TOP", "BOTTOM"};

	for (const auto& faceName : mainFaceNames) {
		SoTexture2* textureNode = m_textureGenerator->getNormalTexture(faceName);
		if (!textureNode) {
			LOG_WRN_S("applyInitialTextures: missing cached texture for face " + faceName);
			continue;
		}

		std::string separatorKey = faceName + "_Texture";
		auto sepIt = m_faceSeparators.find(separatorKey);
		if (sepIt == m_faceSeparators.end() || sepIt->second == nullptr) {
			LOG_WRN_S("applyInitialTextures: texture separator not found for key " + separatorKey);
			continue;
		}

		SoSeparator* textureSep = sepIt->second;

		// Avoid adding duplicate texture nodes
		bool alreadyAttached = false;
		for (int i = 0; i < textureSep->getNumChildren(); ++i) {
			if (textureSep->getChild(i) == textureNode) {
				alreadyAttached = true;
				break;
			}
		}
		if (alreadyAttached) {
			continue;
		}

		int insertIndex = textureSep->getNumChildren();
		for (int i = 0; i < textureSep->getNumChildren(); ++i) {
			SoNode* child = textureSep->getChild(i);
			if (child->isOfType(SoTextureCoordinate2::getClassTypeId()) ||
				child->isOfType(SoIndexedFaceSet::getClassTypeId())) {
				insertIndex = i;
				break;
			}
		}

		textureSep->insertChild(textureNode, insertIndex);
		LOG_INF_S("applyInitialTextures: attached texture node for face " + faceName + " at index " + std::to_string(insertIndex));
	}
}

void CuteNavCube::startCameraAnimation(const SbVec3f& position, const SbRotation& orientation, const std::string& faceName) {
	if (!m_enableAnimation || !m_cameraAnimator || !m_orthoCamera) {
		if (m_cameraMoveCallback) {
			m_cameraMoveCallback(position, orientation);
		} else if (m_viewChangeCallback) {
			m_viewChangeCallback(faceName);
		}
		return;
	}

	SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_orthoCamera);
	if (!orthoCam) {
		if (m_cameraMoveCallback) {
			m_cameraMoveCallback(position, orientation);
		} else if (m_viewChangeCallback) {
			m_viewChangeCallback(faceName);
		}
		return;
	}

	m_cameraAnimator->stopAnimation();

	CameraAnimation::CameraState startState(
		orthoCam->position.getValue(),
		orthoCam->orientation.getValue(),
		orthoCam->focalDistance.getValue(),
		orthoCam->height.getValue());

	CameraAnimation::CameraState endState(position, orientation, startState.focalDistance, startState.height);

	if (!m_cameraMoveCallback) {
		m_pendingViewName = faceName;
	} else {
		m_pendingViewName.clear();
	}

	m_cameraAnimator->setAnimationType(m_animationType);
	m_cameraAnimator->setCamera(m_orthoCamera);

	if (!m_cameraAnimator->startAnimation(startState, endState, m_animationDuration)) {
		if (m_cameraMoveCallback) {
			m_cameraMoveCallback(position, orientation);
		} else if (m_viewChangeCallback) {
			m_viewChangeCallback(faceName);
		}
	}
}

void CuteNavCube::stopCameraAnimation() {
	if (m_cameraAnimator && m_cameraAnimator->isAnimating()) {
		m_cameraAnimator->stopAnimation();
	}
	m_pendingViewName.clear();
}


