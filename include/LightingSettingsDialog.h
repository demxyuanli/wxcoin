#pragma once

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/LightingConfig.h"

class LightingSettingsDialog : public wxDialog
{
public:
    LightingSettingsDialog(wxWindow* parent, wxWindowID id = wxID_ANY, 
                          const wxString& title = "Lighting Settings",
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize);
    ~LightingSettingsDialog();

private:
    // UI components
    wxNotebook* m_notebook;
    wxPanel* m_environmentPage;
    wxPanel* m_lightsPage;
    wxPanel* m_presetsPage;
    
    // Environment controls
    wxButton* m_ambientColorButton;
    wxSlider* m_ambientIntensitySlider;
    wxStaticText* m_ambientIntensityLabel;
    
    // Lights list
    wxListBox* m_lightsList;
    wxButton* m_addLightButton;
    wxButton* m_removeLightButton;
    
    // Light properties
    wxTextCtrl* m_lightNameText;
    wxChoice* m_lightTypeChoice;
    wxCheckBox* m_lightEnabledCheck;
    
    // Position and direction
    wxSpinCtrlDouble* m_positionXSpin;
    wxSpinCtrlDouble* m_positionYSpin;
    wxSpinCtrlDouble* m_positionZSpin;
    wxSpinCtrlDouble* m_directionXSpin;
    wxSpinCtrlDouble* m_directionYSpin;
    wxSpinCtrlDouble* m_directionZSpin;
    
    // Color and intensity
    wxButton* m_lightColorButton;
    wxSlider* m_lightIntensitySlider;
    wxStaticText* m_lightIntensityLabel;
    
    // Spot light specific
    wxSlider* m_spotAngleSlider;
    wxStaticText* m_spotAngleLabel;
    wxSlider* m_spotExponentSlider;
    wxStaticText* m_spotExponentLabel;
    
    // Attenuation
    wxSpinCtrlDouble* m_constantAttenSpin;
    wxSpinCtrlDouble* m_linearAttenSpin;
    wxSpinCtrlDouble* m_quadraticAttenSpin;
    
    // Presets
    wxChoice* m_presetChoice;
    wxButton* m_applyPresetButton;
    wxStaticText* m_currentPresetLabel;
    
    // Action buttons
    wxBoxSizer* m_buttonSizer;
    wxButton* m_applyButton;
    wxButton* m_okButton;
    wxButton* m_cancelButton;
    wxButton* m_resetButton;
    
    // Data
    LightingConfig& m_config;
    int m_currentLightIndex;
    std::vector<LightSettings> m_tempLights;
    LightSettings m_tempEnvironment;
    
    // Methods
    void createEnvironmentPage();
    void createLightsPage();
    void createPresetsPage();
    void createButtons();
    
    void updateLightList();
    void updateLightProperties();
    void updateEnvironmentProperties();
    void updateColorButton(wxButton* button, const Quantity_Color& color);
    Quantity_Color wxColourToQuantityColor(const wxColour& wxColor) const;
    wxColour quantityColorToWxColour(const Quantity_Color& color) const;
    
    void onLightSelected(wxCommandEvent& event);
    void onAddLight(wxCommandEvent& event);
    void onRemoveLight(wxCommandEvent& event);
    void onLightPropertyChanged(wxCommandEvent& event);
    void onEnvironmentPropertyChanged(wxCommandEvent& event);
    void onColorButtonClicked(wxCommandEvent& event);
    void onPresetSelected(wxCommandEvent& event);
    void onApplyPreset(wxCommandEvent& event);
    void onStudioPreset(wxCommandEvent& event);
    void onOutdoorPreset(wxCommandEvent& event);
    void onDramaticPreset(wxCommandEvent& event);
    void onWarmPreset(wxCommandEvent& event);
    void onCoolPreset(wxCommandEvent& event);
    void onMinimalPreset(wxCommandEvent& event);
    void onApply(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);
    
    void applySettings();
    void saveSettings();
    void applyPresetAndUpdate(const std::string& presetName, const std::string& description);
    
    DECLARE_EVENT_TABLE()
}; 