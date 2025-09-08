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
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/bitmap.h>
#include <wx/statbmp.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/RenderingConfig.h"

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
    std::string getTextureImagePath() const { return m_textureImagePath; }
    RenderingConfig::TextureMode getTextureMode() const { return m_textureMode; }
    
    // Blend settings
    RenderingConfig::BlendMode getBlendMode() const { return m_blendMode; }
    bool isDepthTestEnabled() const { return m_depthTest; }
    bool isDepthWriteEnabled() const { return m_depthWrite; }
    bool isCullFaceEnabled() const { return m_cullFace; }
    double getAlphaThreshold() const { return m_alphaThreshold; }

    // Shading settings
    bool isSmoothNormalsEnabled() const { return m_smoothNormals; }
    double getWireframeWidth() const { return m_wireframeWidth; }
    double getPointSize() const { return m_pointSize; }
    
    // Normal consistency settings
    bool isNormalConsistencyEnabled() const { return m_enableNormalConsistency; }
    bool isAutoFixNormalsEnabled() const { return m_autoFixNormals; }
    bool isNormalDebugEnabled() const { return m_showNormalDebug; }
    double getNormalConsistencyThreshold() const { return m_normalConsistencyThreshold; }
    
    // Display settings
    RenderingConfig::DisplayMode getDisplayMode() const { return m_displayMode; }
    bool isShowEdgesEnabled() const { return m_showEdges; }
    bool isShowVerticesEnabled() const { return m_showVertices; }
    double getEdgeWidth() const { return m_edgeWidth; }
    double getVertexSize() const { return m_vertexSize; }
    Quantity_Color getEdgeColor() const { return m_edgeColor; }
    Quantity_Color getVertexColor() const { return m_vertexColor; }
    
    // Quality settings
    RenderingConfig::RenderingQuality getRenderingQuality() const { return m_renderingQuality; }
    int getTessellationLevel() const { return m_tessellationLevel; }
    int getAntiAliasingSamples() const { return m_antiAliasingSamples; }
    bool isLODEnabled() const { return m_enableLOD; }
    double getLODDistance() const { return m_lodDistance; }
    
    // Shadow settings
    RenderingConfig::ShadowMode getShadowMode() const { return m_shadowMode; }
    double getShadowIntensity() const { return m_shadowIntensity; }
    double getShadowSoftness() const { return m_shadowSoftness; }
    int getShadowMapSize() const { return m_shadowMapSize; }
    double getShadowBias() const { return m_shadowBias; }
    
    // Lighting model settings
    RenderingConfig::LightingModel getLightingModel() const { return m_lightingModel; }
    double getRoughness() const { return m_roughness; }
    double getMetallic() const { return m_metallic; }
    double getFresnel() const { return m_fresnel; }
    double getSubsurfaceScattering() const { return m_subsurfaceScattering; }

private:
    void createControls();
    void createMaterialPage();
    void createLightingPage();
    void createTexturePage();
    void createBlendPage();
    // Removed createShadingPage - functionality not needed
    void createNormalConsistencyPage();
    void createDisplayPage();
    void createQualityPage();
    void createShadowPage();
    void createLightingModelPage();
    void layoutControls();
    void bindEvents();
    void updateControls();
    
    // Material events
    void onMaterialPresetChoice(wxCommandEvent& event);
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
    void onTextureImageButton(wxCommandEvent& event);
    void onTextureModeChoice(wxCommandEvent& event);
    
    // Blend events
    void onBlendModeChoice(wxCommandEvent& event);
    void onDepthTestCheckbox(wxCommandEvent& event);
    void onDepthWriteCheckbox(wxCommandEvent& event);
    void onCullFaceCheckbox(wxCommandEvent& event);
    void onAlphaThresholdSlider(wxCommandEvent& event);
    
    // Shading events
    void onSmoothNormalsCheckbox(wxCommandEvent& event);
    void onWireframeWidthSlider(wxCommandEvent& event);
    void onPointSizeSlider(wxCommandEvent& event);
    
    // Normal consistency events
    void onEnableNormalConsistencyCheckbox(wxCommandEvent& event);
    void onAutoFixNormalsCheckbox(wxCommandEvent& event);
    void onShowNormalDebugCheckbox(wxCommandEvent& event);
    void onNormalConsistencyThresholdSlider(wxCommandEvent& event);
    
    // Display events
    void onDisplayModeChoice(wxCommandEvent& event);
    void onShowEdgesCheckbox(wxCommandEvent& event);
    void onShowVerticesCheckbox(wxCommandEvent& event);
    void onEdgeWidthSlider(wxCommandEvent& event);
    void onVertexSizeSlider(wxCommandEvent& event);
    void onEdgeColorButton(wxCommandEvent& event);
    void onVertexColorButton(wxCommandEvent& event);
    
    // Quality events
    void onRenderingQualityChoice(wxCommandEvent& event);
    void onTessellationLevelSlider(wxCommandEvent& event);
    void onAntiAliasingSamplesSlider(wxCommandEvent& event);
    void onEnableLODCheckbox(wxCommandEvent& event);
    void onLODDistanceSlider(wxCommandEvent& event);
    
    // Shadow events
    void onShadowModeChoice(wxCommandEvent& event);
    void onShadowIntensitySlider(wxCommandEvent& event);
    void onShadowSoftnessSlider(wxCommandEvent& event);
    void onShadowMapSizeSlider(wxCommandEvent& event);
    void onShadowBiasSlider(wxCommandEvent& event);
    
    // Lighting model events
    void onLightingModelChoice(wxCommandEvent& event);
    void onRoughnessSlider(wxCommandEvent& event);
    void onMetallicSlider(wxCommandEvent& event);
    void onFresnelSlider(wxCommandEvent& event);
    void onSubsurfaceScatteringSlider(wxCommandEvent& event);
    
    // Dialog events
    void onApply(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);
    
    // Helper methods
    void applySettings();
    void resetToDefaults();
    void applyMaterialPreset(const std::string& presetName);
    void updateMaterialControls();
    void updateTexturePreview();
    wxColour quantityColorToWxColour(const Quantity_Color& color);
    Quantity_Color wxColourToQuantityColor(const wxColour& color);
    void updateColorButton(wxButton* button, const wxColour& color);
    
    OCCViewer* m_occViewer;
    RenderingEngine* m_renderingEngine;
    
    // UI components
    wxNotebook* m_notebook;
    
    // Material page
    wxPanel* m_materialPage;
    wxChoice* m_materialPresetChoice;
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
    wxButton* m_textureImageButton;
    wxStaticBitmap* m_texturePreview;
    wxStaticText* m_texturePathLabel;
    wxChoice* m_textureModeChoice;
    
    // Blend page
    wxPanel* m_blendPage;
    wxChoice* m_blendModeChoice;
    wxCheckBox* m_depthTestCheckbox;
    wxCheckBox* m_depthWriteCheckbox;
    wxCheckBox* m_cullFaceCheckbox;
    wxSlider* m_alphaThresholdSlider;
    wxStaticText* m_alphaThresholdLabel;
    
    // Shading page
    wxPanel* m_shadingPage;
    wxChoice* m_shadingModeChoice;
    wxCheckBox* m_smoothNormalsCheckbox;
    wxSlider* m_wireframeWidthSlider;
    wxStaticText* m_wireframeWidthLabel;
    wxSlider* m_pointSizeSlider;
    wxStaticText* m_pointSizeLabel;
    
    // Normal consistency controls
    wxCheckBox* m_enableNormalConsistencyCheckbox;
    wxCheckBox* m_autoFixNormalsCheckbox;
    wxCheckBox* m_showNormalDebugCheckbox;
    wxSlider* m_normalConsistencyThresholdSlider;
    wxStaticText* m_normalConsistencyThresholdLabel;
    
    // Display page
    wxPanel* m_displayPage;
    wxChoice* m_displayModeChoice;
    wxCheckBox* m_showEdgesCheckbox;
    wxCheckBox* m_showVerticesCheckbox;
    wxSlider* m_edgeWidthSlider;
    wxStaticText* m_edgeWidthLabel;
    wxSlider* m_vertexSizeSlider;
    wxStaticText* m_vertexSizeLabel;
    wxButton* m_edgeColorButton;
    wxButton* m_vertexColorButton;
    
    // Quality page
    wxPanel* m_qualityPage;
    wxChoice* m_renderingQualityChoice;
    wxSlider* m_tessellationLevelSlider;
    wxStaticText* m_tessellationLevelLabel;
    wxSlider* m_antiAliasingSamplesSlider;
    wxStaticText* m_antiAliasingSamplesLabel;
    wxCheckBox* m_enableLODCheckbox;
    wxSlider* m_lodDistanceSlider;
    wxStaticText* m_lodDistanceLabel;
    
    // Shadow page
    wxPanel* m_shadowPage;
    wxChoice* m_shadowModeChoice;
    wxSlider* m_shadowIntensitySlider;
    wxStaticText* m_shadowIntensityLabel;
    wxSlider* m_shadowSoftnessSlider;
    wxStaticText* m_shadowSoftnessLabel;
    wxSlider* m_shadowMapSizeSlider;
    wxStaticText* m_shadowMapSizeLabel;
    wxSlider* m_shadowBiasSlider;
    wxStaticText* m_shadowBiasLabel;
    
    // Lighting model page
    wxPanel* m_lightingModelPage;
    wxChoice* m_lightingModelChoice;
    wxSlider* m_roughnessSlider;
    wxStaticText* m_roughnessLabel;
    wxSlider* m_metallicSlider;
    wxStaticText* m_metallicLabel;
    wxSlider* m_fresnelSlider;
    wxStaticText* m_fresnelLabel;
    wxSlider* m_subsurfaceScatteringSlider;
    wxStaticText* m_subsurfaceScatteringLabel;
    
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
    std::string m_textureImagePath;
    RenderingConfig::TextureMode m_textureMode;
    
    RenderingConfig::BlendMode m_blendMode;
    bool m_depthTest;
    bool m_depthWrite;
    bool m_cullFace;
    double m_alphaThreshold;
    
    // Shading settings values
    bool m_smoothNormals;
    double m_wireframeWidth;
    double m_pointSize;
    
    // Normal consistency settings
    bool m_enableNormalConsistency;
    bool m_autoFixNormals;
    bool m_showNormalDebug;
    double m_normalConsistencyThreshold;
    
    // Display settings values
    RenderingConfig::DisplayMode m_displayMode;
    bool m_showEdges;
    bool m_showVertices;
    double m_edgeWidth;
    double m_vertexSize;
    Quantity_Color m_edgeColor;
    Quantity_Color m_vertexColor;
    
    // Quality settings values
    RenderingConfig::RenderingQuality m_renderingQuality;
    int m_tessellationLevel;
    int m_antiAliasingSamples;
    bool m_enableLOD;
    double m_lodDistance;
    
    // Shadow settings values
    RenderingConfig::ShadowMode m_shadowMode;
    double m_shadowIntensity;
    double m_shadowSoftness;
    int m_shadowMapSize;
    double m_shadowBias;
    
    // Lighting model settings values
    RenderingConfig::LightingModel m_lightingModel;
    double m_roughness;
    double m_metallic;
    double m_fresnel;
    double m_subsurfaceScattering;
}; 