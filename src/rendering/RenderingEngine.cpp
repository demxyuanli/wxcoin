#include "RenderingEngine.h"
#include "SceneManager.h"
#include "NavigationCubeManager.h"
#include "logger/Logger.h"
#include "utils/PerformanceBus.h"
#include "config/RenderingConfig.h"
#include "config/ConfigManager.h"
#include "SoFCBackgroundGradient.h"
#include <Inventor/actions/SoGLRenderAction.h>
#include <wx/msgdlg.h>
#include <stdexcept>
#include <GL/gl.h>
#include <chrono> // Added for rendering timing

const int RenderingEngine::RENDER_INTERVAL = 16; // ~60 FPS (milliseconds)

RenderingEngine::RenderingEngine(wxGLCanvas* canvas)
	: m_canvas(canvas)
	, m_glContext(nullptr)
	, m_sceneManager(nullptr)
	, m_navigationCubeManager(nullptr)
	, m_backgroundMode(0)
	, m_backgroundTexture(nullptr)
	, m_backgroundTextureLoaded(false)
	, m_backgroundGradient(nullptr)
	, m_backgroundImage(nullptr)
	, m_backgroundTextureFitMode(1)
	, m_isInitialized(false)
	, m_isRendering(false)
	, m_lastRenderTime(0)
{
	// Initialize background color arrays
	m_backgroundColor[0] = m_backgroundColor[1] = m_backgroundColor[2] = 0.0f;
	m_backgroundGradientTop[0] = m_backgroundGradientTop[1] = m_backgroundGradientTop[2] = 0.0f;
	m_backgroundGradientBottom[0] = m_backgroundGradientBottom[1] = m_backgroundGradientBottom[2] = 0.0f;

	LOG_INF_S("RenderingEngine::RenderingEngine: Initializing");
}

RenderingEngine::~RenderingEngine() {
	// Clean up background texture
	if (m_backgroundTexture) {
		m_backgroundTexture->unref();
		m_backgroundTexture = nullptr;
	}

	// Clean up background gradient
	if (m_backgroundGradient) {
		m_backgroundGradient->unref();
		m_backgroundGradient = nullptr;
	}

	// Clean up background image
	if (m_backgroundImage) {
		m_backgroundImage->unref();
		m_backgroundImage = nullptr;
	}

	LOG_INF_S("RenderingEngine::~RenderingEngine: Destroying");
}

bool RenderingEngine::initialize() {
	if (m_isInitialized) {
		LOG_WRN_S("RenderingEngine::initialize: Already initialized");
		return true;
	}

	try {
		setupGLContext();

		// Load background configuration
		m_backgroundMode = ConfigManager::getInstance().getInt("Canvas", "BackgroundMode", 0);

		m_backgroundColor[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorR", 1.0));
		m_backgroundColor[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorG", 1.0));
		m_backgroundColor[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorB", 1.0));

	m_backgroundGradientTop[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopR", 0.9));
	m_backgroundGradientTop[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopG", 0.95));
	m_backgroundGradientTop[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopB", 1.0));

	m_backgroundGradientBottom[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomR", 0.6));
	m_backgroundGradientBottom[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomG", 0.8));
	m_backgroundGradientBottom[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomB", 1.0));

	LOG_INF_S("RenderingEngine::initialize: Loaded background config - mode: " + std::to_string(m_backgroundMode) +
		", solid color: (" + std::to_string(m_backgroundColor[0]) + ", " + std::to_string(m_backgroundColor[1]) + ", " + std::to_string(m_backgroundColor[2]) + ")" +
		", gradient top: (" + std::to_string(m_backgroundGradientTop[0]) + ", " + std::to_string(m_backgroundGradientTop[1]) + ", " + std::to_string(m_backgroundGradientTop[2]) + ")" +
		", gradient bottom: (" + std::to_string(m_backgroundGradientBottom[0]) + ", " + std::to_string(m_backgroundGradientBottom[1]) + ", " + std::to_string(m_backgroundGradientBottom[2]) + ")");

		// Initialize and configure FreeCAD background gradient node for gradient mode
		if (m_backgroundMode == 1) {
			if (SoFCBackgroundGradient::getClassTypeId() == SoType::badType()) {
				SoFCBackgroundGradient::initClass();
			}
			m_backgroundGradient = new SoFCBackgroundGradient;
			m_backgroundGradient->ref();
		LOG_INF_S("RenderingEngine::initialize: Created gradient node for mode 1");

		// Configure gradient colors: first parameter must be top color
		SbColor topColor(m_backgroundGradientTop[0], m_backgroundGradientTop[1], m_backgroundGradientTop[2]);
		SbColor bottomColor(m_backgroundGradientBottom[0], m_backgroundGradientBottom[1], m_backgroundGradientBottom[2]);
		m_backgroundGradient->setGradient(SoFCBackgroundGradient::LINEAR);
		m_backgroundGradient->setColorGradient(topColor, bottomColor);
		LOG_INF_S("RenderingEngine::initialize: Linear gradient top=(" +
			std::to_string(m_backgroundGradientTop[0]) + "," +
			std::to_string(m_backgroundGradientTop[1]) + "," +
			std::to_string(m_backgroundGradientTop[2]) + ") bottom=(" +
			std::to_string(m_backgroundGradientBottom[0]) + "," +
			std::to_string(m_backgroundGradientBottom[1]) + "," +
			std::to_string(m_backgroundGradientBottom[2]) + ")");
		}

		// Initialize FreeCAD gradient for radial mode too
		if (m_backgroundMode == 2) {
			if (!m_backgroundGradient) {
				if (SoFCBackgroundGradient::getClassTypeId() == SoType::badType()) {
					SoFCBackgroundGradient::initClass();
				}
		m_backgroundGradient = new SoFCBackgroundGradient;
		m_backgroundGradient->ref();
	}
	
	SbColor topColor(m_backgroundGradientTop[0], m_backgroundGradientTop[1], m_backgroundGradientTop[2]);
	SbColor bottomColor(m_backgroundGradientBottom[0], m_backgroundGradientBottom[1], m_backgroundGradientBottom[2]);
	m_backgroundGradient->setGradient(SoFCBackgroundGradient::RADIAL);
	m_backgroundGradient->setColorGradient(topColor, bottomColor);
	LOG_INF_S("RenderingEngine::initialize: Radial gradient top=(" +
		std::to_string(m_backgroundGradientTop[0]) + "," +
		std::to_string(m_backgroundGradientTop[1]) + "," +
		std::to_string(m_backgroundGradientTop[2]) + ") bottom=(" +
		std::to_string(m_backgroundGradientBottom[0]) + "," +
		std::to_string(m_backgroundGradientBottom[1]) + "," +
		std::to_string(m_backgroundGradientBottom[2]) + ")");
		}

		// Load background image fit mode
		m_backgroundTextureFitMode = ConfigManager::getInstance().getInt("Canvas", "BackgroundTextureFitMode", 1);

		// Initialize and configure FreeCAD background image node for image mode
		if (m_backgroundMode == 3) {
			if (SoFCBackgroundImage::getClassTypeId() == SoType::badType()) {
				SoFCBackgroundImage::initClass();
			}
			m_backgroundImage = new SoFCBackgroundImage;
			m_backgroundImage->ref();
			
			std::string texturePath = ConfigManager::getInstance().getString("Canvas", "BackgroundTexturePath", "");
			if (!texturePath.empty()) {
				m_backgroundImage->setImagePath(texturePath);
			}
			m_backgroundImage->setFitMode(m_backgroundTextureFitMode);
		}

		m_isInitialized = true;

		const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		LOG_INF_S("RenderingEngine::initialize: GL Context created. OpenGL version: " +
			std::string(glVersion ? glVersion : "unknown") +
			", Background mode: " + std::to_string(m_backgroundMode));

		// After initialization, reload config to ensure latest values from file/system
		// This is important because ConfigManager might load config file after RenderingEngine initialization
		reloadBackgroundConfig();

		return true;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("RenderingEngine::initialize: Failed: " + std::string(e.what()));
		return false;
	}
}

void RenderingEngine::setupGLContext() {
	if (!m_canvas) {
		throw std::runtime_error("Canvas is null");
	}

	m_glContext = std::make_unique<wxGLContext>(m_canvas);
	if (!m_glContext || !m_canvas->SetCurrent(*m_glContext)) {
		throw std::runtime_error("Failed to create/set GL context");
	}
}

void RenderingEngine::render(bool fastMode) {
	renderWithoutSwap(fastMode);
	presentFrame();
}

void RenderingEngine::renderWithoutSwap(bool fastMode) {
	auto renderStartTime = std::chrono::high_resolution_clock::now();

	if (!m_isInitialized) {
		return;
	}

	if (!m_canvas->IsShown() || !m_glContext || !m_sceneManager) {
		return;
	}

	if (m_isRendering) {
		return;
	}

	wxLongLong currentTime = wxGetLocalTimeMillis();
	if (currentTime - m_lastRenderTime < RENDER_INTERVAL) {
		return;
	}

	m_isRendering = true;
	m_lastRenderTime = currentTime;

	try {
		auto contextStartTime = std::chrono::high_resolution_clock::now();
		
		// Check if GL context is still valid
		if (!m_glContext) {
			LOG_ERR_S("RenderingEngine::renderWithoutSwap: GL context is null");
			m_isRendering = false;
			return;
		}
		
		if (!m_canvas->SetCurrent(*m_glContext)) {
			LOG_ERR_S("RenderingEngine::renderWithoutSwap: Failed to set GL context");
			m_isRendering = false;
			return;
		}
		
		// Verify OpenGL context is working
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			LOG_WRN_S("RenderingEngine::renderWithoutSwap: OpenGL error before rendering: " + std::to_string(err));
		}
		
		auto contextEndTime = std::chrono::high_resolution_clock::now();
		auto contextDuration = std::chrono::duration_cast<std::chrono::microseconds>(contextEndTime - contextStartTime);

		wxSize size = m_canvas->GetClientSize();
		if (size.x <= 0 || size.y <= 0) {
			m_isRendering = false;
			return;
		}

		auto clearStartTime = std::chrono::high_resolution_clock::now();
		// clearBuffers() will set viewport internally
		clearBuffers();
		auto clearEndTime = std::chrono::high_resolution_clock::now();
		auto clearDuration = std::chrono::duration_cast<std::chrono::microseconds>(clearEndTime - clearStartTime);

		// Viewport is already set in clearBuffers(), no need to set again
		std::chrono::microseconds viewportDuration(0);

		// Render main scene
		auto sceneStartTime = std::chrono::high_resolution_clock::now();
		m_sceneManager->render(size, fastMode);
		auto sceneEndTime = std::chrono::high_resolution_clock::now();
		auto sceneDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sceneEndTime - sceneStartTime);

		// Render navigation cube
		if (m_navigationCubeManager) {
			auto navCubeStartTime = std::chrono::high_resolution_clock::now();
			m_navigationCubeManager->render();
			auto navCubeEndTime = std::chrono::high_resolution_clock::now();
			auto navCubeDuration = std::chrono::duration_cast<std::chrono::milliseconds>(navCubeEndTime - navCubeStartTime);
		}

		auto renderEndTime = std::chrono::high_resolution_clock::now();
		auto renderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(renderEndTime - renderStartTime);

		// Publish to PerformanceDataBus
		perf::EnginePerfSample e;
		e.contextUs = static_cast<int>(contextDuration.count());
		e.clearUs = static_cast<int>(clearDuration.count());
		e.viewportUs = static_cast<int>(viewportDuration.count());
		e.sceneMs = static_cast<int>(sceneDuration.count());
		e.totalMs = static_cast<int>(renderDuration.count());
		e.fps = 1000.0 / std::max(1, e.totalMs);
		perf::PerformanceBus::instance().setEngine(e);
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception during render: " + std::string(e.what()));
		clearBuffers();
		m_isRendering = false;

		wxMessageDialog dialog(nullptr,
			"Rendering failed: " + std::string(e.what()) +
			". Please check system resources or restart the application.",
			"Rendering Error", wxOK | wxICON_ERROR);
		dialog.ShowModal();
	}

	m_isRendering = false;
}

void RenderingEngine::swapBuffers() {
	presentFrame();
}

void RenderingEngine::clearBuffers() {
	// Get viewport size and set it
	wxSize size = m_canvas->GetClientSize();
	glViewport(0, 0, size.x, size.y);
	
	// Render background based on mode (with correct viewport)
	renderBackground(size);

	// After background rendering (mode 0: solid color clears both, modes 1/2: gradient clears depth, mode 3: image clears both)
	// Clear depth buffer to ensure clean state for scene rendering
	glClear(GL_DEPTH_BUFFER_BIT);
}

void RenderingEngine::presentFrame() {
	m_canvas->SwapBuffers();
}

void RenderingEngine::renderBackground() {
	// Get canvas size for background rendering
	wxSize size = m_canvas->GetClientSize();
	renderBackground(size);
}

void RenderingEngine::renderBackground(const wxSize& size) {
	switch (m_backgroundMode) {
		case 0: // Solid color
			glClearColor(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			break;

		case 1: // Linear Gradient - use FreeCAD-style gradient
			if (m_backgroundGradient) {
				// Set gradient type if not already set
				if (m_backgroundGradient->getGradient() != SoFCBackgroundGradient::LINEAR) {
					m_backgroundGradient->setGradient(SoFCBackgroundGradient::LINEAR);
				}
				// Create viewport region with correct size
				SbViewportRegion vpRegion(size.x, size.y);
				SoGLRenderAction* action = new SoGLRenderAction(vpRegion);
				m_backgroundGradient->GLRender(action);
				delete action;
				LOG_DBG_S("RenderingEngine::renderBackground: Rendered linear gradient");
			} else {
				LOG_WRN_S("RenderingEngine::renderBackground: No gradient node available, using fallback");
				renderGradientBackground(size);
			}
			break;

		case 2: // Radial Gradient - use FreeCAD-style gradient
			if (m_backgroundGradient) {
				// Set gradient type if not already set
				if (m_backgroundGradient->getGradient() != SoFCBackgroundGradient::RADIAL) {
					m_backgroundGradient->setGradient(SoFCBackgroundGradient::RADIAL);
				}
				// Create viewport region with correct size
				SbViewportRegion vpRegion(size.x, size.y);
				SoGLRenderAction* action = new SoGLRenderAction(vpRegion);
				m_backgroundGradient->GLRender(action);
				delete action;
				LOG_INF_S("RenderingEngine::renderBackground: Rendered radial gradient");
			} else {
				LOG_WRN_S("RenderingEngine::renderBackground: No gradient node available, using fallback");
				renderGradientBackground(size);
			}
			break;

		case 3: // Texture - use FreeCAD-style image
			// Clear with background color first (for uncovered areas in fit mode)
			glClearColor(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			if (m_backgroundImage) {
				// Create viewport region with correct size
				SbViewportRegion vpRegion(size.x, size.y);
				SoGLRenderAction* action = new SoGLRenderAction(vpRegion);
				m_backgroundImage->GLRender(action);
				delete action;
			} else {
				renderTextureBackground(size);
			}
			break;

		default:
			// Fallback to solid color
			glClearColor(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			break;
	}
}

void RenderingEngine::renderGradientBackground(const wxSize& size) {
	int width = size.x;
	int height = size.y;

	if (width <= 0 || height <= 0) {
		// Invalid size, use solid color fallback
		glClearColor(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	// Save current states
	GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
	GLint matrixMode;
	glGetIntegerv(GL_MATRIX_MODE, &matrixMode);

	// Disable depth testing for background
	glDisable(GL_DEPTH_TEST);

	// Set up orthographic projection for 2D rendering
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Disable texturing and enable smooth shading for gradient
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	// Draw gradient quad
	glBegin(GL_QUADS);
		// Bottom-left (darker)
		glColor3f(m_backgroundGradientBottom[0], m_backgroundGradientBottom[1], m_backgroundGradientBottom[2]);
		glVertex2f(0.0f, 0.0f);

		// Bottom-right (darker)
		glColor3f(m_backgroundGradientBottom[0], m_backgroundGradientBottom[1], m_backgroundGradientBottom[2]);
		glVertex2f((GLfloat)width, 0.0f);

		// Top-right (lighter)
		glColor3f(m_backgroundGradientTop[0], m_backgroundGradientTop[1], m_backgroundGradientTop[2]);
		glVertex2f((GLfloat)width, (GLfloat)height);

		// Top-left (lighter)
		glColor3f(m_backgroundGradientTop[0], m_backgroundGradientTop[1], m_backgroundGradientTop[2]);
		glVertex2f(0.0f, (GLfloat)height);
	glEnd();

	// Restore matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// Restore depth testing state
	if (depthTestEnabled) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}

	// Restore matrix mode
	glMatrixMode(matrixMode);
}

void RenderingEngine::renderTextureBackground(const wxSize& size) {
	if (!m_backgroundTextureLoaded) {
		// Fallback to solid color if texture not loaded
		glClearColor(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	// Disable depth testing for background
	glDisable(GL_DEPTH_TEST);

	// Set up orthographic projection for 2D rendering
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	int width = size.x;
	int height = size.y;

	glOrtho(0, width, 0, height, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Bind the background texture
	if (m_backgroundTexture) {
		// For Open Inventor texture, we need to access the raw OpenGL texture
		// This is a simplified implementation - in practice, you might need
		// to extract the texture ID from SoTexture2
		glBindTexture(GL_TEXTURE_2D, 0); // Placeholder - needs proper texture binding
	}

	// Draw textured quad covering entire viewport
	glColor3f(1.0f, 1.0f, 1.0f); // White to not tint the texture
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(width, 0);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(width, height);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0, height);
	glEnd();

	// Disable texturing
	glDisable(GL_TEXTURE_2D);

	// Restore matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// Re-enable depth testing
	glEnable(GL_DEPTH_TEST);
}

bool RenderingEngine::loadBackgroundTexture(const std::string& texturePath) {
	try {
		// For now, just mark as not loaded since proper texture loading
		// from Open Inventor would require more complex implementation
		m_backgroundTextureLoaded = false;
		LOG_INF_S("RenderingEngine::loadBackgroundTexture: Texture loading not implemented yet: " + texturePath);
		return false;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("RenderingEngine::loadBackgroundTexture: Failed to load texture '" + texturePath + "': " + e.what());
		m_backgroundTextureLoaded = false;
		return false;
	}
}

void RenderingEngine::handleResize(const wxSize& size) {
	if (!m_isInitialized || !m_glContext) {
		LOG_WRN_S("RenderingEngine::handleResize: Not initialized or invalid context");
		return;
	}

	if (size.x > 0 && size.y > 0 && m_canvas->SetCurrent(*m_glContext)) {
		if (m_sceneManager) {
			m_sceneManager->updateAspectRatio(size);
		}

		if (m_navigationCubeManager) {
			m_navigationCubeManager->handleSizeChange();
		}

		m_canvas->Refresh();
	}
	else {
		LOG_WRN_S("RenderingEngine::handleResize: Skipped: Invalid size or context");
	}
}

void RenderingEngine::updateViewport(const wxSize& size, float dpiScale) {
	if (!m_isInitialized) {
		return;
	}

	glViewport(0, 0, static_cast<int>(size.x * dpiScale), static_cast<int>(size.y * dpiScale));
}

void RenderingEngine::updateCoordinateSystemColorsForBackground()
{
	LOG_INF_S("RenderingEngine::updateCoordinateSystemColorsForBackground: Called");
	
	if (!m_sceneManager) {
		LOG_WRN_S("RenderingEngine::updateCoordinateSystemColorsForBackground: SceneManager is null!");
		return;
	}

	// Calculate average background brightness
	float avgBrightness = 0.0f;

	switch (m_backgroundMode) {
	case 0: // Solid color
		avgBrightness = (m_backgroundColor[0] + m_backgroundColor[1] + m_backgroundColor[2]) / 3.0f;
		break;
	case 1: // Linear gradient
	case 2: // Radial gradient
		avgBrightness = ((m_backgroundGradientTop[0] + m_backgroundGradientTop[1] + m_backgroundGradientTop[2]) +
			(m_backgroundGradientBottom[0] + m_backgroundGradientBottom[1] + m_backgroundGradientBottom[2])) / 6.0f;
		break;
	case 3: // Texture/Image
		// For images, assume medium brightness (will adjust if needed)
		avgBrightness = 0.5f;
		break;
	default:
		avgBrightness = 0.5f;
		break;
	}

	// Update coordinate system colors via SceneManager
	m_sceneManager->updateCoordinateSystemColorsForBackground(avgBrightness);
}

void RenderingEngine::reloadBackgroundConfig() {
	if (!m_isInitialized) {
		return;
	}

	LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Reloading background configuration");

	// Reload background configuration from ConfigManager
	int newBackgroundMode = ConfigManager::getInstance().getInt("Canvas", "BackgroundMode", 0);

	m_backgroundColor[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorR", 1.0));
	m_backgroundColor[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorG", 1.0));
	m_backgroundColor[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorB", 1.0));

	m_backgroundGradientTop[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopR", 0.9));
	m_backgroundGradientTop[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopG", 0.95));
	m_backgroundGradientTop[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopB", 1.0));

	m_backgroundGradientBottom[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomR", 0.6));
	m_backgroundGradientBottom[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomG", 0.8));
	m_backgroundGradientBottom[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomB", 1.0));

	// Handle mode changes: create or destroy background nodes as needed
	if (newBackgroundMode != m_backgroundMode) {
		// Mode changed - handle node lifecycle
		if (newBackgroundMode == 1 || newBackgroundMode == 2) {
			// Switching to gradient mode - create node if not exists
			if (!m_backgroundGradient) {
				if (SoFCBackgroundGradient::getClassTypeId() == SoType::badType()) {
					SoFCBackgroundGradient::initClass();
				}
				m_backgroundGradient = new SoFCBackgroundGradient;
				m_backgroundGradient->ref();
				LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Created gradient node for mode " + std::to_string(newBackgroundMode));
			}
			// Clean up image node if exists
			if (m_backgroundImage) {
				m_backgroundImage->unref();
				m_backgroundImage = nullptr;
				LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Removed image node");
			}
		} else if (newBackgroundMode == 3) {
			// Switching to image mode - create node if not exists
			if (!m_backgroundImage) {
				if (SoFCBackgroundImage::getClassTypeId() == SoType::badType()) {
					SoFCBackgroundImage::initClass();
				}
				m_backgroundImage = new SoFCBackgroundImage;
				m_backgroundImage->ref();
				LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Created image node");
			}
			// Clean up gradient node if exists
			if (m_backgroundGradient) {
				m_backgroundGradient->unref();
				m_backgroundGradient = nullptr;
				LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Removed gradient node");
			}
		} else {
			// Switching to solid color - cleanup all nodes
			if (m_backgroundGradient) {
				m_backgroundGradient->unref();
				m_backgroundGradient = nullptr;
				LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Removed gradient node");
			}
			if (m_backgroundImage) {
				m_backgroundImage->unref();
				m_backgroundImage = nullptr;
				LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Removed image node");
			}
		}
		m_backgroundMode = newBackgroundMode;
	}

	// Update gradient node if exists and mode requires it
	if (m_backgroundGradient && (m_backgroundMode == 1 || m_backgroundMode == 2)) {
		SoFCBackgroundGradient::Gradient gradType = (m_backgroundMode == 1) ? SoFCBackgroundGradient::LINEAR : SoFCBackgroundGradient::RADIAL;
		m_backgroundGradient->setGradient(gradType);
		
		SbColor topColor(m_backgroundGradientTop[0], m_backgroundGradientTop[1], m_backgroundGradientTop[2]);
		SbColor bottomColor(m_backgroundGradientBottom[0], m_backgroundGradientBottom[1], m_backgroundGradientBottom[2]);
		m_backgroundGradient->setColorGradient(topColor, bottomColor);
		LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Updated gradient colors top=(" +
			std::to_string(m_backgroundGradientTop[0]) + "," +
			std::to_string(m_backgroundGradientTop[1]) + "," +
			std::to_string(m_backgroundGradientTop[2]) + ") bottom=(" +
			std::to_string(m_backgroundGradientBottom[0]) + "," +
			std::to_string(m_backgroundGradientBottom[1]) + "," +
			std::to_string(m_backgroundGradientBottom[2]) + ")");
	}

	// Update image node if exists and mode requires it
	if (m_backgroundImage && m_backgroundMode == 3) {
		std::string texturePath = ConfigManager::getInstance().getString("Canvas", "BackgroundTexturePath", "");
		if (!texturePath.empty()) {
			m_backgroundImage->setImagePath(texturePath);
			LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Updated image path");
		}
		m_backgroundTextureFitMode = ConfigManager::getInstance().getInt("Canvas", "BackgroundTextureFitMode", 1);
		m_backgroundImage->setFitMode(m_backgroundTextureFitMode);
		LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Updated fit mode to " + std::to_string(m_backgroundTextureFitMode));
	}

	// Update coordinate system colors based on new background
	// Only update if SceneManager is available (it may not be during early initialization)
	if (m_sceneManager) {
		updateCoordinateSystemColorsForBackground();
	}

	LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Background configuration reloaded - mode: " + std::to_string(m_backgroundMode));
}

void RenderingEngine::triggerRefresh() {
	if (m_canvas) {
		m_canvas->Refresh();
		m_canvas->Update();
	}
}