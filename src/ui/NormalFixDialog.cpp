#include "NormalFixDialog.h"
#include "OCCViewer.h"
#include "NormalValidator.h"
#include "logger/Logger.h"
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/slider.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <sstream>
#include <iomanip>

// OpenCASCADE includes for face analysis
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>

BEGIN_EVENT_TABLE(NormalFixDialog, wxDialog)
EVT_LISTBOX(wxID_ANY, NormalFixDialog::onGeometrySelectionChanged)
EVT_CHECKBOX(wxID_ANY, NormalFixDialog::onSettingsChanged)
EVT_SPINCTRLDOUBLE(wxID_ANY, NormalFixDialog::onSpinCtrlChanged)
EVT_COMMAND_SCROLL(wxID_ANY, NormalFixDialog::onSliderChanged)
EVT_BUTTON(ID_PREVIEW_NORMALS, NormalFixDialog::onPreviewNormals)
EVT_BUTTON(wxID_APPLY, NormalFixDialog::onApply)
EVT_BUTTON(wxID_OK, NormalFixDialog::onOK)
EVT_BUTTON(wxID_CANCEL, NormalFixDialog::onCancel)
EVT_BUTTON(wxID_RESET, NormalFixDialog::onReset)
END_EVENT_TABLE()

NormalFixDialog::NormalFixDialog(wxWindow* parent, OCCViewer* viewer, wxWindowID id, 
                               const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_viewer(viewer) {
    
    if (!m_viewer) {
        wxMessageBox("Viewer is not available", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    createControls();
    updateGeometryInfo();
}

void NormalFixDialog::createControls() {
    // Create main layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for different sections
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    // Create pages
    createInfoPage();
    createSettingsPage();
    createPreviewPage();
    
    m_notebook->AddPage(m_infoPage, "Geometry Info", true);
    m_notebook->AddPage(m_settingsPage, "Fix Settings", false);
    m_notebook->AddPage(m_previewPage, "Preview", false);
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
    
    // Create button panel
    wxPanel* buttonPanel = new wxPanel(this);
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_previewButton = new wxButton(buttonPanel, ID_PREVIEW_NORMALS, "Preview Normals");
    m_applyButton = new wxButton(buttonPanel, wxID_APPLY, "Apply Fix");
    m_okButton = new wxButton(buttonPanel, wxID_OK, "OK");
    m_cancelButton = new wxButton(buttonPanel, wxID_CANCEL, "Cancel");
    m_resetButton = new wxButton(buttonPanel, wxID_RESET, "Reset");
    
    buttonSizer->Add(m_previewButton, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_resetButton, 0, wxALL, 5);
    buttonSizer->Add(m_applyButton, 0, wxALL, 5);
    buttonSizer->Add(m_okButton, 0, wxALL, 5);
    buttonSizer->Add(m_cancelButton, 0, wxALL, 5);
    
    buttonPanel->SetSizer(buttonSizer);
    mainSizer->Add(buttonPanel, 0, wxEXPAND);
    
    SetSizer(mainSizer);
}

void NormalFixDialog::createInfoPage() {
    // Create scrolled window for the info page content
    m_infoPage = new wxScrolledWindow(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxHSCROLL);
    m_infoPage->SetScrollRate(10, 10);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Geometry list
    wxStaticText* listLabel = new wxStaticText(m_infoPage, wxID_ANY, "Available Geometries:");
    sizer->Add(listLabel, 0, wxALL, 5);
    
    m_geometryList = new wxListBox(m_infoPage, wxID_ANY, wxDefaultPosition, wxSize(300, 150));
    sizer->Add(m_geometryList, 0, wxEXPAND | wxALL, 5);
    
    // Geometry information
    wxStaticBoxSizer* infoSizer = new wxStaticBoxSizer(wxVERTICAL, m_infoPage, "Selected Geometry Information");
    
    m_geometryName = new wxStaticText(m_infoPage, wxID_ANY, "Name: None");
    m_faceCount = new wxStaticText(m_infoPage, wxID_ANY, "Face Count: 0");
    m_normalQuality = new wxStaticText(m_infoPage, wxID_ANY, "Normal Quality: 0.0%");
    m_normalStatus = new wxStaticText(m_infoPage, wxID_ANY, "Status: Unknown");
    
    infoSizer->Add(m_geometryName, 0, wxALL, 2);
    infoSizer->Add(m_faceCount, 0, wxALL, 2);
    infoSizer->Add(m_normalQuality, 0, wxALL, 2);
    infoSizer->Add(m_normalStatus, 0, wxALL, 2);
    
    sizer->Add(infoSizer, 0, wxEXPAND | wxALL, 5);
    
    // Normal statistics
    wxStaticBoxSizer* statsSizer = new wxStaticBoxSizer(wxVERTICAL, m_infoPage, "Normal Statistics");
    
    m_correctFacesCount = new wxStaticText(m_infoPage, wxID_ANY, "Correct Faces: 0");
    m_incorrectFacesCount = new wxStaticText(m_infoPage, wxID_ANY, "Incorrect Faces: 0");
    m_noNormalFacesCount = new wxStaticText(m_infoPage, wxID_ANY, "No Normal Faces: 0");
    m_qualityScore = new wxStaticText(m_infoPage, wxID_ANY, "Quality Score: 0.0%");
    
    statsSizer->Add(m_correctFacesCount, 0, wxALL, 2);
    statsSizer->Add(m_incorrectFacesCount, 0, wxALL, 2);
    statsSizer->Add(m_noNormalFacesCount, 0, wxALL, 2);
    statsSizer->Add(m_qualityScore, 0, wxALL, 2);
    
    sizer->Add(statsSizer, 0, wxEXPAND | wxALL, 5);
    
    // Fix comparison statistics
    wxStaticBoxSizer* comparisonSizer = new wxStaticBoxSizer(wxVERTICAL, m_infoPage, "Fix Comparison");
    
    m_preFixCorrectFaces = new wxStaticText(m_infoPage, wxID_ANY, "Before Fix - Correct Faces: N/A");
    m_preFixIncorrectFaces = new wxStaticText(m_infoPage, wxID_ANY, "Before Fix - Incorrect Faces: N/A");
    m_preFixQualityScore = new wxStaticText(m_infoPage, wxID_ANY, "Before Fix - Quality Score: N/A");
    m_improvementInfo = new wxStaticText(m_infoPage, wxID_ANY, "Improvement: N/A");
    
    comparisonSizer->Add(m_preFixCorrectFaces, 0, wxALL, 2);
    comparisonSizer->Add(m_preFixIncorrectFaces, 0, wxALL, 2);
    comparisonSizer->Add(m_preFixQualityScore, 0, wxALL, 2);
    comparisonSizer->Add(m_improvementInfo, 0, wxALL, 2);
    
    sizer->Add(comparisonSizer, 0, wxEXPAND | wxALL, 5);
    
    m_infoPage->SetSizer(sizer);
}

void NormalFixDialog::createSettingsPage() {
    // Create scrolled window for the settings page content
    m_settingsPage = new wxScrolledWindow(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxHSCROLL);
    m_settingsPage->SetScrollRate(10, 10);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Auto correction settings
    wxStaticBoxSizer* correctionSizer = new wxStaticBoxSizer(wxVERTICAL, m_settingsPage, "Auto Correction");
    
    m_autoCorrectCheck = new wxCheckBox(m_settingsPage, wxID_ANY, "Enable automatic normal correction");
    m_autoCorrectCheck->SetValue(m_settings.autoCorrect);
    
    m_qualityThresholdSlider = new wxSlider(m_settingsPage, wxID_ANY, 
                                           static_cast<int>(m_settings.qualityThreshold * 100), 
                                           0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_qualityThresholdLabel = new wxStaticText(m_settingsPage, wxID_ANY, 
                                             wxString::Format("Quality Threshold: %.1f%%", m_settings.qualityThreshold * 100));
    
    correctionSizer->Add(m_autoCorrectCheck, 0, wxALL, 5);
    correctionSizer->Add(m_qualityThresholdLabel, 0, wxALL, 5);
    correctionSizer->Add(m_qualityThresholdSlider, 0, wxEXPAND | wxALL, 5);
    
    sizer->Add(correctionSizer, 0, wxEXPAND | wxALL, 5);
    
    // Normal visualization settings
    wxStaticBoxSizer* visualSizer = new wxStaticBoxSizer(wxVERTICAL, m_settingsPage, "Normal Visualization");
    
    m_showNormalsCheck = new wxCheckBox(m_settingsPage, wxID_ANY, "Show face normal vectors");
    m_showNormalsCheck->SetValue(m_settings.showNormals);
    
    wxBoxSizer* lengthSizer = new wxBoxSizer(wxHORIZONTAL);
    lengthSizer->Add(new wxStaticText(m_settingsPage, wxID_ANY, "Normal Length:"), 0, wxALL, 5);
    m_normalLengthSpin = new wxSpinCtrlDouble(m_settingsPage, wxID_ANY, 
                                            wxString::Format("%.2f", m_settings.normalLength),
                                            wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS, 0.1, 10.0, m_settings.normalLength, 0.1);
    lengthSizer->Add(m_normalLengthSpin, 0, wxALL, 5);
    
    m_showCorrectCheck = new wxCheckBox(m_settingsPage, wxID_ANY, "Show correct normals");
    m_showCorrectCheck->SetValue(m_settings.showCorrectNormals);
    
    m_showIncorrectCheck = new wxCheckBox(m_settingsPage, wxID_ANY, "Show incorrect normals");
    m_showIncorrectCheck->SetValue(m_settings.showIncorrectNormals);
    
    visualSizer->Add(m_showNormalsCheck, 0, wxALL, 5);
    visualSizer->Add(lengthSizer, 0, wxALL, 5);
    visualSizer->Add(m_showCorrectCheck, 0, wxALL, 5);
    visualSizer->Add(m_showIncorrectCheck, 0, wxALL, 5);
    
    sizer->Add(visualSizer, 0, wxEXPAND | wxALL, 5);
    
    // Application settings
    wxStaticBoxSizer* applySizer = new wxStaticBoxSizer(wxVERTICAL, m_settingsPage, "Application Scope");
    
    m_applyToSelectedCheck = new wxCheckBox(m_settingsPage, wxID_ANY, "Apply to selected geometries only");
    m_applyToSelectedCheck->SetValue(m_settings.applyToSelected);
    
    m_applyToAllCheck = new wxCheckBox(m_settingsPage, wxID_ANY, "Apply to all geometries");
    m_applyToAllCheck->SetValue(m_settings.applyToAll);
    
    applySizer->Add(m_applyToSelectedCheck, 0, wxALL, 5);
    applySizer->Add(m_applyToAllCheck, 0, wxALL, 5);
    
    sizer->Add(applySizer, 0, wxEXPAND | wxALL, 5);
    
    m_settingsPage->SetSizer(sizer);
}

void NormalFixDialog::createPreviewPage() {
    // Create scrolled window for the preview page content
    m_previewPage = new wxScrolledWindow(m_notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxHSCROLL);
    m_previewPage->SetScrollRate(10, 10);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticText* previewLabel = new wxStaticText(m_previewPage, wxID_ANY, 
                                                 "Preview face normal vectors before applying fixes:");
    sizer->Add(previewLabel, 0, wxALL, 5);
    
    m_previewStatus = new wxStaticText(m_previewPage, wxID_ANY, "No preview generated yet");
    sizer->Add(m_previewStatus, 0, wxALL, 5);
    
    wxStaticText* noteLabel = new wxStaticText(m_previewPage, wxID_ANY, 
                                             "Note: Preview will show face normal vectors as arrows. "
                                             "Green arrows indicate correct face normals, red arrows indicate incorrect face normals.");
    noteLabel->Wrap(400);
    sizer->Add(noteLabel, 0, wxALL, 5);
    
    m_previewPage->SetSizer(sizer);
}

void NormalFixDialog::updateGeometryInfo() {
    if (!m_viewer) return;
    
    m_geometryList->Clear();
    
    // Get all geometries
    auto geometries = m_viewer->getAllGeometry();
    for (const auto& geometry : geometries) {
        if (geometry) {
            m_geometryList->AppendString(geometry->getName());
        }
    }
    
    // Select first geometry if available
    if (m_geometryList->GetCount() > 0) {
        m_geometryList->SetSelection(0);
        updateNormalInfo();
    }
}

void NormalFixDialog::updateNormalInfo() {
    if (!m_viewer) return;
    
    int selection = m_geometryList->GetSelection();
    if (selection == wxNOT_FOUND) {
        m_geometryName->SetLabel("Name: None");
        m_faceCount->SetLabel("Face Count: 0");
        m_normalQuality->SetLabel("Normal Quality: 0.0%");
        m_normalStatus->SetLabel("Status: Unknown");
        m_correctFacesCount->SetLabel("Correct Faces: 0");
        m_incorrectFacesCount->SetLabel("Incorrect Faces: 0");
        m_noNormalFacesCount->SetLabel("No Normal Faces: 0");
        m_qualityScore->SetLabel("Quality Score: 0.0%");
        m_preFixCorrectFaces->SetLabel("Before Fix - Correct Faces: N/A");
        m_preFixIncorrectFaces->SetLabel("Before Fix - Incorrect Faces: N/A");
        m_preFixQualityScore->SetLabel("Before Fix - Quality Score: N/A");
        m_improvementInfo->SetLabel("Improvement: N/A");
        return;
    }
    
    wxString geometryName = m_geometryList->GetString(selection);
    auto geometry = m_viewer->findGeometry(geometryName.ToStdString());
    
    if (!geometry) {
        m_geometryName->SetLabel("Name: Not Found");
        return;
    }
    
    m_geometryName->SetLabel("Name: " + geometryName);
    
    try {
        const TopoDS_Shape& shape = geometry->getShape();
        if (shape.IsNull()) {
            m_faceCount->SetLabel("Face Count: 0");
            m_normalQuality->SetLabel("Normal Quality: 0.0%");
            m_normalStatus->SetLabel("Status: Invalid Shape");
            m_correctFacesCount->SetLabel("Correct Faces: 0");
            m_incorrectFacesCount->SetLabel("Incorrect Faces: 0");
            m_noNormalFacesCount->SetLabel("No Normal Faces: 0");
            m_qualityScore->SetLabel("Quality Score: 0.0%");
            m_preFixCorrectFaces->SetLabel("Before Fix - Correct Faces: N/A");
            m_preFixIncorrectFaces->SetLabel("Before Fix - Incorrect Faces: N/A");
            m_preFixQualityScore->SetLabel("Before Fix - Quality Score: N/A");
            m_improvementInfo->SetLabel("Improvement: N/A");
            return;
        }
        
        // Analyze each face individually
        analyzeFaceNormals(shape, geometryName.ToStdString());
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Error analyzing normals: " + std::string(e.what()));
        m_faceCount->SetLabel("Face Count: Error");
        m_normalQuality->SetLabel("Normal Quality: Error");
        m_normalStatus->SetLabel("Status: Analysis Failed");
        m_correctFacesCount->SetLabel("Correct Faces: Error");
        m_incorrectFacesCount->SetLabel("Incorrect Faces: Error");
        m_noNormalFacesCount->SetLabel("No Normal Faces: Error");
        m_qualityScore->SetLabel("Quality Score: Error");
        m_preFixCorrectFaces->SetLabel("Before Fix - Correct Faces: N/A");
        m_preFixIncorrectFaces->SetLabel("Before Fix - Incorrect Faces: N/A");
        m_preFixQualityScore->SetLabel("Before Fix - Quality Score: N/A");
        m_improvementInfo->SetLabel("Improvement: N/A");
    }
}

void NormalFixDialog::analyzeFaceNormals(const TopoDS_Shape& shape, const std::string& shapeName) {
    // Calculate shape center for normal direction analysis
    gp_Pnt shapeCenter = NormalValidator::calculateShapeCenter(shape);
    
    int totalFaces = 0;
    int correctFaces = 0;
    int incorrectFaces = 0;
    int noNormalFaces = 0;
    
    // Analyze each face
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        totalFaces++;
        
        // Analyze this face
        bool hasNormal = NormalValidator::analyzeFaceNormal(face, shapeCenter);
        
        if (hasNormal) {
            bool isCorrect = NormalValidator::isNormalOutward(face, shapeCenter);
            if (isCorrect) {
                correctFaces++;
            } else {
                incorrectFaces++;
            }
        } else {
            noNormalFaces++;
        }
    }
    
    // Update summary information
    m_faceCount->SetLabel(wxString::Format("Face Count: %d", totalFaces));
    
    double qualityScore = totalFaces > 0 ? static_cast<double>(correctFaces) / totalFaces : 0.0;
    m_normalQuality->SetLabel(wxString::Format("Normal Quality: %.1f%%", qualityScore * 100));
    
    if (qualityScore >= 0.8) {
        m_normalStatus->SetLabel("Status: Good");
    } else if (qualityScore >= 0.5) {
        m_normalStatus->SetLabel("Status: Fair");
    } else {
        m_normalStatus->SetLabel("Status: Poor");
    }
    
    // Update statistics
    m_correctFacesCount->SetLabel(wxString::Format("Correct Faces: %d", correctFaces));
    m_incorrectFacesCount->SetLabel(wxString::Format("Incorrect Faces: %d", incorrectFaces));
    m_noNormalFacesCount->SetLabel(wxString::Format("No Normal Faces: %d", noNormalFaces));
    m_qualityScore->SetLabel(wxString::Format("Quality Score: %.1f%%", qualityScore * 100));
    
    // Update comparison statistics if we have pre-fix data
    if (m_preFixStats.hasData) {
        m_preFixCorrectFaces->SetLabel(wxString::Format("Before Fix - Correct Faces: %d", m_preFixStats.correctFaces));
        m_preFixIncorrectFaces->SetLabel(wxString::Format("Before Fix - Incorrect Faces: %d", m_preFixStats.incorrectFaces));
        m_preFixQualityScore->SetLabel(wxString::Format("Before Fix - Quality Score: %.1f%%", m_preFixStats.qualityScore * 100));
        
        // Calculate improvement
        double improvement = qualityScore - m_preFixStats.qualityScore;
        if (improvement > 0) {
            m_improvementInfo->SetLabel(wxString::Format("Improvement: +%.1f%%", improvement * 100));
        } else if (improvement < 0) {
            m_improvementInfo->SetLabel(wxString::Format("Improvement: %.1f%%", improvement * 100));
        } else {
            m_improvementInfo->SetLabel("Improvement: No change");
        }
    }
    
    LOG_INF_S("Face analysis completed for " + shapeName + ": " + 
             std::to_string(totalFaces) + " faces, " + 
             std::to_string(correctFaces) + " correct, " + 
             std::to_string(incorrectFaces) + " incorrect, " + 
             std::to_string(noNormalFaces) + " no normals");
}

void NormalFixDialog::saveCurrentStatistics() {
    if (!m_viewer) return;
    
    int selection = m_geometryList->GetSelection();
    if (selection == wxNOT_FOUND) {
        m_preFixStats.hasData = false;
        return;
    }
    
    wxString geometryName = m_geometryList->GetString(selection);
    auto geometry = m_viewer->findGeometry(geometryName.ToStdString());
    
    if (!geometry) {
        m_preFixStats.hasData = false;
        return;
    }
    
    try {
        const TopoDS_Shape& shape = geometry->getShape();
        if (shape.IsNull()) {
            m_preFixStats.hasData = false;
            return;
        }
        
        // Calculate shape center for normal direction analysis
        gp_Pnt shapeCenter = NormalValidator::calculateShapeCenter(shape);
        
        int totalFaces = 0;
        int correctFaces = 0;
        int incorrectFaces = 0;
        int noNormalFaces = 0;
        
        // Analyze each face
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            TopoDS_Face face = TopoDS::Face(exp.Current());
            totalFaces++;
            
            // Analyze this face
            bool hasNormal = NormalValidator::analyzeFaceNormal(face, shapeCenter);
            
            if (hasNormal) {
                bool isCorrect = NormalValidator::isNormalOutward(face, shapeCenter);
                if (isCorrect) {
                    correctFaces++;
                } else {
                    incorrectFaces++;
                }
            } else {
                noNormalFaces++;
            }
        }
        
        // Save statistics
        m_preFixStats.correctFaces = correctFaces;
        m_preFixStats.incorrectFaces = incorrectFaces;
        m_preFixStats.noNormalFaces = noNormalFaces;
        m_preFixStats.qualityScore = totalFaces > 0 ? static_cast<double>(correctFaces) / totalFaces : 0.0;
        m_preFixStats.hasData = true;
        
        LOG_INF_S("Pre-fix statistics saved: " + std::to_string(correctFaces) + " correct, " + 
                 std::to_string(incorrectFaces) + " incorrect, " + 
                 std::to_string(noNormalFaces) + " no normals, quality: " + 
                 std::to_string(m_preFixStats.qualityScore * 100) + "%");
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Error saving pre-fix statistics: " + std::string(e.what()));
        m_preFixStats.hasData = false;
    }
}

void NormalFixDialog::onGeometrySelectionChanged(wxCommandEvent& event) {
    updateNormalInfo();
}

void NormalFixDialog::onSettingsChanged(wxCommandEvent& event) {
    // Update settings from controls
    m_settings.autoCorrect = m_autoCorrectCheck->GetValue();
    m_settings.showNormals = m_showNormalsCheck->GetValue();
    m_settings.normalLength = m_normalLengthSpin->GetValue();
    m_settings.showCorrectNormals = m_showCorrectCheck->GetValue();
    m_settings.showIncorrectNormals = m_showIncorrectCheck->GetValue();
    m_settings.qualityThreshold = m_qualityThresholdSlider->GetValue() / 100.0;
    m_settings.applyToSelected = m_applyToSelectedCheck->GetValue();
    m_settings.applyToAll = m_applyToAllCheck->GetValue();
    
    // Update quality threshold label
    m_qualityThresholdLabel->SetLabel(wxString::Format("Quality Threshold: %.1f%%", m_settings.qualityThreshold * 100));
}

void NormalFixDialog::onSpinCtrlChanged(wxSpinDoubleEvent& event) {
    // Update settings from spin control
    m_settings.normalLength = m_normalLengthSpin->GetValue();
}

void NormalFixDialog::updateSettings() {
    m_settings.autoCorrect = m_autoCorrectCheck->GetValue();
    m_settings.showNormals = m_showNormalsCheck->GetValue();
    m_settings.showCorrectNormals = m_showCorrectCheck->GetValue();
    m_settings.showIncorrectNormals = m_showIncorrectCheck->GetValue();
    m_settings.applyToSelected = m_applyToSelectedCheck->GetValue();
    m_settings.applyToAll = m_applyToAllCheck->GetValue();
    m_settings.qualityThreshold = m_qualityThresholdSlider->GetValue() / 100.0;
    m_settings.normalLength = m_normalLengthSpin->GetValue();
}

void NormalFixDialog::onSliderChanged(wxScrollEvent& event) {
    // Update settings from slider
    m_settings.qualityThreshold = m_qualityThresholdSlider->GetValue() / 100.0;
    
    // Update quality threshold label
    m_qualityThresholdLabel->SetLabel(wxString::Format("Quality Threshold: %.1f%%", m_settings.qualityThreshold * 100));
}

void NormalFixDialog::onPreviewNormals(wxCommandEvent& event) {
    if (!m_viewer) {
        m_previewStatus->SetLabel("Error: Viewer not available");
        return;
    }
    
    // Get selected geometry
    int selection = m_geometryList->GetSelection();
    if (selection == wxNOT_FOUND) {
        m_previewStatus->SetLabel("Error: No geometry selected");
        return;
    }
    
    wxString geometryName = m_geometryList->GetString(selection);
    auto geometry = m_viewer->findGeometry(geometryName.ToStdString());
    
    if (!geometry) {
        m_previewStatus->SetLabel("Error: Geometry not found");
        return;
    }
    
    // Toggle face normal display
    bool showNormals = m_showNormalsCheck->GetValue();
    if (showNormals) {
        // Enable face normal display
        m_viewer->setShowFaceNormalLines(true);
        m_previewStatus->SetLabel("Face normal vectors displayed for: " + geometryName);
    } else {
        // Disable face normal display
        m_viewer->setShowFaceNormalLines(false);
        m_previewStatus->SetLabel("Face normal vectors hidden");
    }
    
    // Refresh the viewer
    m_viewer->requestViewRefresh();
}

void NormalFixDialog::onApply(wxCommandEvent& event) {
    if (!m_viewer) {
        wxMessageBox("Viewer not available", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    // Get current settings
    updateSettings();
    
    // Determine which geometries to process
    std::vector<std::shared_ptr<OCCGeometry>> geometries;
    
    if (m_settings.applyToSelected) {
        geometries = m_viewer->getSelectedGeometries();
        if (geometries.empty()) {
            wxMessageBox("No geometries selected. Please select geometries first.", "Warning", wxOK | wxICON_WARNING);
            return;
        }
    } else if (m_settings.applyToAll) {
        geometries = m_viewer->getAllGeometry();
        if (geometries.empty()) {
            wxMessageBox("No geometries available", "Warning", wxOK | wxICON_WARNING);
            return;
        }
    } else {
        wxMessageBox("Please select application scope (selected or all geometries)", "Warning", wxOK | wxICON_WARNING);
        return;
    }
    
    // Check if current selected geometry will be processed
    bool currentGeometryProcessed = false;
    int selection = m_geometryList->GetSelection();
    std::string currentGeometryName;
    if (selection != wxNOT_FOUND) {
        currentGeometryName = m_geometryList->GetString(selection).ToStdString();
    }
    
    // Save current statistics before fix (for comparison) only if current geometry will be processed
    if (!currentGeometryName.empty()) {
        for (const auto& geometry : geometries) {
            if (geometry && geometry->getName() == currentGeometryName) {
                currentGeometryProcessed = true;
                saveCurrentStatistics();
                break;
            }
        }
    }
    
    // Apply normal correction
    int correctedCount = 0;
    int totalCount = geometries.size();
    
    for (auto& geometry : geometries) {
        if (!geometry) continue;
        
        const TopoDS_Shape& originalShape = geometry->getShape();
        if (originalShape.IsNull()) continue;
        
        // Check if correction is needed based on quality threshold
        if (m_settings.autoCorrect) {
            double quality = NormalValidator::getNormalQualityScore(originalShape);
            LOG_INF_S("Geometry " + geometry->getName() + " quality score: " + std::to_string(quality));
            
            if (quality < m_settings.qualityThreshold) {
                LOG_INF_S("Applying normal correction to: " + geometry->getName());
                
                // Apply normal correction
                TopoDS_Shape correctedShape = NormalValidator::autoCorrectNormals(originalShape, geometry->getName());
                
                // Verify the correction worked
                double newQuality = NormalValidator::getNormalQualityScore(correctedShape);
                LOG_INF_S("After correction, quality score: " + std::to_string(newQuality));
                
                // Update the geometry with corrected shape
                geometry->setShape(correctedShape);
                correctedCount++;
                
                LOG_INF_S("Successfully corrected normals for: " + geometry->getName());
            } else {
                LOG_INF_S("Geometry " + geometry->getName() + " already has good normals (quality: " + std::to_string(quality) + ")");
            }
        }
    }
    
    // Refresh the viewer to show changes
    m_viewer->requestViewRefresh();
    
    // Show results
    wxString message = wxString::Format("Normal fix applied to %d out of %d geometries", correctedCount, totalCount);
    wxMessageBox(message, "Normal Fix Complete", wxOK | wxICON_INFORMATION);
    
    // Update current geometry info to reflect changes only if current geometry was processed
    if (currentGeometryProcessed) {
        updateNormalInfo();
    }
}

void NormalFixDialog::onOK(wxCommandEvent& event) {
    onApply(event);
    EndModal(wxID_OK);
}

void NormalFixDialog::onCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void NormalFixDialog::onReset(wxCommandEvent& event) {
    // Reset to default settings
    m_settings = NormalFixSettings();
    setSettings(m_settings);
}

NormalFixDialog::NormalFixSettings NormalFixDialog::getSettings() const {
    return m_settings;
}

void NormalFixDialog::setSettings(const NormalFixSettings& settings) {
    m_settings = settings;
    
    // Update controls
    m_autoCorrectCheck->SetValue(m_settings.autoCorrect);
    m_showNormalsCheck->SetValue(m_settings.showNormals);
    m_normalLengthSpin->SetValue(m_settings.normalLength);
    m_showCorrectCheck->SetValue(m_settings.showCorrectNormals);
    m_showIncorrectCheck->SetValue(m_settings.showIncorrectNormals);
    m_qualityThresholdSlider->SetValue(static_cast<int>(m_settings.qualityThreshold * 100));
    m_applyToSelectedCheck->SetValue(m_settings.applyToSelected);
    m_applyToAllCheck->SetValue(m_settings.applyToAll);
    
    m_qualityThresholdLabel->SetLabel(wxString::Format("Quality Threshold: %.1f%%", m_settings.qualityThreshold * 100));
}
