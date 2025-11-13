#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/colour.h>
#include <map>
#include <string>
#include <functional>
#include <Inventor/SbLinear.h>
#include <memory>
#include <Eigen/Dense>
#include <vector>

#include "NavigationCubeGeometryBuilder.h"
#include "NavigationCubeTypes.h" // For ShapeId and PickId enums
#include "CameraAnimation.h"

class NavigationCubeTextureGenerator;
struct CubeConfig;

class CuteNavCube {
public:
	CuteNavCube(std::function<void(const std::string&)> viewChangeCallback, float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config);
	CuteNavCube(std::function<void(const std::string&)> viewChangeCallback,
			   std::function<void(const SbVec3f&, const SbRotation&)> cameraMoveCallback,
			   float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config);
	CuteNavCube(std::function<void(const std::string&)> viewChangeCallback,
			   std::function<void(const SbVec3f&, const SbRotation&)> cameraMoveCallback,
			   std::function<void()> refreshCallback,
			   float dpiScale, int windowWidth, int windowHeight, const CubeConfig& config);
	~CuteNavCube();

	void initialize();
	void updateMaterialProperties(const CubeConfig& config);
	SoSeparator* getRoot() const { return m_root; }
	void setEnabled(bool enabled);
	bool isEnabled() const { return m_enabled; }
	bool handleMouseEvent(const wxMouseEvent& event, const wxSize& viewportSize);
	void render(int x, int y, const wxSize& size);
	void applyConfig(const CubeConfig& config);
	void setWindowSize(int width, int height) {
		m_windowWidth = width;
		m_windowHeight = height;
	}
	SoCamera* getCamera() const { return m_orthoCamera; }
	void setCameraPosition(const SbVec3f& position);
	void setCameraOrientation(const SbRotation& orientation);
	void setRotationChangedCallback(std::function<void()> callback) { m_rotationChangedCallback = callback; }
	void setRefreshCallback(std::function<void()> callback) { m_refreshCallback = callback; }
	void setCanvas(class Canvas* canvas) { m_canvas = canvas; }

	// CuteNavCube specific methods
	void setPosition(int x, int y) { m_positionX = x; m_positionY = y; }
	void getPosition(int& x, int& y) const { x = m_positionX; y = m_positionY; }
	void setSize(int size) { m_cubeSize = size; }
	int getSize() const { return m_cubeSize; }


	// Hover effect configuration
	void setHoverColors(const SbColor& normalColor, const SbColor& hoverColor) {
		m_normalFaceColor = normalColor;
		m_hoverFaceColor = hoverColor;
	}

private:
	void setupGeometry();
	std::string pickRegion(const SbVec2s& mousePos, const wxSize& viewportSize);
	void updateCameraRotation();
	void updateSeparatorMaterials(SoSeparator* sep);
	bool generateFaceTexture(const std::string& text, unsigned char* imageData, int width, int height, const wxColour& bgColor, float faceSize = 0.55f, PickId pickId = PickId::Front);
	static int calculateVerticalBalance(const wxBitmap& bitmap, int fontSizeHint);
	void createCubeFaceTextures();
	std::string getFaceLabel(PickId pickId);
	void regenerateFaceTexture(const std::string& faceName, bool isHover);
	void updateFaceMaterialColor(const std::string& faceName, bool isHover);  // FreeCAD-style direct material color update
	void calculateCameraPositionForFace(const std::string& faceName, SbVec3f& position, SbRotation& orientation) const;
	std::string findFaceFromCameraDirection(const SbVec3f& cameraPos, const SbRotation& cameraOrient) const;
	void generateAndCacheTextures();  // Generate and cache all textures at initialization
	SoTexture2* createTextureForFace(const std::string& faceName, bool isHover);  // Helper to create a texture
	void applyInitialTextures();
	void startCameraAnimation(const SbVec3f& position, const SbRotation& orientation, const std::string& faceName);
	void stopCameraAnimation();

	using FaceData = NavigationCubeGeometryBuilder::FaceData;
	using LabelTextureData = NavigationCubeGeometryBuilder::LabelTextureData;

	// Texture cache entry
	struct TextureData {
		unsigned char* data;
		int width, height;
		TextureData(unsigned char* d, int w, int h) : data(d), width(w), height(h) {}
		~TextureData() { delete[] data; }
	};

	SoSeparator* m_root;
	SoCamera* m_orthoCamera;
	SoDirectionalLight* m_mainLight;
	SoDirectionalLight* m_fillLight;
	SoDirectionalLight* m_sideLight;
	SoTransform* m_cameraTransform;
	SoTransform* m_geometryTransform;
	bool m_enabled;
	float m_dpiScale;
	std::map<std::string, std::string> m_faceToView;
	std::function<void(const std::string&)> m_viewChangeCallback;
	std::function<void(const SbVec3f&, const SbRotation&)> m_cameraMoveCallback;
	std::function<void()> m_rotationChangedCallback;
	std::function<void()> m_refreshCallback;

	// Canvas reference for refresh operations
	class Canvas* m_canvas;
	bool m_isDragging;
	SbVec2s m_lastMousePos;
	float m_rotationX;
	float m_rotationY;
	wxLongLong m_lastDragTime;
	int m_windowWidth;
	int m_windowHeight;

	// CuteNavCube specific members
	int m_positionX;
	int m_positionY;
	int m_cubeSize;

	// Current rendering position (for picking coordinate conversion)
	float m_currentX;
	float m_currentY;
	
	// Configuration properties
	float m_geometrySize;
	float m_chamferSize;
	float m_cameraDistance;
	bool m_needsGeometryRebuild;

	// Display options
	bool m_showEdges;
	bool m_showCorners;
	bool m_showTextures;
	bool m_enableAnimation;

	// Colors - DEPRECATED: Colors are now read directly from ConfigManager
	// These fields are kept for backward compatibility only
	wxColour m_textColor;
	wxColour m_edgeColor;
	wxColour m_cornerColor;

	// Material properties
	float m_transparency;
	float m_shininess;
	float m_ambientIntensity;

	// Circle navigation area
	int m_circleRadius;
	int m_circleMarginX;
	int m_circleMarginY;

	// Hover effect state management
	std::string m_hoveredFace;                                    // Currently hovered face
	std::map<std::string, SoMaterial*> m_faceMaterials;          // Materials for each face
	std::map<std::string, SoSeparator*> m_faceSeparators;        // Separators for each face (for texture replacement)
	std::map<std::string, SbColor> m_faceBaseColors;             // Base color for each face
	std::map<std::string, SbColor> m_faceHoverColors;            // Hover color for each face (FreeCAD-style direct color switching)
	std::map<std::string, SoMaterial*> m_faceTextureMaterials;   // Materials for texture overlays per face
	SbColor m_normalFaceColor;                                   // Normal face color
	SbColor m_hoverFaceColor;                                    // Hover face color
	std::string m_lastLoggedFacing;
	

	// Texture generator
	std::unique_ptr<NavigationCubeTextureGenerator> m_textureGenerator; // Texture generation tool

	// Face normal vectors and center points for camera positioning
	std::map<std::string, std::pair<SbVec3f, SbVec3f>> m_faceNormals;

	// Face vertex data for each pickable region
	std::map<PickId, FaceData> m_Faces;

	// Label texture quad vertices for main faces
	std::map<PickId, LabelTextureData> m_LabelTextures;

	// Font settings like FreeCAD
	float m_fontZoom;

	// Animation
	std::unique_ptr<CameraAnimation> m_cameraAnimator;
	float m_animationDuration;
	CameraAnimation::AnimationType m_animationType;
	std::string m_pendingViewName;
};