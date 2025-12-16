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
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/splitter.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/GeometryRenderContext.h"
#include "config/RenderingConfig.h"
#include "widgets/FramelessModalPopup.h"
#include "ui/DisplayModePreviewCanvas.h"
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
        
        wxCheckBox* requireSurface;
        wxCheckBox* requireOriginalEdges;
        wxCheckBox* requireMeshEdges;
        wxCheckBox* requirePoints;
        wxCheckBox* surfaceWithPoints;
        
        wxChoice* lightModel;
        wxCheckBox* textureEnabled;
        wxChoice* blendMode;
        
        wxCheckBox* materialOverrideEnabled;
        wxButton* materialAmbientColor;
        wxButton* materialDiffuseColor;
        wxButton* materialSpecularColor;
        wxButton* materialEmissiveColor;
        wxSpinCtrlDouble* materialShininess;
        wxSpinCtrlDouble* materialTransparency;
        
        wxCheckBox* originalEdgeEnabled;
        wxButton* originalEdgeColor;
        wxSpinCtrlDouble* originalEdgeWidth;
        
        wxCheckBox* meshEdgeEnabled;
        wxButton* meshEdgeColor;
        wxSpinCtrlDouble* meshEdgeWidth;
        wxCheckBox* meshEdgeUseEffectiveColor;
        
        wxCheckBox* polygonOffsetEnabled;
        wxSpinCtrlDouble* polygonOffsetFactor;
        wxSpinCtrlDouble* polygonOffsetUnits;
        
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

