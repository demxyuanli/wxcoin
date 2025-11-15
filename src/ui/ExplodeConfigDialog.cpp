#include "ExplodeConfigDialog.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/scrolwin.h>
#include <wx/statbox.h>

ExplodeConfigDialog::ExplodeConfigDialog(wxWindow* parent,
    ExplodeMode currentMode,
    double currentFactor)
    : FramelessModalPopup(parent, "Explode Configuration", wxSize(500, 600))
{
    // Set up title bar with icon
    SetTitleIcon("explosion", wxSize(20, 20));
    ShowTitleIcon(true);

    // Create main layout
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create scrolled window for content
    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(m_contentPanel, wxID_ANY, 
        wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxHSCROLL);
    scrolledWindow->SetScrollRate(10, 10);
    
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

    // Mode selection section
    wxStaticBoxSizer* modeBox = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "Explode Mode");
    
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
    modes.Add("Smart");

    m_mode = new wxRadioBox(scrolledWindow, wxID_ANY, "Mode", wxDefaultPosition, wxDefaultSize, modes, 3, wxRA_SPECIFY_COLS);
    m_mode->SetSelection(modeToSelection(currentMode));
    modeBox->Add(m_mode, 0, wxEXPAND | wxALL, 5);
    
    contentSizer->Add(modeBox, 0, wxEXPAND | wxALL, 5);

    // Distance factor section
    wxStaticBoxSizer* factorBox = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "Distance Factor");
    
    wxFlexGridSizer* factorGrid = new wxFlexGridSizer(2, 8, 8);
    factorGrid->AddGrowableCol(1);
    
    m_factor = new wxSpinCtrlDouble(scrolledWindow, wxID_ANY);
    m_factor->SetRange(0.01, 10.0);
    m_factor->SetIncrement(0.05);
    m_factor->SetValue(currentFactor);
    
    factorGrid->Add(new wxStaticText(scrolledWindow, wxID_ANY, "Factor:"), 0, wxALIGN_CENTER_VERTICAL);
    factorGrid->Add(m_factor, 1, wxEXPAND);
    
    factorBox->Add(factorGrid, 0, wxEXPAND | wxALL, 5);
    contentSizer->Add(factorBox, 0, wxEXPAND | wxALL, 5);

    // Direction weights section
    wxStaticBoxSizer* weightsBox = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "Directional Weights");
    
    // Sliders helper function
    auto makeSliderRow = [&](const wxString& label, wxSlider** out, int value, int minV, int maxV){
        wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(scrolledWindow, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        *out = new wxSlider(scrolledWindow, wxID_ANY, value, minV, maxV);
        row->Add(*out, 1, wxEXPAND);
        return row;
    };

    weightsBox->Add(makeSliderRow("Radial Weight", &m_weightRadial, 100, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    weightsBox->Add(makeSliderRow("Axis X Weight", &m_weightX, 0, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    weightsBox->Add(makeSliderRow("Axis Y Weight", &m_weightY, 0, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    weightsBox->Add(makeSliderRow("Axis Z Weight", &m_weightZ, 0, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    weightsBox->Add(makeSliderRow("Diagonal Weight", &m_weightDiag, 0, 0, 200), 0, wxEXPAND);
    
    contentSizer->Add(weightsBox, 0, wxEXPAND | wxALL, 5);

    // Advanced parameters section
    wxStaticBoxSizer* advancedBox = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "Advanced Parameters");
    
    advancedBox->Add(makeSliderRow("Per-Level Scale", &m_perLevelScale, 60, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    advancedBox->Add(makeSliderRow("Size Influence", &m_sizeInfluence, 0, 0, 200), 0, wxEXPAND | wxBOTTOM, 4);
    advancedBox->Add(makeSliderRow("Jitter", &m_jitter, 0, 0, 100), 0, wxEXPAND | wxBOTTOM, 4);
    advancedBox->Add(makeSliderRow("Min Spacing", &m_minSpacing, 0, 0, 200), 0, wxEXPAND);
    
    contentSizer->Add(advancedBox, 0, wxEXPAND | wxALL, 5);

    // Collision detection section
    wxStaticBoxSizer* collisionBox = new wxStaticBoxSizer(wxVERTICAL, scrolledWindow, "Collision Detection");
    
    m_enableCollision = new wxCheckBox(scrolledWindow, wxID_ANY, "Enable Collision Resolution");
    m_enableCollision->SetValue(false);
    collisionBox->Add(m_enableCollision, 0, wxALL, 5);
    
    collisionBox->Add(makeSliderRow("Collision Threshold", &m_collisionThreshold, 60, 0, 100), 0, wxEXPAND);
    
    contentSizer->Add(collisionBox, 0, wxEXPAND | wxALL, 5);

    // Set sizer for scrolled window
    scrolledWindow->SetSizer(contentSizer);
    scrolledWindow->FitInside();
    
    mainSizer->Add(scrolledWindow, 1, wxEXPAND | wxALL, 5);

    // Button section
    wxStdDialogButtonSizer* btns = new wxStdDialogButtonSizer();
    btns->AddButton(new wxButton(m_contentPanel, wxID_OK));
    btns->AddButton(new wxButton(m_contentPanel, wxID_CANCEL));
    btns->Realize();
    mainSizer->Add(btns, 0, wxALL | wxALIGN_RIGHT, 5);

    m_contentPanel->SetSizer(mainSizer);
    
    // Set reasonable size constraints
    SetMinSize(wxSize(450, 500));
    SetMaxSize(wxSize(600, 800));

    // Bind events
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
    bool enSmart = (sel == 9);
    if (m_weightRadial) m_weightRadial->Enable(enRadial || enSmart);
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
    p.enableCollisionResolution = m_enableCollision ? m_enableCollision->GetValue() : false;
    p.collisionThreshold = v100(m_collisionThreshold);
    if (p.collisionThreshold < 0.1) p.collisionThreshold = 0.6;
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
    case ExplodeMode::Smart: return 9;
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
    if (sel == 9) return ExplodeMode::Smart;
    return ExplodeMode::Radial;
}


