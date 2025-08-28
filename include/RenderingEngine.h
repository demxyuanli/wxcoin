#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <wx/longlong.h>
#include "interfaces/IRenderingEngine.h"

// Forward declarations
class SceneManager;
class NavigationCubeManager;

class RenderingEngine : public IRenderingEngine {
public:
	static const int RENDER_INTERVAL;

	explicit RenderingEngine(wxGLCanvas* canvas);
	~RenderingEngine();

	bool initialize() override;
	void render(bool fastMode = false) override;
	void renderWithoutSwap(bool fastMode = false) override;
	void swapBuffers() override;
	void handleResize(const wxSize& size) override;
	void updateViewport(const wxSize& size, float dpiScale) override;

	bool isInitialized() const { return m_isInitialized; }
	bool isRendering() const { return m_isRendering; }

	void setSceneManager(SceneManager* sceneManager) { m_sceneManager = sceneManager; }
	void setSceneManager(ISceneManager* sceneManager) override { m_sceneManager = reinterpret_cast<SceneManager*>(sceneManager); }
	void setNavigationCubeManager(NavigationCubeManager* navCubeManager) { m_navigationCubeManager = navCubeManager; }

private:
	void setupGLContext();
	void clearBuffers();
	void presentFrame();

	wxGLCanvas* m_canvas;
	std::unique_ptr<wxGLContext> m_glContext;
	SceneManager* m_sceneManager;
	NavigationCubeManager* m_navigationCubeManager;

	bool m_isInitialized;
	bool m_isRendering;
	wxLongLong m_lastRenderTime;
};