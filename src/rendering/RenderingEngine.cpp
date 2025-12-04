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

#ifdef _DEBUG
int RenderingEngine::s_glErrorLogCount = 0;
int RenderingEngine::s_contextErrorLogCount = 0;
int RenderingEngine::s_configLoadLogCount = 0;
#endif

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

#ifdef _DEBUG
	if (s_configLoadLogCount < 3) {
		LOG_INF_S("RenderingEngine::~RenderingEngine: Destroying");
		s_configLoadLogCount++;
	}
#endif
}

void RenderingEngine::setSceneManager(SceneManager* sceneManager)
{
	m_sceneManager = sceneManager;

	// If we already have a gradient node, keep its colors in sync with the
	// current configuration and, for gradient modes, attach it to the scene
	// graph so Coin3D renders the background (like PreviewCanvas).
	if (!m_sceneManager || !m_backgroundGradient) {
		return;
	}

	SoSeparator* root = m_sceneManager->getSceneRoot();
	if (!root) {
		return;
	}

	int idx = root->findChild(m_backgroundGradient);
	if (m_backgroundMode == 1 || m_backgroundMode == 2) {
		if (idx < 0) {
			// Insert at the beginning so it renders before lights and geometry.
			root->insertChild(m_backgroundGradient, 0);
		}
	} else if (idx >= 0) {
		root->removeChild(idx);
	}
}

void RenderingEngine::setSceneManager(ISceneManager* sceneManager)
{
	// In this project ISceneManager is implemented by SceneManager.
	setSceneManager(static_cast<SceneManager*>(sceneManager));
}

bool RenderingEngine::initialize() {
	if (m_isInitialized) {
		LOG_WRN_S("RenderingEngine::initialize: Already initialized");
		return true;
	}

	try {
		setupGLContext();

		// Load background configuration using cached method
		loadBackgroundConfig();

#ifdef _DEBUG
		if (s_configLoadLogCount < 3) {
			LOG_INF_S("RenderingEngine::initialize: Loaded background config - mode: " + std::to_string(m_backgroundMode) +
				", solid color: (" + std::to_string(m_backgroundColor[0]) + ", " + std::to_string(m_backgroundColor[1]) + ", " + std::to_string(m_backgroundColor[2]) + ")" +
				", gradient top: (" + std::to_string(m_backgroundGradientTop[0]) + ", " + std::to_string(m_backgroundGradientTop[1]) + ", " + std::to_string(m_backgroundGradientTop[2]) + ")" +
				", gradient bottom: (" + std::to_string(m_backgroundGradientBottom[0]) + ", " + std::to_string(m_backgroundGradientBottom[1]) + ", " + std::to_string(m_backgroundGradientBottom[2]) + ")");
			s_configLoadLogCount++;
		}
#endif

		// Initialize and configure FreeCAD background gradient node for gradient mode
		if (m_backgroundMode == 1) {
			if (SoFCBackgroundGradient::getClassTypeId() == SoType::badType()) {
				SoFCBackgroundGradient::initClass();
			}
			m_backgroundGradient = new SoFCBackgroundGradient;
			m_backgroundGradient->ref();
#ifdef _DEBUG
			if (s_configLoadLogCount < 3) {
				LOG_INF_S("RenderingEngine::initialize: Created gradient node for mode 1");
				s_configLoadLogCount++;
			}
#endif

		// Configure gradient colors: first parameter must be top color
		SbColor topColor(m_backgroundGradientTop[0], m_backgroundGradientTop[1], m_backgroundGradientTop[2]);
		SbColor bottomColor(m_backgroundGradientBottom[0], m_backgroundGradientBottom[1], m_backgroundGradientBottom[2]);
		m_backgroundGradient->setGradient(SoFCBackgroundGradient::LINEAR);
		m_backgroundGradient->setColorGradient(topColor, bottomColor);
#ifdef _DEBUG
		if (s_configLoadLogCount < 3) {
			LOG_INF_S("RenderingEngine::initialize: Linear gradient top=(" +
				std::to_string(m_backgroundGradientTop[0]) + "," +
				std::to_string(m_backgroundGradientTop[1]) + "," +
				std::to_string(m_backgroundGradientTop[2]) + ") bottom=(" +
				std::to_string(m_backgroundGradientBottom[0]) + "," +
				std::to_string(m_backgroundGradientBottom[1]) + "," +
				std::to_string(m_backgroundGradientBottom[2]) + ")");
			s_configLoadLogCount++;
		}
#endif
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
#ifdef _DEBUG
	if (s_configLoadLogCount < 3) {
		LOG_INF_S("RenderingEngine::initialize: Radial gradient top=(" +
			std::to_string(m_backgroundGradientTop[0]) + "," +
			std::to_string(m_backgroundGradientTop[1]) + "," +
			std::to_string(m_backgroundGradientTop[2]) + ") bottom=(" +
			std::to_string(m_backgroundGradientBottom[0]) + "," +
			std::to_string(m_backgroundGradientBottom[1]) + "," +
			std::to_string(m_backgroundGradientBottom[2]) + ")");
		s_configLoadLogCount++;
	}
#endif
		}

		// Initialize and configure FreeCAD background image node for image mode
		if (m_backgroundMode == 3) {
			if (SoFCBackgroundImage::getClassTypeId() == SoType::badType()) {
				SoFCBackgroundImage::initClass();
			}
			m_backgroundImage = new SoFCBackgroundImage;
			m_backgroundImage->ref();

			if (!m_cachedConfig.texturePath.empty()) {
				m_backgroundImage->setImagePath(m_cachedConfig.texturePath);
			}
			m_backgroundImage->setFitMode(m_cachedConfig.textureFitMode);
		}

		m_isInitialized = true;

		const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
#ifdef _DEBUG
		if (s_configLoadLogCount < 3) {
			LOG_INF_S("RenderingEngine::initialize: GL Context created. OpenGL version: " +
				std::string(glVersion ? glVersion : "unknown") +
				", Background mode: " + std::to_string(m_backgroundMode));
			s_configLoadLogCount++;
		}
#endif

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

bool RenderingEngine::ensureGLContext() const {
	if (!m_isInitialized) {
		return false;
	}

	if (!m_glContext) {
#ifdef _DEBUG
		if (s_contextErrorLogCount < 5) {
			LOG_ERR_S("RenderingEngine::ensureGLContext: GL context is null");
			s_contextErrorLogCount++;
		}
#endif
		return false;
	}

	if (!m_canvas->SetCurrent(*m_glContext)) {
#ifdef _DEBUG
		if (s_contextErrorLogCount < 5) {
			LOG_ERR_S("RenderingEngine::ensureGLContext: Failed to set GL context");
			s_contextErrorLogCount++;
		}
#endif
		return false;
	}

	return true;
}

void RenderingEngine::loadBackgroundConfig() {
	// Load all background configuration at once and cache it
	m_cachedConfig.mode = ConfigManager::getInstance().getInt("Canvas", "BackgroundMode", 0);
	m_cachedConfig.color[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorR", 1.0));
	m_cachedConfig.color[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorG", 1.0));
	m_cachedConfig.color[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorB", 1.0));
	m_cachedConfig.gradientTop[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopR", 0.9));
	m_cachedConfig.gradientTop[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopG", 0.95));
	m_cachedConfig.gradientTop[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientTopB", 1.0));
	m_cachedConfig.gradientBottom[0] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomR", 0.6));
	m_cachedConfig.gradientBottom[1] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomG", 0.8));
	m_cachedConfig.gradientBottom[2] = static_cast<float>(ConfigManager::getInstance().getDouble("Canvas", "BackgroundGradientBottomB", 1.0));
	m_cachedConfig.textureFitMode = ConfigManager::getInstance().getInt("Canvas", "BackgroundTextureFitMode", 1);
	m_cachedConfig.texturePath = ConfigManager::getInstance().getString("Canvas", "BackgroundTexturePath", "");
	m_cachedConfig.isValid = true;

	// Update local member variables from cache
	m_backgroundMode = m_cachedConfig.mode;
	m_backgroundColor[0] = m_cachedConfig.color[0];
	m_backgroundColor[1] = m_cachedConfig.color[1];
	m_backgroundColor[2] = m_cachedConfig.color[2];
	m_backgroundGradientTop[0] = m_cachedConfig.gradientTop[0];
	m_backgroundGradientTop[1] = m_cachedConfig.gradientTop[1];
	m_backgroundGradientTop[2] = m_cachedConfig.gradientTop[2];
	m_backgroundGradientBottom[0] = m_cachedConfig.gradientBottom[0];
	m_backgroundGradientBottom[1] = m_cachedConfig.gradientBottom[1];
	m_backgroundGradientBottom[2] = m_cachedConfig.gradientBottom[2];
	m_backgroundTextureFitMode = m_cachedConfig.textureFitMode;
}

void RenderingEngine::render(bool fastMode) {
	// Convenience wrapper: perform a full frame render (background + scene + swap)
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

		// Ensure GL context is valid
		if (!ensureGLContext()) {
			m_isRendering = false;
			return;
		}

		// CRITICAL FIX: Verify OpenGL context is actually functional
		// SetCurrent() might return true even if the context is lost/reset
		const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		if (!glVersion) {
			LOG_ERR_S("RenderingEngine::renderWithoutSwap: Context set but glGetString(GL_VERSION) returned NULL. Aborting frame.");
			m_isRendering = false;
			return;
		}

		// Verify OpenGL context is working
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
#ifdef _DEBUG
			if (s_glErrorLogCount < 5) {
				LOG_WRN_S("RenderingEngine::renderWithoutSwap: OpenGL error before rendering: " + std::to_string(err));
				s_glErrorLogCount++;
			}
#endif
		}
		
		auto contextEndTime = std::chrono::high_resolution_clock::now();
		auto contextDuration = std::chrono::duration_cast<std::chrono::microseconds>(contextEndTime - contextStartTime);

		wxSize size = m_canvas->GetClientSize();
		if (size.x <= 0 || size.y <= 0) {
			m_isRendering = false;
			return;
		}

		auto clearStartTime = std::chrono::high_resolution_clock::now();
		// clearBuffers() will render the background and clear the depth buffer
		clearBuffers();
		auto clearEndTime = std::chrono::high_resolution_clock::now();
		auto clearDuration = std::chrono::duration_cast<std::chrono::microseconds>(clearEndTime - clearStartTime);

		// Viewport is already set in clearBuffers(), no need to set again
		std::chrono::microseconds viewportDuration(0);

		// Optional debug path: render background only (no scene), controlled by config.
		int debugBackgroundOnly = ConfigManager::getInstance().getInt("Canvas", "DebugBackgroundOnly", 0);
		if (debugBackgroundOnly != 0) {
			auto renderEndTime = std::chrono::high_resolution_clock::now();
			auto renderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(renderEndTime - renderStartTime);

			perf::EnginePerfSample e;
			e.contextUs = static_cast<int>(contextDuration.count());
			e.clearUs = static_cast<int>(clearDuration.count());
			e.viewportUs = static_cast<int>(viewportDuration.count());
			e.sceneMs = 0;
			e.totalMs = static_cast<int>(renderDuration.count());
			e.fps = 1000.0 / std::max(1, e.totalMs);
			perf::PerformanceBus::instance().setEngine(e);

			m_isRendering = false;
			return;
		}

		// Render main scene
		auto sceneStartTime = std::chrono::high_resolution_clock::now();
		m_sceneManager->render(size, fastMode);
		auto sceneEndTime = std::chrono::high_resolution_clock::now();
		auto sceneDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sceneEndTime - sceneStartTime);

		// Render navigation cube (optional debug flag to disable it)
		int debugDisableNavigationCube = ConfigManager::getInstance().getInt("Canvas", "DebugDisableNavigationCube", 0);
		if (m_navigationCubeManager && debugDisableNavigationCube == 0) {
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
	if (!m_isInitialized || !m_canvas) {
		return;
	}

	if (!m_canvas->IsShown()) {
		return;
	}

	if (!ensureGLContext()) {
		return;
	}

	wxSize size = m_canvas->GetClientSize();
	if (size.x <= 0 || size.y <= 0) {
		return;
	}

	glViewport(0, 0, size.x, size.y);
	renderBackground(size);

	glClear(GL_DEPTH_BUFFER_BIT);
}

void RenderingEngine::presentFrame() {
	m_canvas->SwapBuffers();
}

void RenderingEngine::renderBackground() {
	if (!m_isInitialized || !m_canvas) {
		return;
	}

	if (!m_canvas->IsShown()) {
		return;
	}

	// Ensure the correct OpenGL context is current before drawing the background
	if (!ensureGLContext()) {
		return;
	}

	wxSize size = m_canvas->GetClientSize();
	if (size.x <= 0 || size.y <= 0) {
		return;
	}

	// Set viewport to cover the full canvas
	glViewport(0, 0, size.x, size.y);

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
#ifdef _DEBUG
				LOG_DBG_S("RenderingEngine::renderBackground: Rendered linear gradient");
#endif
			} else {
#ifdef _DEBUG
				if (s_configLoadLogCount < 3) {
					LOG_WRN_S("RenderingEngine::renderBackground: No gradient node available, using fallback");
					s_configLoadLogCount++;
				}
#endif
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
#ifdef _DEBUG
				LOG_INF_S("RenderingEngine::renderBackground: Rendered radial gradient");
#endif
			} else {
#ifdef _DEBUG
				if (s_configLoadLogCount < 3) {
					LOG_WRN_S("RenderingEngine::renderBackground: No gradient node available, using fallback");
					s_configLoadLogCount++;
				}
#endif
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

	// Save current OpenGL states
	GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
	GLboolean texture2DEnabled = glIsEnabled(GL_TEXTURE_2D);
	GLint matrixMode;
	glGetIntegerv(GL_MATRIX_MODE, &matrixMode);

	// Disable depth testing and texturing for background
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);

	// Set up orthographic projection for 2D rendering
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, width, 0, height, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glShadeModel(GL_SMOOTH);

	// Draw gradient quad - optimized immediate mode
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

	// Restore OpenGL states
	if (depthTestEnabled) {
		glEnable(GL_DEPTH_TEST);
	}
	if (texture2DEnabled) {
		glEnable(GL_TEXTURE_2D);
	}
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
	if (!m_isInitialized) {
#ifdef _DEBUG
		if (s_contextErrorLogCount < 3) {
			LOG_WRN_S("RenderingEngine::handleResize: Not initialized");
			s_contextErrorLogCount++;
		}
#endif
		return;
	}

	if (size.x > 0 && size.y > 0 && ensureGLContext()) {
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

#ifdef _DEBUG
	if (s_configLoadLogCount < 3) {
		LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Reloading background configuration");
		s_configLoadLogCount++;
	}
#endif

	// Reload background configuration using cached method
	loadBackgroundConfig();

	// Check if background mode changed and handle node lifecycle
	int newBackgroundMode = m_cachedConfig.mode;

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
#ifdef _DEBUG
				if (s_configLoadLogCount < 3) {
					LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Created gradient node for mode " + std::to_string(newBackgroundMode));
					s_configLoadLogCount++;
				}
#endif
			}
			// Clean up image node if exists
			if (m_backgroundImage) {
				m_backgroundImage->unref();
				m_backgroundImage = nullptr;
#ifdef _DEBUG
				if (s_configLoadLogCount < 3) {
					LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Removed image node");
					s_configLoadLogCount++;
				}
#endif
			}
		} else if (newBackgroundMode == 3) {
			// Switching to image mode - create node if not exists
			if (!m_backgroundImage) {
				if (SoFCBackgroundImage::getClassTypeId() == SoType::badType()) {
					SoFCBackgroundImage::initClass();
				}
				m_backgroundImage = new SoFCBackgroundImage;
				m_backgroundImage->ref();
#ifdef _DEBUG
				if (s_configLoadLogCount < 3) {
					LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Created image node");
					s_configLoadLogCount++;
				}
#endif
			}
			// Clean up gradient node if exists
			if (m_backgroundGradient) {
				m_backgroundGradient->unref();
				m_backgroundGradient = nullptr;
#ifdef _DEBUG
				if (s_configLoadLogCount < 3) {
					LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Removed gradient node");
					s_configLoadLogCount++;
				}
#endif
			}
		} else {
			// Switching to solid color - cleanup all nodes
			if (m_backgroundGradient) {
				m_backgroundGradient->unref();
				m_backgroundGradient = nullptr;
#ifdef _DEBUG
				if (s_configLoadLogCount < 3) {
					LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Removed gradient node");
					s_configLoadLogCount++;
				}
#endif
			}
			if (m_backgroundImage) {
				m_backgroundImage->unref();
				m_backgroundImage = nullptr;
#ifdef _DEBUG
				if (s_configLoadLogCount < 3) {
					LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Removed image node");
					s_configLoadLogCount++;
				}
#endif
			}
		}
		m_backgroundMode = newBackgroundMode;
	}

	// Update gradient node if exists and mode requires it
	if (m_backgroundGradient && (m_backgroundMode == 1 || m_backgroundMode == 2)) {
		SoFCBackgroundGradient::Gradient gradType = (m_backgroundMode == 1) ? SoFCBackgroundGradient::LINEAR : SoFCBackgroundGradient::RADIAL;
		m_backgroundGradient->setGradient(gradType);

		SbColor topColor(m_cachedConfig.gradientTop[0], m_cachedConfig.gradientTop[1], m_cachedConfig.gradientTop[2]);
		SbColor bottomColor(m_cachedConfig.gradientBottom[0], m_cachedConfig.gradientBottom[1], m_cachedConfig.gradientBottom[2]);
		m_backgroundGradient->setColorGradient(topColor, bottomColor);
#ifdef _DEBUG
		if (s_configLoadLogCount < 3) {
			LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Updated gradient colors top=(" +
				std::to_string(m_cachedConfig.gradientTop[0]) + "," +
				std::to_string(m_cachedConfig.gradientTop[1]) + "," +
				std::to_string(m_cachedConfig.gradientTop[2]) + ") bottom=(" +
				std::to_string(m_cachedConfig.gradientBottom[0]) + "," +
				std::to_string(m_cachedConfig.gradientBottom[1]) + "," +
				std::to_string(m_cachedConfig.gradientBottom[2]) + ")");
			s_configLoadLogCount++;
		}
#endif
	}

	// Update image node if exists and mode requires it
	if (m_backgroundImage && m_backgroundMode == 3) {
		if (!m_cachedConfig.texturePath.empty()) {
			m_backgroundImage->setImagePath(m_cachedConfig.texturePath);
#ifdef _DEBUG
			if (s_configLoadLogCount < 3) {
				LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Updated image path");
				s_configLoadLogCount++;
			}
#endif
		}
		m_backgroundImage->setFitMode(m_cachedConfig.textureFitMode);
#ifdef _DEBUG
		if (s_configLoadLogCount < 3) {
			LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Updated fit mode to " + std::to_string(m_cachedConfig.textureFitMode));
			s_configLoadLogCount++;
		}
#endif
	}

	// Update coordinate system colors based on new background
	// Only update if SceneManager is available (it may not be during early initialization)
	if (m_sceneManager) {
		updateCoordinateSystemColorsForBackground();
	}

#ifdef _DEBUG
	if (s_configLoadLogCount < 3) {
		LOG_INF_S("RenderingEngine::reloadBackgroundConfig: Background configuration reloaded - mode: " + std::to_string(m_backgroundMode));
		s_configLoadLogCount++;
	}
#endif
}

void RenderingEngine::triggerRefresh() {
	if (m_canvas) {
		m_canvas->Refresh();
		m_canvas->Update();
	}
}

bool RenderingEngine::isGLContextValid() const {
	if (!m_isInitialized || !m_glContext) {
		return false;
	}

	// Try to set the context and check if glGetString works
	if (!m_canvas->SetCurrent(*m_glContext)) {
		return false;
	}

	// Check if glGetString returns a valid string
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	return version != nullptr;
}

bool RenderingEngine::reinitialize() {
	LOG_INF_S("RenderingEngine::reinitialize: Attempting to reinitialize after context loss");

	try {
		// Clean up existing resources
		m_isInitialized = false;

		// Reinitialize
		if (initialize()) {
			LOG_INF_S("RenderingEngine::reinitialize: Successfully reinitialized");
			return true;
		} else {
			LOG_ERR_S("RenderingEngine::reinitialize: Failed to reinitialize");
			return false;
		}
	}
	catch (const std::exception& e) {
		LOG_ERR_S("RenderingEngine::reinitialize: Exception during reinitialize: " + std::string(e.what()));
		return false;
	}
}