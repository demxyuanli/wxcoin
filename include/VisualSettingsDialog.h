#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/colour.h>
#include <wx/statbmp.h>
#include <string>
#include <map>
#include <vector>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbColor.h>
#include "config/RenderingConfig.h"
#include "config/LightingConfig.h"
#include "GeometryDialogTypes.h"

class VisualSettingsDialog : public wxDialog {
public:
	VisualSettingsDialog(wxWindow* parent, const wxString& title,
		const BasicGeometryParameters& basicParams = BasicGeometryParameters());
	~VisualSettingsDialog() {}

	void SetBasicParameters(const BasicGeometryParameters& basicParams);
	void SetAdvancedParameters(const AdvancedGeometryParameters& advancedParams);
	AdvancedGeometryParameters GetAdvancedParameters() const;
	BasicGeometryParameters GetBasicParameters() const;

private:
	// Basic parameters (read-only display)
	BasicGeometryParameters m_basicParams;
	AdvancedGeometryParameters m_advancedParams;

	// Material controls
	wxTextCtrl* m_diffuseRTextCtrl;
	wxTextCtrl* m_diffuseGTextCtrl;
	wxTextCtrl* m_diffuseBTextCtrl;
	wxTextCtrl* m_ambientRTextCtrl;
	wxTextCtrl* m_ambientGTextCtrl;
	wxTextCtrl* m_ambientBTextCtrl;
	wxTextCtrl* m_specularRTextCtrl;
	wxTextCtrl* m_specularGTextCtrl;
	wxTextCtrl* m_specularBTextCtrl;
	wxTextCtrl* m_emissiveRTextCtrl;
	wxTextCtrl* m_emissiveGTextCtrl;
	wxTextCtrl* m_emissiveBTextCtrl;
	wxTextCtrl* m_shininessTextCtrl;
	wxTextCtrl* m_transparencyTextCtrl;

	// Texture controls
	wxTextCtrl* m_texturePathTextCtrl;
	wxButton* m_browseTextureButton;
	wxChoice* m_textureModeChoice;
	wxCheckBox* m_textureEnabledCheckBox;

	// Rendering controls
	wxChoice* m_renderingQualityChoice;
	wxChoice* m_blendModeChoice;
	wxChoice* m_lightingModelChoice;
	wxCheckBox* m_backfaceCullingCheckBox;
	wxCheckBox* m_depthTestCheckBox;

	// Subdivision controls
	wxCheckBox* m_subdivisionEnabledCheckBox;
	wxSpinCtrl* m_subdivisionLevelsSpinCtrl;

	// Color picker buttons
	wxButton* m_diffuseColorButton;
	wxButton* m_ambientColorButton;
	wxButton* m_specularColorButton;
	wxButton* m_emissiveColorButton;

	// Texture preview
	wxStaticBitmap* m_texturePreview;

	// Basic info display (read-only)
	wxStaticText* m_geometryTypeLabel;
	wxStaticText* m_positionLabel;
	wxStaticText* m_dimensionsLabel;

	void CreateBasicInfoPanel(wxPanel* panel);
	void CreateMaterialPanel(wxPanel* panel);
	void CreateTexturePanel(wxPanel* panel);
	void CreateRenderingPanel(wxPanel* panel);
	void CreateSubdivisionPanel(wxPanel* panel);

	void LoadAdvancedParametersFromControls();
	void SaveAdvancedParametersToControls();
	void UpdateBasicInfoDisplay();

	// Color picker event handlers
	void OnDiffuseColorButton(wxCommandEvent& event);
	void OnAmbientColorButton(wxCommandEvent& event);
	void OnSpecularColorButton(wxCommandEvent& event);
	void OnEmissiveColorButton(wxCommandEvent& event);

	void OnBrowseTexture(wxCommandEvent& event);
	void OnOkButton(wxCommandEvent& event);
	void OnCancelButton(wxCommandEvent& event);
	void OnApplyButton(wxCommandEvent& event);

	// Helper methods
	void UpdateColorButton(wxButton* button, const Quantity_Color& color);
	void UpdateTexturePreview();
	wxColour QuantityColorToWxColour(const Quantity_Color& color);
	Quantity_Color WxColourToQuantityColor(const wxColour& color);

	DECLARE_EVENT_TABLE()
};