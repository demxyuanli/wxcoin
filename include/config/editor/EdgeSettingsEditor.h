#ifndef EDGE_SETTINGS_EDITOR_H
#define EDGE_SETTINGS_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/EdgeSettingsConfig.h"
#include "OCCViewer.h"

struct EdgeSettings;

class EdgeSettingsEditor : public ConfigCategoryEditor {
public:
    EdgeSettingsEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~EdgeSettingsEditor();
    
    virtual void loadConfig() override;
    virtual void saveConfig() override;
    virtual void resetConfig() override;
    
    void setOCCViewer(OCCViewer* viewer) { m_viewer = viewer; }
    
private:
    void createUI();
    void createGlobalPage();
    void createSelectedPage();
    void createHoverPage();
    void createFeatureEdgePage();
    void bindEvents();
    void updateControls();
    void applySettings();
    void loadSettings();
    void saveSettings();
    
    // Event handlers
    void onGlobalShowEdgesCheckbox(wxCommandEvent& event);
    void onGlobalEdgeWidthSlider(wxCommandEvent& event);
    void onGlobalEdgeColorButton(wxCommandEvent& event);
    void onGlobalEdgeColorEnabledCheckbox(wxCommandEvent& event);
    void onGlobalEdgeStyleChoice(wxCommandEvent& event);
    void onGlobalEdgeOpacitySlider(wxCommandEvent& event);
    
    void onSelectedShowEdgesCheckbox(wxCommandEvent& event);
    void onSelectedEdgeWidthSlider(wxCommandEvent& event);
    void onSelectedEdgeColorButton(wxCommandEvent& event);
    void onSelectedEdgeColorEnabledCheckbox(wxCommandEvent& event);
    void onSelectedEdgeStyleChoice(wxCommandEvent& event);
    void onSelectedEdgeOpacitySlider(wxCommandEvent& event);
    
    void onHoverShowEdgesCheckbox(wxCommandEvent& event);
    void onHoverEdgeWidthSlider(wxCommandEvent& event);
    void onHoverEdgeColorButton(wxCommandEvent& event);
    void onHoverEdgeColorEnabledCheckbox(wxCommandEvent& event);
    void onHoverEdgeStyleChoice(wxCommandEvent& event);
    void onHoverEdgeOpacitySlider(wxCommandEvent& event);
    
    void onFeatureEdgeAngleSlider(wxCommandEvent& event);
    void onFeatureEdgeMinLengthSlider(wxCommandEvent& event);
    void onFeatureEdgeConvexCheckbox(wxCommandEvent& event);
    void onFeatureEdgeConcaveCheckbox(wxCommandEvent& event);
    
    void onShowNormalLinesCheckbox(wxCommandEvent& event);
    void onShowFaceNormalLinesCheckbox(wxCommandEvent& event);
    void onNormalLengthSlider(wxCommandEvent& event);
    
    void updateColorButtons();
    wxColour quantityColorToWxColour(const Quantity_Color& color);
    Quantity_Color wxColourToQuantityColor(const wxColour& color);
    
    OCCViewer* m_viewer;
    
    // UI components
    wxNotebook* m_notebook;
    wxPanel* m_globalPage;
    wxPanel* m_selectedPage;
    wxPanel* m_hoverPage;
    wxPanel* m_featureEdgePage;
    
    // Global page controls
    wxCheckBox* m_globalShowEdgesCheckbox;
    wxSlider* m_globalEdgeWidthSlider;
    wxStaticText* m_globalEdgeWidthLabel;
    wxButton* m_globalEdgeColorButton;
    wxCheckBox* m_globalEdgeColorEnabledCheckbox;
    wxChoice* m_globalEdgeStyleChoice;
    wxSlider* m_globalEdgeOpacitySlider;
    wxStaticText* m_globalEdgeOpacityLabel;
    
    // Selected page controls
    wxCheckBox* m_selectedShowEdgesCheckbox;
    wxSlider* m_selectedEdgeWidthSlider;
    wxStaticText* m_selectedEdgeWidthLabel;
    wxButton* m_selectedEdgeColorButton;
    wxCheckBox* m_selectedEdgeColorEnabledCheckbox;
    wxChoice* m_selectedEdgeStyleChoice;
    wxSlider* m_selectedEdgeOpacitySlider;
    wxStaticText* m_selectedEdgeOpacityLabel;
    
    // Hover page controls
    wxCheckBox* m_hoverShowEdgesCheckbox;
    wxSlider* m_hoverEdgeWidthSlider;
    wxStaticText* m_hoverEdgeWidthLabel;
    wxButton* m_hoverEdgeColorButton;
    wxCheckBox* m_hoverEdgeColorEnabledCheckbox;
    wxChoice* m_hoverEdgeStyleChoice;
    wxSlider* m_hoverEdgeOpacitySlider;
    wxStaticText* m_hoverEdgeOpacityLabel;
    
    // Feature edge controls
    wxSlider* m_featureEdgeAngleSlider;
    wxStaticText* m_featureEdgeAngleLabel;
    wxSlider* m_featureEdgeMinLengthSlider;
    wxStaticText* m_featureEdgeMinLengthLabel;
    wxCheckBox* m_onlyConvexCheckbox;
    wxCheckBox* m_onlyConcaveCheckbox;
    
    // Normal display controls
    wxCheckBox* m_showNormalLinesCheckbox;
    wxCheckBox* m_showFaceNormalLinesCheckbox;
    wxSlider* m_normalLengthSlider;
    wxStaticText* m_normalLengthLabel;
    
    // Settings
    EdgeSettings m_globalSettings;
    EdgeSettings m_selectedSettings;
    EdgeSettings m_hoverSettings;
    int m_featureEdgeAngle;
    double m_featureEdgeMinLength;
    bool m_onlyConvex;
    bool m_onlyConcave;
    double m_normalLength;
};

#endif // EDGE_SETTINGS_EDITOR_H

