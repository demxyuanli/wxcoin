#include "NavigationCubeConfigDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/msgdlg.h>

enum {
    ID_BACKGROUND_COLOR = 1000,
    ID_TEXT_COLOR,
    ID_EDGE_COLOR,
    ID_CORNER_COLOR,
    ID_TRANSPARENCY_SLIDER,
    ID_SHININESS_SLIDER,
    ID_AMBIENT_SLIDER,
    ID_CUBE_SIZE_SLIDER,
    ID_CHAMFER_SIZE_SLIDER,
    ID_CAMERA_DISTANCE_SLIDER,
    ID_CENTER_CUBE_BUTTON
};

BEGIN_EVENT_TABLE(NavigationCubeConfigDialog, FramelessModalPopup)
EVT_BUTTON(wxID_OK, NavigationCubeConfigDialog::OnOK)
EVT_BUTTON(wxID_CANCEL, NavigationCubeConfigDialog::OnCancel)
EVT_BUTTON(ID_BACKGROUND_COLOR, NavigationCubeConfigDialog::OnChooseBackgroundColor)
EVT_BUTTON(ID_TEXT_COLOR, NavigationCubeConfigDialog::OnChooseTextColor)
EVT_BUTTON(ID_EDGE_COLOR, NavigationCubeConfigDialog::OnChooseEdgeColor)
EVT_BUTTON(ID_CORNER_COLOR, NavigationCubeConfigDialog::OnChooseCornerColor)
EVT_COMMAND_SCROLL(ID_TRANSPARENCY_SLIDER, NavigationCubeConfigDialog::OnTransparencyChanged)
EVT_COMMAND_SCROLL(ID_SHININESS_SLIDER, NavigationCubeConfigDialog::OnShininessChanged)
EVT_COMMAND_SCROLL(ID_AMBIENT_SLIDER, NavigationCubeConfigDialog::OnAmbientChanged)
EVT_COMMAND_SCROLL(ID_CUBE_SIZE_SLIDER, NavigationCubeConfigDialog::OnCubeSizeChanged)
EVT_COMMAND_SCROLL(ID_CHAMFER_SIZE_SLIDER, NavigationCubeConfigDialog::OnChamferSizeChanged)
EVT_COMMAND_SCROLL(ID_CAMERA_DISTANCE_SLIDER, NavigationCubeConfigDialog::OnCameraDistanceChanged)
EVT_BUTTON(ID_CENTER_CUBE_BUTTON, NavigationCubeConfigDialog::OnCenterCube)
EVT_SPINCTRL(wxID_ANY, NavigationCubeConfigDialog::OnSizeChanged)
EVT_CHECKBOX(wxID_ANY, NavigationCubeConfigDialog::OnCheckBoxChanged)
END_EVENT_TABLE()

NavigationCubeConfigDialog::NavigationCubeConfigDialog(wxWindow* parent, const CubeConfig& config, int maxX, int maxY,
											   ConfigChangedCallback callback)
    : FramelessModalPopup(parent, "Navigation Cube Configuration", wxSize(450, 500))
    , m_config(config)
    , m_maxX(maxX)
    , m_maxY(maxY)
    , m_configChangedCallback(callback)
{
	// Set up title bar with icon
	SetTitleIcon("cube", wxSize(20, 20));
	ShowTitleIcon(true);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for tabbed interface
    wxNotebook* notebook = new wxNotebook(m_contentPanel, wxID_ANY);
    
    // Create tabs
    wxPanel* positionPanel = new wxPanel(notebook);
    CreatePositionTab(positionPanel, maxX, maxY);
    notebook->AddPage(positionPanel, "Position & Size");
    
    wxPanel* colorsPanel = new wxPanel(notebook);
    CreateColorsTab(colorsPanel);
    notebook->AddPage(colorsPanel, "Colors");
    
    wxPanel* materialPanel = new wxPanel(notebook);
    CreateMaterialTab(materialPanel);
    notebook->AddPage(materialPanel, "Material");
    
    wxPanel* displayPanel = new wxPanel(notebook);
    CreateDisplayTab(displayPanel);
    notebook->AddPage(displayPanel, "Display");
    
    wxPanel* geometryPanel = new wxPanel(notebook);
    CreateGeometryTab(geometryPanel);
    notebook->AddPage(geometryPanel, "Geometry");
    
    mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 10);
    
    // Button sizer
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(m_contentPanel, wxID_OK, "OK"), 0, wxRIGHT, 5);
	buttonSizer->Add(new wxButton(m_contentPanel, wxID_CANCEL, "Cancel"));
	mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);

	m_contentPanel->SetSizer(mainSizer);
	Layout();
	Centre();
}

CubeConfig NavigationCubeConfigDialog::GetConfig() const {
    CubeConfig config = m_config;

    // Position and Size - only update if user has made changes
    // This preserves the original config values if no changes were made
    if (m_xCtrl->GetValue() != m_config.x) {
        config.x = m_xCtrl->GetValue();
    }
    if (m_yCtrl->GetValue() != m_config.y) {
        config.y = m_yCtrl->GetValue();
    }
    if (m_sizeCtrl->GetValue() != m_config.size) {
        config.size = m_sizeCtrl->GetValue();
    }
    if (m_viewportSizeCtrl->GetValue() != m_config.viewportSize) {
        config.viewportSize = m_viewportSizeCtrl->GetValue();
    }

    // Material properties - always update as sliders might have default precision issues
    config.transparency = m_transparencySlider->GetValue() / 100.0f;
    config.shininess = m_shininessSlider->GetValue() / 100.0f;
    config.ambientIntensity = m_ambientSlider->GetValue() / 100.0f;

    // Display options - always update as checkbox state is reliable
    config.showEdges = m_showEdgesCheck->GetValue();
    config.showCorners = m_showCornersCheck->GetValue();
    config.showTextures = m_showTexturesCheck->GetValue();
    config.enableAnimation = m_enableAnimationCheck->GetValue();

    // Geometry - always update as sliders might have default precision issues
    config.cubeSize = m_cubeSizeSlider->GetValue() / 100.0f;
    config.chamferSize = m_chamferSizeSlider->GetValue() / 100.0f;
    config.cameraDistance = m_cameraDistanceSlider->GetValue() / 100.0f;

    // Circle area - only update if user has made changes
    if (m_circleMarginXCtrl->GetValue() != m_config.circleMarginX) {
        config.circleMarginX = m_circleMarginXCtrl->GetValue();
    }
    if (m_circleMarginYCtrl->GetValue() != m_config.circleMarginY) {
        config.circleMarginY = m_circleMarginYCtrl->GetValue();
    }
    if (m_circleRadiusCtrl->GetValue() != m_config.circleRadius) {
        config.circleRadius = m_circleRadiusCtrl->GetValue();
    }
    
    return config;
}

void NavigationCubeConfigDialog::CreatePositionTab(wxPanel* panel, int maxX, int maxY) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 2, 10, 10);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Right Margin:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_xCtrl = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                             wxSP_ARROW_KEYS, 0, maxX, m_config.x);
    gridSizer->Add(m_xCtrl, 0, wxEXPAND);

    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Top Margin:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_yCtrl = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                             wxSP_ARROW_KEYS, 0, maxY, m_config.y);
    gridSizer->Add(m_yCtrl, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Size:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_sizeCtrl = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 
                                wxSP_ARROW_KEYS, 50, std::min(maxX, maxY) / 2, m_config.size);
    gridSizer->Add(m_sizeCtrl, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Viewport Size:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_viewportSizeCtrl = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 
                                        wxSP_ARROW_KEYS, 50, std::min(maxX, maxY), m_config.viewportSize);
    gridSizer->Add(m_viewportSizeCtrl, 0, wxEXPAND);
    
    sizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);
    
    // Add center button
    wxButton* centerButton = new wxButton(panel, ID_CENTER_CUBE_BUTTON, "Center Cube in Circle");
    sizer->Add(centerButton, 0, wxEXPAND | wxALL, 10);
    
    // Add circle area configuration
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Circle Navigation Area:"), 0, wxEXPAND | wxALL, 5);
    
    wxFlexGridSizer* circleGridSizer = new wxFlexGridSizer(3, 2, 5, 5);
    circleGridSizer->AddGrowableCol(1);
    
    circleGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Circle Margin X:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_circleMarginXCtrl = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 
                                         wxSP_ARROW_KEYS, 20, maxX/2, m_config.circleMarginX);
    circleGridSizer->Add(m_circleMarginXCtrl, 0, wxEXPAND);
    
    circleGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Circle Margin Y:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_circleMarginYCtrl = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 
                                         wxSP_ARROW_KEYS, 20, maxY/2, m_config.circleMarginY);
    circleGridSizer->Add(m_circleMarginYCtrl, 0, wxEXPAND);
    
    circleGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Circle Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_circleRadiusCtrl = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 
                                        wxSP_ARROW_KEYS, 50, 300, m_config.circleRadius);
    circleGridSizer->Add(m_circleRadiusCtrl, 0, wxEXPAND);
    
    sizer->Add(circleGridSizer, 0, wxEXPAND | wxALL, 10);
    
    panel->SetSizer(sizer);
}

void NavigationCubeConfigDialog::CreateColorsTab(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 2, 10, 10);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Background Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_backgroundColorButton = new wxButton(panel, ID_BACKGROUND_COLOR, "Choose Color");
    m_backgroundColorButton->SetBackgroundColour(m_config.backgroundColor);
    gridSizer->Add(m_backgroundColorButton, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Text Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_textColorButton = new wxButton(panel, ID_TEXT_COLOR, "Choose Color");
    m_textColorButton->SetBackgroundColour(m_config.textColor);
    gridSizer->Add(m_textColorButton, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Edge Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_edgeColorButton = new wxButton(panel, ID_EDGE_COLOR, "Choose Color");
    m_edgeColorButton->SetBackgroundColour(m_config.edgeColor);
    gridSizer->Add(m_edgeColorButton, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Corner Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_cornerColorButton = new wxButton(panel, ID_CORNER_COLOR, "Choose Color");
    m_cornerColorButton->SetBackgroundColour(m_config.cornerColor);
    gridSizer->Add(m_cornerColorButton, 0, wxEXPAND);
    
    sizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);
    panel->SetSizer(sizer);
}

void NavigationCubeConfigDialog::CreateMaterialTab(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Transparency
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Transparency:"), 0, wxEXPAND | wxALL, 5);
    m_transparencySlider = new wxSlider(panel, ID_TRANSPARENCY_SLIDER, 
                                        static_cast<int>(m_config.transparency * 100), 
                                        0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(m_transparencySlider, 0, wxEXPAND | wxALL, 5);
    
    // Shininess
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Shininess:"), 0, wxEXPAND | wxALL, 5);
    m_shininessSlider = new wxSlider(panel, ID_SHININESS_SLIDER, 
                                     static_cast<int>(m_config.shininess * 100), 
                                     0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(m_shininessSlider, 0, wxEXPAND | wxALL, 5);
    
    // Ambient Intensity
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Ambient Intensity:"), 0, wxEXPAND | wxALL, 5);
    m_ambientSlider = new wxSlider(panel, ID_AMBIENT_SLIDER, 
                                   static_cast<int>(m_config.ambientIntensity * 100), 
                                   0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(m_ambientSlider, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void NavigationCubeConfigDialog::CreateDisplayTab(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    m_showEdgesCheck = new wxCheckBox(panel, wxID_ANY, "Show Edges");
    m_showEdgesCheck->SetValue(m_config.showEdges);
    sizer->Add(m_showEdgesCheck, 0, wxEXPAND | wxALL, 5);
    
    m_showCornersCheck = new wxCheckBox(panel, wxID_ANY, "Show Corners");
    m_showCornersCheck->SetValue(m_config.showCorners);
    sizer->Add(m_showCornersCheck, 0, wxEXPAND | wxALL, 5);
    
    m_showTexturesCheck = new wxCheckBox(panel, wxID_ANY, "Show Textures");
    m_showTexturesCheck->SetValue(m_config.showTextures);
    sizer->Add(m_showTexturesCheck, 0, wxEXPAND | wxALL, 5);
    
    m_enableAnimationCheck = new wxCheckBox(panel, wxID_ANY, "Enable Animation");
    m_enableAnimationCheck->SetValue(m_config.enableAnimation);
    sizer->Add(m_enableAnimationCheck, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void NavigationCubeConfigDialog::CreateGeometryTab(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Cube Size
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Cube Size:"), 0, wxEXPAND | wxALL, 5);
    m_cubeSizeSlider = new wxSlider(panel, ID_CUBE_SIZE_SLIDER, 
                                    static_cast<int>(m_config.cubeSize * 100), 
                                    30, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(m_cubeSizeSlider, 0, wxEXPAND | wxALL, 5);
    
    // Chamfer Size
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Chamfer Size:"), 0, wxEXPAND | wxALL, 5);
    m_chamferSizeSlider = new wxSlider(panel, ID_CHAMFER_SIZE_SLIDER, 
                                       static_cast<int>(m_config.chamferSize * 100), 
                                       5, 30, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(m_chamferSizeSlider, 0, wxEXPAND | wxALL, 5);
    
    // Camera Distance
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Camera Distance:"), 0, wxEXPAND | wxALL, 5);
    m_cameraDistanceSlider = new wxSlider(panel, ID_CAMERA_DISTANCE_SLIDER, 
                                          static_cast<int>(m_config.cameraDistance * 100), 
                                          200, 800, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    sizer->Add(m_cameraDistanceSlider, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void NavigationCubeConfigDialog::OnOK(wxCommandEvent& event) {
	EndModal(wxID_OK);
}

void NavigationCubeConfigDialog::OnCancel(wxCommandEvent& event) {
	EndModal(wxID_CANCEL);
}

void NavigationCubeConfigDialog::OnChooseBackgroundColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(m_config.backgroundColor);
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.backgroundColor = colorDialog.GetColourData().GetColour();
        m_backgroundColorButton->SetBackgroundColour(m_config.backgroundColor);
        m_backgroundColorButton->Refresh();
        if (m_configChangedCallback) {
            m_configChangedCallback(m_config);
        }
    }
}

void NavigationCubeConfigDialog::OnChooseTextColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(m_config.textColor);
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.textColor = colorDialog.GetColourData().GetColour();
        m_textColorButton->SetBackgroundColour(m_config.textColor);
        m_textColorButton->Refresh();
        if (m_configChangedCallback) {
            m_configChangedCallback(m_config);
        }
    }
}

void NavigationCubeConfigDialog::OnChooseEdgeColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(m_config.edgeColor);
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.edgeColor = colorDialog.GetColourData().GetColour();
        m_edgeColorButton->SetBackgroundColour(m_config.edgeColor);
        m_edgeColorButton->Refresh();
        if (m_configChangedCallback) {
            m_configChangedCallback(m_config);
        }
    }
}

void NavigationCubeConfigDialog::OnChooseCornerColor(wxCommandEvent& event) {
	wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(m_config.cornerColor);
	if (colorDialog.ShowModal() == wxID_OK) {
        m_config.cornerColor = colorDialog.GetColourData().GetColour();
        m_cornerColorButton->SetBackgroundColour(m_config.cornerColor);
        m_cornerColorButton->Refresh();
        if (m_configChangedCallback) {
            m_configChangedCallback(m_config);
        }
    }
}

void NavigationCubeConfigDialog::OnTransparencyChanged(wxScrollEvent& event) {
    m_config.transparency = event.GetPosition() / 100.0f;
    if (m_configChangedCallback) {
        m_configChangedCallback(m_config);
    }
}

void NavigationCubeConfigDialog::OnShininessChanged(wxScrollEvent& event) {
    m_config.shininess = event.GetPosition() / 100.0f;
    if (m_configChangedCallback) {
        m_configChangedCallback(m_config);
    }
}

void NavigationCubeConfigDialog::OnAmbientChanged(wxScrollEvent& event) {
    m_config.ambientIntensity = event.GetPosition() / 100.0f;
    if (m_configChangedCallback) {
        m_configChangedCallback(m_config);
    }
}

void NavigationCubeConfigDialog::OnCubeSizeChanged(wxScrollEvent& event) {
    m_config.cubeSize = event.GetPosition() / 100.0f;
    if (m_configChangedCallback) {
        m_configChangedCallback(m_config);
    }
}

void NavigationCubeConfigDialog::OnChamferSizeChanged(wxScrollEvent& event) {
    m_config.chamferSize = event.GetPosition() / 100.0f;
    if (m_configChangedCallback) {
        m_configChangedCallback(m_config);
    }
}

void NavigationCubeConfigDialog::OnCameraDistanceChanged(wxScrollEvent& event) {
    m_config.cameraDistance = event.GetPosition() / 100.0f;
    if (m_configChangedCallback) {
        m_configChangedCallback(m_config);
    }
}

void NavigationCubeConfigDialog::OnCheckBoxChanged(wxCommandEvent& event) {
    wxCheckBox* checkBox = dynamic_cast<wxCheckBox*>(event.GetEventObject());
    if (!checkBox) return;

    // Handle different check boxes
    if (checkBox == m_showEdgesCheck) {
        m_config.showEdges = checkBox->GetValue();
        LOG_INF_S(std::string("NavigationCubeConfigDialog::OnCheckBoxChanged: Show edges changed to ") +
                 (m_config.showEdges ? "true" : "false"));
    }
    else if (checkBox == m_showCornersCheck) {
        m_config.showCorners = checkBox->GetValue();
        LOG_INF_S(std::string("NavigationCubeConfigDialog::OnCheckBoxChanged: Show corners changed to ") +
                 (m_config.showCorners ? "true" : "false"));
    }
    else if (checkBox == m_showTexturesCheck) {
        m_config.showTextures = checkBox->GetValue();
        LOG_INF_S(std::string("NavigationCubeConfigDialog::OnCheckBoxChanged: Show textures changed to ") +
                 (m_config.showTextures ? "true" : "false"));
    }
    else if (checkBox == m_enableAnimationCheck) {
        m_config.enableAnimation = checkBox->GetValue();
        LOG_INF_S(std::string("NavigationCubeConfigDialog::OnCheckBoxChanged: Enable animation changed to ") +
                 (m_config.enableAnimation ? "true" : "false"));
    }

    // Notify about configuration change
    if (m_configChangedCallback) {
        m_configChangedCallback(m_config);
    }
}

void NavigationCubeConfigDialog::OnSizeChanged(wxSpinEvent& event) {
    wxSpinCtrl* ctrl = dynamic_cast<wxSpinCtrl*>(event.GetEventObject());
    if (!ctrl) return;

    bool configChanged = false;

    // Handle different spin controls
    if (ctrl == m_xCtrl) {
        int newX = ctrl->GetValue();
        if (newX >= 0 && newX <= m_maxX) {
            m_config.x = newX;
            configChanged = true;
            LOG_INF_S("NavigationCubeConfigDialog::OnSizeChanged: X margin changed from " + std::to_string(m_config.x) +
                " to " + std::to_string(newX) + " (max: " + std::to_string(m_maxX) + ")");
        }
    }
    else if (ctrl == m_yCtrl) {
        int newY = ctrl->GetValue();
        if (newY >= 0 && newY <= m_maxY) {
            m_config.y = newY;
            configChanged = true;
            LOG_INF_S("NavigationCubeConfigDialog::OnSizeChanged: Y margin changed from " + std::to_string(m_config.y) +
                " to " + std::to_string(newY) + " (max: " + std::to_string(m_maxY) + ")");
        }
    }
    else if (ctrl == m_sizeCtrl) {
        int newSize = ctrl->GetValue();
        if (newSize >= 50 && newSize <= std::min(m_maxX, m_maxY) / 2) {
            m_config.size = newSize;
            configChanged = true;
            LOG_INF_S("NavigationCubeConfigDialog::OnSizeChanged: Size changed from " + std::to_string(m_config.size) +
                " to " + std::to_string(newSize) + " (window: " + std::to_string(m_maxX) + "x" + std::to_string(m_maxY) + ")");
        }
    }
    else if (ctrl == m_viewportSizeCtrl) {
        int newViewportSize = ctrl->GetValue();
        if (newViewportSize >= 50 && newViewportSize <= std::min(m_maxX, m_maxY) / 2) {
            m_config.viewportSize = newViewportSize;
            configChanged = true;
            LOG_INF_S("NavigationCubeConfigDialog::OnSizeChanged: Viewport size changed from " + std::to_string(m_config.viewportSize) +
                " to " + std::to_string(newViewportSize) + " (window: " + std::to_string(m_maxX) + "x" + std::to_string(m_maxY) + ")");
        }
    }
    else if (ctrl == m_circleMarginXCtrl) {
        int newMarginX = ctrl->GetValue();
        if (newMarginX >= 0 && newMarginX <= m_maxX / 2) {
            m_config.circleMarginX = newMarginX;
            configChanged = true;
            LOG_INF_S("NavigationCubeConfigDialog::OnSizeChanged: Circle margin X changed from " + std::to_string(m_config.circleMarginX) +
                " to " + std::to_string(newMarginX) + " (max: " + std::to_string(m_maxX / 2) + ")");
        }
    }
    else if (ctrl == m_circleMarginYCtrl) {
        int newMarginY = ctrl->GetValue();
        if (newMarginY >= 0 && newMarginY <= m_maxY / 2) {
            m_config.circleMarginY = newMarginY;
            configChanged = true;
            LOG_INF_S("NavigationCubeConfigDialog::OnSizeChanged: Circle margin Y changed from " + std::to_string(m_config.circleMarginY) +
                " to " + std::to_string(newMarginY) + " (max: " + std::to_string(m_maxY / 2) + ")");
        }
    }
    else if (ctrl == m_circleRadiusCtrl) {
        int newRadius = ctrl->GetValue();
        if (newRadius >= 50 && newRadius <= std::min(m_maxX, m_maxY) / 2) {
            m_config.circleRadius = newRadius;
            configChanged = true;
            LOG_INF_S("NavigationCubeConfigDialog::OnSizeChanged: Circle radius changed from " + std::to_string(m_config.circleRadius) +
                " to " + std::to_string(newRadius) + " (window: " + std::to_string(m_maxX) + "x" + std::to_string(m_maxY) + ")");
        }
    }

    // Notify about configuration change
    if (configChanged && m_configChangedCallback) {
        m_configChangedCallback(m_config);
    }
}

void NavigationCubeConfigDialog::OnCenterCube(wxCommandEvent& event) {
    // Calculate centered position within circle navigation area
    // Note: m_maxX and m_maxY are now in logical coordinates

    int cubeSize = m_sizeCtrl->GetValue();
    int circleMarginX = m_circleMarginXCtrl->GetValue();
    int circleMarginY = m_circleMarginYCtrl->GetValue();

    // Calculate circle center in logical coordinates
    int circleCenterX = m_maxX - circleMarginX;
    int circleCenterY = circleMarginY;

    // Center cube within circle
    int cubeLeftX = circleCenterX - cubeSize / 2;
    int cubeTopY = circleCenterY - cubeSize / 2;

    // Ensure within bounds
    cubeLeftX = std::max(0, std::min(cubeLeftX, m_maxX - cubeSize));
    cubeTopY = std::max(0, std::min(cubeTopY, m_maxY - cubeSize));

    // Convert to margins (X=right margin, Y=top margin)
    int rightMargin = m_maxX - cubeLeftX - cubeSize;
    int topMargin = cubeTopY;

    // Update the controls with margins
    m_xCtrl->SetValue(rightMargin);
    m_yCtrl->SetValue(topMargin);

    // Update config with margins
    m_config.x = rightMargin;
    m_config.y = topMargin;

    wxMessageBox(wxString::Format("Cube centered in circle navigation area!\n"
                                  "Right margin: %d px\n"
                                  "Top margin: %d px\n"
                                  "(Circle center: %d, %d)\n"
                                  "(Window: %dx%d)",
                                  rightMargin, topMargin, circleCenterX, circleCenterY,
                                  m_maxX, m_maxY),
                 "Cube Centered in Circle", wxOK | wxICON_INFORMATION);
}