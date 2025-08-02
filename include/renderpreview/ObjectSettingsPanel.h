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

class ObjectSettingsPanel : public wxPanel
{
public:
    ObjectSettingsPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~ObjectSettingsPanel();

    // Object material settings
    float getAmbient() const;
    float getDiffuse() const;
    float getSpecular() const;
    float getShininess() const;
    float getTransparency() const;
    
    // Object texture settings
    bool isTextureEnabled() const;
    int getTextureMode() const;
    float getTextureScale() const;
    
    // Configuration methods
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

private:
    void createUI();
    wxSizer* createMaterialTab(wxWindow* parent);
    wxSizer* createTextureTab(wxWindow* parent);
    void bindEvents();
    
    // Event handlers
    void onMaterialChanged(wxCommandEvent& event);
    void onMaterialChangedSpin(wxSpinEvent& event);
    void onTextureChanged(wxCommandEvent& event);
    
    // UI Components for Material
    wxSlider* m_ambientSlider;
    wxSlider* m_diffuseSlider;
    wxSlider* m_specularSlider;
    wxSpinCtrl* m_shininessSpin;
    wxSlider* m_transparencySlider;
    
    // UI Components for Texture
    wxCheckBox* m_textureCheckBox;
    wxChoice* m_textureModeChoice;
    wxSlider* m_textureScaleSlider;
    
    // Tab notebook
    wxNotebook* m_notebook;
    
    DECLARE_EVENT_TABLE()
}; 