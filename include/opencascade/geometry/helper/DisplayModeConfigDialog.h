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
#include "widgets/FlatNotebook.h"
#include <wx/scrolwin.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/GeometryRenderContext.h"
#include "config/RenderingConfig.h"
#include "widgets/FramelessModalPopup.h"
#include "opencascade/geometry/helper/DisplayModePreviewCanvas.h"
#include "widgets/FlatButton.h"
#include "widgets/FlatCheckBox.h"
#include "widgets/FlatComboBox.h"
#include "widgets/FlatSlider.h"
#include <map>
#include <functional>

class FlatProgressBar;
class wxStaticText;

class DisplayModeConfigDialog : public FramelessModalPopup
{
public:
    // Progress callback type: (current, total, message) -> void
    using ProgressCallback = std::function<void(int current, int total, const wxString& message)>;
    
    DisplayModeConfigDialog(wxWindow* parent, RenderingConfig::DisplayMode initialMode = RenderingConfig::DisplayMode::Solid,
                           const std::map<RenderingConfig::DisplayMode, DisplayModeConfig>& preloadedConfigs = std::map<RenderingConfig::DisplayMode, DisplayModeConfig>(),
                           ProgressCallback progressCallback = nullptr);
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
    
    // Layout helper functions
    FlatComboBox* createComboBox(wxWindow* parent, const wxString& label, const std::vector<wxString>& items, int defaultSelection = 0);
    FlatButton* createColorButton(wxWindow* parent, const wxString& label);
    wxBoxSizer* createSliderWithLabel(wxWindow* parent, FlatSlider*& slider, wxStaticText*& label, 
                                      int value, int minValue, int maxValue, const wxString& format = "%.1f");
    void addGridRow(wxFlexGridSizer* grid, wxWindow* parent, const wxString& label, wxWindow* control);
    void addGridRow(wxFlexGridSizer* grid, wxWindow* parent, const wxString& label, wxSizer* sizer);
    void addCheckBox(wxSizer* sizer, FlatCheckBox* checkbox, int flags = wxLEFT | wxRIGHT, int border = 3);
    
    void loadConfigForMode(RenderingConfig::DisplayMode mode);
    void saveConfigForMode(RenderingConfig::DisplayMode mode);
    void updateConfigFromControls(RenderingConfig::DisplayMode mode);
    void enforceModeRequirements(RenderingConfig::DisplayMode mode);
    
    // Synchronization helper methods
    void updateDrawStyleFromCheckboxes(RenderingConfig::DisplayMode mode);
    void updateCheckboxesFromDrawStyle(RenderingConfig::DisplayMode mode);
    void syncOriginalEdgeEnabled(RenderingConfig::DisplayMode mode);
    void updatePreviewForMode(RenderingConfig::DisplayMode mode);
    
    RenderingConfig::DisplayMode getModeFromPageIndex(int pageIndex) const;
    int getPageIndexFromMode(RenderingConfig::DisplayMode mode) const;
    
    wxColour quantityColorToWxColour(const Quantity_Color& color) const;
    Quantity_Color wxColourToQuantityColor(const wxColour& color) const;
    void updateColorButton(FlatButton* button, const wxColour& color);
    
    void onColorButtonClicked(wxCommandEvent& event);
    void onApply(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);
    
    wxColour getColorFromDialog(const wxColour& initialColor);
    
    FlatNotebook* m_notebook;
    
    struct ModeControls {
        wxPanel* page;
        
        wxStaticBox* nodeRequirementsBox;
        wxStaticBox* renderingPropertiesBox;
        wxStaticBox* edgeConfigBox;
        wxStaticBox* postProcessingBox;
        
        FlatCheckBox* requireSurface;
        FlatCheckBox* requireOriginalEdges;
        FlatCheckBox* requireMeshEdges;
        FlatCheckBox* requirePoints;

        FlatComboBox* drawStyle;
        
        FlatComboBox* lightModel;
        FlatCheckBox* textureEnabled;
        FlatComboBox* blendMode;
        
        FlatCheckBox* materialOverrideEnabled;
        FlatButton* materialAmbientColor;
        FlatButton* materialDiffuseColor;
        FlatButton* materialSpecularColor;
        FlatButton* materialEmissiveColor;
        FlatSlider* materialShininess;
        wxStaticText* materialShininessLabel;
        FlatSlider* materialTransparency;
        wxStaticText* materialTransparencyLabel;
        
        FlatCheckBox* originalEdgeEnabled;
        FlatButton* originalEdgeColor;
        FlatSlider* originalEdgeWidth;
        wxStaticText* originalEdgeWidthLabel;
        
        wxStaticLine* meshEdgeSeparator;
        wxStaticText* meshEdgeLabel;
        FlatCheckBox* meshEdgeEnabled;
        FlatButton* meshEdgeColor;
        FlatSlider* meshEdgeWidth;
        wxStaticText* meshEdgeWidthLabel;
        FlatCheckBox* meshEdgeUseEffectiveColor;

        wxStaticLine* silhouetteEdgeSeparator;
        wxStaticText* silhouetteEdgeLabel;
        FlatCheckBox* silhouetteEdgeEnabled;
        FlatButton* silhouetteEdgeColor;
        FlatSlider* silhouetteEdgeWidth;
        wxStaticText* silhouetteEdgeWidthLabel;
        
        FlatCheckBox* polygonOffsetEnabled;
        FlatSlider* polygonOffsetFactor;
        wxStaticText* polygonOffsetFactorLabel;
        FlatSlider* polygonOffsetUnits;
        wxStaticText* polygonOffsetUnitsLabel;
        
        DisplayModeConfig config;
    };
    
    std::map<RenderingConfig::DisplayMode, ModeControls> m_modeControls;
    RenderingConfig::DisplayMode m_customModeKey;
    GeometryRenderContext m_defaultContext;
    
    FlatButton* m_applyButton;
    FlatButton* m_okButton;
    FlatButton* m_cancelButton;
    FlatButton* m_resetButton;
    
    wxSplitterWindow* m_splitter;
    DisplayModePreviewCanvas* m_previewCanvas;

    ProgressCallback m_progressCallback;

    // Debouncing for slider controls
    wxTimer m_previewUpdateTimer;
    RenderingConfig::DisplayMode m_pendingPreviewMode;
    bool m_previewUpdatePending;
    
    void updatePreview();
    void schedulePreviewUpdate(RenderingConfig::DisplayMode mode, int delayMs = 150);
    void onPreviewUpdateTimer(wxTimerEvent& event);
};

