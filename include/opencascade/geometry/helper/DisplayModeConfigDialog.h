#pragma once

#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/colour.h>
#include <wx/colordlg.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/splitter.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/GeometryRenderContext.h"
#include "config/RenderingConfig.h"
#include "widgets/FramelessModalPopup.h"
#include "opencascade/geometry/helper/DisplayModePreviewCanvas.h"
#include <map>

class DisplayModeConfigDialog : public FramelessModalPopup
{
public:
    DisplayModeConfigDialog(wxWindow* parent);
    virtual ~DisplayModeConfigDialog();

    DisplayModeConfig getConfig(RenderingConfig::DisplayMode mode) const;

private:
    void createControls();
    void layoutControls();
    void bindEvents();
    void updateControls();
    void applyThemeAndFonts();
    void updateModeVisibility(RenderingConfig::DisplayMode mode);
    
    void createModePage(RenderingConfig::DisplayMode mode);
    void createCustomModePage();
    void createNodeRequirementsPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode);
    void createRenderingPropertiesPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode);
    void createEdgeConfigPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode);
    void createPostProcessingPanel(wxPanel* parent, wxSizer* sizer, RenderingConfig::DisplayMode mode);
    
    void loadConfigForMode(RenderingConfig::DisplayMode mode);
    void saveConfigForMode(RenderingConfig::DisplayMode mode);
    void updateConfigFromControls(RenderingConfig::DisplayMode mode);
    
    RenderingConfig::DisplayMode getModeFromPageIndex(int pageIndex) const;
    
    wxColour quantityColorToWxColour(const Quantity_Color& color) const;
    Quantity_Color wxColourToQuantityColor(const wxColour& color) const;
    void updateColorButton(wxButton* button, const wxColour& color);
    
    void onColorButtonClicked(wxCommandEvent& event);
    void onApply(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);
    
    wxColour getColorFromDialog(const wxColour& initialColor);
    
    wxNotebook* m_notebook;
    
    struct ModeControls {
        wxPanel* page;
        
        wxStaticBox* nodeRequirementsBox;
        wxStaticBox* renderingPropertiesBox;
        wxStaticBox* edgeConfigBox;
        wxStaticBox* postProcessingBox;
        
        wxCheckBox* requireSurface;
        wxCheckBox* requireOriginalEdges;
        wxCheckBox* requireMeshEdges;
        wxCheckBox* requirePoints;
        
        wxChoice* lightModel;
        wxCheckBox* textureEnabled;
        wxChoice* blendMode;
        
        wxCheckBox* materialOverrideEnabled;
        wxButton* materialAmbientColor;
        wxButton* materialDiffuseColor;
        wxButton* materialSpecularColor;
        wxButton* materialEmissiveColor;
        wxSlider* materialShininess;
        wxStaticText* materialShininessLabel;
        wxSlider* materialTransparency;
        wxStaticText* materialTransparencyLabel;
        
        wxCheckBox* originalEdgeEnabled;
        wxButton* originalEdgeColor;
        wxSlider* originalEdgeWidth;
        wxStaticText* originalEdgeWidthLabel;
        
        wxStaticLine* meshEdgeSeparator;
        wxStaticText* meshEdgeLabel;
        wxCheckBox* meshEdgeEnabled;
        wxButton* meshEdgeColor;
        wxSlider* meshEdgeWidth;
        wxStaticText* meshEdgeWidthLabel;
        wxCheckBox* meshEdgeUseEffectiveColor;
        
        wxCheckBox* polygonOffsetEnabled;
        wxSlider* polygonOffsetFactor;
        wxStaticText* polygonOffsetFactorLabel;
        wxSlider* polygonOffsetUnits;
        wxStaticText* polygonOffsetUnitsLabel;
        
        DisplayModeConfig config;
    };
    
    std::map<RenderingConfig::DisplayMode, ModeControls> m_modeControls;
    RenderingConfig::DisplayMode m_customModeKey;
    GeometryRenderContext m_defaultContext;
    
    wxButton* m_applyButton;
    wxButton* m_okButton;
    wxButton* m_cancelButton;
    wxButton* m_resetButton;
    
    wxSplitterWindow* m_splitter;
    DisplayModePreviewCanvas* m_previewCanvas;
    
    void updatePreview();
};

