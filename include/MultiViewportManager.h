#pragma once

#include <wx/glcanvas.h>
#include <wx/gdicmn.h>
#include <memory>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <Inventor/nodes/SoSeparator.h>
#include "interfaces/IMultiViewportManager.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/SbName.h>
#include <wx/menu.h>
#include "CoordinateTransformer.h"
#include "ViewportConfig.h"
#include "logger/Logger.h"

// Define viewport logging macros
#ifdef DEBUG_VIEWPORT_LOGS
	#define LOG_VIEWPORT_DBG(msg) LOG_DBG_S(msg)
	#define LOG_VIEWPORT_TRACE(msg) LOG_INF_S(msg)
#else
	#define LOG_VIEWPORT_DBG(msg) do {} while(0)
	#define LOG_VIEWPORT_TRACE(msg) do {} while(0)
#endif

#define LOG_VIEWPORT_ERR(msg) LOG_ERR_S(msg)
#define LOG_VIEWPORT_WRN(msg) LOG_WRN_S(msg)
#define LOG_VIEWPORT_INFO(msg) LOG_INF_S(msg)

// Forward declarations
class SoPickedPoint;
class SoPath;

class Canvas;
class SceneManager;
class NavigationCubeManager;

struct ViewportInfo {
	int x, y, width, height;
	bool enabled;

	ViewportInfo() : x(0), y(0), width(100), height(100), enabled(true) {}
	ViewportInfo(int _x, int _y, int _w, int _h, bool _enabled = true)
		: x(_x), y(_y), width(_w), height(_h), enabled(_enabled) {
	}
};

class MultiViewportManager : public IMultiViewportManager {
public:
	enum ViewportType {
		VIEWPORT_NAVIGATION_CUBE = 0,
		VIEWPORT_CUBE_OUTLINE = 1,
		VIEWPORT_COORDINATE_SYSTEM = 2,
		VIEWPORT_COUNT = 3
	};

public:
	MultiViewportManager(Canvas* canvas, SceneManager* sceneManager);
	~MultiViewportManager();

	// Core functionality
	void render() override;
	void handleSizeChange(const wxSize& canvasSize) override;
	void handleDPIChange() override;
	bool handleMouseEvent(wxMouseEvent& event) override;

	// Viewport management
	void setViewportEnabled(ViewportType type, bool enabled);
	bool isViewportEnabled(ViewportType type) const;
	void setViewportRect(ViewportType type, int x, int y, int width, int height);
	ViewportInfo getViewportInfo(ViewportType type) const;

	// Navigation cube integration
	void setNavigationCubeManager(NavigationCubeManager* manager) override;

private:
	void syncCoordinateSystemCameraToMain();
	void initializeViewports();
	void createCubeOutlineScene();
	void createCoordinateSystemScene();
	void updateViewportLayouts(const wxSize& canvasSize);

	void renderNavigationCube();
	void renderCubeOutline();
	void renderCoordinateSystem();
	
	// Generic viewport rendering function
	void renderViewport(const ViewportInfo& viewport, SoSeparator* root);

	void setViewport(const ViewportInfo& viewport);
	void syncCameraWithMain(SoCamera* targetCamera);

	// Navigation shapes creation methods
	void createNavigationShapes();
	void createTopArrow();
	void createSideArrows();
	void createBottomTriangle();
	void createTopRightCircle(float scale);
	void createLeftRightTriangles(float scale);
	void createSmallCube(float scale);
	void createCurvedArrow(int dir, float scale);
	void createSideTriangle(int dir);
	void createEquilateralTriangle(float x, float y, float angleRad);

	// Event handling methods
	static void onMouseEvent(void* userData, SoEventCallback* node);
	void handleShapeClick(const std::string& shapeName);
	void handleShapeHover(const std::string& shapeName, bool isHovering);
	void addEventCallbackToShape(SoSeparator* shapeRoot, const std::string& shapeName);
	std::string findShapeNameFromPath(SoPath* path);
	
	// Popup menu methods
	void showCubeContextMenu(const wxPoint& screenPos);
	void showSphereContextMenu(const wxPoint& screenPos);
	void onMenuResetView(wxCommandEvent& event);
	void onMenuToggleVisibility(wxCommandEvent& event);
	void onMenuCubeSettings(wxCommandEvent& event);
	void onMenuViewBookmark(wxCommandEvent& event);
	void onMenuZoom(wxCommandEvent& event);
	void onMenuAnimationType(wxCommandEvent& event);
	
	// Hover effect methods
	void updateCubeHoverState(bool isHovering);
	void setCubeMaterialColor(const SbColor& color);
	void updateShapeHoverState(const std::string& shapeName, bool isHovering);
	void setShapeMaterialColor(SoMaterial* material, const SbColor& color);
	void updateArrowHeadMaterials(SoSeparator* arrowNode, const SbColor& color);

	Canvas* m_canvas;
	SceneManager* m_sceneManager;
	NavigationCubeManager* m_navigationCubeManager;

	ViewportInfo m_viewports[VIEWPORT_COUNT];

	// Scene graphs for additional viewports
	SoSeparator* m_cubeOutlineRoot;
	SoSeparator* m_coordinateSystemRoot;

	// Cameras for additional viewports
	SoOrthographicCamera* m_cubeOutlineCamera;
	SoOrthographicCamera* m_coordinateSystemCamera;

	// Layout parameters
	int m_margin;
	float m_dpiScale;
	bool m_initialized;  // Add this flag
	
	// Coordinate transformation helper
	std::unique_ptr<CoordinateTransformer> m_coordTransformer;

	// Shape name mapping for click detection
	std::map<std::string, std::string> m_shapeNames; // position -> shape name

	// Composite shape management
	struct CompositeShape {
		SoSeparator* rootNode;
		std::string shapeName;
		std::vector<SoNode*> childNodes;
		SoMaterial* material;  // Primary material for hover effect
		std::vector<SoMaterial*> allMaterials;  // All materials in this shape (for arrows with multiple parts)

		CompositeShape(SoSeparator* root, const std::string& name, SoMaterial* mat = nullptr)
			: rootNode(root), shapeName(name), material(mat) {
			if (mat) {
				allMaterials.push_back(mat);
			}
		}

		// Collect all materials from the scene graph
		void collectMaterials(SoNode* node) {
			if (!node) return;

			if (node->isOfType(SoMaterial::getClassTypeId())) {
				SoMaterial* mat = static_cast<SoMaterial*>(node);
				// Avoid duplicates
				if (std::find(allMaterials.begin(), allMaterials.end(), mat) == allMaterials.end()) {
					allMaterials.push_back(mat);
				}
			}

			if (node->isOfType(SoSeparator::getClassTypeId())) {
				SoSeparator* sep = static_cast<SoSeparator*>(node);
				for (int i = 0; i < sep->getNumChildren(); ++i) {
					collectMaterials(sep->getChild(i));
				}
			}
		}

		// Update all materials to a given color
		void setAllMaterialsColor(const SbColor& color) {
			for (SoMaterial* mat : allMaterials) {
				if (mat) {
					mat->diffuseColor.setValue(color);
				}
			}
		}
	};

	// Picking cache for performance optimization
	struct PickingCache {
		wxPoint lastPickPos;       // Last picking position
		std::string lastResult;    // Last picked shape name
		bool isValid;              // Cache validity flag
		int pickThreshold;         // Minimum pixel distance to trigger new pick

		PickingCache() : lastPickPos(-1, -1), lastResult(""), isValid(false), pickThreshold(3) {}

		bool shouldRepick(const wxPoint& currentPos) const {
			if (!isValid) return true;
			int dx = currentPos.x - lastPickPos.x;
			int dy = currentPos.y - lastPickPos.y;
			return (dx * dx + dy * dy) > (pickThreshold * pickThreshold);
		}

		void update(const wxPoint& pos, const std::string& result) {
			lastPickPos = pos;
			lastResult = result;
			isValid = true;
		}

		void invalidate() {
			isValid = false;
			lastResult = "";
		}
	};

	std::vector<CompositeShape> m_compositeShapes;
	std::unordered_map<std::string, size_t> m_shapeNameToIndex;  // Fast shape lookup
	
	// Context menu IDs
	enum MenuIds {
		ID_MENU_RESET_VIEW = wxID_HIGHEST + 1,
		ID_MENU_TOGGLE_CUBE_VISIBILITY,
		ID_MENU_TOGGLE_COORD_VISIBILITY,
		ID_MENU_CUBE_SETTINGS
	};
	
	// Last click position for menu
	wxPoint m_lastClickPos;
	
	// Hover state tracking
	bool m_isCubeHovered;
	std::string m_lastHoveredShape;
	SoMaterial* m_cubeMaterial;
	SbColor m_normalColor;
	SbColor m_hoverColor;

	// Picking cache instance
	PickingCache m_pickingCache;
};