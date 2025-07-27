#pragma once

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/colour.h>
#include <wx/button.h>
#include <memory>

class PreviewCanvas;

class RenderPreviewDialog : public wxDialog
{
public:
    RenderPreviewDialog(wxWindow* parent);

private:
    void createUI();
    void createLightingPanel(wxNotebook* notebook);
    void createMaterialPanel(wxNotebook* notebook);
    void createTexturePanel(wxNotebook* notebook);
    void createAntiAliasingPanel(wxNotebook* notebook);
    
    void onLightingChanged(wxCommandEvent& event);
    void onMaterialChanged(wxCommandEvent& event);
    void onTextureChanged(wxCommandEvent& event);
    void onAntiAliasingChanged(wxCommandEvent& event);
    void onLightColorButton(wxCommandEvent& event);
    
    void OnReset(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    
    void saveConfiguration();
    void loadConfiguration();
    void resetToDefaults();

    // UI Components
    PreviewCanvas* m_renderCanvas;
    wxNotebook* m_configNotebook;
    
    // Lighting controls
    wxSlider* m_ambientLightSlider;
    wxSlider* m_diffuseLightSlider;
    wxSlider* m_specularLightSlider;
    wxButton* m_lightColorButton;
    wxSlider* m_lightIntensitySlider;
    wxColour m_lightColor;
    
    // Material controls
    wxSlider* m_ambientMaterialSlider;
    wxSlider* m_diffuseMaterialSlider;
    wxSlider* m_specularMaterialSlider;
    wxSlider* m_shininessSlider;
    wxSlider* m_transparencySlider;
    
    // Texture controls
    wxChoice* m_textureModeChoice;
    wxSlider* m_textureScaleSlider;
    wxCheckBox* m_enableTextureCheckBox;
    
    // Anti-aliasing controls
    wxChoice* m_antiAliasingChoice;
    wxSlider* m_msaaSamplesSlider;
    wxCheckBox* m_enableFXAACheckBox;

    wxDECLARE_EVENT_TABLE();
};
