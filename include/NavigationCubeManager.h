#pragma once

#include <memory>
#include <wx/gdicmn.h>
#include "NavigationCubeConfigDialog.h"

class Canvas;
class SceneManager;
class wxMouseEvent;
class CuteNavCube;
class wxColour;

class NavigationCubeManager {
public:
	NavigationCubeManager(Canvas* canvas, SceneManager* sceneManager);
	~NavigationCubeManager();

	void render();
	bool handleMouseEvent(wxMouseEvent& event);
	void handleSizeChange();
	void handleDPIChange();

	void setEnabled(bool enabled);
	bool isEnabled() const;

	void showConfigDialog();

	// Configuration management
	void setConfig(const CubeConfig& config);
	CubeConfig getConfig() const;
	void applyConfig(const CubeConfig& config);
	
	// Persistent configuration
	void saveConfigToPersistent();
	void loadConfigFromPersistent();
	
	// Positioning utilities
	void centerCubeInViewport();
	void calculateCenteredPosition(int& x, int& y, int cubeSize, const wxSize& windowSize);
	
	// Legacy methods for backward compatibility
	void setRect(int x, int y, int size);
	void setColor(const wxColour& color);
	void setViewportSize(int size);

	void syncMainCameraToCube();
	void syncCubeCameraToMain();

private:
	void initCube();

	struct Layout {
		int x{ 20 }, y{ 20 }, cubeSize{ 280 };
		void update(int newX_logical, int newY_logical, int newSize_logical,
			const wxSize& windowSize_logical, float dpiScale);
	} m_cubeLayout;

	Canvas* m_canvas;
	SceneManager* m_sceneManager;

	std::unique_ptr<CuteNavCube> m_navCube;
	bool m_isEnabled;
	float m_marginx = 40.0f;
	float m_marginy = 40.0f;
	CubeConfig m_cubeConfig;
};