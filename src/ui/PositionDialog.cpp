#include "PositionDialog.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "GeometryFactory.h"
#include "MouseHandler.h"
#include "InputManager.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/panel.h>

// Custom event ID
enum {
    ID_PICK_BUTTON = wxID_HIGHEST + 1000,
    ID_REFERENCE_Z_TEXT,
    ID_SHOW_GRID_CHECK
};

BEGIN_EVENT_TABLE(PositionDialog, wxDialog)
EVT_BUTTON(ID_PICK_BUTTON, PositionDialog::OnPickButton)
EVT_BUTTON(wxID_OK, PositionDialog::OnOkButton)
EVT_BUTTON(wxID_CANCEL, PositionDialog::OnCancelButton)
EVT_TEXT(ID_REFERENCE_Z_TEXT, PositionDialog::OnReferenceZChanged)
EVT_CHECKBOX(ID_SHOW_GRID_CHECK, PositionDialog::OnShowGridChanged)
EVT_CLOSE(PositionDialog::OnClose)
END_EVENT_TABLE()

PositionDialog::PositionDialog(wxWindow* parent, const wxString& title, PickingAidManager* pickingAidManager, const std::string& geometryType)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
    , m_pickingAidManager(pickingAidManager)
{
    LOG_INF_S("Creating position dialog for geometry type: " + geometryType);
    SetName("PositionDialog");
    
    m_geometryParams.geometryType = geometryType;

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create notebook for tabs
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    CreatePositionTab();
    CreateParametersTab();
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);

    // OK and Cancel buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_okButton = new wxButton(this, wxID_OK, "OK");
    m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    buttonSizer->Add(m_okButton, 0, wxALL, 5);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    SetSizer(mainSizer);
    SetGeometryType(geometryType);
    mainSizer->Fit(this);
    Center();
}

void PositionDialog::CreatePositionTab()
{
    m_positionPanel = new wxPanel(m_notebook);
    m_notebook->AddPage(m_positionPanel, "Position", true);
    
    wxBoxSizer* positionSizer = new wxBoxSizer(wxVERTICAL);

    // Create coordinate input area
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 2, 5, 10);

    // X coordinate
    gridSizer->Add(new wxStaticText(m_positionPanel, wxID_ANY, "X:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_xTextCtrl = new wxTextCtrl(m_positionPanel, wxID_ANY, "0.0");
    gridSizer->Add(m_xTextCtrl, 0, wxEXPAND);

    // Y coordinate
    gridSizer->Add(new wxStaticText(m_positionPanel, wxID_ANY, "Y:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_yTextCtrl = new wxTextCtrl(m_positionPanel, wxID_ANY, "0.0");
    gridSizer->Add(m_yTextCtrl, 0, wxEXPAND);

    // Z coordinate
    gridSizer->Add(new wxStaticText(m_positionPanel, wxID_ANY, "Z:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_zTextCtrl = new wxTextCtrl(m_positionPanel, wxID_ANY, "0.0");
    gridSizer->Add(m_zTextCtrl, 0, wxEXPAND);

    // Reference Z coordinate for picking
    gridSizer->Add(new wxStaticText(m_positionPanel, wxID_ANY, "Reference Z:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_referenceZTextCtrl = new wxTextCtrl(m_positionPanel, ID_REFERENCE_Z_TEXT, "0.0");
    m_referenceZTextCtrl->SetToolTip("Z coordinate plane for mouse picking");
    gridSizer->Add(m_referenceZTextCtrl, 0, wxEXPAND);

    positionSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);

    // Grid display checkbox
    m_showGridCheckBox = new wxCheckBox(m_positionPanel, ID_SHOW_GRID_CHECK, "Show Reference Grid");
    m_showGridCheckBox->SetToolTip("Display reference grid at the specified Z coordinate");
    positionSizer->Add(m_showGridCheckBox, 0, wxALIGN_CENTER | wxALL, 5);

    // Coordinate pick button
    m_pickButton = new wxButton(m_positionPanel, ID_PICK_BUTTON, "Pick Coordinates");
    positionSizer->Add(m_pickButton, 0, wxALIGN_CENTER | wxALL, 5);

    m_positionPanel->SetSizer(positionSizer);
}

void PositionDialog::CreateParametersTab()
{
    m_parametersPanel = new wxPanel(m_notebook);
    m_notebook->AddPage(m_parametersPanel, "Parameters", false);
    
    wxBoxSizer* parametersSizer = new wxBoxSizer(wxVERTICAL);
    
    // Geometry type label
    m_geometryTypeLabel = new wxStaticText(m_parametersPanel, wxID_ANY, "Geometry Type: ");
    parametersSizer->Add(m_geometryTypeLabel, 0, wxALL, 10);
    
    m_parametersPanel->SetSizer(parametersSizer);
}

void PositionDialog::SetGeometryType(const std::string& geometryType)
{
    m_geometryParams.geometryType = geometryType;
    UpdateParametersTab();
}

void PositionDialog::UpdateParametersTab()
{
    // Clear existing parameter controls
    for (auto& pair : m_parameterControls) {
        pair.second->Destroy();
    }
    m_parameterControls.clear();
    
    // Update geometry type label
    m_geometryTypeLabel->SetLabel("Geometry Type: " + m_geometryParams.geometryType);
    
    wxSizer* parametersSizer = m_parametersPanel->GetSizer();
    
    // Remove all items except the label
    while (parametersSizer->GetItemCount() > 1) {
        parametersSizer->Remove(1);
    }
    
    // Create parameter grid
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(0, 2, 5, 10);
    
    if (m_geometryParams.geometryType == "Box") {
        // Width
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Width:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["width"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.width));
        gridSizer->Add(m_parameterControls["width"], 0, wxEXPAND);
        
        // Height
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Height:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["height"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.height));
        gridSizer->Add(m_parameterControls["height"], 0, wxEXPAND);
        
        // Depth
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Depth:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["depth"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.depth));
        gridSizer->Add(m_parameterControls["depth"], 0, wxEXPAND);
    }
    else if (m_geometryParams.geometryType == "Sphere") {
        // Radius
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["radius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.radius));
        gridSizer->Add(m_parameterControls["radius"], 0, wxEXPAND);
    }
    else if (m_geometryParams.geometryType == "Cylinder") {
        // Radius
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["cylinderRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.cylinderRadius));
        gridSizer->Add(m_parameterControls["cylinderRadius"], 0, wxEXPAND);
        
        // Height
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Height:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["cylinderHeight"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.cylinderHeight));
        gridSizer->Add(m_parameterControls["cylinderHeight"], 0, wxEXPAND);
    }
    else if (m_geometryParams.geometryType == "Cone") {
        // Bottom Radius
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Bottom Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["bottomRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.bottomRadius));
        gridSizer->Add(m_parameterControls["bottomRadius"], 0, wxEXPAND);
        
        // Top Radius
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Top Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["topRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.topRadius));
        gridSizer->Add(m_parameterControls["topRadius"], 0, wxEXPAND);
        
        // Height
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Height:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["coneHeight"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.coneHeight));
        gridSizer->Add(m_parameterControls["coneHeight"], 0, wxEXPAND);
    }
    else if (m_geometryParams.geometryType == "Torus") {
        // Major Radius
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Major Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["majorRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.majorRadius));
        gridSizer->Add(m_parameterControls["majorRadius"], 0, wxEXPAND);
        
        // Minor Radius
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Minor Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["minorRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.minorRadius));
        gridSizer->Add(m_parameterControls["minorRadius"], 0, wxEXPAND);
    }
    else if (m_geometryParams.geometryType == "TruncatedCylinder") {
        // Bottom Radius
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Bottom Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["truncatedBottomRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.truncatedBottomRadius));
        gridSizer->Add(m_parameterControls["truncatedBottomRadius"], 0, wxEXPAND);
        
        // Top Radius
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Top Radius:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["truncatedTopRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.truncatedTopRadius));
        gridSizer->Add(m_parameterControls["truncatedTopRadius"], 0, wxEXPAND);
        
        // Height
        gridSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Height:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
        m_parameterControls["truncatedHeight"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, wxString::Format("%.2f", m_geometryParams.truncatedHeight));
        gridSizer->Add(m_parameterControls["truncatedHeight"], 0, wxEXPAND);
    }
    
    parametersSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);
    
    m_parametersPanel->Layout();
}

void PositionDialog::LoadParametersFromControls()
{
    for (auto& pair : m_parameterControls) {
        double value;
        if (pair.second->GetValue().ToDouble(&value)) {
            if (pair.first == "width") m_geometryParams.width = value;
            else if (pair.first == "height") m_geometryParams.height = value;
            else if (pair.first == "depth") m_geometryParams.depth = value;
            else if (pair.first == "radius") m_geometryParams.radius = value;
            else if (pair.first == "cylinderRadius") m_geometryParams.cylinderRadius = value;
            else if (pair.first == "cylinderHeight") m_geometryParams.cylinderHeight = value;
            else if (pair.first == "bottomRadius") m_geometryParams.bottomRadius = value;
            else if (pair.first == "topRadius") m_geometryParams.topRadius = value;
            else if (pair.first == "coneHeight") m_geometryParams.coneHeight = value;
            else if (pair.first == "majorRadius") m_geometryParams.majorRadius = value;
            else if (pair.first == "minorRadius") m_geometryParams.minorRadius = value;
            else if (pair.first == "truncatedBottomRadius") m_geometryParams.truncatedBottomRadius = value;
            else if (pair.first == "truncatedTopRadius") m_geometryParams.truncatedTopRadius = value;
            else if (pair.first == "truncatedHeight") m_geometryParams.truncatedHeight = value;
        }
    }
}

void PositionDialog::SaveParametersToControls()
{
    for (auto& pair : m_parameterControls) {
        double value = 0.0;
        if (pair.first == "width") value = m_geometryParams.width;
        else if (pair.first == "height") value = m_geometryParams.height;
        else if (pair.first == "depth") value = m_geometryParams.depth;
        else if (pair.first == "radius") value = m_geometryParams.radius;
        else if (pair.first == "cylinderRadius") value = m_geometryParams.cylinderRadius;
        else if (pair.first == "cylinderHeight") value = m_geometryParams.cylinderHeight;
        else if (pair.first == "bottomRadius") value = m_geometryParams.bottomRadius;
        else if (pair.first == "topRadius") value = m_geometryParams.topRadius;
        else if (pair.first == "coneHeight") value = m_geometryParams.coneHeight;
        else if (pair.first == "majorRadius") value = m_geometryParams.majorRadius;
        else if (pair.first == "minorRadius") value = m_geometryParams.minorRadius;
        else if (pair.first == "truncatedBottomRadius") value = m_geometryParams.truncatedBottomRadius;
        else if (pair.first == "truncatedTopRadius") value = m_geometryParams.truncatedTopRadius;
        else if (pair.first == "truncatedHeight") value = m_geometryParams.truncatedHeight;
        
        pair.second->SetValue(wxString::Format("%.2f", value));
    }
}

GeometryParameters PositionDialog::GetGeometryParameters() const
{
    GeometryParameters params = m_geometryParams;
    
    // Load current values from controls
    for (auto& pair : m_parameterControls) {
        double value;
        if (pair.second->GetValue().ToDouble(&value)) {
            if (pair.first == "width") params.width = value;
            else if (pair.first == "height") params.height = value;
            else if (pair.first == "depth") params.depth = value;
            else if (pair.first == "radius") params.radius = value;
            else if (pair.first == "cylinderRadius") params.cylinderRadius = value;
            else if (pair.first == "cylinderHeight") params.cylinderHeight = value;
            else if (pair.first == "bottomRadius") params.bottomRadius = value;
            else if (pair.first == "topRadius") params.topRadius = value;
            else if (pair.first == "coneHeight") params.coneHeight = value;
            else if (pair.first == "majorRadius") params.majorRadius = value;
            else if (pair.first == "minorRadius") params.minorRadius = value;
            else if (pair.first == "truncatedBottomRadius") params.truncatedBottomRadius = value;
            else if (pair.first == "truncatedTopRadius") params.truncatedTopRadius = value;
            else if (pair.first == "truncatedHeight") params.truncatedHeight = value;
        }
    }
    
    return params;
}

void PositionDialog::SetPosition(const SbVec3f& position)
{
    m_xTextCtrl->SetValue(wxString::Format("%.3f", position[0]));
    m_yTextCtrl->SetValue(wxString::Format("%.3f", position[1]));
    m_zTextCtrl->SetValue(wxString::Format("%.3f", position[2]));
}

SbVec3f PositionDialog::GetPosition() const
{
    double x, y, z;
    m_xTextCtrl->GetValue().ToDouble(&x);
    m_yTextCtrl->GetValue().ToDouble(&y);
    m_zTextCtrl->GetValue().ToDouble(&z);

    return SbVec3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

void PositionDialog::OnPickButton(wxCommandEvent& event) {
    LOG_INF_S("Pick button clicked - entering picking mode");
    if (m_pickingAidManager) {
        m_pickingAidManager->startPicking();
        m_pickingAidManager->showPickingAidLines(GetPosition());
    }
    m_pickButton->SetLabel("Picking...");
    m_pickButton->Enable(false);
    this->Hide();
    if (m_pickingAidManager) {
        LOG_INF("Dialog hidden, picking mode active: " + std::to_string(m_pickingAidManager->isPicking()),"PositionDialog");
    }
}

void PositionDialog::OnOkButton(wxCommandEvent& event) {
    LOG_INF_S("Position confirmed: " + std::to_string(GetPosition()[0]) + ", " + std::to_string(GetPosition()[1]) + ", " + std::to_string(GetPosition()[2]));
    if (m_pickingAidManager) {
        m_pickingAidManager->stopPicking();
    }

    // Load parameters from controls
    LoadParametersFromControls();

    wxWindow* parentWindow = GetParent();
    if (parentWindow) {
        wxWindow* canvasWindow = wxWindow::FindWindowByName("Canvas", parentWindow);
        if (canvasWindow) {
            Canvas* canvas = dynamic_cast<Canvas*>(canvasWindow);
            if (canvas) {
                canvas->getSceneManager()->getPickingAidManager()->hidePickingAidLines();
                SbVec3f finalPos = GetPosition();
                MouseHandler* mouseHandler = canvas->getInputManager()->getMouseHandler();
                if (mouseHandler) {
                    std::string geometryType = mouseHandler->getCreationGeometryType();
                    GeometryFactory factory(
                        canvas->getSceneManager()->getObjectRoot(),
                        canvas->getObjectTreePanel(),
                        canvas->getObjectTreePanel()->getPropertyPanel(),
                        canvas->getCommandManager(),
                        canvas->getOCCViewer() 
                    );
                    
                    // Create geometry with parameters
                    GeometryParameters params = GetGeometryParameters();
                    factory.createOCCGeometryWithParameters(geometryType, finalPos, params);
                    LOG_INF_S("Creating geometry at position from dialog with parameters");
                    mouseHandler->setOperationMode(MouseHandler::OperationMode::VIEW);
                    mouseHandler->setCreationGeometryType("");
                    LOG_INF_S("Reset operation mode to VIEW");
                }
                else {
                    LOG_ERR_S("MouseHandler not found");
                }
            }
            else {
                LOG_ERR_S("Canvas cast failed");
            }
        }
        else {
            LOG_ERR_S("Canvas window not found");
        }
    }
    else {
        LOG_ERR_S("Parent window not found");
    }

    Hide();
    event.Skip();
}

void PositionDialog::OnCancelButton(wxCommandEvent& event) {
    LOG_INF_S("Position input cancelled");
    if (m_pickingAidManager) {
        m_pickingAidManager->stopPicking();
    }

    wxWindow* parentWindow = GetParent();
    if (parentWindow) {
        wxWindow* canvasWindow = wxWindow::FindWindowByName("Canvas", parentWindow);
        if (canvasWindow) {
            Canvas* canvas = dynamic_cast<Canvas*>(canvasWindow);
            if (canvas) {
                canvas->getSceneManager()->getPickingAidManager()->hidePickingAidLines();
                MouseHandler* mouseHandler = canvas->getInputManager()->getMouseHandler();
                if (mouseHandler) {
                    mouseHandler->setOperationMode(MouseHandler::OperationMode::VIEW);
                    mouseHandler->setCreationGeometryType("");
                    LOG_INF_S("Reset operation mode to VIEW on cancel");
                }
            }
            else {
                LOG_ERR_S("Canvas cast failed");
            }
        }
        else {
            LOG_ERR_S("Canvas window not found");
        }
    }
    else {
        LOG_ERR_S("Parent window not found");
    }

    Hide();
    event.Skip();
}

void PositionDialog::OnReferenceZChanged(wxCommandEvent& event) {
    double referenceZ;
    if (m_referenceZTextCtrl->GetValue().ToDouble(&referenceZ)) {
        wxWindow* parentWindow = GetParent();
        if (parentWindow) {
            wxWindow* canvasWindow = wxWindow::FindWindowByName("Canvas", parentWindow);
            if (canvasWindow) {
                Canvas* canvas = dynamic_cast<Canvas*>(canvasWindow);
                if (canvas) {
                    canvas->getSceneManager()->getPickingAidManager()->setReferenceZ(static_cast<float>(referenceZ));
                    LOG_INF_S("Reference Z set to: " + std::to_string(referenceZ));
                }
            }
        }
    }
    event.Skip();
}

void PositionDialog::OnShowGridChanged(wxCommandEvent& event) {
    bool showGrid = m_showGridCheckBox->GetValue();
    
    // Update reference Z first
    OnReferenceZChanged(event);
    
    wxWindow* parentWindow = GetParent();
    if (parentWindow) {
        wxWindow* canvasWindow = wxWindow::FindWindowByName("Canvas", parentWindow);
        if (canvasWindow) {
            Canvas* canvas = dynamic_cast<Canvas*>(canvasWindow);
            if (canvas) {
                canvas->getSceneManager()->getPickingAidManager()->showReferenceGrid(showGrid);
                LOG_INF("Reference grid display: " + std::string(showGrid ? "enabled" : "disabled"),"PositionDialog");
            }
        }
    }
    event.Skip();
}

void PositionDialog::OnClose(wxCloseEvent& event)
{
    LOG_INF("Position dialog closed, ensuring picking mode is off.","PositionDialog");
    if (m_pickingAidManager) {
        m_pickingAidManager->stopPicking();
    }
    
    wxWindow* parentWindow = GetParent();
    if (parentWindow) {
        wxWindow* canvasWindow = wxWindow::FindWindowByName("Canvas", parentWindow);
        if (canvasWindow) {
            Canvas* canvas = dynamic_cast<Canvas*>(canvasWindow);
            if (canvas) {
                canvas->getSceneManager()->getPickingAidManager()->hidePickingAidLines();
                MouseHandler* mouseHandler = canvas->getInputManager()->getMouseHandler();
                if (mouseHandler) {
                    mouseHandler->setOperationMode(MouseHandler::OperationMode::VIEW);
                    mouseHandler->setCreationGeometryType("");
                    LOG_INF_S("Reset operation mode to VIEW on close");
                }
            }
        }
    }
    
    event.Skip();
}
