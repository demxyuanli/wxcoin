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
#include <functional>
#include "renderpreview/RenderLightSettings.h"

class RenderPreviewDialog;

class LightingPanel : public wxPanel
{
public:
    LightingPanel(wxWindow* parent, RenderPreviewDialog* dialog);
    ~LightingPanel();

    std::vector<RenderLightSettings> getLights() const;
    void setLights(const std::vector<RenderLightSettings>& lights);

    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    
    // Set callback for parameter changes
    void setParameterChangeCallback(std::function<void()> callback) { m_parameterChangeCallback = callback; }
    
    // Apply fonts to all controls
    void applyFonts();

private:
    void createUI();
    wxSizer* createLightingTab(wxWindow* parent);
    wxSizer* createLightPresetsTab(wxWindow* parent);
    void bindEvents();
    void updateLightList();
    void updateControlStates();
    void notifyParameterChanged();

    void onLightSelected(wxCommandEvent& event);
    void onAddLight(wxCommandEvent& event);
    void onRemoveLight(wxCommandEvent& event);
    void onLightPropertyChanged(wxCommandEvent& event);
    void onLightPropertyChangedSpin(wxSpinDoubleEvent& event);
    void onLightingChanged(wxCommandEvent& event);

    void onStudioPreset(wxCommandEvent& event);
    void onOutdoorPreset(wxCommandEvent& event);
    void onDramaticPreset(wxCommandEvent& event);
    void onWarmPreset(wxCommandEvent& event);
    void onCoolPreset(wxCommandEvent& event);
    void onMinimalPreset(wxCommandEvent& event);
    void onFreeCADPreset(wxCommandEvent& event);
    void onNavcubePreset(wxCommandEvent& event);

    RenderPreviewDialog* m_parentDialog;
    std::vector<RenderLightSettings> m_lights;
    int m_currentLightIndex;
    std::function<void()> m_parameterChangeCallback;

    wxBoxSizer* m_lightListSizer;
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

    wxButton* m_studioButton;
    wxButton* m_outdoorButton;
    wxButton* m_dramaticButton;
    wxButton* m_warmButton;
    wxButton* m_coolButton;
    wxButton* m_minimalButton;
    wxButton* m_freeCADButton;
    wxButton* m_navcubeButton;
    wxStaticText* m_currentPresetLabel;
    
    wxNotebook* m_notebook;

    DECLARE_EVENT_TABLE()
};
