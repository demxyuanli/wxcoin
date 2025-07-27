#include "PositionBasicDialog.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "MouseHandler.h"
#include "InputManager.h"
#include "GeometryFactory.h"
#include "ObjectTreePanel.h"
#include "PropertyPanel.h"
#include "VisualSettingsDialog.h"
#include "logger/Logger.h"
#include <wx/notebook.h>
#include <wx/grid.h>
#include <wx/valnum.h>
#include <wx/textctrl.h>
#include <sstream>
#include <iomanip>

// Custom event IDs
enum {
    ID_PICK_BUTTON = wxID_HIGHEST + 1000,
    ID_REFERENCE_Z_TEXT,
    ID_SHOW_GRID_CHECK,
    ID_VISUAL_SETTINGS_BUTTON
};

BEGIN_EVENT_TABLE(PositionBasicDialog, wxDialog)
    EVT_BUTTON(wxID_OK, PositionBasicDialog::OnOkButton)
    EVT_BUTTON(wxID_CANCEL, PositionBasicDialog::OnCancelButton)
    EVT_BUTTON(ID_PICK_BUTTON, PositionBasicDialog::OnPickButton)
    EVT_BUTTON(ID_VISUAL_SETTINGS_BUTTON, PositionBasicDialog::OnVisualSettingsButton)
    EVT_CHECKBOX(ID_SHOW_GRID_CHECK, PositionBasicDialog::OnShowGridChanged)
    EVT_TEXT(ID_REFERENCE_Z_TEXT, PositionBasicDialog::OnReferenceZChanged)
END_EVENT_TABLE()

PositionBasicDialog::PositionBasicDialog(wxWindow* parent, const wxString& title, PickingAidManager* pickingAidManager, const std::string& geometryType)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(400, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_positionPanel(nullptr)
    , m_parametersPanel(nullptr)
    , m_parametersSizer(nullptr)
    , m_pickingAidManager(pickingAidManager)
{
    m_basicParams.geometryType = geometryType;
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for tabs - use 'this' as parent
    wxNotebook* notebook = new wxNotebook(this, wxID_ANY);
    
    // Create tabs with notebook as parent
    m_positionPanel = new wxPanel(notebook, wxID_ANY);
    m_parametersPanel = new wxPanel(notebook, wxID_ANY);
    
    CreatePositionTab();
    CreateParametersTab();
    
    notebook->AddPage(m_positionPanel, "Position");
    notebook->AddPage(m_parametersPanel, "Parameters");
    
    mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    
    // Add buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(this, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);
    
    SetSizer(mainSizer);
    
    // Initialize controls with default values
    SaveParametersToControls();
}

void PositionBasicDialog::CreatePositionTab()
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Geometry type display (read-only)
    wxStaticText* typeLabel = new wxStaticText(m_positionPanel, wxID_ANY, "Geometry Type:");
    m_geometryTypeLabel = new wxStaticText(m_positionPanel, wxID_ANY, "");
    sizer->Add(typeLabel, 0, wxALL, 5);
    sizer->Add(m_geometryTypeLabel, 0, wxALL, 5);
    
    // Position controls
    wxStaticText* positionLabel = new wxStaticText(m_positionPanel, wxID_ANY, "Position:");
    sizer->Add(positionLabel, 0, wxALL, 5);
    
    wxFlexGridSizer* positionSizer = new wxFlexGridSizer(3, 2, 5, 5);
    
    wxStaticText* xLabel = new wxStaticText(m_positionPanel, wxID_ANY, "X:");
    m_xTextCtrl = new wxTextCtrl(m_positionPanel, wxID_ANY, "0.0");
    positionSizer->Add(xLabel, 0, wxALIGN_CENTER_VERTICAL);
    positionSizer->Add(m_xTextCtrl, 1, wxEXPAND);
    
    wxStaticText* yLabel = new wxStaticText(m_positionPanel, wxID_ANY, "Y:");
    m_yTextCtrl = new wxTextCtrl(m_positionPanel, wxID_ANY, "0.0");
    positionSizer->Add(yLabel, 0, wxALIGN_CENTER_VERTICAL);
    positionSizer->Add(m_yTextCtrl, 1, wxEXPAND);
    
    wxStaticText* zLabel = new wxStaticText(m_positionPanel, wxID_ANY, "Z:");
    m_zTextCtrl = new wxTextCtrl(m_positionPanel, wxID_ANY, "0.0");
    positionSizer->Add(zLabel, 0, wxALIGN_CENTER_VERTICAL);
    positionSizer->Add(m_zTextCtrl, 1, wxEXPAND);
    
    sizer->Add(positionSizer, 0, wxEXPAND | wxALL, 5);
    
    // Reference Z
    wxStaticText* refZLabel = new wxStaticText(m_positionPanel, wxID_ANY, "Reference Z:");
    m_referenceZTextCtrl = new wxTextCtrl(m_positionPanel, ID_REFERENCE_Z_TEXT, "0.0");
    sizer->Add(refZLabel, 0, wxALL, 5);
    sizer->Add(m_referenceZTextCtrl, 0, wxEXPAND | wxALL, 5);
    
    // Show grid checkbox
    m_showGridCheckBox = new wxCheckBox(m_positionPanel, ID_SHOW_GRID_CHECK, "Show Grid");
    sizer->Add(m_showGridCheckBox, 0, wxALL, 5);
    
    // Button sizer for action buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Pick button
    m_pickButton = new wxButton(m_positionPanel, ID_PICK_BUTTON, "Pick Position");
    buttonSizer->Add(m_pickButton, 1, wxEXPAND | wxALL, 5);
    
    // Visual Settings button
    m_visualSettingsButton = new wxButton(m_positionPanel, ID_VISUAL_SETTINGS_BUTTON, "Visual Settings");
    buttonSizer->Add(m_visualSettingsButton, 1, wxEXPAND | wxALL, 5);
    
    sizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    m_positionPanel->SetSizer(sizer);
}

void PositionBasicDialog::CreateParametersTab()
{
    m_parametersSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create parameter controls based on geometry type
    UpdateParametersTab();
    
    m_parametersPanel->SetSizer(m_parametersSizer);
}

void PositionBasicDialog::UpdateParametersTab()
{
    // Clear existing controls
    m_parameterControls.clear();
    
    // Remove old parameter controls
    m_parametersSizer->Clear();
    
    wxStaticText* paramsLabel = new wxStaticText(m_parametersPanel, wxID_ANY, "Geometry Parameters:");
    m_parametersSizer->Add(paramsLabel, 0, wxALL, 5);
    
    wxFlexGridSizer* paramsSizer = new wxFlexGridSizer(0, 2, 5, 5);
    
    if (m_basicParams.geometryType == "Box") {
        m_parameterControls["width"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        m_parameterControls["height"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        m_parameterControls["depth"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Width:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["width"], 1, wxEXPAND);
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["height"], 1, wxEXPAND);
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Depth:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["depth"], 1, wxEXPAND);
    }
    else if (m_basicParams.geometryType == "Sphere") {
        m_parameterControls["radius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Radius:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["radius"], 1, wxEXPAND);
    }
    else if (m_basicParams.geometryType == "Cylinder") {
        m_parameterControls["cylinderRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        m_parameterControls["cylinderHeight"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Radius:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["cylinderRadius"], 1, wxEXPAND);
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["cylinderHeight"], 1, wxEXPAND);
    }
    else if (m_basicParams.geometryType == "Cone") {
        m_parameterControls["bottomRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        m_parameterControls["topRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        m_parameterControls["coneHeight"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Bottom Radius:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["bottomRadius"], 1, wxEXPAND);
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Top Radius:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["topRadius"], 1, wxEXPAND);
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["coneHeight"], 1, wxEXPAND);
    }
    else if (m_basicParams.geometryType == "Torus") {
        m_parameterControls["majorRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        m_parameterControls["minorRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Major Radius:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["majorRadius"], 1, wxEXPAND);
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Minor Radius:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["minorRadius"], 1, wxEXPAND);
    }
    else if (m_basicParams.geometryType == "TruncatedCylinder") {
        m_parameterControls["truncatedBottomRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        m_parameterControls["truncatedTopRadius"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        m_parameterControls["truncatedHeight"] = new wxTextCtrl(m_parametersPanel, wxID_ANY, "");
        
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Bottom Radius:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["truncatedBottomRadius"], 1, wxEXPAND);
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Top Radius:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["truncatedTopRadius"], 1, wxEXPAND);
        paramsSizer->Add(new wxStaticText(m_parametersPanel, wxID_ANY, "Height:"), 0, wxALIGN_CENTER_VERTICAL);
        paramsSizer->Add(m_parameterControls["truncatedHeight"], 1, wxEXPAND);
    }
    
    m_parametersSizer->Add(paramsSizer, 0, wxEXPAND | wxALL, 5);
    
    // Update geometry type label
    m_geometryTypeLabel->SetLabel(m_basicParams.geometryType);
    
    Layout();
}

void PositionBasicDialog::LoadParametersFromControls()
{
    // Load position
    double x, y, z, refZ;
    m_xTextCtrl->GetValue().ToDouble(&x);
    m_yTextCtrl->GetValue().ToDouble(&y);
    m_zTextCtrl->GetValue().ToDouble(&z);
    m_referenceZTextCtrl->GetValue().ToDouble(&refZ);
    
    // Load parameters based on geometry type
    for (auto& pair : m_parameterControls) {
        double value;
        if (pair.second->GetValue().ToDouble(&value)) {
            if (pair.first == "width") m_basicParams.width = value;
            else if (pair.first == "height") m_basicParams.height = value;
            else if (pair.first == "depth") m_basicParams.depth = value;
            else if (pair.first == "radius") m_basicParams.radius = value;
            else if (pair.first == "cylinderRadius") m_basicParams.cylinderRadius = value;
            else if (pair.first == "cylinderHeight") m_basicParams.cylinderHeight = value;
            else if (pair.first == "bottomRadius") m_basicParams.bottomRadius = value;
            else if (pair.first == "topRadius") m_basicParams.topRadius = value;
            else if (pair.first == "coneHeight") m_basicParams.coneHeight = value;
            else if (pair.first == "majorRadius") m_basicParams.majorRadius = value;
            else if (pair.first == "minorRadius") m_basicParams.minorRadius = value;
            else if (pair.first == "truncatedBottomRadius") m_basicParams.truncatedBottomRadius = value;
            else if (pair.first == "truncatedTopRadius") m_basicParams.truncatedTopRadius = value;
            else if (pair.first == "truncatedHeight") m_basicParams.truncatedHeight = value;
        }
    }
}

void PositionBasicDialog::SaveParametersToControls()
{
    // Save position
    m_xTextCtrl->SetValue(wxString::Format("%.2f", 0.0));
    m_yTextCtrl->SetValue(wxString::Format("%.2f", 0.0));
    m_zTextCtrl->SetValue(wxString::Format("%.2f", 0.0));
    m_referenceZTextCtrl->SetValue(wxString::Format("%.2f", 0.0));
    
    // Save parameters based on geometry type
    for (auto& pair : m_parameterControls) {
        double value = 0.0;
        if (pair.first == "width") value = m_basicParams.width;
        else if (pair.first == "height") value = m_basicParams.height;
        else if (pair.first == "depth") value = m_basicParams.depth;
        else if (pair.first == "radius") value = m_basicParams.radius;
        else if (pair.first == "cylinderRadius") value = m_basicParams.cylinderRadius;
        else if (pair.first == "cylinderHeight") value = m_basicParams.cylinderHeight;
        else if (pair.first == "bottomRadius") value = m_basicParams.bottomRadius;
        else if (pair.first == "topRadius") value = m_basicParams.topRadius;
        else if (pair.first == "coneHeight") value = m_basicParams.coneHeight;
        else if (pair.first == "majorRadius") value = m_basicParams.majorRadius;
        else if (pair.first == "minorRadius") value = m_basicParams.minorRadius;
        else if (pair.first == "truncatedBottomRadius") value = m_basicParams.truncatedBottomRadius;
        else if (pair.first == "truncatedTopRadius") value = m_basicParams.truncatedTopRadius;
        else if (pair.first == "truncatedHeight") value = m_basicParams.truncatedHeight;
        
        pair.second->SetValue(wxString::Format("%.2f", value));
    }
    
    // Update geometry type label
    m_geometryTypeLabel->SetLabel(m_basicParams.geometryType);
}

void PositionBasicDialog::SetPosition(const SbVec3f& position)
{
    m_xTextCtrl->SetValue(wxString::Format("%.2f", position[0]));
    m_yTextCtrl->SetValue(wxString::Format("%.2f", position[1]));
    m_zTextCtrl->SetValue(wxString::Format("%.2f", position[2]));
}

SbVec3f PositionBasicDialog::GetPosition() const
{
    double x, y, z;
    m_xTextCtrl->GetValue().ToDouble(&x);
    m_yTextCtrl->GetValue().ToDouble(&y);
    m_zTextCtrl->GetValue().ToDouble(&z);
    return SbVec3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

void PositionBasicDialog::SetGeometryType(const std::string& geometryType)
{
    m_basicParams.geometryType = geometryType;
    UpdateParametersTab();
    SaveParametersToControls();
}

BasicGeometryParameters PositionBasicDialog::GetBasicParameters() const
{
    return m_basicParams;
}

AdvancedGeometryParameters PositionBasicDialog::GetAdvancedParameters() const
{
    return m_advancedParams;
}

void PositionBasicDialog::OnPickButton(wxCommandEvent& event)
{
    if (m_pickingAidManager) {
        m_pickingAidManager->startPicking();
        Hide(); // Hide dialog during picking
        LOG_INF_S("PositionBasicDialog: Started picking mode");
    } else {
        LOG_ERR_S("PositionBasicDialog: PickingAidManager is null");
    }
}

void PositionBasicDialog::OnOkButton(wxCommandEvent& event)
{
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
                    
                    // Create geometry with basic parameters
                    BasicGeometryParameters basicParams = GetBasicParameters();
                    std::shared_ptr<OCCGeometry> geometry = factory.createOCCGeometryWithParameters(geometryType, finalPos, basicParams);
                    
                    // Apply advanced parameters if geometry was created successfully
                    if (geometry) {
                        AdvancedGeometryParameters advancedParams = GetAdvancedParameters();
                        geometry->applyAdvancedParameters(advancedParams);
                        
                        LOG_INF_S("Created geometry with advanced parameters:");
                        LOG_INF_S("  - Material diffuse color: " + std::to_string(advancedParams.materialDiffuseColor.Red()) + "," + 
                                  std::to_string(advancedParams.materialDiffuseColor.Green()) + "," + 
                                  std::to_string(advancedParams.materialDiffuseColor.Blue()));
                        LOG_INF_S("  - Transparency: " + std::to_string(advancedParams.materialTransparency));
                        LOG_INF_S("  - Texture enabled: " + std::string(advancedParams.textureEnabled ? "true" : "false"));
                    }
                    
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

void PositionBasicDialog::OnCancelButton(wxCommandEvent& event)
{
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

void PositionBasicDialog::OnShowGridChanged(wxCommandEvent& event)
{
    bool showGrid = m_showGridCheckBox->GetValue();
    
    // Update reference Z first
    OnReferenceZChanged(event);
    
    if (m_pickingAidManager) {
        m_pickingAidManager->showReferenceGrid(showGrid);
        LOG_INF_S("Reference grid display: " + std::string(showGrid ? "enabled" : "disabled"));
    } else {
        LOG_ERR_S("PositionBasicDialog: PickingAidManager is null");
    }
}

void PositionBasicDialog::OnReferenceZChanged(wxCommandEvent& event)
{
    double referenceZ;
    if (m_referenceZTextCtrl->GetValue().ToDouble(&referenceZ)) {
        if (m_pickingAidManager) {
            m_pickingAidManager->setReferenceZ(static_cast<float>(referenceZ));
            LOG_INF_S("Reference Z set to: " + std::to_string(referenceZ));
        } else {
            LOG_ERR_S("PositionBasicDialog: PickingAidManager is null");
        }
    }
}

void PositionBasicDialog::OnPickingComplete(const SbVec3f& position)
{
    // Update the position text controls with the picked coordinates
    std::ostringstream xStream, yStream, zStream;
    xStream << std::fixed << std::setprecision(3) << position[0];
    yStream << std::fixed << std::setprecision(3) << position[1];
    zStream << std::fixed << std::setprecision(3) << position[2];
    
    m_xTextCtrl->SetValue(wxString(xStream.str()));
    m_yTextCtrl->SetValue(wxString(yStream.str()));
    m_zTextCtrl->SetValue(wxString(zStream.str()));
    
    // Show the dialog again
    Show();
    
    LOG_INF_S("Position picked: X=" + std::to_string(position[0]) + 
              ", Y=" + std::to_string(position[1]) + 
              ", Z=" + std::to_string(position[2]));
}

void PositionBasicDialog::OnVisualSettingsButton(wxCommandEvent& event)
{
    LOG_INF_S("Opening VisualSettingsDialog for geometry type: " + m_basicParams.geometryType);
    
    // Create and show VisualSettingsDialog
    VisualSettingsDialog* visualDialog = new VisualSettingsDialog(this, "Visual Settings", m_basicParams);
    
    // Set the current advanced parameters
    visualDialog->SetAdvancedParameters(m_advancedParams);
    
    // Show the dialog modally
    if (visualDialog->ShowModal() == wxID_OK) {
        // Get the updated parameters
        m_advancedParams = visualDialog->GetAdvancedParameters();
        
        LOG_INF_S("Visual settings updated for geometry: " + m_basicParams.geometryType);
        LOG_INF_S("  - Material diffuse color: " + std::to_string(m_advancedParams.materialDiffuseColor.Red()) + "," + 
                  std::to_string(m_advancedParams.materialDiffuseColor.Green()) + "," + 
                  std::to_string(m_advancedParams.materialDiffuseColor.Blue()));
        LOG_INF_S("  - Transparency: " + std::to_string(m_advancedParams.materialTransparency));
        LOG_INF_S("  - Texture enabled: " + std::string(m_advancedParams.textureEnabled ? "true" : "false"));
    } else {
        LOG_INF_S("Visual settings dialog cancelled");
    }
    
    // Clean up the dialog
    visualDialog->Destroy();
} 