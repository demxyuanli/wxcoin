#pragma once

#include <wx/wx.h>
#include <wx/clrpicker.h>
#include "viewer/ImageOutlinePass.h"

class OutlinePreviewCanvas;

// Extended outline parameters including colors
struct ExtendedOutlineParams : public ImageOutlineParams {
	wxColour backgroundColor{ 51, 51, 51 };  // Dark gray
	wxColour outlineColor{ 0, 0, 0 };        // Black
	wxColour hoverColor{ 255, 128, 0 };      // Orange
	wxColour geometryColor{ 200, 200, 200 }; // Light gray
};

class OutlineSettingsDialog : public wxDialog {
public:
	OutlineSettingsDialog(wxWindow* parent, const ImageOutlineParams& params);
	ImageOutlineParams getParams() const { return m_params; }
	ExtendedOutlineParams getExtendedParams() const { return m_extParams; }

private:
	ImageOutlineParams m_params;
	ExtendedOutlineParams m_extParams;
	
	// UI controls - Basic parameters
	wxSlider* m_depthW{ nullptr };
	wxSlider* m_normalW{ nullptr };
	wxSlider* m_depthTh{ nullptr };
	wxSlider* m_normalTh{ nullptr };
	wxSlider* m_intensity{ nullptr };
	wxSlider* m_thickness{ nullptr };
	
	// UI controls - Color pickers
	wxColourPickerCtrl* m_bgColorPicker{ nullptr };
	wxColourPickerCtrl* m_outlineColorPicker{ nullptr };
	wxColourPickerCtrl* m_hoverColorPicker{ nullptr };
	wxColourPickerCtrl* m_geomColorPicker{ nullptr };
	
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
	void onColorChange(wxColourPickerEvent& event);
	void onOk(wxCommandEvent&);
	void updatePreview();
	void updateLabels();
};
