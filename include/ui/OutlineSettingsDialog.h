#pragma once

#include <wx/wx.h>
#include "viewer/ImageOutlinePass.h"

class OutlineSettingsDialog : public wxDialog {
public:
    OutlineSettingsDialog(wxWindow* parent, const ImageOutlineParams& params);
    ImageOutlineParams getParams() const { return m_params; }

private:
    ImageOutlineParams m_params;
    wxSlider* m_depthW{nullptr};
    wxSlider* m_normalW{nullptr};
    wxSlider* m_depthTh{nullptr};
    wxSlider* m_normalTh{nullptr};
    wxSlider* m_intensity{nullptr};
    wxSlider* m_thickness{nullptr};
    void onOk(wxCommandEvent&);
};


