#include "MeshQualityDialog.h"
#include "OCCViewer.h"
#include "logger/Logger.h"
#include <wx/statbox.h>

MeshQualityDialog::MeshQualityDialog(wxWindow* parent, OCCViewer* occViewer)
    : wxDialog(parent, wxID_ANY, "Mesh Quality Control", wxDefaultPosition, wxSize(480, 600))
    , m_occViewer(occViewer)
    , m_deflectionSlider(nullptr)
    , m_deflectionSpinCtrl(nullptr)
    , m_lodEnableCheckBox(nullptr)
    , m_lodRoughDeflectionSlider(nullptr)
    , m_lodRoughDeflectionSpinCtrl(nullptr)
    , m_lodFineDeflectionSlider(nullptr)
    , m_lodFineDeflectionSpinCtrl(nullptr)
    , m_lodTransitionTimeSlider(nullptr)
    , m_lodTransitionTimeSpinCtrl(nullptr)
{
    if (!m_occViewer) {
        LOG_ERR_S("OCCViewer is null in MeshQualityDialog");
        return;
    }
    
    m_currentDeflection = m_occViewer->getMeshDeflection();
    m_currentLODEnabled = m_occViewer->isLODEnabled();
    m_currentLODRoughDeflection = m_occViewer->getLODRoughDeflection();
    m_currentLODFineDeflection = m_occViewer->getLODFineDeflection();
    m_currentLODTransitionTime = m_occViewer->getLODTransitionTime();
    
    createControls();
    layoutControls();
    bindEvents();
    updateControls();

    m_deflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onDeflectionSpinCtrl, this);
    m_lodRoughDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onLODRoughDeflectionSpinCtrl, this);
    m_lodFineDeflectionSpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &MeshQualityDialog::onLODFineDeflectionSpinCtrl, this);
    m_lodTransitionTimeSpinCtrl->Bind(wxEVT_SPINCTRL, &MeshQualityDialog::onLODTransitionTimeSpinCtrl, this);

    m_deflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onDeflectionSlider, this);
    m_lodRoughDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODRoughDeflectionSlider, this);
    m_lodFineDeflectionSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODFineDeflectionSlider, this);
    m_lodTransitionTimeSlider->Bind(wxEVT_SLIDER, &MeshQualityDialog::onLODTransitionTimeSlider, this);
    m_lodEnableCheckBox->Bind(wxEVT_CHECKBOX, &MeshQualityDialog::onLODEnable, this);
    FindWindow(wxID_APPLY)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onApply, this);
    FindWindow(wxID_CANCEL)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onCancel, this);
    FindWindow(wxID_OK)->Bind(wxEVT_BUTTON, &MeshQualityDialog::onOK, this);

    Fit();
    SetMinSize(GetBestSize());
}

MeshQualityDialog::~MeshQualityDialog()
{
}

void MeshQualityDialog::createControls()
{
    m_deflectionSlider = new wxSlider(this, wxID_ANY, 
        static_cast<int>(m_currentDeflection * 1000), 1, 1000,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    
    m_deflectionSpinCtrl = new wxSpinCtrlDouble(this, wxID_ANY, 
        wxString::Format("%.3f", m_currentDeflection),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
        0.001, 1.0, m_currentDeflection, 0.001);
    
    m_lodEnableCheckBox = new wxCheckBox(this, wxID_ANY, "Enable Level of Detail (LOD)");
    m_lodEnableCheckBox->SetValue(m_currentLODEnabled);
    
    m_lodRoughDeflectionSlider = new wxSlider(this, wxID_ANY,
        static_cast<int>(m_currentLODRoughDeflection * 1000), 1, 1000,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    
    m_lodRoughDeflectionSpinCtrl = new wxSpinCtrlDouble(this, wxID_ANY,
        wxString::Format("%.3f", m_currentLODRoughDeflection),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
        0.001, 1.0, m_currentLODRoughDeflection, 0.001);
    
    m_lodFineDeflectionSlider = new wxSlider(this, wxID_ANY,
        static_cast<int>(m_currentLODFineDeflection * 1000), 1, 1000,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    
    m_lodFineDeflectionSpinCtrl = new wxSpinCtrlDouble(this, wxID_ANY,
        wxString::Format("%.3f", m_currentLODFineDeflection),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
        0.001, 1.0, m_currentLODFineDeflection, 0.001);
    
    m_lodTransitionTimeSlider = new wxSlider(this, wxID_ANY,
        m_currentLODTransitionTime, 100, 2000,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    
    m_lodTransitionTimeSpinCtrl = new wxSpinCtrl(this, wxID_ANY,
        wxString::Format("%d", m_currentLODTransitionTime),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
        100, 2000, m_currentLODTransitionTime);
}

void MeshQualityDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxStaticBox* deflectionBox = new wxStaticBox(this, wxID_ANY, "Mesh Deflection");
    wxStaticBoxSizer* deflectionSizer = new wxStaticBoxSizer(deflectionBox, wxVERTICAL);
    
    deflectionSizer->Add(new wxStaticText(this, wxID_ANY, "Deflection controls mesh precision (lower = higher quality):"), 0, wxALL, 5);
    deflectionSizer->Add(m_deflectionSlider, 0, wxEXPAND | wxALL, 5);
    deflectionSizer->Add(m_deflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);
    
    mainSizer->Add(deflectionSizer, 0, wxEXPAND | wxALL, 10);
    
    wxStaticBox* lodBox = new wxStaticBox(this, wxID_ANY, "Level of Detail (LOD)");
    wxStaticBoxSizer* lodSizer = new wxStaticBoxSizer(lodBox, wxVERTICAL);
    
    lodSizer->Add(new wxStaticText(this, wxID_ANY, "LOD automatically adjusts mesh quality during interaction:"), 0, wxALL, 5);
    lodSizer->Add(m_lodEnableCheckBox, 0, wxALL, 5);
    
    lodSizer->Add(new wxStaticText(this, wxID_ANY, "Rough deflection (during interaction):"), 0, wxALL, 5);
    lodSizer->Add(m_lodRoughDeflectionSlider, 0, wxEXPAND | wxALL, 5);
    lodSizer->Add(m_lodRoughDeflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);
    
    lodSizer->Add(new wxStaticText(this, wxID_ANY, "Fine deflection (after interaction):"), 0, wxALL, 5);
    lodSizer->Add(m_lodFineDeflectionSlider, 0, wxEXPAND | wxALL, 5);
    lodSizer->Add(m_lodFineDeflectionSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);
    
    lodSizer->Add(new wxStaticText(this, wxID_ANY, "Transition time (milliseconds):"), 0, wxALL, 5);
    lodSizer->Add(m_lodTransitionTimeSlider, 0, wxEXPAND | wxALL, 5);
    lodSizer->Add(m_lodTransitionTimeSpinCtrl, 0, wxALIGN_CENTER | wxALL, 5);
    
    mainSizer->Add(lodSizer, 0, wxEXPAND | wxALL, 10);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_APPLY, "Apply"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    
    SetSizer(mainSizer);
    Layout();
}

void MeshQualityDialog::bindEvents()
{
    // Note: Event table handles most events
    // Additional bindings can be added here if needed
}

void MeshQualityDialog::updateControls()
{
    bool lodEnabled = m_lodEnableCheckBox->GetValue();
    m_lodRoughDeflectionSlider->Enable(lodEnabled);
    m_lodRoughDeflectionSpinCtrl->Enable(lodEnabled);
    m_lodFineDeflectionSlider->Enable(lodEnabled);
    m_lodFineDeflectionSpinCtrl->Enable(lodEnabled);
    m_lodTransitionTimeSlider->Enable(lodEnabled);
    m_lodTransitionTimeSpinCtrl->Enable(lodEnabled);
}

void MeshQualityDialog::onDeflectionSlider(wxCommandEvent& event)
{
    double value = static_cast<double>(m_deflectionSlider->GetValue()) / 1000.0;
    m_deflectionSpinCtrl->SetValue(value);
    m_currentDeflection = value;
}

void MeshQualityDialog::onDeflectionSpinCtrl(wxSpinDoubleEvent& event)
{
    double value = m_deflectionSpinCtrl->GetValue();
    m_deflectionSlider->SetValue(static_cast<int>(value * 1000));
    m_currentDeflection = value;
}

void MeshQualityDialog::onLODEnable(wxCommandEvent& event)
{
    m_currentLODEnabled = m_lodEnableCheckBox->GetValue();
    updateControls();
}

void MeshQualityDialog::onLODRoughDeflectionSlider(wxCommandEvent& event)
{
    double value = static_cast<double>(m_lodRoughDeflectionSlider->GetValue()) / 1000.0;
    m_lodRoughDeflectionSpinCtrl->SetValue(value);
    m_currentLODRoughDeflection = value;
}

void MeshQualityDialog::onLODRoughDeflectionSpinCtrl(wxSpinDoubleEvent& event)
{
    double value = m_lodRoughDeflectionSpinCtrl->GetValue();
    m_lodRoughDeflectionSlider->SetValue(static_cast<int>(value * 1000));
    m_currentLODRoughDeflection = value;
}

void MeshQualityDialog::onLODFineDeflectionSlider(wxCommandEvent& event)
{
    double value = static_cast<double>(m_lodFineDeflectionSlider->GetValue()) / 1000.0;
    m_lodFineDeflectionSpinCtrl->SetValue(value);
    m_currentLODFineDeflection = value;
}

void MeshQualityDialog::onLODFineDeflectionSpinCtrl(wxSpinDoubleEvent& event)
{
    double value = m_lodFineDeflectionSpinCtrl->GetValue();
    m_lodFineDeflectionSlider->SetValue(static_cast<int>(value * 1000));
    m_currentLODFineDeflection = value;
}

void MeshQualityDialog::onLODTransitionTimeSlider(wxCommandEvent& event)
{
    int value = m_lodTransitionTimeSlider->GetValue();
    m_lodTransitionTimeSpinCtrl->SetValue(value);
    m_currentLODTransitionTime = value;
}

void MeshQualityDialog::onLODTransitionTimeSpinCtrl(wxSpinEvent& event)
{
    int value = m_lodTransitionTimeSpinCtrl->GetValue();
    m_lodTransitionTimeSlider->SetValue(value);
    m_currentLODTransitionTime = value;
}

void MeshQualityDialog::onApply(wxCommandEvent& event)
{
    if (!m_occViewer) {
        LOG_ERR("OCCViewer is null, cannot apply settings","MeshQualityDialog");
        return;
    }
    
    m_occViewer->setMeshDeflection(m_currentDeflection, true);
    
    m_occViewer->setLODEnabled(m_currentLODEnabled);
    m_occViewer->setLODRoughDeflection(m_currentLODRoughDeflection);
    m_occViewer->setLODFineDeflection(m_currentLODFineDeflection);
    m_occViewer->setLODTransitionTime(m_currentLODTransitionTime);
    
    LOG_INF_S("Mesh quality settings applied");
}

void MeshQualityDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void MeshQualityDialog::onOK(wxCommandEvent& event)
{
    onApply(event);
    EndModal(wxID_OK);
} 
