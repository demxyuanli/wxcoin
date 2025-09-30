#pragma once

#include "widgets/FramelessModalPopup.h"
#include <wx/spinctrl.h>
#include <wx/colour.h>
#include <wx/colordlg.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/notebook.h>
#include "Logger.h"

struct CubeConfig {
    // Position and Size
    int x = 20;
    int y = 20;
    int size = 280;
    int viewportSize = 280;
    
    // Colors
    wxColour backgroundColor = wxColour(240, 240, 240);
    wxColour textColor = wxColour(0, 0, 0);
    wxColour edgeColor = wxColour(128, 128, 128);
    wxColour cornerColor = wxColour(200, 200, 200);
    
    // Material Properties
    float transparency = 0.0f;
    float shininess = 0.5f;
    float ambientIntensity = 0.8f;
    
    // Display Options
    bool showEdges = true;
    bool showCorners = true;
    bool showTextures = true;
    bool enableAnimation = true;
    
    // Cube Geometry
    float cubeSize = 0.55f;
    float chamferSize = 0.14f;
    float cameraDistance = 3.5f;
    
    // Circle Navigation Area
    int circleRadius = 150;
    int circleMarginX = 50;  // Distance from right edge to circle center
    int circleMarginY = 50;  // Distance from top edge to circle center
};

class NavigationCubeConfigDialog : public FramelessModalPopup {
public:
	using ConfigChangedCallback = std::function<void(const CubeConfig&)>;

	NavigationCubeConfigDialog(wxWindow* parent, const CubeConfig& config, int maxX, int maxY,
							   ConfigChangedCallback callback = nullptr);

	CubeConfig GetConfig() const;

private:
	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnChooseBackgroundColor(wxCommandEvent& event);
	void OnChooseTextColor(wxCommandEvent& event);
	void OnChooseEdgeColor(wxCommandEvent& event);
	void OnChooseCornerColor(wxCommandEvent& event);
	void OnTransparencyChanged(wxScrollEvent& event);
	void OnShininessChanged(wxScrollEvent& event);
	void OnAmbientChanged(wxScrollEvent& event);
	void OnCubeSizeChanged(wxScrollEvent& event);
	void OnChamferSizeChanged(wxScrollEvent& event);
	void OnCameraDistanceChanged(wxScrollEvent& event);
	void OnCenterCube(wxCommandEvent& event);
	void OnSizeChanged(wxSpinEvent& event);
	void OnCheckBoxChanged(wxCommandEvent& event);
	
	// Tab creation methods
	void CreatePositionTab(wxPanel* panel, int maxX, int maxY);
	void CreateColorsTab(wxPanel* panel);
	void CreateMaterialTab(wxPanel* panel);
	void CreateDisplayTab(wxPanel* panel);
	void CreateGeometryTab(wxPanel* panel);

	// Position and Size controls
	wxSpinCtrl* m_xCtrl;
	wxSpinCtrl* m_yCtrl;
	wxSpinCtrl* m_sizeCtrl;
	wxSpinCtrl* m_viewportSizeCtrl;
	
	// Circle area controls
	wxSpinCtrl* m_circleMarginXCtrl;
	wxSpinCtrl* m_circleMarginYCtrl;
	wxSpinCtrl* m_circleRadiusCtrl;
	
	// Color controls
	wxButton* m_backgroundColorButton;
	wxButton* m_textColorButton;
	wxButton* m_edgeColorButton;
	wxButton* m_cornerColorButton;
	
	// Material controls
	wxSlider* m_transparencySlider;
	wxSlider* m_shininessSlider;
	wxSlider* m_ambientSlider;
	
	// Display options
	wxCheckBox* m_showEdgesCheck;
	wxCheckBox* m_showCornersCheck;
	wxCheckBox* m_showTexturesCheck;
	wxCheckBox* m_enableAnimationCheck;
	
	// Geometry controls
	wxSlider* m_cubeSizeSlider;
	wxSlider* m_chamferSizeSlider;
	wxSlider* m_cameraDistanceSlider;
	
	// Configuration data
	CubeConfig m_config;
	int m_maxX, m_maxY; // Store max values for centering calculation
	ConfigChangedCallback m_configChangedCallback;

	DECLARE_EVENT_TABLE()
};