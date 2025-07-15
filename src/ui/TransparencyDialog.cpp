#include "TransparencyDialog.h"
#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"
#include <wx/statbox.h>
#include <wx/msgdlg.h>

TransparencyDialog::TransparencyDialog(wxWindow* parent, OCCViewer* occViewer,
                                     const std::vector<std::shared_ptr<OCCGeometry>>& selectedGeometries)
    : wxDialog(parent, wxID_ANY, "Set Transparency", wxDefaultPosition, wxSize(400, 300))
    , m_occViewer(occViewer)
    , m_selectedGeometries(selectedGeometries)
    , m_transparencySlider(nullptr)
    , m_transparencySpinCtrl(nullptr)
    , m_infoText(nullptr)
    , m_currentTransparency(0.0)
    , m_originalTransparency(0.0)
{
    if (!m_occViewer) {
        LOG_ERR_S("OCCViewer is null in TransparencyDialog");
        return;
    }
    
    if (m_selectedGeometries.empty()) {
        LOG_ERR_S("No selected geometries in TransparencyDialog");
        return;
    }

    // Get current transparency from the first selected geometry
    // In a real implementation, you might want to handle mixed transparency values
    m_currentTransparency = 0.0; // Default value
    if (!m_selectedGeometries.empty() && m_selectedGeometries[0]) {
        m_currentTransparency = m_selectedGeometries[0]->getTransparency();
    }
    m_originalTransparency = m_currentTransparency;
    
    createControls();
    layoutControls();
    bindEvents();
    updateControls();
    
    Center();
    Fit();
    SetMinSize(GetBestSize());
}

TransparencyDialog::~TransparencyDialog()
{
}

void TransparencyDialog::createControls()
{
    // Info text
    m_infoText = new wxStaticText(this, wxID_ANY, 
        wxString::Format("Setting transparency for %zu selected object(s)", m_selectedGeometries.size()));
    
    // Transparency slider (0-100 for percentage)
    m_transparencySlider = new wxSlider(this, ID_TRANSPARENCY_SLIDER, 
        static_cast<int>(m_currentTransparency * 100), 0, 100,
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    
    // Transparency spin control
    m_transparencySpinCtrl = new wxSpinCtrlDouble(this, ID_TRANSPARENCY_SPIN, 
        wxString::Format("%.1f", m_currentTransparency * 100),
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS,
        0.0, 100.0, m_currentTransparency * 100, 0.1);
    m_transparencySpinCtrl->SetDigits(1);
}

void TransparencyDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Info section
    mainSizer->Add(m_infoText, 0, wxALL | wxEXPAND, 10);
    
    // Transparency control section
    wxStaticBox* transparencyBox = new wxStaticBox(this, wxID_ANY, "Transparency Settings");
    wxStaticBoxSizer* transparencySizer = new wxStaticBoxSizer(transparencyBox, wxVERTICAL);
    
    transparencySizer->Add(new wxStaticText(this, wxID_ANY, "Transparency (0% = Opaque, 100% = Transparent):"), 
                          0, wxALL, 5);
    transparencySizer->Add(m_transparencySlider, 0, wxEXPAND | wxALL, 5);
    
    // Spin control in horizontal layout
    wxBoxSizer* spinSizer = new wxBoxSizer(wxHORIZONTAL);
    spinSizer->Add(new wxStaticText(this, wxID_ANY, "Precise value (%):"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    spinSizer->Add(m_transparencySpinCtrl, 0, wxALIGN_CENTER_VERTICAL);
    
    transparencySizer->Add(spinSizer, 0, wxALL | wxALIGN_CENTER, 5);
    
    mainSizer->Add(transparencySizer, 1, wxEXPAND | wxALL, 10);
    
    // Button section
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_APPLY, "Apply"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, wxID_OK, "OK"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);
    
    SetSizer(mainSizer);
}

void TransparencyDialog::bindEvents()
{
    m_transparencySlider->Bind(wxEVT_SLIDER, &TransparencyDialog::onTransparencySlider, this);
    m_transparencySpinCtrl->Bind(wxEVT_SPINCTRLDOUBLE, &TransparencyDialog::onTransparencySpinCtrl, this);
    
    FindWindow(wxID_APPLY)->Bind(wxEVT_BUTTON, &TransparencyDialog::onApply, this);
    FindWindow(wxID_OK)->Bind(wxEVT_BUTTON, &TransparencyDialog::onOK, this);
    FindWindow(wxID_CANCEL)->Bind(wxEVT_BUTTON, &TransparencyDialog::onCancel, this);
}

void TransparencyDialog::updateControls()
{
    m_transparencySlider->SetValue(static_cast<int>(m_currentTransparency * 100));
    m_transparencySpinCtrl->SetValue(m_currentTransparency * 100);
}

void TransparencyDialog::applyTransparency()
{
    if (!m_occViewer || m_selectedGeometries.empty()) {
        return;
    }
    
    try {
        // Apply transparency to all selected geometries
        for (const auto& geometry : m_selectedGeometries) {
            if (geometry) {
                m_occViewer->setGeometryTransparency(geometry->getName(), m_currentTransparency);
            }
        }
        
        LOG_INF_S("Applied transparency " + std::to_string(m_currentTransparency) + 
                  " to " + std::to_string(m_selectedGeometries.size()) + " geometries");
    } catch (const std::exception& e) {
        LOG_ERR_S("Error applying transparency: " + std::string(e.what()));
        wxMessageBox("Error applying transparency: " + wxString(e.what()), "Error", wxOK | wxICON_ERROR);
    }
}

void TransparencyDialog::onTransparencySlider(wxCommandEvent& event)
{
    int sliderValue = m_transparencySlider->GetValue();
    m_currentTransparency = sliderValue / 100.0;
    
    // Update spin control
    m_transparencySpinCtrl->SetValue(sliderValue);
    
    // Apply transparency in real-time
    applyTransparency();
}

void TransparencyDialog::onTransparencySpinCtrl(wxSpinDoubleEvent& event)
{
    double spinValue = m_transparencySpinCtrl->GetValue();
    m_currentTransparency = spinValue / 100.0;
    
    // Update slider
    m_transparencySlider->SetValue(static_cast<int>(spinValue));
    
    // Apply transparency in real-time
    applyTransparency();
}

void TransparencyDialog::onApply(wxCommandEvent& event)
{
    applyTransparency();
}

void TransparencyDialog::onOK(wxCommandEvent& event)
{
    applyTransparency();
    EndModal(wxID_OK);
}

void TransparencyDialog::onCancel(wxCommandEvent& event)
{
    // Restore original transparency
    if (m_occViewer && !m_selectedGeometries.empty()) {
        try {
            for (const auto& geometry : m_selectedGeometries) {
                if (geometry) {
                    m_occViewer->setGeometryTransparency(geometry->getName(), m_originalTransparency);
                }
            }
        } catch (const std::exception& e) {
            LOG_ERR_S("Error restoring transparency: " + std::string(e.what()));
        }
    }
    
    EndModal(wxID_CANCEL);
} 