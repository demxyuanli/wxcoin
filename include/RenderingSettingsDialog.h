#pragma once

#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/colour.h>
#include <wx/colordlg.h>
#include <wx/sizer.h>
#include <OpenCASCADE/Quantity_Color.hxx>

class OCCViewer;
class RenderingEngine;

class RenderingSettingsDialog : public wxDialog
{
public:
    RenderingSettingsDialog(wxWindow* parent, OCCViewer* occViewer, RenderingEngine* renderingEngine);
    virtual ~RenderingSettingsDialog();

    // Material settings
    Quantity_Color getMaterialAmbientColor() const { return m_materialAmbientColor; }
    Quantity_Color getMaterialDiffuseColor() const { return m_materialDiffuseColor; }
    Quantity_Color getMaterialSpecularColor() const { return m_materialSpecularColor; }
    double getMaterialShininess() const { return m_materialShininess; }
    double getMaterialTransparency() const { return m_materialTransparency; }
    
    // Lighting settings
    Quantity_Color getLightAmbientColor() const { return m_lightAmbientColor; }
    Quantity_Color getLightDiffuseColor() const { return m_lightDiffuseColor; }
    Quantity_Color getLightSpecularColor() const { return m_lightSpecularColor; }
    double getLightIntensity() const { return m_lightIntensity; }
    double getLightAmbientIntensity() const { return m_lightAmbientIntensity; }
    
    // Texture settings
    Quantity_Color getTextureColor() const { return m_textureColor; }
    double getTextureIntensity() const { return m_textureIntensity; }
    bool isTextureEnabled() const { return m_textureEnabled; }

private:
    void createControls();
    void createMaterialPage();
    void createLightingPage();
    void createTexturePage();
    void layoutControls();
    void bindEvents();
    void updateControls();
    
    // Material events
    void onMaterialAmbientColorButton(wxCommandEvent& event);
    void onMaterialDiffuseColorButton(wxCommandEvent& event);
    void onMaterialSpecularColorButton(wxCommandEvent& event);
    void onMaterialShininessSlider(wxCommandEvent& event);
    void onMaterialTransparencySlider(wxCommandEvent& event);
    
    // Lighting events
    void onLightAmbientColorButton(wxCommandEvent& event);
    void onLightDiffuseColorButton(wxCommandEvent& event);
    void onLightSpecularColorButton(wxCommandEvent& event);
    void onLightIntensitySlider(wxCommandEvent& event);
    void onLightAmbientIntensitySlider(wxCommandEvent& event);
    
    // Texture events
    void onTextureColorButton(wxCommandEvent& event);
    void onTextureIntensitySlider(wxCommandEvent& event);
    void onTextureEnabledCheckbox(wxCommandEvent& event);
    
    // Dialog events
    void onApply(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);
    
    // Helper methods
    void applySettings();
    void resetToDefaults();
    wxColour quantityColorToWxColour(const Quantity_Color& color);
    Quantity_Color wxColourToQuantityColor(const wxColour& color);
    void updateColorButton(wxButton* button, const wxColour& color);
    
    OCCViewer* m_occViewer;
    RenderingEngine* m_renderingEngine;
    
    // UI components
    wxNotebook* m_notebook;
    
    // Material page
    wxPanel* m_materialPage;
    wxButton* m_materialAmbientColorButton;
    wxButton* m_materialDiffuseColorButton;
    wxButton* m_materialSpecularColorButton;
    wxSlider* m_materialShininessSlider;
    wxStaticText* m_materialShininessLabel;
    wxSlider* m_materialTransparencySlider;
    wxStaticText* m_materialTransparencyLabel;
    
    // Lighting page
    wxPanel* m_lightingPage;
    wxButton* m_lightAmbientColorButton;
    wxButton* m_lightDiffuseColorButton;
    wxButton* m_lightSpecularColorButton;
    wxSlider* m_lightIntensitySlider;
    wxStaticText* m_lightIntensityLabel;
    wxSlider* m_lightAmbientIntensitySlider;
    wxStaticText* m_lightAmbientIntensityLabel;
    
    // Texture page
    wxPanel* m_texturePage;
    wxButton* m_textureColorButton;
    wxSlider* m_textureIntensitySlider;
    wxStaticText* m_textureIntensityLabel;
    wxCheckBox* m_textureEnabledCheckbox;
    
    // Dialog buttons
    wxButton* m_applyButton;
    wxButton* m_cancelButton;
    wxButton* m_okButton;
    wxButton* m_resetButton;
    
    // Settings values
    Quantity_Color m_materialAmbientColor;
    Quantity_Color m_materialDiffuseColor;
    Quantity_Color m_materialSpecularColor;
    double m_materialShininess;
    double m_materialTransparency;
    
    Quantity_Color m_lightAmbientColor;
    Quantity_Color m_lightDiffuseColor;
    Quantity_Color m_lightSpecularColor;
    double m_lightIntensity;
    double m_lightAmbientIntensity;
    
    Quantity_Color m_textureColor;
    double m_textureIntensity;
    bool m_textureEnabled;
}; 