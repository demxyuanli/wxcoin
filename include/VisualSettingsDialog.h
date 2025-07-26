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
    
    // Display controls
    wxCheckBox* m_showNormalsCheckBox;
    wxCheckBox* m_showEdgesCheckBox;
    wxCheckBox* m_showWireframeCheckBox;
    wxCheckBox* m_showSilhouetteCheckBox;
    wxCheckBox* m_showFeatureEdgesCheckBox;
    wxCheckBox* m_showMeshEdgesCheckBox;
    wxCheckBox* m_showOriginalEdgesCheckBox;
    wxCheckBox* m_showFaceNormalsCheckBox;
    
    // Subdivision controls
    wxCheckBox* m_subdivisionEnabledCheckBox;
    wxSpinCtrl* m_subdivisionLevelsSpinCtrl;
    
    // Edge settings controls
    wxChoice* m_edgeTypeChoice;
    wxTextCtrl* m_edgeWidthTextCtrl;
    wxTextCtrl* m_edgeColorRTextCtrl;
    wxTextCtrl* m_edgeColorGTextCtrl;
    wxTextCtrl* m_edgeColorBTextCtrl;
    wxCheckBox* m_edgeEnabledCheckBox;
    
    // Basic info display (read-only)
    wxStaticText* m_geometryTypeLabel;
    wxStaticText* m_positionLabel;
    wxStaticText* m_dimensionsLabel;
    
    void CreateBasicInfoPanel();
    void CreateMaterialPanel();
    void CreateTexturePanel();
    void CreateRenderingPanel();
    void CreateDisplayPanel();
    void CreateSubdivisionPanel();
    void CreateEdgeSettingsPanel();
    
    void LoadAdvancedParametersFromControls();
    void SaveAdvancedParametersToControls();
    void UpdateBasicInfoDisplay();
    
    void OnBrowseTexture(wxCommandEvent& event);
    void OnOkButton(wxCommandEvent& event);
    void OnCancelButton(wxCommandEvent& event);
    void OnApplyButton(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
}; 