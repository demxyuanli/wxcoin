#pragma once

#include "widgets/FramelessModalPopup.h"
#include <wx/radiobox.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>
#include "viewer/ExplodeTypes.h"

class ExplodeConfigDialog : public FramelessModalPopup {
public:
    ExplodeConfigDialog(wxWindow* parent,
        ExplodeMode currentMode,
        double currentFactor);

    ExplodeMode getMode() const;
    double getFactor() const { return m_factor ? m_factor->GetValue() : 1.0; }

    // Advanced params: build from current UI
    ExplodeParams getParams() const;

private:
    static int modeToSelection(ExplodeMode mode);
    static ExplodeMode selectionToMode(int sel);
    void updateSliderEnableByMode();

private:
    wxRadioBox* m_mode{ nullptr };
    wxSpinCtrlDouble* m_factor{ nullptr };
    // Weight sliders (0-200 => 0.0-2.0)
    wxSlider* m_weightRadial{ nullptr };
    wxSlider* m_weightX{ nullptr };
    wxSlider* m_weightY{ nullptr };
    wxSlider* m_weightZ{ nullptr };
    wxSlider* m_weightDiag{ nullptr };
    // Advanced sliders
    wxSlider* m_perLevelScale{ nullptr };   // 0-200 => 0.0-2.0
    wxSlider* m_sizeInfluence{ nullptr };   // 0-200 => 0.0-2.0
    wxSlider* m_jitter{ nullptr };          // 0-100 => 0.0-1.0
    wxSlider* m_minSpacing{ nullptr };      // 0-200 => 0.0-2.0
};



