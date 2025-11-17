#pragma once

#include <wx/glcanvas.h>
#include <memory>
#include <wx/longlong.h>
#include <Inventor/nodes/SoTexture2.h>
#include "interfaces/IRenderingEngine.h"
#include "SoFCBackgroundGradient.h"
#include "SoFCBackgroundImage.h"

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

	void setSceneManager(SceneManager* sceneManager);
	void setSceneManager(ISceneManager* sceneManager) override;
	void setNavigationCubeManager(NavigationCubeManager* navCubeManager) { m_navigationCubeManager = navCubeManager; }

	// Background rendering - public methods for external access
	void renderBackground();
	void reloadBackgroundConfig();
	void updateCoordinateSystemColorsForBackground();
	void triggerRefresh();

private:
	void renderBackground(const wxSize& size);
	void setupGLContext();
	void clearBuffers();
	void presentFrame();
	bool loadBackgroundTexture(const std::string& texturePath);
	void renderGradientBackground(const wxSize& size);
	void renderTextureBackground(const wxSize& size);

	wxGLCanvas* m_canvas;
	std::unique_ptr<wxGLContext> m_glContext;
	SceneManager* m_sceneManager;
	NavigationCubeManager* m_navigationCubeManager;

	// Background rendering
	int m_backgroundMode;
	float m_backgroundColor[3];
	float m_backgroundGradientTop[3];
	float m_backgroundGradientBottom[3];
	SoTexture2* m_backgroundTexture;
	bool m_backgroundTextureLoaded;
	
	// FreeCAD-style background gradient
	SoFCBackgroundGradient* m_backgroundGradient;
	
	// FreeCAD-style background image
	SoFCBackgroundImage* m_backgroundImage;
	int m_backgroundTextureFitMode;

	bool m_isInitialized;
	bool m_isRendering;
	wxLongLong m_lastRenderTime;
};