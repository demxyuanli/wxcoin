#include "NavigationCubeManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include "CuteNavCube.h"
#include "DPIManager.h"
#include "NavigationCubeConfigDialog.h"
#include "config/ConfigManager.h"
#include "ViewRefreshManager.h"
#include "CameraAnimation.h"
#include <wx/wx.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>

void NavigationCubeManager::Layout::update(int newX_logical, int newY_logical, int newSize_logical,
	const wxSize& windowSize_logical, float dpiScale)
{
	cubeSize = (std::max)(120, (std::min)(newSize_logical, windowSize_logical.x / 2));
	cubeSize = (std::max)(120, (std::min)(cubeSize, windowSize_logical.y / 2));
	x = (std::max)(0, (std::min)(newX_logical, windowSize_logical.x - cubeSize));
	y = (std::max)(0, (std::min)(newY_logical, windowSize_logical.y - cubeSize));
}

NavigationCubeManager::NavigationCubeManager(Canvas* canvas, SceneManager* sceneManager)
	: m_canvas(canvas), m_sceneManager(sceneManager), m_isEnabled(true)
{
	// Initialize default configuration
	m_cubeConfig = CubeConfig{};

	// Load configuration from persistent storage (quietly)

	// Override default configuration values
	m_cubeConfig.size = 80;        // geometry size
	m_cubeConfig.viewportSize = 80; // viewport layout size
	m_cubeConfig.cubeSize = 0.5f;   // geometry size scale (0.0-1.0)

	// Set margins from configuration if available
	auto& dpiManager = DPIManager::getInstance();
	if (m_cubeConfig.x >= 0) {
		m_marginx = m_cubeConfig.x;
	} else {
		m_marginx = dpiManager.getScaledSize(20);
	}
	if (m_cubeConfig.y >= 0) {
		m_marginy = m_cubeConfig.y;
	} else {
		m_marginy = dpiManager.getScaledSize(20);
	}

	// Don't initialize immediately - wait for proper canvas sizing
	// initCube() will be called when needed
}

NavigationCubeManager::~NavigationCubeManager()
{
	LOG_DBG_S("NavigationCubeManager: Destroying");
}

void NavigationCubeManager::initCube() {
	if (!m_isEnabled || m_navCube) {
		// Skip initialization - already done or disabled
		return;
	}

	// Check if canvas has valid size
	wxSize clientSize = m_canvas->GetClientSize();
	if (clientSize.x <= 50 || clientSize.y <= 50) {
		return;
	}

	try {
		auto cubeCallback = [this](const std::string& faceName) {
			// Handle both face names and view names
			std::string viewName = faceName;

			// If this is a face name, map it to the corresponding view
			static const std::map<std::string, std::string> faceToView = {
				// 6 Main faces - Click face -> View direction
				{ "Front",  "Front" },
				{ "Back",   "Back" },
				{ "Left",   "Left" },
				{ "Right",  "Right" },
				{ "Top",    "Top" },
				{ "Bottom", "Bottom" },

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

			auto it = faceToView.find(faceName);
			if (it != faceToView.end()) {
				viewName = it->second;
				LOG_DBG_S("NavigationCubeManager::cubeCallback: Mapped face " + faceName + " to view " + viewName);
			} else {
				LOG_DBG_S("NavigationCubeManager::cubeCallback: Using face name " + faceName + " as view");
			}

			m_sceneManager->setView(viewName);
			m_canvas->Refresh(true);
			};

		wxSize clientSize = m_canvas->GetClientSize();
		float dpiScale = m_canvas->getDPIScale();

		// Convert logical coordinates to physical pixels for CuteNavCube
		int windowWidthPx = static_cast<int>(clientSize.x * dpiScale);
		int windowHeightPx = static_cast<int>(clientSize.y * dpiScale);

		// Create camera move callback that sets navigation cube camera position and orientation
		auto cameraMoveCallback = [this](const SbVec3f& position, const SbRotation& orientation) {
			// Stop any ongoing main camera animation when navigation cube camera moves
			NavigationAnimator::getInstance().stopCurrentAnimation();

			// Set navigation cube camera position and orientation
			if (m_navCube) {
				m_navCube->setCameraPosition(position);
				m_navCube->setCameraOrientation(orientation);

				// Sync main camera to match navigation cube camera direction but keep current distance
				// Use the same refresh pattern as SceneManager::setView
				if (m_sceneManager) {
					SoCamera* mainCamera = m_sceneManager->getCamera();
					if (mainCamera) {
						// Get current main camera distance to origin
						SbVec3f currentMainPos = mainCamera->position.getValue();
						float mainCamDistance = currentMainPos.length();
						if (mainCamDistance < 1e-3f) {
							mainCamDistance = 10.0f; // Default distance if too close to origin
						}

						// Calculate new main camera position: same direction as nav camera but keep distance
						SbVec3f mainCamDir = position;
						if (mainCamDir.normalize() != 0.0f) { // Avoid zero vector
							SbVec3f mainCamPos = mainCamDir * mainCamDistance;

							// Set camera position and orientation
							mainCamera->position.setValue(mainCamPos);
							mainCamera->orientation.setValue(orientation);

							// Set focal distance based on position (similar to setView)
							mainCamera->focalDistance.setValue(mainCamDistance);

							// Ensure reasonable near/far planes (similar to setView)
							mainCamera->nearDistance.setValue(0.001f);
							mainCamera->farDistance.setValue(10000.0f);

							// CRITICAL: Call viewAll to ensure scene objects are visible (same as setView)
							SbViewportRegion viewport(m_canvas->GetClientSize().x, m_canvas->GetClientSize().y);
							mainCamera->viewAll(m_sceneManager->getSceneRoot(), viewport, 1.1f);
						}

						// Use the same refresh pattern as setView method
						if (m_canvas->getRefreshManager()) {
							m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
						} else {
							m_canvas->Refresh(true);
						}

					} else {
						LOG_ERR_S("NavigationCubeManager::cameraMoveCallback: Main camera is null");
					}
				}
			} else {
				LOG_ERR_S("NavigationCubeManager::cameraMoveCallback: Navigation cube is null");
			}
		};

		// Create refresh callback that requests refresh from canvas
		auto refreshCallback = [this]() {
			// Request refresh from the refresh manager if available, otherwise use canvas directly
			if (m_canvas && m_canvas->getRefreshManager()) {
				m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::MANUAL_REQUEST, true);
			} else if (m_canvas) {
				m_canvas->Refresh(true);
			}
		};

		m_navCube = std::make_unique<CuteNavCube>(cubeCallback, cameraMoveCallback, refreshCallback, dpiScale, windowWidthPx, windowHeightPx, m_cubeConfig);
		m_navCube->setRotationChangedCallback([this]() {
			syncMainCameraToCube();
			m_canvas->Refresh(true);
			});
		if (clientSize.x > 0 && clientSize.y > 0) {
			// Use loaded configuration size or default
			if (m_cubeConfig.size > 0) {
				m_cubeLayout.cubeSize = m_cubeConfig.size;
			} else {
			m_cubeLayout.cubeSize = 0.5f;
				m_cubeConfig.size = 80;
			}
			// Use loaded position or calculate centered position
		int cubeX, cubeY;
		if (m_cubeConfig.x >= 0 && m_cubeConfig.y >= 0) {
			// Use loaded position - convert from margins to coordinates
			cubeX = clientSize.x - m_cubeLayout.cubeSize - m_cubeConfig.x;
			cubeY = m_cubeConfig.y;
		} else {
			// Calculate centered position as fallback
			calculateCenteredPosition(cubeX, cubeY, m_cubeLayout.cubeSize, clientSize);
			m_cubeConfig.x = cubeX;
			m_cubeConfig.y = cubeY;
		}
			// Update layout with position from config
			m_cubeLayout.update(cubeX, cubeY, m_cubeLayout.cubeSize, clientSize, dpiScale);
		}
	}
	catch (const std::exception& e) {
		LOG_ERR_S("NavigationCubeManager: Failed to initialize navigation cube: " + std::string(e.what()));
		m_canvas->showErrorDialog("Failed to initialize navigation cube.");
		m_navCube.reset();
		m_isEnabled = false;
	}
}

void NavigationCubeManager::render() {
	// Initialize cube if not already done and canvas has valid size
	if (!m_navCube && m_isEnabled) {
		wxSize clientSize = m_canvas->GetClientSize();
		if (clientSize.x > 50 && clientSize.y > 50) {
			initCube();
		}
	}

	if (m_navCube && m_isEnabled) {
		syncCubeCameraToMain();

		// Check if window size changed and log only when necessary
		static wxSize lastClientSize(-1, -1);
		static wxPoint lastPosition(-1, -1);
		static int lastSize = -1;

		wxSize currentClientSize = m_canvas->GetClientSize();
		wxPoint currentPosition(m_cubeLayout.x, m_cubeLayout.y);
		int currentSize = m_cubeLayout.cubeSize;

		bool shouldLog = (currentClientSize != lastClientSize) ||
		                (currentPosition != lastPosition) ||
		                (currentSize != lastSize);

		if (shouldLog) {
			lastClientSize = currentClientSize;
			lastPosition = currentPosition;
			lastSize = currentSize;
		}

		m_navCube->render(m_cubeLayout.x, m_cubeLayout.y, wxSize(m_cubeLayout.cubeSize, m_cubeLayout.cubeSize));
	}
}

bool NavigationCubeManager::handleMouseEvent(wxMouseEvent& event) {
	if (!m_navCube || !m_isEnabled) {
		return false;
	}

	float dpiScale = m_canvas->getDPIScale();
	float x = event.GetX() / dpiScale;
	float y = event.GetY() / dpiScale;

	// Use the layout size for mouse detection (viewport size where cube is rendered)
	int cubeSize = m_cubeLayout.cubeSize;
	if (x >= m_cubeLayout.x && x <= (m_cubeLayout.x + cubeSize) &&
		y >= m_cubeLayout.y && y <= (m_cubeLayout.y + cubeSize)) {
		// Mouse is within cube area - intercept all mouse events to prevent them from reaching canvas
		wxMouseEvent cubeEvent(event);
		cubeEvent.m_x = static_cast<int>((x - m_cubeLayout.x) * dpiScale);
		cubeEvent.m_y = static_cast<int>((y - m_cubeLayout.y) * dpiScale);

		int scaled_cube_dimension = static_cast<int>(cubeSize * dpiScale);
		wxSize cube_viewport_scaled_size(scaled_cube_dimension, scaled_cube_dimension);

		if (event.GetEventType() == wxEVT_LEFT_DOWN ||
			event.GetEventType() == wxEVT_LEFT_UP ||
			event.GetEventType() == wxEVT_MOTION ||
			event.GetEventType() == wxEVT_LEAVE_WINDOW) {
			bool cubeHandled = m_navCube->handleMouseEvent(cubeEvent, cube_viewport_scaled_size);
			// Note: Removed m_canvas->Refresh(true) to prevent recursive refresh during mouse event handling
			// The main rendering loop will handle refresh appropriately
			return true; // Always consume events in cube area
		}
		
		// For other event types in cube area, still consume them
		return true; // Event consumed by cube area
	}
	return false; // Mouse not in cube area, let event continue to canvas
}

void NavigationCubeManager::handleSizeChange() {
	wxSize size = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();


	// Update cube layout with logical coordinates
	m_cubeLayout.update(size.x - m_cubeLayout.cubeSize - m_marginx,
		m_marginy,
		m_cubeLayout.cubeSize, size, dpiScale);


	// Update cube's window size with physical dimensions
	if (m_navCube) {
		int windowWidthPx = static_cast<int>(size.x * dpiScale);
		int windowHeightPx = static_cast<int>(size.y * dpiScale);
		m_navCube->setWindowSize(windowWidthPx, windowHeightPx);
	}
}

void NavigationCubeManager::handleDPIChange() {
	auto& dpiManager = DPIManager::getInstance();

	// Only update margins if they are still at default values
	// This prevents overriding user-configured margins with DPI defaults
	float dpiScale = dpiManager.getDPIScale();
	int defaultScaledMargin = static_cast<int>(20 * dpiScale);

	if (m_marginx == defaultScaledMargin && m_marginy == defaultScaledMargin) {
		// Margins are still at default, update them for new DPI
	m_marginx = dpiManager.getScaledSize(20);
	m_marginy = dpiManager.getScaledSize(20);
	}

	// The cube should handle its own DPI-related texture updates.
	// We just ensure it's redrawn and its position is updated on next size event.
	m_canvas->Refresh(true);
}

void NavigationCubeManager::setEnabled(bool enabled) {
	m_isEnabled = enabled;
	if (enabled && !m_navCube) {
		initCube();
	}
	if (m_navCube) {
		m_navCube->setEnabled(enabled);
	}
	m_canvas->Refresh(true);
}

bool NavigationCubeManager::isEnabled() const {
	return m_isEnabled && m_navCube && m_navCube->isEnabled();
}

void NavigationCubeManager::setRect(int x, int y, int size) {
	if (size < 50 || x < 0 || y < 0) {
		return;
	}
	wxSize clientSize = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();

	// Update layout with new parameters
	m_cubeLayout.update(x, y, size, clientSize, dpiScale);

	// Note: Don't update m_cubeConfig here as it should preserve the user's intended configuration
	// Only update config when explicitly setting configuration through setConfig()

	m_canvas->Refresh(true);

}

void NavigationCubeManager::setColor(const wxColour& color) {
	if (!m_navCube) {
		LOG_DBG_S("NavigationCubeManager::setColor: Skipped: nav cube not created");
		return;
	}
	m_canvas->Refresh(true);
}

void NavigationCubeManager::setViewportSize(int size) {
	if (!m_navCube) {
		LOG_DBG_S("NavigationCubeManager::setViewportSize: Skipped: nav cube not created");
		return;
	}
	if (size < 50) {
		LOG_DBG_S("NavigationCubeManager::setViewportSize: Invalid size: " + std::to_string(size));
		return;
	}

	// Update the cube layout size and recalculate position
	wxSize clientSize = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();

	// Log before changes

	// Update layout size
	m_cubeLayout.cubeSize = size;

	// Recalculate position to maintain centering
	int centeredX, centeredY;
	calculateCenteredPosition(centeredX, centeredY, size, clientSize);
	m_cubeLayout.update(centeredX, centeredY, size, clientSize, dpiScale);

	// Note: Don't update m_cubeConfig here as it should preserve the user's intended viewport size
	// Only update config when explicitly setting configuration through setConfig()


	m_canvas->Refresh(true);
}

void NavigationCubeManager::syncCubeCameraToMain() {
	if (!m_navCube || !m_sceneManager) {
		LOG_DBG_S("NavigationCubeManager::syncCubeCameraToMain: Skipped: components missing");
		return;
	}

	// Skip synchronization during camera animation to prevent conflicts
	if (NavigationAnimator::getInstance().isAnimating()) {
		return;
	}

	SoCamera* mainCamera = m_sceneManager->getCamera();
	if (mainCamera) {
		SbRotation mainOrient = mainCamera->orientation.getValue();
		float navDistance = 5.0f;
		SbVec3f srcVec(0, 0, -1);
		SbVec3f mainCamViewVector;
		mainOrient.multVec(srcVec, mainCamViewVector);
		SbVec3f navCubeCamPos = -mainCamViewVector * navDistance;

		m_navCube->setCameraPosition(navCubeCamPos);
		m_navCube->setCameraOrientation(mainOrient);

		//LOG_DBG("NavigationCubeManager::syncCubeCameraToMain: Synced navigation cube camera based on main camera orientation.");
	}
}

void NavigationCubeManager::syncMainCameraToCube() {
	if (!m_navCube || !m_sceneManager) {
		LOG_DBG_S("NavigationCubeManager::syncMainCameraToCube: Skipped: components missing");
		return;
	}

	SoCamera* navCamera = m_navCube->getCamera();
	if (!navCamera) {
		LOG_DBG_S("NavigationCubeManager::syncMainCameraToCube: Navigation cube camera is null.");
		return;
	}

	SoCamera* mainCamera = m_sceneManager->getCamera();
	if (!mainCamera) {
		LOG_DBG_S("NavigationCubeManager::syncMainCameraToCube: Main scene camera is null.");
		return;
	}

	SbVec3f mainCamCurrentPos = mainCamera->position.getValue();
	float mainCamDistanceToOrigin = mainCamCurrentPos.length();

	if (mainCamDistanceToOrigin < 1e-3) {
		mainCamDistanceToOrigin = 10.0f;
	}

	SbVec3f navCamPos = navCamera->position.getValue();
	SbRotation navCamOrient = navCamera->orientation.getValue();

	SbVec3f newMainCamDir = navCamPos;
	if (newMainCamDir.normalize() == 0.0f) {
		LOG_DBG_S("NavigationCubeManager::syncMainCameraToCube: NavCam position is origin, cannot determine direction.");
		return;
	}

	SbVec3f newMainCamPos = newMainCamDir * mainCamDistanceToOrigin;
	mainCamera->position.setValue(newMainCamPos);
	mainCamera->orientation.setValue(navCamOrient);

	//LOG_DBG("NavigationCubeManager::syncMainCameraToCube: Synced main camera to orbit origin, pos: (" +
	//    std::to_string(newMainCamPos[0]) + ", " + std::to_string(newMainCamPos[1]) + ", " + std::to_string(newMainCamPos[2]) +
	//    "), dist: " + std::to_string(mainCamDistanceToOrigin));
}

void NavigationCubeManager::showConfigDialog() { 
	if (!m_canvas) {
		LOG_ERR_S("NavigationCubeManager::showConfigDialog: Canvas is null");
		return;
	}

	wxSize clientSize = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();

	// Convert physical pixels to logical coordinates
	int clientWidthLogical = static_cast<int>(clientSize.x / dpiScale);
	int clientHeightLogical = static_cast<int>(clientSize.y / dpiScale);

	// Sync m_cubeConfig with current actual state before opening dialog
	// This ensures the dialog shows the current cube state, not outdated config
	CubeConfig currentConfig = m_cubeConfig;
	
	// Convert coordinates: X is margin from right edge, Y is margin from top edge
	currentConfig.x = clientWidthLogical - m_cubeLayout.x - m_cubeLayout.cubeSize; // Right margin
	currentConfig.y = m_cubeLayout.y; // Top margin
	currentConfig.size = m_navCube ? m_navCube->getSize() : 140; // Cube geometric size
	currentConfig.viewportSize = m_cubeLayout.cubeSize; // Layout size in viewport

	// Create dialog with real-time config change callback
	NavigationCubeConfigDialog dialog(m_canvas->GetParent(), currentConfig, clientWidthLogical, clientHeightLogical,
		[this](const CubeConfig& config) {
			// Apply configuration changes in real-time
			setConfig(config);
		});

	if (dialog.ShowModal() == wxID_OK) {
		CubeConfig newConfig = dialog.GetConfig();
		setConfig(newConfig);

		// Save configuration to persistent storage
		saveConfigToPersistent();
	}
}

void NavigationCubeManager::setConfig(const CubeConfig& config) {
	m_cubeConfig = config;
	applyConfig(config);
}

CubeConfig NavigationCubeManager::getConfig() const {
	return m_cubeConfig;
}

void NavigationCubeManager::applyConfig(const CubeConfig& config) {

	// Apply position and size
	// Convert margins to coordinates: X is margin from right edge, Y is margin from top edge
	wxSize clientSize = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();
	int clientWidthLogical = static_cast<int>(clientSize.x / dpiScale);
	
	int cubeX = clientWidthLogical - config.x - config.viewportSize; // Convert right margin to X coordinate
	int cubeY = config.y; // Y is already top margin
	
	// Update cube geometric size independently from layout size
	// size affects cube internal geometry, not layout position
	if (m_navCube && config.size != m_navCube->getSize()) {
		m_navCube->setSize(config.size);
	}
	
	// Update layout size using viewportSize
	// viewportSize affects cube position and layout area
	if (config.viewportSize != m_cubeLayout.cubeSize) {
		setRect(cubeX, cubeY, config.viewportSize);
	}

	// Apply colors and material properties to the cube
	if (m_navCube) {
		m_navCube->applyConfig(config);
	}

	// Refresh display
	m_canvas->Refresh(true);
}

void NavigationCubeManager::centerCubeInViewport() {
	wxSize clientSize = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();

	LOG_DBG_S("NavigationCubeManager::centerCubeInViewport: Centering cube - current position: (" +
		std::to_string(m_cubeLayout.x) + "," + std::to_string(m_cubeLayout.y) + "), size: " + std::to_string(m_cubeLayout.cubeSize) +
		", clientSize: " + std::to_string(clientSize.x) + "x" + std::to_string(clientSize.y) +
		", dpiScale: " + std::to_string(dpiScale));

	int centeredX, centeredY;

	calculateCenteredPosition(centeredX, centeredY, m_cubeLayout.cubeSize, clientSize);

	// Update configuration with centered position
	m_cubeConfig.x = centeredX;
	m_cubeConfig.y = centeredY;

	// Apply the centered position
	setRect(centeredX, centeredY, m_cubeLayout.cubeSize);
}

void NavigationCubeManager::calculateCenteredPosition(int& x, int& y, int cubeSize, const wxSize& windowSize) {
	// X,Y in config represent margins from right and top edges
	// Convert margins to actual coordinates
	// Note: windowSize is in physical pixels, but we work in logical coordinates

	// If X,Y are set in config, use them as margins from right/top edges
	if (m_cubeConfig.x >= 0 && m_cubeConfig.y >= 0) {
		// X is margin from right edge, Y is margin from top edge
		// We need to work in logical coordinates, so convert windowSize to logical
		float dpiScale = m_canvas->getDPIScale();
		int windowWidthLogical = static_cast<int>(windowSize.x / dpiScale);
		int windowHeightLogical = static_cast<int>(windowSize.y / dpiScale);

		x = windowWidthLogical - cubeSize - m_cubeConfig.x;
		y = m_cubeConfig.y;

		// Ensure cube stays within window bounds
		x = std::max(0, std::min(x, windowWidthLogical - cubeSize));
		y = std::max(0, std::min(y, windowHeightLogical - cubeSize));

		LOG_DBG_S("NavigationCubeManager::calculateCenteredPosition: Using configured margins - "
			"right margin=" + std::to_string(m_cubeConfig.x) + ", top margin=" + std::to_string(m_cubeConfig.y) +
			" -> position: x=" + std::to_string(x) + ", y=" + std::to_string(y) +
			" (window: " + std::to_string(windowWidthLogical) + "x" + std::to_string(windowHeightLogical) + ")");
		return;
	}

	// Fallback: center within circular navigation area
	float dpiScale = m_canvas->getDPIScale();
	int windowWidthLogical = static_cast<int>(windowSize.x / dpiScale);
	int windowHeightLogical = static_cast<int>(windowSize.y / dpiScale);

	int circleCenterX = windowWidthLogical - m_cubeConfig.circleMarginX;
	int circleCenterY = m_cubeConfig.circleMarginY;

	// Center the cube within the circular area
	x = circleCenterX - cubeSize / 2;
	y = circleCenterY - cubeSize / 2;

	// Ensure cube stays within window bounds
	x = std::max(0, std::min(x, windowWidthLogical - cubeSize));
	y = std::max(0, std::min(y, windowHeightLogical - cubeSize));
}

void NavigationCubeManager::saveConfigToPersistent() {
	ConfigManager& configManager = ConfigManager::getInstance();
	
	const std::string section = "NavigationCube";
	
	// Position and Size
	configManager.setInt(section, "X", m_cubeConfig.x);
	configManager.setInt(section, "Y", m_cubeConfig.y);
	configManager.setInt(section, "Size", m_cubeConfig.size);
	configManager.setInt(section, "ViewportSize", m_cubeConfig.viewportSize);
	
	// Colors (save as RGB values)
	configManager.setInt(section, "BackgroundColorR", m_cubeConfig.backgroundColor.Red());
	configManager.setInt(section, "BackgroundColorG", m_cubeConfig.backgroundColor.Green());
	configManager.setInt(section, "BackgroundColorB", m_cubeConfig.backgroundColor.Blue());
	
	configManager.setInt(section, "TextColorR", m_cubeConfig.textColor.Red());
	configManager.setInt(section, "TextColorG", m_cubeConfig.textColor.Green());
	configManager.setInt(section, "TextColorB", m_cubeConfig.textColor.Blue());
	
	configManager.setInt(section, "EdgeColorR", m_cubeConfig.edgeColor.Red());
	configManager.setInt(section, "EdgeColorG", m_cubeConfig.edgeColor.Green());
	configManager.setInt(section, "EdgeColorB", m_cubeConfig.edgeColor.Blue());
	
	configManager.setInt(section, "CornerColorR", m_cubeConfig.cornerColor.Red());
	configManager.setInt(section, "CornerColorG", m_cubeConfig.cornerColor.Green());
	configManager.setInt(section, "CornerColorB", m_cubeConfig.cornerColor.Blue());
	
	// Material Properties
	configManager.setDouble(section, "Transparency", m_cubeConfig.transparency);
	configManager.setDouble(section, "Shininess", m_cubeConfig.shininess);
	configManager.setDouble(section, "AmbientIntensity", m_cubeConfig.ambientIntensity);
	
	// Display Options
	configManager.setBool(section, "ShowEdges", m_cubeConfig.showEdges);
	configManager.setBool(section, "ShowCorners", m_cubeConfig.showCorners);
	configManager.setBool(section, "ShowTextures", m_cubeConfig.showTextures);
	configManager.setBool(section, "EnableAnimation", m_cubeConfig.enableAnimation);
	
	// Geometry
	configManager.setDouble(section, "CubeSize", m_cubeConfig.cubeSize);
	configManager.setDouble(section, "ChamferSize", m_cubeConfig.chamferSize);
	configManager.setDouble(section, "CameraDistance", m_cubeConfig.cameraDistance);
	
	// Circle Navigation Area
	configManager.setInt(section, "CircleRadius", m_cubeConfig.circleRadius);
	configManager.setInt(section, "CircleMarginX", m_cubeConfig.circleMarginX);
	configManager.setInt(section, "CircleMarginY", m_cubeConfig.circleMarginY);
	
	// Save to file
	if (!configManager.save()) {
		LOG_ERR_S("NavigationCubeManager::saveConfigToPersistent: Failed to save configuration");
	}
}

void NavigationCubeManager::loadConfigFromPersistent() {
	ConfigManager& configManager = ConfigManager::getInstance();
	
	const std::string section = "NavigationCube";
	
	// Position and Size
	m_cubeConfig.x = configManager.getInt(section, "X", m_cubeConfig.x);
	m_cubeConfig.y = configManager.getInt(section, "Y", m_cubeConfig.y);
	m_cubeConfig.size = configManager.getInt(section, "Size", m_cubeConfig.size);
	m_cubeConfig.viewportSize = configManager.getInt(section, "ViewportSize", m_cubeConfig.viewportSize);
	
	// Colors
	int r = configManager.getInt(section, "BackgroundColorR", m_cubeConfig.backgroundColor.Red());
	int g = configManager.getInt(section, "BackgroundColorG", m_cubeConfig.backgroundColor.Green());
	int b = configManager.getInt(section, "BackgroundColorB", m_cubeConfig.backgroundColor.Blue());
	m_cubeConfig.backgroundColor = wxColour(r, g, b);
	
	r = configManager.getInt(section, "TextColorR", m_cubeConfig.textColor.Red());
	g = configManager.getInt(section, "TextColorG", m_cubeConfig.textColor.Green());
	b = configManager.getInt(section, "TextColorB", m_cubeConfig.textColor.Blue());
	m_cubeConfig.textColor = wxColour(r, g, b);
	
	r = configManager.getInt(section, "EdgeColorR", m_cubeConfig.edgeColor.Red());
	g = configManager.getInt(section, "EdgeColorG", m_cubeConfig.edgeColor.Green());
	b = configManager.getInt(section, "EdgeColorB", m_cubeConfig.edgeColor.Blue());
	m_cubeConfig.edgeColor = wxColour(r, g, b);
	
	r = configManager.getInt(section, "CornerColorR", m_cubeConfig.cornerColor.Red());
	g = configManager.getInt(section, "CornerColorG", m_cubeConfig.cornerColor.Green());
	b = configManager.getInt(section, "CornerColorB", m_cubeConfig.cornerColor.Blue());
	m_cubeConfig.cornerColor = wxColour(r, g, b);
	
	// Material Properties
	m_cubeConfig.transparency = static_cast<float>(configManager.getDouble(section, "Transparency", m_cubeConfig.transparency));
	m_cubeConfig.shininess = static_cast<float>(configManager.getDouble(section, "Shininess", m_cubeConfig.shininess));
	m_cubeConfig.ambientIntensity = static_cast<float>(configManager.getDouble(section, "AmbientIntensity", m_cubeConfig.ambientIntensity));
	
	// Display Options
	m_cubeConfig.showEdges = configManager.getBool(section, "ShowEdges", m_cubeConfig.showEdges);
	m_cubeConfig.showCorners = configManager.getBool(section, "ShowCorners", m_cubeConfig.showCorners);
	m_cubeConfig.showTextures = configManager.getBool(section, "ShowTextures", m_cubeConfig.showTextures);
	m_cubeConfig.enableAnimation = configManager.getBool(section, "EnableAnimation", m_cubeConfig.enableAnimation);
	
	// Geometry
	m_cubeConfig.cubeSize = static_cast<float>(configManager.getDouble(section, "CubeSize", m_cubeConfig.cubeSize));
	m_cubeConfig.chamferSize = static_cast<float>(configManager.getDouble(section, "ChamferSize", m_cubeConfig.chamferSize));
	m_cubeConfig.cameraDistance = static_cast<float>(configManager.getDouble(section, "CameraDistance", m_cubeConfig.cameraDistance));
	
	// Circle Navigation Area
	m_cubeConfig.circleRadius = configManager.getInt(section, "CircleRadius", m_cubeConfig.circleRadius);
	m_cubeConfig.circleMarginX = configManager.getInt(section, "CircleMarginX", m_cubeConfig.circleMarginX);
	m_cubeConfig.circleMarginY = configManager.getInt(section, "CircleMarginY", m_cubeConfig.circleMarginY);
}