#include "ui/OutlineSettingsDialog.h"

OutlineSettingsDialog::OutlineSettingsDialog(wxWindow* parent, const ImageOutlineParams& params)
    : wxDialog(parent, wxID_ANY, "Outline Settings", wxDefaultPosition, wxSize(360, 300)), m_params(params) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto makeSlider = [&](const wxString& label, int min, int max, int value) {
        auto* box = new wxBoxSizer(wxHORIZONTAL);
        box->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        auto* slider = new wxSlider(this, wxID_ANY, value, min, max, wxDefaultPosition, wxSize(220, -1));
        box->Add(slider, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
        sizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        return slider;
    };
    m_depthW   = makeSlider("Depth Weight", 0, 200, int(params.depthWeight * 100));
    m_normalW  = makeSlider("Normal Weight", 0, 200, int(params.normalWeight * 100));
    m_depthTh  = makeSlider("Depth Threshold", 0, 50, int(params.depthThreshold * 1000));
    m_normalTh = makeSlider("Normal Threshold", 0, 200, int(params.normalThreshold * 100));
    m_intensity= makeSlider("Edge Intensity", 0, 200, int(params.edgeIntensity * 100));
    m_thickness= makeSlider("Thickness", 10, 400, int(params.thickness * 100));
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* ok = new wxButton(this, wxID_OK, "OK");
    auto* cancel = new wxButton(this, wxID_CANCEL, "Cancel");
    ok->Bind(wxEVT_BUTTON, &OutlineSettingsDialog::onOk, this);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(ok, 0, wxALL, 5);
    btnSizer->Add(cancel, 0, wxALL, 5);
    sizer->AddStretchSpacer();
    sizer->Add(btnSizer, 0, wxEXPAND | wxALL, 8);
    SetSizerAndFit(sizer);
}

void OutlineSettingsDialog::onOk(wxCommandEvent&) {
    m_params.depthWeight    = m_depthW->GetValue() / 100.f;
    m_params.normalWeight   = m_normalW->GetValue() / 100.f;
    m_params.depthThreshold = m_depthTh->GetValue() / 1000.f;
    m_params.normalThreshold= m_normalTh->GetValue() / 100.f;
    m_params.edgeIntensity  = m_intensity->GetValue() / 100.f;
    m_params.thickness      = m_thickness->GetValue() / 100.f;
    EndModal(wxID_OK);
}


