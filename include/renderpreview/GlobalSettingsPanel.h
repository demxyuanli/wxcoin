#pragma once

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/colordlg.h>
#include <vector>
#include "renderpreview/RenderLightSettings.h"

class RenderPreviewDialog;

class GlobalSettingsPanel : public wxPanel
{
public:
    GlobalSettingsPanel(wxWindow* parent, RenderPreviewDialog* dialog, wxWindowID id = wxID_ANY);
    ~GlobalSettingsPanel();

    // Lighting methods
    std::vector<RenderLightSettings> getLights() const;
    void setLights(const std::vector<RenderLightSettings>& lights);
    void addLight(const RenderLightSettings& light);
    void removeLight(int index);
    void updateLight(int index, const RenderLightSettings& light);
    
    // Anti-aliasing methods
    int getAntiAliasingMethod() const;
    int getMSAASamples() const;
    bool isFXAAEnabled() const;
    
    // Rendering mode methods
    int getRenderingMode() const;
    
    // Configuration methods
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

private:
    void createUI();
    wxSizer* createLightingTab(wxWindow* parent);
    wxSizer* createLightPresetsTab(wxWindow* parent);
    wxSizer* createAntiAliasingTab(wxWindow* parent);
    wxSizer* createRenderingModeTab(wxWindow* parent);
    void bindEvents();
    void updateLightList();
    
    // Event handlers
    void onLightSelected(wxCommandEvent& event);
    void onAddLight(wxCommandEvent& event);
    void onRemoveLight(wxCommandEvent& event);
    void onLightPropertyChanged(wxCommandEvent& event);
    void onLightPropertyChangedSpin(wxSpinDoubleEvent& event);
    void onLightingChanged(wxCommandEvent& event);
    void onAntiAliasingChanged(wxCommandEvent& event);
    void onRenderingModeChanged(wxCommandEvent& event);
    
    // Light preset event handlers
    void onStudioPreset(wxCommandEvent& event);
    void onOutdoorPreset(wxCommandEvent& event);
    void onDramaticPreset(wxCommandEvent& event);
    void onWarmPreset(wxCommandEvent& event);
    void onCoolPreset(wxCommandEvent& event);
    void onMinimalPreset(wxCommandEvent& event);
    void onFreeCADPreset(wxCommandEvent& event);
    void onNavcubePreset(wxCommandEvent& event);
    
    // UI Components for Lighting
    wxListBox* m_lightListBox;
    wxButton* m_addLightButton;
    wxButton* m_removeLightButton;
    wxTextCtrl* m_lightNameText;
    wxChoice* m_lightTypeChoice;
    wxSpinCtrlDouble* m_positionXSpin;
    wxSpinCtrlDouble* m_positionYSpin;
    wxSpinCtrlDouble* m_positionZSpin;
    wxSpinCtrlDouble* m_directionXSpin;
    wxSpinCtrlDouble* m_directionYSpin;
    wxSpinCtrlDouble* m_directionZSpin;
    wxSpinCtrlDouble* m_intensitySpin;
    wxButton* m_lightColorButton;
    wxCheckBox* m_lightEnabledCheckBox;
    
    // UI Components for Light Presets
    wxButton* m_studioButton;
    wxButton* m_outdoorButton;
    wxButton* m_dramaticButton;
    wxButton* m_warmButton;
    wxButton* m_coolButton;
    wxButton* m_minimalButton;
    wxButton* m_freeCADButton;
    wxButton* m_navcubeButton;
    wxStaticText* m_currentPresetLabel;
    
    // UI Components for Anti-aliasing
    wxChoice* m_antiAliasingChoice;
    wxSlider* m_msaaSamplesSlider;
    wxCheckBox* m_fxaaCheckBox;
    
    // UI Components for Rendering Mode
    wxChoice* m_renderingModeChoice;
    
    // Internal data
    std::vector<RenderLightSettings> m_lights;
    int m_currentLightIndex;
    RenderPreviewDialog* m_parentDialog;
    
    // Tab notebook
    wxNotebook* m_notebook;
    
    DECLARE_EVENT_TABLE()
}; 