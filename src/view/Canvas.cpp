#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "Canvas.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "ObjectTreePanel.h"
#include "logger/Logger.h"
#include "NavigationCubeManager.h"
#include "utils/PerformanceBus.h"
#include "ViewRefreshManager.h"
#include "RenderingEngine.h"
#include "EventCoordinator.h"
#include "ViewportManager.h"
#include "interfaces/ISubsystemFactory.h"
#include "interfaces/ServiceLocator.h"
#include "OCCViewer.h"
#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <wx/msgdlg.h>
#include "MultiViewportManager.h"
#include <chrono>
const int Canvas::s_canvasAttribs[] = {
	WX_GL_RGBA,
	WX_GL_DOUBLEBUFFER,
	WX_GL_DEPTH_SIZE, 24,
	WX_GL_STENCIL_SIZE, 8,
	// Request multisample buffers for MSAA
	WX_GL_SAMPLE_BUFFERS, 1,
	WX_GL_SAMPLES, 4,
	0 // Terminator
};

BEGIN_EVENT_TABLE(Canvas, wxGLCanvas)
EVT_PAINT(Canvas::onPaint)
EVT_SIZE(Canvas::onSize)
EVT_ERASE_BACKGROUND(Canvas::onEraseBackground)
EVT_LEFT_DOWN(Canvas::onMouseEvent)
EVT_LEFT_UP(Canvas::onMouseEvent)
EVT_MIDDLE_DOWN(Canvas::onMouseEvent)
EVT_MIDDLE_UP(Canvas::onMouseEvent)
EVT_RIGHT_DOWN(Canvas::onMouseEvent)
EVT_RIGHT_UP(Canvas::onMouseEvent)
EVT_MOTION(Canvas::onMouseEvent)
EVT_MOUSEWHEEL(Canvas::onMouseEvent)
EVT_LEAVE_WINDOW(Canvas::onMouseEvent)
END_EVENT_TABLE()

Canvas::Canvas(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
	: wxGLCanvas(parent, id, s_canvasAttribs, pos, size, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS | wxBORDER_NONE)
	, m_objectTreePanel(nullptr)
	, m_commandManager(nullptr)
	, m_occViewer(nullptr)
{
	LOG_INF_S("Canvas::Canvas: Initializing");

	SetName("Canvas");
	wxSize clientSize = GetClientSize();
	if (clientSize.x <= 0 || clientSize.y <= 0) {
		clientSize = wxSize(400, 300);
		SetSize(clientSize);
		SetMinSize(clientSize);
	}

	try {
		initializeSubsystems();
		connectSubsystems();

		if (m_sceneManager && !m_sceneManager->initScene()) {
			LOG_ERR_S("Canvas::Canvas: Failed to initialize main scene");
			showErrorDialog("Failed to initialize 3D scene. The application may not function correctly.");
			throw std::runtime_error("Scene initialization failed");
		}

		Refresh(true);
		Update();
		LOG_INF_S("Canvas::Canvas: Initialized successfully");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Canvas::Canvas: Initialization failed: " + std::string(e.what()));
		throw;
	}
}

Canvas::~Canvas() {
	LOG_INF_S("Canvas::Canvas: Destroying");
}

void Canvas::SetSubsystemFactory(ISubsystemFactory* factory) {
	ServiceLocator::setFactory(factory);
}

void Canvas::initializeSubsystems() {
	LOG_INF_S("Canvas::initializeSubsystems: Creating subsystems");

	// Create core subsystems
	m_refreshManager = std::make_unique<ViewRefreshManager>(this);

	ISubsystemFactory* factory = ServiceLocator::getFactory();

	m_renderingEngine.reset(factory ? factory->createRenderingEngine(this) : new RenderingEngine(this));
	m_viewportManager.reset(factory ? factory->createViewportManager(this) : new ViewportManager(this));
	m_eventCoordinator.reset(factory ? factory->createEventCoordinator() : new EventCoordinator());

	m_sceneManager.reset(factory ? factory->createSceneManager(this) : new SceneManager(this));
	m_inputManager.reset(factory ? factory->createInputManager(this) : new InputManager(this));
	m_navigationCubeManager.reset(factory ? factory->createNavigationCubeManager(this, m_sceneManager.get()) : new NavigationCubeManager(this, m_sceneManager.get()));

	// Initialize rendering engine FIRST
	if (!m_renderingEngine->initialize()) {
		showErrorDialog("Failed to initialize OpenGL context. Please check your graphics drivers.");
		throw std::runtime_error("RenderingEngine initialization failed");
	}

	// Create multi-viewport manager AFTER OpenGL context is ready
	// Do NOT create it here - delay until first render
	m_multiViewportEnabled = true;
}

void Canvas::connectSubsystems() {
	LOG_INF_S("Canvas::connectSubsystems: Connecting subsystems");

	// Connect rendering engine
	m_renderingEngine->setSceneManager(m_sceneManager.get());
	m_renderingEngine->setNavigationCubeManager(m_navigationCubeManager.get());

	// Connect viewport manager
	m_viewportManager->setRenderingEngine(m_renderingEngine.get());
	m_viewportManager->setNavigationCubeManager(m_navigationCubeManager.get());

	// Connect event coordinator
	m_eventCoordinator->setNavigationCubeManager(m_navigationCubeManager.get());
	m_eventCoordinator->setInputManager(m_inputManager.get());

	// Connect multi-viewport manager
	if (m_multiViewportManager) {
		m_multiViewportManager->setNavigationCubeManager(m_navigationCubeManager.get());
	}
}

void Canvas::showErrorDialog(const std::string& message) const {
	wxMessageDialog dialog(nullptr, message, "Error", wxOK | wxICON_ERROR);
	dialog.ShowModal();
}

void Canvas::render(bool fastMode) {
	LOG_DBG_S("=== CANVAS: STARTING RENDER (mode=" + std::string(fastMode ? "FAST" : "QUALITY") + ") ===");

	// Skip rendering if we're already rendering (prevents recursive calls)
	static bool isRendering = false;
	if (isRendering) {
		LOG_WRN_S("CANVAS: Recursive render call detected, skipping");
		return;
	}

	// Guard to ensure we reset the flag
	struct RenderGuard {
		bool& flag;
		RenderGuard(bool& f) : flag(f) { flag = true; }
		~RenderGuard() { flag = false; }
	} guard(isRendering);

	auto renderStartTime = std::chrono::high_resolution_clock::now();

	if (m_renderingEngine) {
		// Create MultiViewportManager on first render when GL context is active
		if (m_multiViewportEnabled && !m_multiViewportManager) {
			auto multiViewportStartTime = std::chrono::high_resolution_clock::now();
			try {
				m_multiViewportManager = std::make_unique<MultiViewportManager>(this, m_sceneManager.get());
				m_multiViewportManager->setNavigationCubeManager(m_navigationCubeManager.get());
				m_multiViewportManager->handleSizeChange(GetClientSize());
				auto multiViewportEndTime = std::chrono::high_resolution_clock::now();
				auto multiViewportDuration = std::chrono::duration_cast<std::chrono::milliseconds>(multiViewportEndTime - multiViewportStartTime);
				LOG_INF_S("MultiViewportManager created in " + std::to_string(multiViewportDuration.count()) + "ms");
			}
			catch (const std::exception& e) {
				LOG_ERR_S("Failed to create MultiViewportManager: " + std::string(e.what()));
				m_multiViewportEnabled = false;
			}
		}

		// Render main scene first (without swapping buffers)
		auto mainRenderStartTime = std::chrono::high_resolution_clock::now();
		m_renderingEngine->renderWithoutSwap(fastMode);
		auto mainRenderEndTime = std::chrono::high_resolution_clock::now();
		auto mainRenderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(mainRenderEndTime - mainRenderStartTime);

		// Render additional viewports on top of main scene
		if (m_multiViewportEnabled && m_multiViewportManager) {
			auto multiRenderStartTime = std::chrono::high_resolution_clock::now();
			m_multiViewportManager->render();
			auto multiRenderEndTime = std::chrono::high_resolution_clock::now();
			auto multiRenderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(multiRenderEndTime - multiRenderStartTime);
		}

		// Finally swap buffers to display everything
		auto swapStartTime = std::chrono::high_resolution_clock::now();
		m_renderingEngine->swapBuffers();
		auto swapEndTime = std::chrono::high_resolution_clock::now();
		auto swapDuration = std::chrono::duration_cast<std::chrono::milliseconds>(swapEndTime - swapStartTime);

		auto renderEndTime = std::chrono::high_resolution_clock::now();
		auto renderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(renderEndTime - renderStartTime);

		// Only publish performance data if render took significant time
		if (renderDuration.count() > 1) {
			perf::CanvasPerfSample c;
			c.mode = fastMode ? "FAST" : "QUALITY";
			c.mainSceneMs = static_cast<int>(mainRenderDuration.count());
			c.swapMs = static_cast<int>(swapDuration.count());
			c.totalMs = static_cast<int>(renderDuration.count());
			c.fps = 1000.0 / std::max(1, c.totalMs);
			perf::PerformanceBus::instance().setCanvas(c);
		}
	} else {
		LOG_WRN_S("CANVAS: No rendering engine available");
	}

	LOG_DBG_S("=== CANVAS: RENDER COMPLETED ===");
}

void Canvas::onPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);
	render(false);
	
	// Draw face info overlay after 3D rendering using Graphics Context for transparency support
	m_faceInfoOverlay.update();
	if (m_faceInfoOverlay.isVisible()) {
		// Use wxGCDC for proper alpha transparency support
		wxGCDC gcdc(dc);
		m_faceInfoOverlay.draw(gcdc, GetClientSize());
	}
	
	if (m_eventCoordinator) {
		m_eventCoordinator->handlePaintEvent(event);
	}
	event.Skip();
}

void Canvas::onSize(wxSizeEvent& event) {
	wxSize size = event.GetSize();
	if (m_viewportManager) {
		m_viewportManager->handleSizeChange(size);
	}
	if (m_multiViewportManager) {
		m_multiViewportManager->handleSizeChange(size);
	}
	if (m_eventCoordinator) {
		m_eventCoordinator->handleSizeEvent(event);
	}
	event.Skip();
}

void Canvas::onEraseBackground(wxEraseEvent& event) {
	// Do nothing to prevent flickering
}

void Canvas::onMouseEvent(wxMouseEvent& event) {
	// Check if this is an interaction event that should trigger LOD
	bool isInteractionEvent = false;
	if (event.GetEventType() == wxEVT_LEFT_DOWN ||
		event.GetEventType() == wxEVT_RIGHT_DOWN ||
		event.GetEventType() == wxEVT_MOTION ||
		event.GetEventType() == wxEVT_MOUSEWHEEL) {
		isInteractionEvent = true;
	}

	// Skip motion events during drag operations to reduce overhead
	static bool isDragging = false;
	if (event.GetEventType() == wxEVT_LEFT_DOWN || event.GetEventType() == wxEVT_RIGHT_DOWN) {
		isDragging = true;
	} else if (event.GetEventType() == wxEVT_LEFT_UP || event.GetEventType() == wxEVT_RIGHT_UP) {
		isDragging = false;
	}

	// Trigger LOD interaction if enabled
	if (isInteractionEvent && m_occViewer) {
		m_occViewer->startLODInteraction();
	}
	
	// Update face highlight on mouse move (but not during drag to reduce overhead)
	if (event.GetEventType() == wxEVT_MOTION && !isDragging && m_occViewer) {
		wxPoint screenPos = event.GetPosition();
		m_occViewer->updateFaceHighlightAt(screenPos);
	}
	
	// Clear face highlight when mouse leaves window
	if (event.GetEventType() == wxEVT_LEAVE_WINDOW && m_occViewer) {
		m_occViewer->updateFaceHighlightAt(wxPoint(-1, -1));
	}

	// Check multi-viewport first - this should have higher priority
	if (m_multiViewportEnabled && m_multiViewportManager) {
		bool handled = m_multiViewportManager->handleMouseEvent(event);
		if (handled) {
			return; // Event was handled, don't propagate further
		}
	}

	// Only pass to EventCoordinator if MultiViewportManager didn't handle it
	if (m_eventCoordinator) {
		bool handled = m_eventCoordinator->handleMouseEvent(event);
		if (handled) {
			return; // Event was handled, don't propagate further
		}
	}
	
	// Important: Don't skip the event if it was a drag-related event
	// This prevents unnecessary propagation to parent windows
	if (!isDragging || (event.GetEventType() != wxEVT_MOTION)) {
		event.Skip();
	}
}

void Canvas::setMultiViewportEnabled(bool enabled) {
	m_multiViewportEnabled = enabled;
	Refresh();
}

bool Canvas::isMultiViewportEnabled() const {
	return m_multiViewportEnabled;
}

void Canvas::setPickingCursor(bool enable) {
	SetCursor(enable ? wxCursor(wxCURSOR_CROSS) : wxCursor(wxCURSOR_DEFAULT));
}

SoCamera* Canvas::getCamera() const {
	if (!m_sceneManager) {
		LOG_WRN_S("Canvas::getCamera: SceneManager is null");
		return nullptr;
	}
	return m_sceneManager->getCamera();
}

void Canvas::resetView() {
	if (!m_sceneManager) {
		LOG_WRN_S("Canvas::resetView: SceneManager is null");
		return;
	}
	m_sceneManager->resetView();
}

void Canvas::setNavigationCubeEnabled(bool enabled) {
	if (m_navigationCubeManager) {
		m_navigationCubeManager->setEnabled(enabled);
	}
}

bool Canvas::isNavigationCubeEnabled() const {
	if (m_navigationCubeManager) {
		return m_navigationCubeManager->isEnabled();
	}
	return false;
}

void Canvas::ShowNavigationCubeConfigDialog() {
	if (m_navigationCubeManager) {
		m_navigationCubeManager->showConfigDialog();
	}
}

float Canvas::getDPIScale() const {
	if (m_viewportManager) {
		return m_viewportManager->getDPIScale();
	}
	return GetContentScaleFactor();
}