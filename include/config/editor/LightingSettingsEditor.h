#ifndef LIGHTING_SETTINGS_EDITOR_H
#define LIGHTING_SETTINGS_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/listbox.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/LightingConfig.h"
#include <vector>

struct LightSettings;

class LightingSettingsEditor : public ConfigCategoryEditor {
public:
    LightingSettingsEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~LightingSettingsEditor();
    
    virtual void loadConfig() override;
    virtual void saveConfig() override;
    virtual void resetConfig() override;
    
private:
    void createUI();
    void createEnvironmentPage();
    void createLightsPage();
    void createPresetsPage();
    
    void updateEnvironmentProperties();
    void updateLightList();
    void updateLightProperties();
    void updateColorButton(wxButton* button, const Quantity_Color& color);
    Quantity_Color wxColourToQuantityColor(const wxColour& wxColor) const;
    wxColour quantityColorToWxColour(const Quantity_Color& color) const;
    
    void onLightSelected(wxCommandEvent& event);
    void onAddLight(wxCommandEvent& event);
    void onRemoveLight(wxCommandEvent& event);
    void onLightPropertyChanged(wxCommandEvent& event);
    void onEnvironmentPropertyChanged(wxCommandEvent& event);
    void onColorButtonClicked(wxCommandEvent& event);
    void onStudioPreset(wxCommandEvent& event);
    void onOutdoorPreset(wxCommandEvent& event);
    void onDramaticPreset(wxCommandEvent& event);
    void onWarmPreset(wxCommandEvent& event);
    void onCoolPreset(wxCommandEvent& event);
    void onMinimalPreset(wxCommandEvent& event);
    void onFreeCADPreset(wxCommandEvent& event);
    void onNavcubePreset(wxCommandEvent& event);
    
    void applySettings();
    void applyPresetAndUpdate(const std::string& presetName, const std::string& description);
    
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
    wxSpinCtrlDouble* m_positionXSpin;
    wxSpinCtrlDouble* m_positionYSpin;
    wxSpinCtrlDouble* m_positionZSpin;
    wxSpinCtrlDouble* m_directionXSpin;
    wxSpinCtrlDouble* m_directionYSpin;
    wxSpinCtrlDouble* m_directionZSpin;
    wxButton* m_lightColorButton;
    wxSlider* m_lightIntensitySlider;
    wxStaticText* m_lightIntensityLabel;
    wxStaticText* m_currentPresetLabel;
    
    // Data
    LightingConfig& m_config;
    int m_currentLightIndex;
    std::vector<LightSettings> m_tempLights;
};

#endif // LIGHTING_SETTINGS_EDITOR_H

