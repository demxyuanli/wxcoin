#pragma once

#include <wx/wx.h>
#include <wx/glcanvas.h>
#include "interfaces/IViewportManager.h"

// Forward declarations
class RenderingEngine;
class NavigationCubeManager;

class ViewportManager : public IViewportManager {
public:
	explicit ViewportManager(wxGLCanvas* canvas);
	~ViewportManager();

	void handleSizeChange(const wxSize& size) override;
	void updateDPISettings() override;
	void applyDPIScalingToUI() override;

	float getDPIScale() const override { return m_dpiScale; }

	void setRenderingEngine(RenderingEngine* renderingEngine) { m_renderingEngine = renderingEngine; }
	void setNavigationCubeManager(NavigationCubeManager* navCubeManager) override { m_navigationCubeManager = navCubeManager; }

private:
	bool shouldProcessSizeEvent(const wxSize& size);

	wxGLCanvas* m_canvas;
	RenderingEngine* m_renderingEngine;
	NavigationCubeManager* m_navigationCubeManager;

	float m_dpiScale;
	wxSize m_lastSize;
	wxLongLong m_lastEventTime;
};