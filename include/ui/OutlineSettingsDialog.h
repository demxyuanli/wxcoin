#pragma once

#include <wx/wx.h>
#include "viewer/ImageOutlinePass.h"

class OutlinePreviewCanvas;

class OutlineSettingsDialog : public wxDialog {
public:
	OutlineSettingsDialog(wxWindow* parent, const ImageOutlineParams& params);
	ImageOutlineParams getParams() const { return m_params; }

private:
	ImageOutlineParams m_params;
	
	// UI controls
	wxSlider* m_depthW{ nullptr };
	wxSlider* m_normalW{ nullptr };
	wxSlider* m_depthTh{ nullptr };
	wxSlider* m_normalTh{ nullptr };
	wxSlider* m_intensity{ nullptr };
	wxSlider* m_thickness{ nullptr };
	
	// Preview
	OutlinePreviewCanvas* m_previewCanvas{ nullptr };
	
	// Value labels
	wxStaticText* m_depthWLabel{ nullptr };
	wxStaticText* m_normalWLabel{ nullptr };
	wxStaticText* m_depthThLabel{ nullptr };
	wxStaticText* m_normalThLabel{ nullptr };
	wxStaticText* m_intensityLabel{ nullptr };
	wxStaticText* m_thicknessLabel{ nullptr };
	
	void onSliderChange(wxCommandEvent& event);
	void onOk(wxCommandEvent&);
	void updatePreview();
	void updateLabels();
};
