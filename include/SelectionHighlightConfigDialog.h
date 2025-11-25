#pragma once

#include "widgets/FramelessModalPopup.h"
#include "config/SelectionHighlightConfig.h"
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/colordlg.h>

class SelectionHighlightConfigDialog : public FramelessModalPopup {
public:
    SelectionHighlightConfigDialog(wxWindow* parent);
    virtual ~SelectionHighlightConfigDialog();

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);
    
    // Color button handlers
    void OnFaceHoverColor(wxCommandEvent& event);
    void OnFaceSelectionColor(wxCommandEvent& event);
    void OnEdgeHoverColor(wxCommandEvent& event);
    void OnEdgeSelectionColor(wxCommandEvent& event);
    void OnEdgeColor(wxCommandEvent& event);
    void OnVertexHoverColor(wxCommandEvent& event);
    void OnVertexSelectionColor(wxCommandEvent& event);
    void OnVertexColor(wxCommandEvent& event);
    void OnFaceQueryHoverColor(wxCommandEvent& event);
    void OnFaceQuerySelectionColor(wxCommandEvent& event);
    
    // Slider handlers
    void OnFaceHoverTransparency(wxScrollEvent& event);
    void OnFaceSelectionTransparency(wxScrollEvent& event);
    void OnEdgeHoverLineWidth(wxScrollEvent& event);
    void OnEdgeSelectionLineWidth(wxScrollEvent& event);
    void OnVertexHoverPointSize(wxScrollEvent& event);
    void OnVertexSelectionPointSize(wxScrollEvent& event);
    
    // Tab creation methods
    void CreateFaceTab(wxPanel* panel);
    void CreateEdgeTab(wxPanel* panel);
    void CreateVertexTab(wxPanel* panel);
    void CreateFaceQueryTab(wxPanel* panel);
    
    // Helper methods
    void updateColorButton(wxButton* button, const ColorRGB& color);
    wxColour colorRGBToWxColour(const ColorRGB& color);
    ColorRGB wxColourToColorRGB(const wxColour& color);
    void loadConfig();
    void saveConfig();
    void resetToDefaults();
    
    // Notebook and tabs
    wxNotebook* m_notebook;
    wxPanel* m_facePanel;
    wxPanel* m_edgePanel;
    wxPanel* m_vertexPanel;
    wxPanel* m_faceQueryPanel;
    
    // Face tab controls
    wxButton* m_faceHoverColorButton;
    wxButton* m_faceSelectionColorButton;
    wxSlider* m_faceHoverTransparencySlider;
    wxSlider* m_faceSelectionTransparencySlider;
    wxStaticText* m_faceHoverTransparencyLabel;
    wxStaticText* m_faceSelectionTransparencyLabel;
    
    // Edge tab controls
    wxButton* m_edgeHoverColorButton;
    wxButton* m_edgeSelectionColorButton;
    wxButton* m_edgeColorButton;
    wxSlider* m_edgeHoverLineWidthSlider;
    wxSlider* m_edgeSelectionLineWidthSlider;
    wxStaticText* m_edgeHoverLineWidthLabel;
    wxStaticText* m_edgeSelectionLineWidthLabel;
    
    // Vertex tab controls
    wxButton* m_vertexHoverColorButton;
    wxButton* m_vertexSelectionColorButton;
    wxButton* m_vertexColorButton;
    wxSlider* m_vertexHoverPointSizeSlider;
    wxSlider* m_vertexSelectionPointSizeSlider;
    wxStaticText* m_vertexHoverPointSizeLabel;
    wxStaticText* m_vertexSelectionPointSizeLabel;
    
    // FaceQuery tab controls
    wxButton* m_faceQueryHoverColorButton;
    wxButton* m_faceQuerySelectionColorButton;
    
    // Configuration data
    SelectionHighlightConfig m_config;
    
    DECLARE_EVENT_TABLE()
};


