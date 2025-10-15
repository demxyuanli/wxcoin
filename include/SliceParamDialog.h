#pragma once

#include <wx/frame.h>
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/panel.h>

class OCCViewer;

class SliceParamDialog : public wxFrame {
public:
	explicit SliceParamDialog(wxWindow* parent, OCCViewer* viewer);

	// Show and position relative to canvas
	void ShowAtCanvasTopLeft();
	void UpdatePosition();

	// Get parameter values
	wxColour getPlaneColor() const;
	double getPlaneOpacity() const;
	bool getShowSectionContours() const;
	int getSliceDirection() const;
	double getSliceOffset() const;

	// Set current values
	void setPlaneColor(const wxColour& color);
	void setPlaneOpacity(double opacity);
	void setShowSectionContours(bool show);
	void setSliceDirection(int direction);
	void setSliceOffset(double offset);
	
	// Mouse mode control
	bool isDragMode() const;

private:
	void OnOpacitySliderChanged(wxCommandEvent& event);
	void OnDirectionChanged(wxCommandEvent& event);
	void OnOffsetChanged(wxSpinDoubleEvent& event);
	void OnColorChanged(wxColourPickerEvent& event);
	void OnContoursChanged(wxCommandEvent& event);
	void OnToggleSizeButton(wxCommandEvent& event);
	void OnParentResize(wxSizeEvent& event);
	void OnMouseModeToggle(wxCommandEvent& event);

	// Auto-apply changes to viewer
	void ApplyChanges();

	OCCViewer* m_viewer{ nullptr };
	wxPanel* m_contentPanel{ nullptr };
	wxButton* m_toggleSizeBtn{ nullptr };
	wxPanel* m_mainContent{ nullptr };
	wxColourPickerCtrl* m_colorPicker{ nullptr };
	wxSlider* m_opacitySlider{ nullptr };
	wxStaticText* m_opacityValue{ nullptr };
	wxCheckBox* m_showContours{ nullptr };
	wxChoice* m_direction{ nullptr };
	wxSpinCtrlDouble* m_offsetCtrl{ nullptr };
	wxButton* m_mouseModeBtn{ nullptr };

	bool m_isMinimized{ false };
	bool m_isDragMode{ true };  // Default to drag mode

	wxDECLARE_EVENT_TABLE();
};

