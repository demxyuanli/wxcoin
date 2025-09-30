#include "ExplodeConfigDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

ExplodeConfigDialog::ExplodeConfigDialog(wxWindow* parent,
    ExplodeMode currentMode,
    double currentFactor)
    : FramelessModalPopup(parent, "Explode", wxSize(400, 400))
{
    // Set up title bar with icon
    SetTitleIcon("explosion", wxSize(20, 20));
    ShowTitleIcon(true);

    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

    wxArrayString modes;
    modes.Add("Radial");
    modes.Add("Axis X");
    modes.Add("Axis Y");
    modes.Add("Axis Z");
    modes.Add("Stack X");
    modes.Add("Stack Y");
    modes.Add("Stack Z");
    modes.Add("Diagonal");
    modes.Add("Assembly");

    m_mode = new wxRadioBox(m_contentPanel, wxID_ANY, "Mode", wxDefaultPosition, wxDefaultSize, modes, 1, wxRA_SPECIFY_ROWS);
    m_factor = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY);
    m_factor->SetRange(0.01, 10.0);
    m_factor->SetIncrement(0.05);

    m_mode->SetSelection(modeToSelection(currentMode));
    m_factor->SetValue(currentFactor);

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 8, 8);
    grid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Distance Factor:"), 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(m_factor, 1, wxEXPAND);

    // Sliders helpers
    auto makeRow = [&](const wxString& label, wxSlider** out, int value, int minV, int maxV){
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(m_contentPanel, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        *out = new wxSlider(m_contentPanel, wxID_ANY, value, minV, maxV);
        row->Add(*out, 1, wxEXPAND);
        return row;
    };

    // Direction weights
    wxBoxSizer* weightsBox = new wxBoxSizer(wxVERTICAL);
    weightsBox->Add(makeRow("Radial Weight", &m_weightRadial, 100, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    weightsBox->Add(makeRow("Axis X Weight", &m_weightX, 0, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    weightsBox->Add(makeRow("Axis Y Weight", &m_weightY, 0, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    weightsBox->Add(makeRow("Axis Z Weight", &m_weightZ, 0, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    weightsBox->Add(makeRow("Diagonal Weight", &m_weightDiag, 0, 0, 200), 0, wxEXPAND);

    // Advanced parameters
    wxStaticText* advTitle = new wxStaticText(m_contentPanel, wxID_ANY, "Advanced:");
    wxBoxSizer* advBox = new wxBoxSizer(wxVERTICAL);
    advBox->Add(makeRow("Per-Level Scale", &m_perLevelScale, 60, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    advBox->Add(makeRow("Size Influence", &m_sizeInfluence, 0, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    advBox->Add(makeRow("Jitter", &m_jitter, 0, 0, 100), 0, wxEXPAND | wxBOTTOM, 4);
    advBox->Add(makeRow("Min Spacing", &m_minSpacing, 0, 0, 200), 0, wxEXPAND);

    top->Add(m_mode, 0, wxALL | wxEXPAND, 8);
    top->Add(grid, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);
    top->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Directional Weights:"), 0, wxLEFT | wxRIGHT | wxTOP, 8);
    top->Add(weightsBox, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);
    top->Add(advTitle, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    top->Add(advBox, 1, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);

    wxStdDialogButtonSizer* btns = new wxStdDialogButtonSizer();
    btns->AddButton(new wxButton(m_contentPanel, wxID_OK));
    btns->AddButton(new wxButton(m_contentPanel, wxID_CANCEL));
    btns->Realize();
    top->Add(btns, 0, wxALL | wxALIGN_RIGHT, 6);

    SetSizerAndFit(top);
    SetMinSize(wxSize(360, 360));
    SetMaxSize(wxSize(400, 400));

    m_mode->Bind(wxEVT_RADIOBOX, [this](wxCommandEvent&){ updateSliderEnableByMode(); });
    updateSliderEnableByMode();
}

void ExplodeConfigDialog::updateSliderEnableByMode() {
    int sel = m_mode ? m_mode->GetSelection() : 0;
    bool enRadial = (sel == 0 || sel == 8);
    bool enX = (sel == 1 || sel == 4);
    bool enY = (sel == 2 || sel == 5);
    bool enZ = (sel == 3 || sel == 6);
    bool enDiag = (sel == 7);
    if (m_weightRadial) m_weightRadial->Enable(enRadial);
    if (m_weightX) m_weightX->Enable(enX);
    if (m_weightY) m_weightY->Enable(enY);
    if (m_weightZ) m_weightZ->Enable(enZ);
    if (m_weightDiag) m_weightDiag->Enable(enDiag);
}

ExplodeMode ExplodeConfigDialog::getMode() const {
    return selectionToMode(m_mode ? m_mode->GetSelection() : 0);
}

ExplodeParams ExplodeConfigDialog::getParams() const {
    ExplodeParams p;
    p.primaryMode = getMode();
    p.baseFactor = getFactor();
    auto v200 = [&](wxSlider* s){ return s ? (s->GetValue() / 100.0) : 1.0; };
    auto v100 = [&](wxSlider* s){ return s ? (s->GetValue() / 100.0) : 0.0; };
    p.weights.radial = v200(m_weightRadial);
    p.weights.axisX = v200(m_weightX);
    p.weights.axisY = v200(m_weightY);
    p.weights.axisZ = v200(m_weightZ);
    p.weights.diagonal = v200(m_weightDiag);
    p.perLevelScale = v200(m_perLevelScale);
    p.sizeInfluence = v200(m_sizeInfluence);
    p.jitter = v100(m_jitter);
    p.minSpacing = v200(m_minSpacing);
    p.centerMode = ExplodeCenterMode::GlobalCenter;
    p.scope = ExplodeScope::All;
    p.customCenter = gp_Pnt(0,0,0);
    return p;
}

int ExplodeConfigDialog::modeToSelection(ExplodeMode mode) {
    switch (mode) {
    case ExplodeMode::AxisX: return 1;
    case ExplodeMode::AxisY: return 2;
    case ExplodeMode::AxisZ: return 3;
    case ExplodeMode::StackX: return 4;
    case ExplodeMode::StackY: return 5;
    case ExplodeMode::StackZ: return 6;
    case ExplodeMode::Diagonal: return 7;
    case ExplodeMode::Assembly: return 8;
    case ExplodeMode::Radial:
    default:
        return 0;
    }
}

ExplodeMode ExplodeConfigDialog::selectionToMode(int sel) {
    if (sel == 1) return ExplodeMode::AxisX;
    if (sel == 2) return ExplodeMode::AxisY;
    if (sel == 3) return ExplodeMode::AxisZ;
    if (sel == 4) return ExplodeMode::StackX;
    if (sel == 5) return ExplodeMode::StackY;
    if (sel == 6) return ExplodeMode::StackZ;
    if (sel == 7) return ExplodeMode::Diagonal;
    if (sel == 8) return ExplodeMode::Assembly;
    return ExplodeMode::Radial;
}


