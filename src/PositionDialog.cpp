#include "PositionDialog.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "GeometryFactory.h"
#include "MouseHandler.h"
#include "InputManager.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "Logger.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

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
END_EVENT_TABLE()

PositionDialog::PositionDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    LOG_INF("Creating position dialog");
    SetName("PositionDialog");

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create coordinate input area
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 2, 5, 10);

    // X coordinate
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "X:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_xTextCtrl = new wxTextCtrl(this, wxID_ANY, "0.0");
    gridSizer->Add(m_xTextCtrl, 0, wxEXPAND);

    // Y coordinate
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Y:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_yTextCtrl = new wxTextCtrl(this, wxID_ANY, "0.0");
    gridSizer->Add(m_yTextCtrl, 0, wxEXPAND);

    // Z coordinate
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Z:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_zTextCtrl = new wxTextCtrl(this, wxID_ANY, "0.0");
    gridSizer->Add(m_zTextCtrl, 0, wxEXPAND);

    // Reference Z coordinate for picking
    gridSizer->Add(new wxStaticText(this, wxID_ANY, "Reference Z:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_referenceZTextCtrl = new wxTextCtrl(this, ID_REFERENCE_Z_TEXT, "0.0");
    m_referenceZTextCtrl->SetToolTip("Z coordinate plane for mouse picking");
    gridSizer->Add(m_referenceZTextCtrl, 0, wxEXPAND);

    mainSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);

    // Grid display checkbox
    m_showGridCheckBox = new wxCheckBox(this, ID_SHOW_GRID_CHECK, "Show Reference Grid");
    m_showGridCheckBox->SetToolTip("Display reference grid at the specified Z coordinate");
    mainSizer->Add(m_showGridCheckBox, 0, wxALIGN_CENTER | wxALL, 5);

    // Coordinate pick button
    m_pickButton = new wxButton(this, ID_PICK_BUTTON, "Pick Coordinates");
    mainSizer->Add(m_pickButton, 0, wxALIGN_CENTER | wxALL, 5);

    // OK and Cancel buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_okButton = new wxButton(this, wxID_OK, "OK");
    m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    buttonSizer->Add(m_okButton, 0, wxALL, 5);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    SetSizer(mainSizer);
    mainSizer->Fit(this);
    Center();
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
    LOG_INF("Pick button clicked - entering picking mode");
    g_isPickingPosition = true;
    m_pickButton->SetLabel("Picking...");
    m_pickButton->Enable(false);

    wxWindow* parentWindow = GetParent();
    if (parentWindow) {
        wxWindow* canvasWindow = wxWindow::FindWindowByName("Canvas", parentWindow);
        if (canvasWindow) {
            Canvas* canvas = dynamic_cast<Canvas*>(canvasWindow);
            if (canvas) {
                canvas->getSceneManager()->getPickingAidManager()->showPickingAidLines(GetPosition());
            }
            else {
                LOG_ERR("Canvas cast failed");
            }
        }
        else {
            LOG_ERR("Canvas window not found");
        }
    }
    else {
        LOG_ERR("Parent window not found");
    }

    this->Hide();
    LOG_INF("Dialog hidden, picking mode active: " + std::to_string(g_isPickingPosition));
}

void PositionDialog::OnOkButton(wxCommandEvent& event) {
    LOG_INF("Position confirmed: " + std::to_string(GetPosition()[0]) + ", " + std::to_string(GetPosition()[1]) + ", " + std::to_string(GetPosition()[2]));
    g_isPickingPosition = false;

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
                    GeometryFactory factory(canvas->getSceneManager()->getObjectRoot(), canvas->getObjectTreePanel(), canvas->getObjectTreePanel()->getPropertyPanel(), canvas->getCommandManager());
                    factory.createGeometry(geometryType, finalPos);
                    LOG_INF("Creating geometry at position from dialog");
                    mouseHandler->setOperationMode(MouseHandler::OperationMode::VIEW);
                    mouseHandler->setCreationGeometryType("");
                    LOG_INF("Reset operation mode to VIEW");
                }
                else {
                    LOG_ERR("MouseHandler not found");
                }
            }
            else {
                LOG_ERR("Canvas cast failed");
            }
        }
        else {
            LOG_ERR("Canvas window not found");
        }
    }
    else {
        LOG_ERR("Parent window not found");
    }

    Hide();
    event.Skip();
}

void PositionDialog::OnCancelButton(wxCommandEvent& event) {
    LOG_INF("Position input cancelled");
    g_isPickingPosition = false;

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
                    LOG_INF("Reset operation mode to VIEW on cancel");
                }
            }
            else {
                LOG_ERR("Canvas cast failed");
            }
        }
        else {
            LOG_ERR("Canvas window not found");
        }
    }
    else {
        LOG_ERR("Parent window not found");
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
                    LOG_INF("Reference Z set to: " + std::to_string(referenceZ));
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
                LOG_INF("Reference grid display: " + std::string(showGrid ? "enabled" : "disabled"));
            }
        }
    }
    event.Skip();
}