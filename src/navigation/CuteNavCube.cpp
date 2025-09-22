#include "CuteNavCube.h"
#include "NavigationCubeConfigDialog.h"
#include "DPIManager.h"
#include "DPIAwareRendering.h"
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
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
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

std::map<std::string, std::shared_ptr<CuteNavCube::TextureData>> CuteNavCube::s_textureCache;

CuteNavCube::CuteNavCube(std::function<void(const std::string&)> viewChangeCallback, float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config)
	: m_root(new SoSeparator)
	, m_orthoCamera(new SoOrthographicCamera)
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
	, m_geometrySize(config.cubeSize > 0.0f ? config.cubeSize : 0.50f)
	, m_chamferSize(config.chamferSize > 0.0f ? config.chamferSize : 0.14f)
	, m_cameraDistance(config.cameraDistance > 0.0f ? config.cameraDistance : 3.5f)
	, m_needsGeometryRebuild(false)
	, m_showEdges(config.showEdges)
	, m_showCorners(config.showCorners)
	, m_showTextures(config.showTextures)
	, m_enableAnimation(config.enableAnimation)
	, m_textColor(config.textColor)
	, m_edgeColor(config.edgeColor)
	, m_cornerColor(config.cornerColor)
	, m_transparency(config.transparency >= 0.0f ? config.transparency : 0.0f)
	, m_shininess(config.shininess >= 0.0f ? config.shininess : 0.5f)
	, m_ambientIntensity(config.ambientIntensity >= 0.0f ? config.ambientIntensity : 0.8f)
	, m_circleRadius(config.circleRadius > 0 ? config.circleRadius : 150)
	, m_circleMarginX(config.circleMarginX >= 0 ? config.circleMarginX : 50)
	, m_circleMarginY(config.circleMarginY >= 0 ? config.circleMarginY : 50)
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
	, m_enabled(true)
	, m_dpiScale(dpiScale)
	, m_viewChangeCallback(viewChangeCallback)
	, m_cameraMoveCallback(cameraMoveCallback)
	, m_rotationChangedCallback(nullptr)
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
	, m_geometrySize(config.cubeSize > 0.0f ? config.cubeSize : 0.50f)
	, m_chamferSize(config.chamferSize > 0.0f ? config.chamferSize : 0.14f)
	, m_cameraDistance(config.cameraDistance > 0.0f ? config.cameraDistance : 3.5f)
	, m_needsGeometryRebuild(false)
	, m_showEdges(config.showEdges)
	, m_showCorners(config.showCorners)
	, m_showTextures(config.showTextures)
	, m_enableAnimation(config.enableAnimation)
	, m_textColor(config.textColor)
	, m_edgeColor(config.edgeColor)
	, m_cornerColor(config.cornerColor)
	, m_transparency(config.transparency >= 0.0f ? config.transparency : 0.0f)
	, m_shininess(config.shininess >= 0.0f ? config.shininess : 0.5f)
	, m_ambientIntensity(config.ambientIntensity >= 0.0f ? config.ambientIntensity : 0.8f)
	, m_circleRadius(config.circleRadius > 0 ? config.circleRadius : 150)
	, m_circleMarginX(config.circleMarginX >= 0 ? config.circleMarginX : 50)
	, m_circleMarginY(config.circleMarginY >= 0 ? config.circleMarginY : 50)
{
	m_root->ref();
	m_orthoCamera->ref(); // Add reference to camera to prevent premature deletion
	initialize();
}

CuteNavCube::~CuteNavCube() {
	m_orthoCamera->unref(); // Release camera reference
	m_root->unref();
}

void CuteNavCube::initialize() {
	setupGeometry();

	m_faceToView = {
		// 6 Main faces
		{ "Front",  "Top" },
		{ "Back",   "Bottom" },
		{ "Left",   "Right" },
		{ "Right",  "Left" },
		{ "Top",    "Front" },
		{ "Bottom", "Back" },

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
		{ "Front",  std::make_pair(SbVec3f(0, 0, 1), SbVec3f(0, 0, 1)) },      // +Z axis
		{ "Back",   std::make_pair(SbVec3f(0, 0, -1), SbVec3f(0, 0, -1)) },    // -Z axis
		{ "Left",   std::make_pair(SbVec3f(-1, 0, 0), SbVec3f(-1, 0, 0)) },   // -X axis
		{ "Right",  std::make_pair(SbVec3f(1, 0, 0), SbVec3f(1, 0, 0)) },    // +X axis
		{ "Top",    std::make_pair(SbVec3f(0, 1, 0), SbVec3f(0, 1, 0)) },    // +Y axis
		{ "Bottom", std::make_pair(SbVec3f(0, -1, 0), SbVec3f(0, -1, 0)) },   // -Y axis

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
}

bool CuteNavCube::generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height, const wxColour& bgColor) {
	wxBitmap bitmap(width, height, 32);
	wxMemoryDC dc;
	dc.SelectObject(bitmap);
	if (!dc.IsOk()) {
		LOG_ERR_S("CuteNavCube::generateFaceTexture: Failed to create wxMemoryDC for texture: " + text);
		// Fallback: Fill with white
		for (int i = 0; i < width * height * 4; i += 4) {
			imageData[i] = 255; // R
			imageData[i + 1] = 255; // G
			imageData[i + 2] = 255; // B
			imageData[i + 3] = 255; // A
		}
		LOG_INF_S("CuteNavCube::generateFaceTexture: Fallback to white texture for: " + text);
		return true;
	}

	dc.SetBackground(wxBrush(bgColor));
	dc.Clear();

	// Use high-quality font rendering for crisp text
	auto& dpiManager = DPIManager::getInstance();
	// Use appropriate font size for high-resolution textures (512x512)
	// The formula width / 4 gives reasonable text size that fits within the texture
	int baseFontSize = std::max(72, static_cast<int>(width / 4.0f)); // More reasonable font size
	LOG_INF_S("CuteNavCube::generateFaceTexture: Starting font setup - text: " + text +
		", width: " + std::to_string(width) + "x" + std::to_string(height) +
		", baseFontSize: " + std::to_string(baseFontSize) +
		", DPI scale: " + std::to_string(dpiManager.getDPIScale()));

	wxFont font(baseFontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Arial");
	font.SetPointSize(baseFontSize); // Set font size directly

	LOG_INF_S("CuteNavCube::generateFaceTexture: Font setup - width: " + std::to_string(width) +
		", baseFontSize: " + std::to_string(baseFontSize) +
		", font point size: " + std::to_string(font.GetPointSize()));

	dc.SetFont(font);
	dc.SetTextForeground(wxColour(0, 0, 0)); // Black text for high contrast

	wxSize textSize = dc.GetTextExtent(text);
	LOG_INF_S("CuteNavCube::generateFaceTexture: Text metrics - text: " + text +
		", textSize: " + std::to_string(textSize.GetWidth()) + "x" + std::to_string(textSize.GetHeight()) +
		", font height: " + std::to_string(font.GetPointSize()) +
		", actual text height: " + std::to_string(textSize.GetHeight()));
	int x = (width - textSize.GetWidth()) / 2;
	int y = (height - textSize.GetHeight()) / 2;
	dc.DrawText(text, x, y);

	// Validate bitmap content
	wxImage image = bitmap.ConvertToImage();
	if (!image.IsOk()) {
		LOG_ERR_S("CuteNavCube::generateFaceTexture: Failed to convert bitmap to image for texture: " + text);
		// Fallback: Fill with white
		for (int i = 0; i < width * height * 4; i += 4) {
			imageData[i] = 255; // R
			imageData[i + 1] = 255; // G
			imageData[i + 2] = 255; // B
			imageData[i + 3] = 255; // A
		}
		LOG_INF_S("CuteNavCube::generateFaceTexture: Fallback to white texture for: " + text);
		return true;
	}

	image.InitAlpha(); // Ensure alpha channel
	unsigned char* rgb = image.GetData();
	unsigned char* alpha = image.GetAlpha();

	// Copy to imageData (RGBA) and validate
	bool hasValidPixels = false;
	for (int i = 0, j = 0; i < width * height * 4; i += 4, j += 3) {
		imageData[i] = rgb[j];     // R
		imageData[i + 1] = rgb[j + 1]; // G
		imageData[i + 2] = rgb[j + 2]; // B
		imageData[i + 3] = alpha[j / 3]; // A
		if (imageData[i] != 0 || imageData[i + 1] != 0 || imageData[i + 2] != 0) {
			hasValidPixels = true;
		}
	}

	if (!hasValidPixels) {
		LOG_WRN_S("CuteNavCube::generateFaceTexture: All pixels are black for texture: " + text);
		// Fallback: Fill with white
		for (int i = 0; i < width * height * 4; i += 4) {
			imageData[i] = 255; // R
			imageData[i + 1] = 255; // G
			imageData[i + 2] = 255; // B
			imageData[i + 3] = 255; // A
		}
		LOG_INF_S("CuteNavCube::generateFaceTexture: Fallback to white texture for: " + text);
	}

	LOG_INF_S("CuteNavCube::generateFaceTexture: Texture generated for " + text +
		", size=" + std::to_string(width) + "x" + std::to_string(height));
	return true;
}

void CuteNavCube::setupGeometry() {
	// Safely clear previous geometry while preserving camera
	bool cameraWasInScene = false;
	if (m_root->getNumChildren() > 0) {
		// Check if camera is already in the scene
		for (int i = 0; i < m_root->getNumChildren(); i++) {
			if (m_root->getChild(i) == m_orthoCamera) {
				cameraWasInScene = true;
				break;
			}
		}
	m_root->removeAllChildren(); // Clear previous geometry
	}

	// Setup camera properties
	m_orthoCamera->viewportMapping = SoOrthographicCamera::ADJUST_CAMERA;
	m_orthoCamera->nearDistance = 0.05f; // Reduced to ensure all faces are visible
	m_orthoCamera->farDistance = 15.0f;  // Increased to include all geometry
	m_orthoCamera->position.setValue(0.0f, 0.0f, 5.0f); // Initial position
	m_orthoCamera->orientation.setValue(SbRotation(SbVec3f(1, 0, 0), -M_PI / 2)); // Rotate to make Z up
	
	// Always add camera back to the scene
	m_root->addChild(m_orthoCamera);

	// --- Lighting Setup ---
	SoEnvironment* env = new SoEnvironment;
	env->ambientColor.setValue(0.8f, 0.8f, 0.85f); // Brighter and more neutral ambient color
	env->ambientIntensity.setValue(m_ambientIntensity); // Use config value
	m_root->addChild(env);

	m_mainLight = new SoDirectionalLight;
	m_mainLight->direction.setValue(0.5f, 0.5f, -0.5f); // Main light from top-right-front
	m_mainLight->intensity.setValue(0.4f); // Reduced from 0.6
	m_mainLight->color.setValue(1.0f, 1.0f, 1.0f);
	m_root->addChild(m_mainLight);

	m_fillLight = new SoDirectionalLight;
	m_fillLight->direction.setValue(-0.5f, -0.5f, 0.5f); // Fill light from bottom-left-back
	m_fillLight->intensity.setValue(0.4f); // Reduced from 0.6
	m_fillLight->color.setValue(0.95f, 0.95f, 1.0f); // Slightly cool
	m_root->addChild(m_fillLight);

	m_sideLight = new SoDirectionalLight;
	m_sideLight->direction.setValue(-0.8f, 0.2f, -0.3f); // Side light from left
	m_sideLight->intensity.setValue(0.3f); // Reduced from 0.4
	m_sideLight->color.setValue(1.0f, 1.0f, 0.95f); // Slightly warm
	m_root->addChild(m_sideLight);

	// --- Add more lights for better coverage ---
	SoDirectionalLight* backLight = new SoDirectionalLight;
	backLight->direction.setValue(0.0f, 0.0f, 1.0f); // Directly from back
	backLight->intensity.setValue(0.3f); // Reduced from 0.5
	backLight->color.setValue(0.9f, 0.9f, 1.0f);
	m_root->addChild(backLight);

	SoDirectionalLight* bottomLight = new SoDirectionalLight;
	bottomLight->direction.setValue(0.4f, -0.8f, 0.2f); // From bottom-right
	bottomLight->intensity.setValue(0.2f); // Reduced from 0.4
	bottomLight->color.setValue(1.0f, 0.95f, 0.95f);
	m_root->addChild(bottomLight);

	SoDirectionalLight* topSideLight = new SoDirectionalLight;
	topSideLight->direction.setValue(0.8f, 0.3f, 0.3f); // From top-left
	topSideLight->intensity.setValue(0.2f); // Reduced from 0.4
	topSideLight->color.setValue(0.95f, 1.0f, 0.95f);
	m_root->addChild(topSideLight);

	updateCameraRotation();

	SoSeparator* cubeAssembly = new SoSeparator;

	// --- Manual Chamfered Cube Definition with Correct Normals ---
	const float s = m_geometrySize;   // Main size (configurable)
	const float c = m_chamferSize;    // Chamfer size (configurable)

	SbVec3f vertices[24] = {
		// Corner 0 (+ + +): Front-Top-Right
		SbVec3f(s,  s - c,  s - c),  // v0
		SbVec3f(s - c,  s,  s - c),  // v1
		SbVec3f(s - c,  s - c,  s),  // v2

		// Corner 1 (- + +): Front-Top-Left
		SbVec3f(-s,  s - c,  s - c),  // v3
		SbVec3f(-s + c,  s,  s - c),  // v4
		SbVec3f(-s + c,  s - c,  s),  // v5

		// Corner 2 (- - +): Front-Bottom-Left
		SbVec3f(-s, -s + c,  s - c),  // v6
		SbVec3f(-s + c, -s,  s - c),  // v7
		SbVec3f(-s + c, -s + c,  s),  // v8

		// Corner 3 (+ - +): Front-Bottom-Right
		SbVec3f(s, -s + c,  s - c),  // v9
		SbVec3f(s - c, -s,  s - c),  // v10
		SbVec3f(s - c, -s + c,  s),  // v11

		// Corner 4 (+ + -): Back-Top-Right
		SbVec3f(s,  s - c, -s + c),  // v12
		SbVec3f(s - c,  s, -s + c),  // v13
		SbVec3f(s - c,  s - c, -s),  // v14

		// Corner 5 (- + -): Back-Top-Left
		SbVec3f(-s,  s - c, -s + c),  // v15
		SbVec3f(-s + c,  s, -s + c),  // v16
		SbVec3f(-s + c,  s - c, -s),  // v17

		// Corner 6 (- - -): Back-Bottom-Left
		SbVec3f(-s, -s + c, -s + c),  // v18
		SbVec3f(-s + c, -s, -s + c),  // v19
		SbVec3f(-s + c, -s + c, -s),  // v20

		// Corner 7 (+ - -): Back-Bottom-Right
		SbVec3f(s, -s + c, -s + c),  // v21
		SbVec3f(s - c, -s, -s + c),  // v22
		SbVec3f(s - c, -s + c, -s)   // v23
	};

	struct FaceDefinition {
		std::string name;
		std::vector<int> indices;
		std::string textureKey;
		int materialType; // 0=main face, 1=edge, 2=corner
	};

	std::vector<FaceDefinition> faces = {
		// 6 Main faces
		{"Top",    {13, 16, 4, 1}, "Front", 0},
		{"Bottom", {19, 22, 10, 7}, "Back", 0},
		{"Front",  {2, 5, 8, 11}, "Top", 0},
		{"Back",   {14, 23, 20, 17}, "Bottom", 0},
		{"Right",  {9, 21, 12, 0}, "Left", 0},
		{"Left",   {3, 15, 18, 6}, "Right", 0},

		// 8 Corner faces (triangular)
		{"Corner0", {0, 1, 2}, "", 2},            // Face 6
		{"Corner1", {3, 5, 4}, "", 2},            // Face 7
		{"Corner2", {6, 7, 8}, "", 2},            // Face 8
		{"Corner3", {9, 11, 10}, "", 2},          // Face 9
		{"Corner4", {12, 14, 13}, "", 2},         // Face 10
		{"Corner5", {15, 16, 17}, "", 2},         // Face 11
		{"Corner6", {20, 19, 18}, "", 2},         // Face 12
		{"Corner7", {22, 23, 21}, "", 2},         // Face 13

		// 12 Edge faces (rectangular)
		{"EdgeTF", {1, 4, 5, 2}, "", 1},          // Face 14: Top-Front edge
		{"EdgeTR", {13, 1, 0, 12}, "", 1},        // Face 15: Top-Right edge
		{"EdgeTB", {16, 13, 14, 17}, "", 1},      // Face 16: Top-Back edge
		{"EdgeTL", {4, 16, 15, 3}, "", 1},        // Face 17: Top-Left edge
		{"EdgeBF", {7, 10, 11, 8}, "", 1},        // Face 18: Bottom-Front edge
		{"EdgeBR", {10, 22, 21, 9}, "", 1},       // Face 19: Bottom-Right edge
		{"EdgeBB", {20, 23, 22, 19}, "", 1},      // Face 20: Bottom-Back edge
		{"EdgeBL", {6, 18, 19, 7}, "", 1},        // Face 21: Bottom-Left edge
		{"EdgeFR", {2, 11, 9, 0}, "", 1},         // Face 22: Front-Right edge
		{"EdgeFL", {3, 6, 8, 5}, "", 1},          // Face 23: Front-Left edge
		{"EdgeBL2", {17, 20, 18, 15}, "", 1},     // Face 24: Back-Left edge
		{"EdgeBR2", {12, 21, 23, 14}, "", 1}      // Face 25: Back-Right edge
	};

	auto& dpiManager = DPIManager::getInstance();

	SoTextureCoordinate2* texCoords = new SoTextureCoordinate2;
	texCoords->point.setValues(0, 4, new SbVec2f[4]{
		SbVec2f(0, 0), SbVec2f(1, 0), SbVec2f(1, 1), SbVec2f(0, 1)
		});
	cubeAssembly->addChild(texCoords);

	SoCoordinate3* coords = new SoCoordinate3;
	coords->point.setNum(24);
	for (int i = 0; i < 24; i++) {
		coords->point.set1Value(i, vertices[i]);
	}
	cubeAssembly->addChild(coords);

	LOG_INF_S("=== Manual Face Definition Analysis ===");
	int faceIndex = 0;

	SoMaterial* mainFaceMaterial = new SoMaterial;
	// Use text color for main faces
	float r = m_textColor.Red() / 255.0f;
	float g = m_textColor.Green() / 255.0f;
	float b = m_textColor.Blue() / 255.0f;
	mainFaceMaterial->diffuseColor.setValue(r, g, b);
	mainFaceMaterial->specularColor.setValue(0.8f, 0.8f, 0.8f);
	mainFaceMaterial->shininess.setValue(m_shininess);
	mainFaceMaterial->transparency.setValue(m_transparency);

	SoMaterial* edgeAndCornerMaterial = new SoMaterial;
	// Use edge color for edge and corner faces
	r = m_edgeColor.Red() / 255.0f;
	g = m_edgeColor.Green() / 255.0f;
	b = m_edgeColor.Blue() / 255.0f;
	edgeAndCornerMaterial->diffuseColor.setValue(r, g, b);
	edgeAndCornerMaterial->specularColor.setValue(0.8f, 0.8f, 0.8f);
	edgeAndCornerMaterial->shininess.setValue(m_shininess);
	edgeAndCornerMaterial->transparency.setValue(m_transparency);

	// --- Pre-generate high-quality textures for edges and corners ---
	// Use DPI manager for optimal texture resolution (already declared above)
	const int baseTexSize = 512; // High-resolution base texture size
	const int texWidth = dpiManager.getScaledTextureSize(baseTexSize);
	const int texHeight = dpiManager.getScaledTextureSize(baseTexSize);
	LOG_INF_S("CuteNavCube::setupGeometry: Texture size - base: " + std::to_string(baseTexSize) +
		", scaled: " + std::to_string(texWidth) + "x" + std::to_string(texHeight) +
		", DPI scale: " + std::to_string(dpiManager.getDPIScale()));

	SoTexture2* whiteTexture = nullptr;
	std::vector<unsigned char> whiteImageData(texWidth * texHeight * 4);
	if (generateFaceTexture("", whiteImageData.data(), texWidth, texHeight, wxColour(255, 255, 255, 160))) {
		whiteTexture = new SoTexture2;
		whiteTexture->image.setValue(SbVec2s(texWidth, texHeight), 4, whiteImageData.data());
		whiteTexture->model = SoTexture2::DECAL;
	}

	SoTexture2* greyTexture = nullptr;
	std::vector<unsigned char> greyImageData(texWidth * texHeight * 4);
	if (generateFaceTexture("", greyImageData.data(), texWidth, texHeight, wxColour(180, 180, 180, 160))) {
		greyTexture = new SoTexture2;
		greyTexture->image.setValue(SbVec2s(texWidth, texHeight), 4, greyImageData.data());
		greyTexture->model = SoTexture2::DECAL;
	}

	LOG_INF_S("--- Logging Face Properties ---");
	for (const auto& faceDef : faces) {
		// Skip faces based on display options
		if (faceDef.materialType == 1 && !m_showEdges) continue;    // Skip edge faces if edges are disabled
		if (faceDef.materialType == 2 && !m_showCorners) continue;  // Skip corner faces if corners are disabled
		std::string indices_str;
		for (int idx : faceDef.indices) { indices_str += std::to_string(idx) + " "; }

		SoSeparator* faceSep = new SoSeparator;
		faceSep->setName(SbName(faceDef.name.c_str()));

		SoIndexedFaceSet* face = new SoIndexedFaceSet;
		for (size_t i = 0; i < faceDef.indices.size(); i++) {
			face->coordIndex.set1Value(i, faceDef.indices[i]);
		}
		face->coordIndex.set1Value(faceDef.indices.size(), -1); // End marker

		if (faceDef.materialType == 0) { // Main face
			faceSep->addChild(mainFaceMaterial);
			if (m_showTextures) {
				// Use DPI manager for dynamic texture resolution (already declared above)
				int faceTexWidth = dpiManager.getScaledTextureSize(baseTexSize);
				int faceTexHeight = dpiManager.getScaledTextureSize(baseTexSize);
				std::vector<unsigned char> imageData(faceTexWidth * faceTexHeight * 4);
				if (generateFaceTexture(faceDef.textureKey, imageData.data(), faceTexWidth, faceTexHeight, wxColour(255, 255, 255, 160))) {
				SoTexture2* texture = new SoTexture2;
					texture->image.setValue(SbVec2s(faceTexWidth, faceTexHeight), 4, imageData.data());
				texture->model = SoTexture2::DECAL;
				faceSep->addChild(texture);
				}
			}

			if (faceDef.textureKey == "Back") {
				// Special UV mapping for the 'Back' face to correct its orientation
				face->textureCoordIndex.set1Value(0, 0);
				face->textureCoordIndex.set1Value(1, 1);
				face->textureCoordIndex.set1Value(2, 2);
				face->textureCoordIndex.set1Value(3, 3);
			}
			else {
				// Default UV mapping for other main faces
				face->textureCoordIndex.set1Value(0, 1);
				face->textureCoordIndex.set1Value(1, 0);
				face->textureCoordIndex.set1Value(2, 3);
				face->textureCoordIndex.set1Value(3, 2);
			}
			face->textureCoordIndex.set1Value(4, -1);
		}
		else { // Edges and Corners
			faceSep->addChild(edgeAndCornerMaterial);
			if (m_showTextures) {
			if (faceDef.materialType == 1) { // Edge
				if (greyTexture) faceSep->addChild(greyTexture);
			}
			else { // Corner (materialType == 2)
				if (whiteTexture) faceSep->addChild(whiteTexture);
				}
			}

			if (faceDef.indices.size() == 4) { // Edge faces (quads)
				face->textureCoordIndex.set1Value(0, 0);
				face->textureCoordIndex.set1Value(1, 1);
				face->textureCoordIndex.set1Value(2, 2);
				face->textureCoordIndex.set1Value(3, 3);
				face->textureCoordIndex.set1Value(4, -1);
			}
			else if (faceDef.indices.size() == 3) { // Corner faces (triangles)
				face->textureCoordIndex.set1Value(0, 0);
				face->textureCoordIndex.set1Value(1, 1);
				face->textureCoordIndex.set1Value(2, 3);
				face->textureCoordIndex.set1Value(3, -1);
			}
		}

		faceSep->addChild(face);

		cubeAssembly->addChild(faceSep);

		faceIndex++;
	}

	m_root->addChild(cubeAssembly);

	LOG_INF_S("CuteNavCube::setupGeometry: Total faces analyzed: " + std::to_string(faceIndex));
	LOG_INF_S("CuteNavCube::setupGeometry: Manual geometry definition with verified normals.");

	// --- Add black outlines to all faces ---
	SoSeparator* outlineSep = new SoSeparator;

	// Enable line smoothing for anti-aliasing
	SoShapeHints* hints = new SoShapeHints;
	hints->shapeType = SoShapeHints::SOLID;
	hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
	outlineSep->addChild(hints);

	// Define the style for the outlines
	SoDrawStyle* drawStyle = new SoDrawStyle;
	drawStyle->style = SoDrawStyle::LINES;
	drawStyle->lineWidth = 1.0f;
	outlineSep->addChild(drawStyle);

	// Define the material for the outlines
	SoMaterial* outlineMaterial = new SoMaterial;
	outlineMaterial->diffuseColor.setValue(0.0f, 0.0f, 0.0f); // Black
	outlineSep->addChild(outlineMaterial);

	// Re-use the same coordinates but re-draw all faces as lines
	outlineSep->addChild(coords); // Re-use the same SoCoordinate3 node

	SoIndexedFaceSet* outlineFaceSet = new SoIndexedFaceSet;
	std::vector<int32_t> all_indices;
	for (const auto& faceDef : faces) {
		for (int index : faceDef.indices) {
			all_indices.push_back(index);
		}
		all_indices.push_back(-1); // Separator for each face
	}
	outlineFaceSet->coordIndex.setValues(0, all_indices.size(), all_indices.data());
	outlineSep->addChild(outlineFaceSet);

	m_root->addChild(outlineSep);

	LOG_INF_S("CuteNavCube::setupGeometry: Final rebuild of cube geometry with definitive winding order and outlines.");
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
	SoRayPickAction pickAction(SbViewportRegion(viewportSize.x, viewportSize.y));
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
				LOG_INF_S("CuteNavCube::pickRegion: Picked face: " + nameStr + ", maps to view: " + viewIt->second);
				return nameStr; // Return the actual face name, not the mapped view
			}
		}
	}

	return "";
}

// Calculate camera position based on clicked face
void CuteNavCube::calculateCameraPositionForFace(const std::string& faceName, SbVec3f& position, SbRotation& orientation) const {
	// Handle main faces (6 faces) - use standard positioning
	static const std::set<std::string> mainFaces = {"Front", "Back", "Left", "Right", "Top", "Bottom"};
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

void CuteNavCube::handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize) {
	if (!m_enabled) return;

	static SbVec2s dragStartPos(0, 0);

	SbVec2s currentPos(
		static_cast<short>(event.GetX()),
		static_cast<short>(event.GetY())
	);

	if (event.LeftDown()) {
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
					if (m_cameraMoveCallback) {
						m_cameraMoveCallback(cameraPos, cameraOrient);
						LOG_INF_S("CuteNavCube::handleMouseEvent: Clicked face: " + region +
							", mapped to view: " + viewName +
							", moved camera to position: (" +
							std::to_string(cameraPos[0]) + ", " +
							std::to_string(cameraPos[1]) + ", " +
							std::to_string(cameraPos[2]) + ")");
					} else if (m_viewChangeCallback) {
						m_viewChangeCallback(region); // Pass face name to callback
						LOG_INF_S("CuteNavCube::handleMouseEvent: Clicked face: " + region +
							", mapped to view: " + viewName);
					}
				}
			}
		}
	}
	else if (event.Dragging() && m_isDragging) {
		SbVec2s delta = currentPos - m_lastMousePos;
		if (delta[0] == 0 && delta[1] == 0) return; // Ignore no-movement events

		float sensitivity = 1.0f;
		m_rotationY += delta[0] * sensitivity;
		m_rotationX -= delta[1] * sensitivity; // Inverted Y-axis for natural feel

		m_rotationX = (std::max)(-89.0f, (std::min)(89.0f, m_rotationX));

		updateCameraRotation();
		m_lastMousePos = currentPos;

		if (m_rotationChangedCallback) {
			m_rotationChangedCallback();
		}

		//LOG_DBG("CuteNavCube::handleMouseEvent: Rotated: X=" + std::to_string(m_rotationX) +
		//    ", Y=" + std::to_string(m_rotationY));
	}
}

void CuteNavCube::render(int x, int y, const wxSize& size) {
	if (!m_enabled || !m_root) return;

	// Check if geometry rebuild is needed
	if (m_needsGeometryRebuild) {
		setupGeometry();
		m_needsGeometryRebuild = false;
		LOG_INF_S(std::string("CuteNavCube::render: Rebuilt geometry with new parameters - ") +
			"geometrySize: " + std::to_string(m_geometrySize) +
			", chamferSize: " + std::to_string(m_chamferSize) +
			", cameraDistance: " + std::to_string(m_cameraDistance) +
			", showEdges: " + std::string(m_showEdges ? "true" : "false") +
			", showCorners: " + std::string(m_showCorners ? "true" : "false") +
			", showTextures: " + std::string(m_showTextures ? "true" : "false"));
	}

	// Only log when position actually changes or for debugging specific issues
	static int lastX = -1, lastY = -1;
	static int lastWidth = -1, lastHeight = -1;
	static float lastDpiScale = -1.0f;

	bool positionChanged = (x != lastX || y != lastY || size.x != lastWidth || size.y != lastHeight || m_dpiScale != lastDpiScale);

	if (positionChanged) {
		LOG_INF_S("CuteNavCube::render: Rendering cube at logical position: x=" + std::to_string(x) +
			", y=" + std::to_string(y) + ", size=" + std::to_string(size.x) + "x" + std::to_string(size.y) +
			", physical position: " + std::to_string(x * m_dpiScale) + "x" +
			std::to_string(y * m_dpiScale) + " to " +
			std::to_string((x + size.x) * m_dpiScale) + "x" +
			std::to_string((y + size.y) * m_dpiScale) +
			", window: " + std::to_string(m_windowWidth * m_dpiScale) + "x" +
			std::to_string(m_windowHeight * m_dpiScale) + ", dpiScale: " + std::to_string(m_dpiScale));

		lastX = x;
		lastY = y;
		lastWidth = size.x;
		lastHeight = size.y;
		lastDpiScale = m_dpiScale;
	}

	// Setup viewport for navigation cube at specified position and size
	SbViewportRegion viewport;
	// Use physical window dimensions for correct coordinate mapping
	// Note: m_windowWidth and m_windowHeight are already in physical pixels from NavigationCubeManager
	viewport.setWindowSize(SbVec2s(static_cast<short>(m_windowWidth), static_cast<short>(m_windowHeight)));

	// Convert logical coordinates to physical pixels
	int xPx = static_cast<int>(x * m_dpiScale);
	int yPx = static_cast<int>(y * m_dpiScale);
	int widthPx = static_cast<int>(size.x * m_dpiScale);
	int heightPx = static_cast<int>(size.y * m_dpiScale);

	// Convert top-left origin (x,y) to bottom-left origin for viewport
	int yBottomPx = m_windowHeight - yPx - heightPx;

	// Set the viewport rectangle where the cube will be rendered (origin bottom-left)
	viewport.setViewportPixels(xPx, yBottomPx, widthPx, heightPx);

	SoGLRenderAction renderAction(viewport);
	renderAction.setSmoothing(true);
	renderAction.setNumPasses(1);

	// Isolate minimal GL state to avoid interference from main scene render
	GLboolean wasTex2D = glIsEnabled(GL_TEXTURE_2D);
	GLboolean wasBlend = glIsEnabled(GL_BLEND);
	GLboolean wasMSAA = glIsEnabled(GL_MULTISAMPLE);
	GLint prevSrc = 0, prevDst = 0;
	glGetIntegerv(GL_BLEND_SRC, &prevSrc);
	glGetIntegerv(GL_BLEND_DST, &prevDst);

	glEnable(GL_TEXTURE_2D);
	// Enable MSAA if available
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderAction.apply(m_root);

	// Restore previous state
	glBlendFunc(prevSrc, prevDst);
	if (!wasBlend) glDisable(GL_BLEND);
	if (!wasTex2D) glDisable(GL_TEXTURE_2D);
	if (!wasMSAA) glDisable(GL_MULTISAMPLE);
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
	
	LOG_INF_S("CuteNavCube::updateMaterialProperties: Updated material properties");
}

void CuteNavCube::updateSeparatorMaterials(SoSeparator* sep) {
	if (!sep) return;

	// Get the separator name to determine material type
	std::string sepName = sep->getName().getString();

	for (int i = 0; i < sep->getNumChildren(); i++) {
		SoNode* child = sep->getChild(i);

		if (child->isOfType(SoMaterial::getClassTypeId())) {
			SoMaterial* material = static_cast<SoMaterial*>(child);

			// Apply transparency
			material->transparency.setValue(m_transparency);

			// Apply shininess
			material->shininess.setValue(m_shininess);

			// Update colors based on material context
			float r, g, b;
			if (sepName.find("Edge") != std::string::npos) {
				// Edge material
				r = m_edgeColor.Red() / 255.0f;
				g = m_edgeColor.Green() / 255.0f;
				b = m_edgeColor.Blue() / 255.0f;
			}
			else if (sepName.find("Corner") != std::string::npos) {
				// Corner material
				r = m_cornerColor.Red() / 255.0f;
				g = m_cornerColor.Green() / 255.0f;
				b = m_cornerColor.Blue() / 255.0f;
			}
			else {
				// Main face material - using text color for main faces
				r = m_textColor.Red() / 255.0f;
				g = m_textColor.Green() / 255.0f;
				b = m_textColor.Blue() / 255.0f;
			}
			material->diffuseColor.setValue(r, g, b);
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
	bool colorChanged = (m_textColor.GetRGB() != config.textColor.GetRGB() ||
	                    m_edgeColor.GetRGB() != config.edgeColor.GetRGB() ||
	                    m_cornerColor.GetRGB() != config.cornerColor.GetRGB());
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
	m_textColor = config.textColor;
	m_edgeColor = config.edgeColor;
	m_cornerColor = config.cornerColor;

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
		// Mark for geometry rebuild on next render
		m_needsGeometryRebuild = true;
	}

	LOG_INF_S("CuteNavCube::applyConfig: Applied configuration - size=" +
		std::to_string(m_geometrySize) + ", chamfer=" + std::to_string(m_chamferSize) +
		", distance=" + std::to_string(m_cameraDistance) +
		", showEdges=" + (m_showEdges ? "true" : "false") +
		", showCorners=" + (m_showCorners ? "true" : "false") +
		", showTextures=" + (m_showTextures ? "true" : "false"));
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