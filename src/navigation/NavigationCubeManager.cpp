#include "NavigationCubeManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include "CuteNavCube.h"
#include "DPIManager.h"
#include "NavigationCubeConfigDialog.h"
#include "config/ConfigManager.h"
#include "ViewRefreshManager.h"
#include <wx/wx.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>

void NavigationCubeManager::Layout::update(int newX_logical, int newY_logical, int newSize_logical,
	const wxSize& windowSize_logical, float dpiScale)
{
	size = (std::max)(120, (std::min)(newSize_logical, windowSize_logical.x / 2));
	size = (std::max)(120, (std::min)(size, windowSize_logical.y / 2));
	x = (std::max)(0, (std::min)(newX_logical, windowSize_logical.x - size));
	y = (std::max)(0, (std::min)(newY_logical, windowSize_logical.y - size));
}

NavigationCubeManager::NavigationCubeManager(Canvas* canvas, SceneManager* sceneManager)
	: m_canvas(canvas), m_sceneManager(sceneManager), m_isEnabled(true)
{
	LOG_INF_S("NavigationCubeManager: Initializing");
	// Initialize default configuration
	m_cubeConfig = CubeConfig{};

	// Load configuration from persistent storage
	loadConfigFromPersistent();

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

	LOG_INF_S("NavigationCubeManager: Initialized with margins from config: " +
		std::to_string(m_marginx) + "x" + std::to_string(m_marginy) +
		", config size: " + std::to_string(m_cubeConfig.size));

	// Don't initialize immediately - wait for proper canvas sizing
	// initCube() will be called when needed
}

NavigationCubeManager::~NavigationCubeManager()
{
	LOG_INF_S("NavigationCubeManager: Destroying");
}

void NavigationCubeManager::initCube() {
	if (!m_isEnabled || m_navCube) return; // Already initialized

	// Check if canvas has valid size
	wxSize clientSize = m_canvas->GetClientSize();
	if (clientSize.x <= 50 || clientSize.y <= 50) {
		LOG_INF_S("NavigationCubeManager::initCube: Skipping initialization - canvas size too small: " +
			std::to_string(clientSize.x) + "x" + std::to_string(clientSize.y));
		return;
	}

	LOG_INF_S("NavigationCubeManager::initCube: Initializing with valid canvas size: " +
		std::to_string(clientSize.x) + "x" + std::to_string(clientSize.y));

	try {
		auto cubeCallback = [this](const std::string& faceName) {
			// Handle both face names and view names
			std::string viewName = faceName;

			// If this is a face name, map it to the corresponding view
			static const std::map<std::string, std::string> faceToView = {
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

			auto it = faceToView.find(faceName);
			if (it != faceToView.end()) {
				viewName = it->second;
				LOG_INF_S("NavigationCubeManager::cubeCallback: Mapped face " + faceName + " to view " + viewName);
			} else {
				LOG_INF_S("NavigationCubeManager::cubeCallback: Using face name " + faceName + " as view");
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

						LOG_INF_S("NavigationCubeManager::cameraMoveCallback: Set navigation camera position: (" +
							std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " +
							std::to_string(position[2]) + ") and synced main camera (distance preserved: " +
							std::to_string(mainCamDistance) + ")");
					} else {
						LOG_ERR_S("NavigationCubeManager::cameraMoveCallback: Main camera is null");
					}
				}
			} else {
				LOG_ERR_S("NavigationCubeManager::cameraMoveCallback: Navigation cube is null");
			}
		};

		m_navCube = std::make_unique<CuteNavCube>(cubeCallback, cameraMoveCallback, dpiScale, windowWidthPx, windowHeightPx, m_cubeConfig);
		m_navCube->setRotationChangedCallback([this]() {
			syncMainCameraToCube();
			m_canvas->Refresh(true);
			});
		LOG_INF_S("NavigationCubeManager: Navigation cube (CuteNavCube) initialized");

		if (clientSize.x > 0 && clientSize.y > 0) {
			// Use loaded configuration size or default
			if (m_cubeConfig.size > 0) {
				m_cubeLayout.size = m_cubeConfig.size;
			} else {
			m_cubeLayout.size = 280;
				m_cubeConfig.size = 280;
			}

			// Log initial configuration
			LOG_INF_S("NavigationCubeManager: Initial configuration - clientSize: " + std::to_string(clientSize.x) + "x" + std::to_string(clientSize.y) +
				", dpiScale: " + std::to_string(dpiScale) +
				", config size: " + std::to_string(m_cubeConfig.size) +
				", config x: " + std::to_string(m_cubeConfig.x) +
				", config y: " + std::to_string(m_cubeConfig.y));

			// Use loaded position or calculate centered position
		int cubeX, cubeY;
		if (m_cubeConfig.x >= 0 && m_cubeConfig.y >= 0) {
			// Use loaded position - convert from margins to coordinates
			cubeX = clientSize.x - m_cubeLayout.size - m_cubeConfig.x;
			cubeY = m_cubeConfig.y;
			LOG_INF_S("NavigationCubeManager: Using config margins - right margin: " + std::to_string(m_cubeConfig.x) +
				", top margin: " + std::to_string(m_cubeConfig.y) +
				", clientSize: " + std::to_string(clientSize.x) + "x" + std::to_string(clientSize.y) +
				", cubeSize: " + std::to_string(m_cubeLayout.size) +
				" -> calculated position: x=" + std::to_string(cubeX) + ", y=" + std::to_string(cubeY) +
				" (formula: " + std::to_string(clientSize.x) + " - " + std::to_string(m_cubeLayout.size) + " - " + std::to_string(m_cubeConfig.x) + " = " + std::to_string(cubeX) + ")");
		} else {
			// Calculate centered position as fallback
			calculateCenteredPosition(cubeX, cubeY, m_cubeLayout.size, clientSize);
			m_cubeConfig.x = cubeX;
			m_cubeConfig.y = cubeY;
			LOG_INF_S("NavigationCubeManager: Calculated fallback position - x=" + std::to_string(cubeX) +
				", y=" + std::to_string(cubeY) + ", size=" + std::to_string(m_cubeLayout.size));
		}

			// Update layout with position from config
			m_cubeLayout.update(cubeX, cubeY, m_cubeLayout.size, clientSize, dpiScale);

			LOG_INF_S("NavigationCubeManager: Initialized navigation cube at position: x=" + std::to_string(m_cubeLayout.x) +
				", y=" + std::to_string(m_cubeLayout.y) + ", size=" + std::to_string(m_cubeLayout.size) +
				", physical pixels: " + std::to_string(m_cubeLayout.x * dpiScale) + "x" +
				std::to_string(m_cubeLayout.y * dpiScale) + " to " +
				std::to_string((m_cubeLayout.x + m_cubeLayout.size) * dpiScale) + "x" +
				std::to_string((m_cubeLayout.y + m_cubeLayout.size) * dpiScale));
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
		int currentSize = m_cubeLayout.size;

		bool shouldLog = (currentClientSize != lastClientSize) ||
		                (currentPosition != lastPosition) ||
		                (currentSize != lastSize);

		if (shouldLog) {
		float dpiScale = m_canvas->getDPIScale();
			LOG_INF_S(std::string("NavigationCubeManager::render: Cube position updated - ") +
				"clientSize: " + std::to_string(currentClientSize.x) + "x" + std::to_string(currentClientSize.y) +
				", position: x=" + std::to_string(m_cubeLayout.x) + ", y=" + std::to_string(m_cubeLayout.y) +
				", size=" + std::to_string(m_cubeLayout.size) +
				", physical: " + std::to_string(m_cubeLayout.x * dpiScale) + "x" +
				std::to_string(m_cubeLayout.y * dpiScale) + " to " +
				std::to_string((m_cubeLayout.x + m_cubeLayout.size) * dpiScale) + "x" +
				std::to_string((m_cubeLayout.y + m_cubeLayout.size) * dpiScale));

			lastClientSize = currentClientSize;
			lastPosition = currentPosition;
			lastSize = currentSize;
		}

		m_navCube->render(m_cubeLayout.x, m_cubeLayout.y, wxSize(m_cubeLayout.size, m_cubeLayout.size));
	}
}

bool NavigationCubeManager::handleMouseEvent(wxMouseEvent& event) {
	if (!m_navCube || !m_isEnabled) {
		return false;
	}

	float dpiScale = m_canvas->getDPIScale();
	float x = event.GetX() / dpiScale;
	float y = event.GetY() / dpiScale;

	if (x >= m_cubeLayout.x && x < (m_cubeLayout.x + m_cubeLayout.size) &&
		y >= m_cubeLayout.y && y < (m_cubeLayout.y + m_cubeLayout.size)) {
		wxMouseEvent cubeEvent(event);
		cubeEvent.m_x = static_cast<int>((x - m_cubeLayout.x) * dpiScale);
		cubeEvent.m_y = static_cast<int>((y - m_cubeLayout.y) * dpiScale);

		int scaled_cube_dimension = static_cast<int>(m_cubeLayout.size * dpiScale);
		wxSize cube_viewport_scaled_size(scaled_cube_dimension, scaled_cube_dimension);

		if (event.GetEventType() == wxEVT_LEFT_DOWN ||
			event.GetEventType() == wxEVT_LEFT_UP ||
			event.GetEventType() == wxEVT_MOTION) {
			m_navCube->handleMouseEvent(cubeEvent, cube_viewport_scaled_size);
			m_canvas->Refresh(true);
			return true; // Event handled
		}
	}
	return false; // Event not handled
}

void NavigationCubeManager::handleSizeChange() {
	wxSize size = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();

	LOG_INF_S("NavigationCubeManager::handleSizeChange: Window size changed to " +
		std::to_string(size.x) + "x" + std::to_string(size.y) +
		", dpiScale: " + std::to_string(dpiScale) +
		", current cube size: " + std::to_string(m_cubeLayout.size) +
		", margins: " + std::to_string(m_marginx) + "x" + std::to_string(m_marginy));

	// Update cube layout with logical coordinates
	m_cubeLayout.update(size.x - m_cubeLayout.size - m_marginx,
		m_marginy,
		m_cubeLayout.size, size, dpiScale);

	LOG_INF_S(std::string("NavigationCubeManager::handleSizeChange: Cube repositioned - ") +
		"clientSize: " + std::to_string(size.x) + "x" + std::to_string(size.y) +
		", cubeSize: " + std::to_string(m_cubeLayout.size) +
		", margins: " + std::to_string(m_marginx) + "x" + std::to_string(m_marginy) +
		", calculated position: x=" + std::to_string(size.x) + " - " + std::to_string(m_cubeLayout.size) + " - " + std::to_string(m_marginx) + " = " +
		std::to_string(size.x - m_cubeLayout.size - m_marginx) +
		", y=" + std::to_string(m_marginy) +
		", final position: x=" + std::to_string(m_cubeLayout.x) + ", y=" + std::to_string(m_cubeLayout.y) + ", size=" + std::to_string(m_cubeLayout.size));

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
		LOG_INF_S("NavigationCubeManager::handleDPIChange: Updated margins to new DPI-scaled values: " +
			std::to_string(m_marginx) + "x" + std::to_string(m_marginy) +
			", dpiScale: " + std::to_string(dpiScale));
	} else {
		LOG_INF_S("NavigationCubeManager::handleDPIChange: Keeping configured margins: " +
			std::to_string(m_marginx) + "x" + std::to_string(m_marginy) +
			", dpiScale: " + std::to_string(dpiScale));
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
		LOG_WRN_S("NavigationCubeManager::setRect: Invalid parameters: x=" + std::to_string(x) +
			", y=" + std::to_string(y) + ", size=" + std::to_string(size));
		return;
	}
	wxSize clientSize = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();

	// Update layout with new parameters
	m_cubeLayout.update(x, y, size, clientSize, dpiScale);

	// Note: Don't update m_cubeConfig here as it should preserve the user's intended configuration
	// Only update config when explicitly setting configuration through setConfig()

	m_canvas->Refresh(true);

	LOG_INF_S("NavigationCubeManager::setRect: Set navigation cube rect - input: x=" + std::to_string(x) +
		", y=" + std::to_string(y) + ", size=" + std::to_string(size) +
		", final: x=" + std::to_string(m_cubeLayout.x) + ", y=" + std::to_string(m_cubeLayout.y) +
		", size=" + std::to_string(m_cubeLayout.size) +
		", clientSize: " + std::to_string(clientSize.x) + "x" + std::to_string(clientSize.y) +
		", dpiScale: " + std::to_string(dpiScale));
}

void NavigationCubeManager::setColor(const wxColour& color) {
	if (!m_navCube) {
		LOG_WRN_S("NavigationCubeManager::setColor: Skipped: nav cube not created");
		return;
	}
	LOG_INF_S("NavigationCubeManager::setColor: Set navigation cube color to R=" + std::to_string(color.GetRed()) +
		", G=" + std::to_string(color.GetGreen()) + ", B=" + std::to_string(color.GetBlue()));
	m_canvas->Refresh(true);
}

void NavigationCubeManager::setViewportSize(int size) {
	if (!m_navCube) {
		LOG_WRN_S("NavigationCubeManager::setViewportSize: Skipped: nav cube not created");
		return;
	}
	if (size < 50) {
		LOG_WRN_S("NavigationCubeManager::setViewportSize: Invalid size: " + std::to_string(size));
		return;
	}

	// Update the cube layout size and recalculate position
	wxSize clientSize = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();

	// Log before changes
	LOG_INF_S("NavigationCubeManager::setViewportSize: Changing size from " + std::to_string(m_cubeLayout.size) +
		" to " + std::to_string(size) +
		", clientSize: " + std::to_string(clientSize.x) + "x" + std::to_string(clientSize.y) +
		", dpiScale: " + std::to_string(dpiScale));

	// Update layout size
	m_cubeLayout.size = size;

	// Recalculate position to maintain centering
	int centeredX, centeredY;
	calculateCenteredPosition(centeredX, centeredY, size, clientSize);
	m_cubeLayout.update(centeredX, centeredY, size, clientSize, dpiScale);

	// Note: Don't update m_cubeConfig here as it should preserve the user's intended viewport size
	// Only update config when explicitly setting configuration through setConfig()

	LOG_INF_S("NavigationCubeManager::setViewportSize: Set navigation cube viewport size to " + std::to_string(size) +
		", repositioned to x=" + std::to_string(m_cubeLayout.x) + ", y=" + std::to_string(m_cubeLayout.y) +
		", physical pixels: " + std::to_string(m_cubeLayout.x * dpiScale) + "x" +
		std::to_string(m_cubeLayout.y * dpiScale) + " to " +
		std::to_string((m_cubeLayout.x + m_cubeLayout.size) * dpiScale) + "x" +
		std::to_string((m_cubeLayout.y + m_cubeLayout.size) * dpiScale));

	m_canvas->Refresh(true);
}

void NavigationCubeManager::syncCubeCameraToMain() {
	if (!m_navCube || !m_sceneManager) {
		LOG_WRN_S("NavigationCubeManager::syncCubeCameraToMain: Skipped: components missing");
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
		LOG_WRN_S("NavigationCubeManager::syncMainCameraToCube: Skipped: components missing");
		return;
	}

	SoCamera* navCamera = m_navCube->getCamera();
	if (!navCamera) {
		LOG_WRN_S("NavigationCubeManager::syncMainCameraToCube: Navigation cube camera is null.");
		return;
	}

	SoCamera* mainCamera = m_sceneManager->getCamera();
	if (!mainCamera) {
		LOG_WRN_S("NavigationCubeManager::syncMainCameraToCube: Main scene camera is null.");
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
		LOG_WRN_S("NavigationCubeManager::syncMainCameraToCube: NavCam position is origin, cannot determine direction.");
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

	// Create dialog with real-time config change callback
	NavigationCubeConfigDialog dialog(m_canvas->GetParent(), m_cubeConfig, clientWidthLogical, clientHeightLogical,
		[this](const CubeConfig& config) {
			// Apply configuration changes in real-time
			setConfig(config);
		});

	if (dialog.ShowModal() == wxID_OK) {
		CubeConfig newConfig = dialog.GetConfig();
		setConfig(newConfig);

		// Save configuration to persistent storage
		saveConfigToPersistent();

		LOG_INF_S("NavigationCubeManager::showConfigDialog: Configuration updated and saved");
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
	// Log configuration being applied
	LOG_INF_S(std::string("NavigationCubeManager::applyConfig: Applying configuration - ") +
		"position: (" + std::to_string(config.x) + "," + std::to_string(config.y) + "), " +
		"size: " + std::to_string(config.size) + ", " +
		"viewportSize: " + std::to_string(config.viewportSize) + ", " +
		"cubeSize: " + std::to_string(config.cubeSize) + ", " +
		"showEdges: " + std::string(config.showEdges ? "true" : "false") + ", " +
		"showCorners: " + std::string(config.showCorners ? "true" : "false") + ", " +
		"showTextures: " + std::string(config.showTextures ? "true" : "false"));

	// Apply position and size
	setRect(config.x, config.y, config.size);
	setViewportSize(config.viewportSize);

	// Apply colors and material properties to the cube
	if (m_navCube) {
		m_navCube->applyConfig(config);
		LOG_INF_S("NavigationCubeManager::applyConfig: Applied cube-specific configuration");
	}

	// Refresh display
	m_canvas->Refresh(true);
	LOG_INF_S("NavigationCubeManager::applyConfig: Applied new configuration - final position: (" +
		std::to_string(m_cubeLayout.x) + "," + std::to_string(m_cubeLayout.y) + "), size: " + std::to_string(m_cubeLayout.size));
}

void NavigationCubeManager::centerCubeInViewport() {
	wxSize clientSize = m_canvas->GetClientSize();
	float dpiScale = m_canvas->getDPIScale();

	LOG_INF_S("NavigationCubeManager::centerCubeInViewport: Centering cube - current position: (" +
		std::to_string(m_cubeLayout.x) + "," + std::to_string(m_cubeLayout.y) + "), size: " + std::to_string(m_cubeLayout.size) +
		", clientSize: " + std::to_string(clientSize.x) + "x" + std::to_string(clientSize.y) +
		", dpiScale: " + std::to_string(dpiScale));

	int centeredX, centeredY;

	calculateCenteredPosition(centeredX, centeredY, m_cubeLayout.size, clientSize);

	// Update configuration with centered position
	m_cubeConfig.x = centeredX;
	m_cubeConfig.y = centeredY;

	// Apply the centered position
	setRect(centeredX, centeredY, m_cubeLayout.size);

	LOG_INF_S("NavigationCubeManager::centerCubeInViewport: Centered cube at x=" +
		std::to_string(centeredX) + ", y=" + std::to_string(centeredY) +
		", logical position: " + std::to_string(centeredX * dpiScale) + "x" +
		std::to_string(centeredY * dpiScale) + " to " +
		std::to_string((centeredX + m_cubeLayout.size) * dpiScale) + "x" +
		std::to_string((centeredY + m_cubeLayout.size) * dpiScale));
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

		LOG_INF_S("NavigationCubeManager::calculateCenteredPosition: Using configured margins - "
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

	LOG_INF_S("NavigationCubeManager::calculateCenteredPosition: Using circle-centered fallback: x=" +
		std::to_string(x) + ", y=" + std::to_string(y) +
		" (circle center: " + std::to_string(circleCenterX) + "," + std::to_string(circleCenterY) + ")" +
		" (window: " + std::to_string(windowWidthLogical) + "x" + std::to_string(windowHeightLogical) + ")");
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
	if (configManager.save()) {
		LOG_INF_S("NavigationCubeManager::saveConfigToPersistent: Configuration saved successfully");
	} else {
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
	
	LOG_INF_S("NavigationCubeManager::loadConfigFromPersistent: Configuration loaded from persistent storage");
}