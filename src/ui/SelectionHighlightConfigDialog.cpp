#include "SelectionHighlightConfigDialog.h"
#include "config/SelectionHighlightConfig.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/grid.h>

enum {
    ID_FACE_HOVER_COLOR = wxID_HIGHEST + 1,
    ID_FACE_SELECTION_COLOR,
    ID_EDGE_HOVER_COLOR,
    ID_EDGE_SELECTION_COLOR,
    ID_EDGE_COLOR,
    ID_VERTEX_HOVER_COLOR,
    ID_VERTEX_SELECTION_COLOR,
    ID_VERTEX_COLOR,
    ID_FACE_QUERY_HOVER_COLOR,
    ID_FACE_QUERY_SELECTION_COLOR,
    ID_RESET_BUTTON,
    ID_FACE_HOVER_TRANSPARENCY_SLIDER = ID_FACE_HOVER_COLOR + 100,
    ID_FACE_SELECTION_TRANSPARENCY_SLIDER,
    ID_EDGE_HOVER_LINEWIDTH_SLIDER,
    ID_EDGE_SELECTION_LINEWIDTH_SLIDER,
    ID_VERTEX_HOVER_POINTSIZE_SLIDER,
    ID_VERTEX_SELECTION_POINTSIZE_SLIDER
};

BEGIN_EVENT_TABLE(SelectionHighlightConfigDialog, FramelessModalPopup)
    EVT_BUTTON(wxID_OK, SelectionHighlightConfigDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, SelectionHighlightConfigDialog::OnCancel)
    EVT_BUTTON(ID_RESET_BUTTON, SelectionHighlightConfigDialog::OnReset)
    EVT_BUTTON(ID_FACE_HOVER_COLOR, SelectionHighlightConfigDialog::OnFaceHoverColor)
    EVT_BUTTON(ID_FACE_SELECTION_COLOR, SelectionHighlightConfigDialog::OnFaceSelectionColor)
    EVT_BUTTON(ID_EDGE_HOVER_COLOR, SelectionHighlightConfigDialog::OnEdgeHoverColor)
    EVT_BUTTON(ID_EDGE_SELECTION_COLOR, SelectionHighlightConfigDialog::OnEdgeSelectionColor)
    EVT_BUTTON(ID_EDGE_COLOR, SelectionHighlightConfigDialog::OnEdgeColor)
    EVT_BUTTON(ID_VERTEX_HOVER_COLOR, SelectionHighlightConfigDialog::OnVertexHoverColor)
    EVT_BUTTON(ID_VERTEX_SELECTION_COLOR, SelectionHighlightConfigDialog::OnVertexSelectionColor)
    EVT_BUTTON(ID_VERTEX_COLOR, SelectionHighlightConfigDialog::OnVertexColor)
    EVT_BUTTON(ID_FACE_QUERY_HOVER_COLOR, SelectionHighlightConfigDialog::OnFaceQueryHoverColor)
    EVT_BUTTON(ID_FACE_QUERY_SELECTION_COLOR, SelectionHighlightConfigDialog::OnFaceQuerySelectionColor)
    EVT_COMMAND_SCROLL(ID_FACE_HOVER_TRANSPARENCY_SLIDER, SelectionHighlightConfigDialog::OnFaceHoverTransparency)
    EVT_COMMAND_SCROLL(ID_FACE_SELECTION_TRANSPARENCY_SLIDER, SelectionHighlightConfigDialog::OnFaceSelectionTransparency)
    EVT_COMMAND_SCROLL(ID_EDGE_HOVER_LINEWIDTH_SLIDER, SelectionHighlightConfigDialog::OnEdgeHoverLineWidth)
    EVT_COMMAND_SCROLL(ID_EDGE_SELECTION_LINEWIDTH_SLIDER, SelectionHighlightConfigDialog::OnEdgeSelectionLineWidth)
    EVT_COMMAND_SCROLL(ID_VERTEX_HOVER_POINTSIZE_SLIDER, SelectionHighlightConfigDialog::OnVertexHoverPointSize)
    EVT_COMMAND_SCROLL(ID_VERTEX_SELECTION_POINTSIZE_SLIDER, SelectionHighlightConfigDialog::OnVertexSelectionPointSize)
END_EVENT_TABLE()

SelectionHighlightConfigDialog::SelectionHighlightConfigDialog(wxWindow* parent)
    : FramelessModalPopup(parent, "Selection Highlight Configuration", wxSize(500, 600))
    , m_notebook(nullptr)
    , m_facePanel(nullptr)
    , m_edgePanel(nullptr)
    , m_vertexPanel(nullptr)
    , m_faceQueryPanel(nullptr)
{
    SetTitleIcon("settings", wxSize(20, 20));
    ShowTitleIcon(true);
    
    // Load current configuration
    loadConfig();
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for tabbed interface
    m_notebook = new wxNotebook(m_contentPanel, wxID_ANY);
    
    // Create tabs
    m_facePanel = new wxPanel(m_notebook);
    CreateFaceTab(m_facePanel);
    m_notebook->AddPage(m_facePanel, "Face Selection");
    
    m_edgePanel = new wxPanel(m_notebook);
    CreateEdgeTab(m_edgePanel);
    m_notebook->AddPage(m_edgePanel, "Edge Selection");
    
    m_vertexPanel = new wxPanel(m_notebook);
    CreateVertexTab(m_vertexPanel);
    m_notebook->AddPage(m_vertexPanel, "Vertex Selection");
    
    m_faceQueryPanel = new wxPanel(m_notebook);
    CreateFaceQueryTab(m_faceQueryPanel);
    m_notebook->AddPage(m_faceQueryPanel, "Face Query");
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);
    
    // Button sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* resetButton = new wxButton(m_contentPanel, ID_RESET_BUTTON, "Reset");
    buttonSizer->Add(resetButton, 0, wxRIGHT, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(new wxButton(m_contentPanel, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(m_contentPanel, wxID_OK, "OK"), 0);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);
    
    m_contentPanel->SetSizer(mainSizer);
    Layout();
    Centre();
}

SelectionHighlightConfigDialog::~SelectionHighlightConfigDialog() {
}

void SelectionHighlightConfigDialog::CreateFaceTab(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer* gridSizer = new wxGridSizer(4, 2, 5, 5);
    
    // Hover color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Hover Highlight Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_faceHoverColorButton = new wxButton(panel, ID_FACE_HOVER_COLOR, "Choose Color");
    updateColorButton(m_faceHoverColorButton, m_config.faceHighlight.hoverDiffuse);
    gridSizer->Add(m_faceHoverColorButton, 0, wxEXPAND);
    
    // Selection color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Selection Highlight Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_faceSelectionColorButton = new wxButton(panel, ID_FACE_SELECTION_COLOR, "Choose Color");
    updateColorButton(m_faceSelectionColorButton, m_config.faceHighlight.selectionDiffuse);
    gridSizer->Add(m_faceSelectionColorButton, 0, wxEXPAND);
    
    // Hover transparency
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Hover Transparency:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* hoverTransparencySizer = new wxBoxSizer(wxHORIZONTAL);
    m_faceHoverTransparencySlider = new wxSlider(panel, ID_FACE_HOVER_TRANSPARENCY_SLIDER, 
        static_cast<int>(m_config.faceHighlight.hoverTransparency * 100), 0, 100, 
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_faceHoverTransparencyLabel = new wxStaticText(panel, wxID_ANY, 
        wxString::Format("%.2f", m_config.faceHighlight.hoverTransparency));
    hoverTransparencySizer->Add(m_faceHoverTransparencySlider, 1, wxEXPAND);
    hoverTransparencySizer->Add(m_faceHoverTransparencyLabel, 0, wxLEFT, 5);
    gridSizer->Add(hoverTransparencySizer, 0, wxEXPAND);
    
    // Selection transparency
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Selection Transparency:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* selectionTransparencySizer = new wxBoxSizer(wxHORIZONTAL);
    m_faceSelectionTransparencySlider = new wxSlider(panel, ID_FACE_SELECTION_TRANSPARENCY_SLIDER, 
        static_cast<int>(m_config.faceHighlight.selectionTransparency * 100), 0, 100, 
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_faceSelectionTransparencyLabel = new wxStaticText(panel, wxID_ANY, 
        wxString::Format("%.2f", m_config.faceHighlight.selectionTransparency));
    selectionTransparencySizer->Add(m_faceSelectionTransparencySlider, 1, wxEXPAND);
    selectionTransparencySizer->Add(m_faceSelectionTransparencyLabel, 0, wxLEFT, 5);
    gridSizer->Add(selectionTransparencySizer, 0, wxEXPAND);
    
    sizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);
    panel->SetSizer(sizer);
}

void SelectionHighlightConfigDialog::CreateEdgeTab(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer* gridSizer = new wxGridSizer(5, 2, 5, 5);
    
    // Hover color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Hover Highlight Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_edgeHoverColorButton = new wxButton(panel, ID_EDGE_HOVER_COLOR, "Choose Color");
    updateColorButton(m_edgeHoverColorButton, m_config.edgeHighlight.hoverDiffuse);
    gridSizer->Add(m_edgeHoverColorButton, 0, wxEXPAND);
    
    // Selection color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Selection Highlight Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_edgeSelectionColorButton = new wxButton(panel, ID_EDGE_SELECTION_COLOR, "Choose Color");
    updateColorButton(m_edgeSelectionColorButton, m_config.edgeHighlight.selectionDiffuse);
    gridSizer->Add(m_edgeSelectionColorButton, 0, wxEXPAND);
    
    // Edge color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Edge Display Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_edgeColorButton = new wxButton(panel, ID_EDGE_COLOR, "Choose Color");
    updateColorButton(m_edgeColorButton, m_config.edgeColor);
    gridSizer->Add(m_edgeColorButton, 0, wxEXPAND);
    
    // Hover line width
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Hover Line Width:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* hoverLineWidthSizer = new wxBoxSizer(wxHORIZONTAL);
    m_edgeHoverLineWidthSlider = new wxSlider(panel, ID_EDGE_HOVER_LINEWIDTH_SLIDER, 
        static_cast<int>(m_config.edgeHighlight.lineWidth * 10), 10, 100, 
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_edgeHoverLineWidthLabel = new wxStaticText(panel, wxID_ANY, 
        wxString::Format("%.1f", m_config.edgeHighlight.lineWidth));
    hoverLineWidthSizer->Add(m_edgeHoverLineWidthSlider, 1, wxEXPAND);
    hoverLineWidthSizer->Add(m_edgeHoverLineWidthLabel, 0, wxLEFT, 5);
    gridSizer->Add(hoverLineWidthSizer, 0, wxEXPAND);
    
    // Selection line width
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Selection Line Width:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* selectionLineWidthSizer = new wxBoxSizer(wxHORIZONTAL);
    m_edgeSelectionLineWidthSlider = new wxSlider(panel, ID_EDGE_SELECTION_LINEWIDTH_SLIDER, 
        static_cast<int>(m_config.edgeHighlight.selectionLineWidth * 10), 10, 100, 
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_edgeSelectionLineWidthLabel = new wxStaticText(panel, wxID_ANY, 
        wxString::Format("%.1f", m_config.edgeHighlight.selectionLineWidth));
    selectionLineWidthSizer->Add(m_edgeSelectionLineWidthSlider, 1, wxEXPAND);
    selectionLineWidthSizer->Add(m_edgeSelectionLineWidthLabel, 0, wxLEFT, 5);
    gridSizer->Add(selectionLineWidthSizer, 0, wxEXPAND);
    
    sizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);
    panel->SetSizer(sizer);
}

void SelectionHighlightConfigDialog::CreateVertexTab(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer* gridSizer = new wxGridSizer(5, 2, 5, 5);
    
    // Hover color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Hover Highlight Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_vertexHoverColorButton = new wxButton(panel, ID_VERTEX_HOVER_COLOR, "Choose Color");
    updateColorButton(m_vertexHoverColorButton, m_config.vertexHighlight.hoverDiffuse);
    gridSizer->Add(m_vertexHoverColorButton, 0, wxEXPAND);
    
    // Selection color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Selection Highlight Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_vertexSelectionColorButton = new wxButton(panel, ID_VERTEX_SELECTION_COLOR, "Choose Color");
    updateColorButton(m_vertexSelectionColorButton, m_config.vertexHighlight.selectionDiffuse);
    gridSizer->Add(m_vertexSelectionColorButton, 0, wxEXPAND);
    
    // Vertex color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Vertex Display Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_vertexColorButton = new wxButton(panel, ID_VERTEX_COLOR, "Choose Color");
    updateColorButton(m_vertexColorButton, m_config.vertexColor);
    gridSizer->Add(m_vertexColorButton, 0, wxEXPAND);
    
    // Hover point size
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Hover Point Size:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* hoverPointSizeSizer = new wxBoxSizer(wxHORIZONTAL);
    m_vertexHoverPointSizeSlider = new wxSlider(panel, ID_VERTEX_HOVER_POINTSIZE_SLIDER, 
        static_cast<int>(m_config.vertexHighlight.pointSize * 10), 10, 200, 
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_vertexHoverPointSizeLabel = new wxStaticText(panel, wxID_ANY, 
        wxString::Format("%.1f", m_config.vertexHighlight.pointSize));
    hoverPointSizeSizer->Add(m_vertexHoverPointSizeSlider, 1, wxEXPAND);
    hoverPointSizeSizer->Add(m_vertexHoverPointSizeLabel, 0, wxLEFT, 5);
    gridSizer->Add(hoverPointSizeSizer, 0, wxEXPAND);
    
    // Selection point size
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Selection Point Size:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* selectionPointSizeSizer = new wxBoxSizer(wxHORIZONTAL);
    m_vertexSelectionPointSizeSlider = new wxSlider(panel, ID_VERTEX_SELECTION_POINTSIZE_SLIDER, 
        static_cast<int>(m_config.vertexHighlight.selectionPointSize * 10), 10, 200, 
        wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    m_vertexSelectionPointSizeLabel = new wxStaticText(panel, wxID_ANY, 
        wxString::Format("%.1f", m_config.vertexHighlight.selectionPointSize));
    selectionPointSizeSizer->Add(m_vertexSelectionPointSizeSlider, 1, wxEXPAND);
    selectionPointSizeSizer->Add(m_vertexSelectionPointSizeLabel, 0, wxLEFT, 5);
    gridSizer->Add(selectionPointSizeSizer, 0, wxEXPAND);
    
    sizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);
    panel->SetSizer(sizer);
}

void SelectionHighlightConfigDialog::CreateFaceQueryTab(wxPanel* panel) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxGridSizer* gridSizer = new wxGridSizer(2, 2, 5, 5);
    
    // Hover color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Hover Highlight Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_faceQueryHoverColorButton = new wxButton(panel, ID_FACE_QUERY_HOVER_COLOR, "Choose Color");
    updateColorButton(m_faceQueryHoverColorButton, m_config.faceQueryHighlight.hoverDiffuse);
    gridSizer->Add(m_faceQueryHoverColorButton, 0, wxEXPAND);
    
    // Selection color
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Selection Highlight Color:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_faceQuerySelectionColorButton = new wxButton(panel, ID_FACE_QUERY_SELECTION_COLOR, "Choose Color");
    updateColorButton(m_faceQuerySelectionColorButton, m_config.faceQueryHighlight.selectionDiffuse);
    gridSizer->Add(m_faceQuerySelectionColorButton, 0, wxEXPAND);
    
    sizer->Add(gridSizer, 0, wxEXPAND | wxALL, 10);
    panel->SetSizer(sizer);
}

void SelectionHighlightConfigDialog::OnOK(wxCommandEvent& event) {
    saveConfig();
    EndModal(wxID_OK);
}

void SelectionHighlightConfigDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void SelectionHighlightConfigDialog::OnReset(wxCommandEvent& event) {
    resetToDefaults();
}

void SelectionHighlightConfigDialog::OnFaceHoverColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.faceHighlight.hoverDiffuse));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.faceHighlight.hoverDiffuse = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_faceHoverColorButton, m_config.faceHighlight.hoverDiffuse);
    }
}

void SelectionHighlightConfigDialog::OnFaceSelectionColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.faceHighlight.selectionDiffuse));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.faceHighlight.selectionDiffuse = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_faceSelectionColorButton, m_config.faceHighlight.selectionDiffuse);
    }
}

void SelectionHighlightConfigDialog::OnEdgeHoverColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.edgeHighlight.hoverDiffuse));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.edgeHighlight.hoverDiffuse = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_edgeHoverColorButton, m_config.edgeHighlight.hoverDiffuse);
    }
}

void SelectionHighlightConfigDialog::OnEdgeSelectionColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.edgeHighlight.selectionDiffuse));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.edgeHighlight.selectionDiffuse = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_edgeSelectionColorButton, m_config.edgeHighlight.selectionDiffuse);
    }
}

void SelectionHighlightConfigDialog::OnEdgeColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.edgeColor));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.edgeColor = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_edgeColorButton, m_config.edgeColor);
    }
}

void SelectionHighlightConfigDialog::OnVertexHoverColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.vertexHighlight.hoverDiffuse));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.vertexHighlight.hoverDiffuse = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_vertexHoverColorButton, m_config.vertexHighlight.hoverDiffuse);
    }
}

void SelectionHighlightConfigDialog::OnVertexSelectionColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.vertexHighlight.selectionDiffuse));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.vertexHighlight.selectionDiffuse = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_vertexSelectionColorButton, m_config.vertexHighlight.selectionDiffuse);
    }
}

void SelectionHighlightConfigDialog::OnVertexColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.vertexColor));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.vertexColor = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_vertexColorButton, m_config.vertexColor);
    }
}

void SelectionHighlightConfigDialog::OnFaceQueryHoverColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.faceQueryHighlight.hoverDiffuse));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.faceQueryHighlight.hoverDiffuse = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_faceQueryHoverColorButton, m_config.faceQueryHighlight.hoverDiffuse);
    }
}

void SelectionHighlightConfigDialog::OnFaceQuerySelectionColor(wxCommandEvent& event) {
    wxColourDialog colorDialog(this);
    colorDialog.GetColourData().SetColour(colorRGBToWxColour(m_config.faceQueryHighlight.selectionDiffuse));
    if (colorDialog.ShowModal() == wxID_OK) {
        m_config.faceQueryHighlight.selectionDiffuse = wxColourToColorRGB(colorDialog.GetColourData().GetColour());
        updateColorButton(m_faceQuerySelectionColorButton, m_config.faceQueryHighlight.selectionDiffuse);
    }
}

void SelectionHighlightConfigDialog::OnFaceHoverTransparency(wxScrollEvent& event) {
    m_config.faceHighlight.hoverTransparency = event.GetPosition() / 100.0f;
    m_faceHoverTransparencyLabel->SetLabel(wxString::Format("%.2f", m_config.faceHighlight.hoverTransparency));
}

void SelectionHighlightConfigDialog::OnFaceSelectionTransparency(wxScrollEvent& event) {
    m_config.faceHighlight.selectionTransparency = event.GetPosition() / 100.0f;
    m_faceSelectionTransparencyLabel->SetLabel(wxString::Format("%.2f", m_config.faceHighlight.selectionTransparency));
}

void SelectionHighlightConfigDialog::OnEdgeHoverLineWidth(wxScrollEvent& event) {
    m_config.edgeHighlight.lineWidth = event.GetPosition() / 10.0f;
    m_edgeHoverLineWidthLabel->SetLabel(wxString::Format("%.1f", m_config.edgeHighlight.lineWidth));
}

void SelectionHighlightConfigDialog::OnEdgeSelectionLineWidth(wxScrollEvent& event) {
    m_config.edgeHighlight.selectionLineWidth = event.GetPosition() / 10.0f;
    m_edgeSelectionLineWidthLabel->SetLabel(wxString::Format("%.1f", m_config.edgeHighlight.selectionLineWidth));
}

void SelectionHighlightConfigDialog::OnVertexHoverPointSize(wxScrollEvent& event) {
    m_config.vertexHighlight.pointSize = event.GetPosition() / 10.0f;
    m_vertexHoverPointSizeLabel->SetLabel(wxString::Format("%.1f", m_config.vertexHighlight.pointSize));
}

void SelectionHighlightConfigDialog::OnVertexSelectionPointSize(wxScrollEvent& event) {
    m_config.vertexHighlight.selectionPointSize = event.GetPosition() / 10.0f;
    m_vertexSelectionPointSizeLabel->SetLabel(wxString::Format("%.1f", m_config.vertexHighlight.selectionPointSize));
}

void SelectionHighlightConfigDialog::updateColorButton(wxButton* button, const ColorRGB& color) {
    wxColour wxColor = colorRGBToWxColour(color);
    button->SetBackgroundColour(wxColor);
    button->Refresh();
}

wxColour SelectionHighlightConfigDialog::colorRGBToWxColour(const ColorRGB& color) {
    return wxColour(
        static_cast<unsigned char>(color.r * 255),
        static_cast<unsigned char>(color.g * 255),
        static_cast<unsigned char>(color.b * 255)
    );
}

ColorRGB SelectionHighlightConfigDialog::wxColourToColorRGB(const wxColour& color) {
    return ColorRGB(
        static_cast<float>(color.Red()) / 255.0f,
        static_cast<float>(color.Green()) / 255.0f,
        static_cast<float>(color.Blue()) / 255.0f
    );
}

void SelectionHighlightConfigDialog::loadConfig() {
    auto& configManager = SelectionHighlightConfigManager::getInstance();
    if (configManager.isInitialized()) {
        m_config = configManager.getConfig();
    } else {
        // Use defaults
        SelectionHighlightConfig defaultConfig;
        m_config = defaultConfig;
    }
}

void SelectionHighlightConfigDialog::saveConfig() {
    auto& configManager = SelectionHighlightConfigManager::getInstance();
    configManager.getConfig() = m_config;
    
    // Save to ConfigManager
    ConfigManager& cm = ConfigManager::getInstance();
    configManager.save(cm);
    
    LOG_INF("Selection highlight configuration saved", "SelectionHighlightConfigDialog");
}

void SelectionHighlightConfigDialog::resetToDefaults() {
    SelectionHighlightConfig defaultConfig;
    m_config = defaultConfig;
    
    // Update UI
    updateColorButton(m_faceHoverColorButton, m_config.faceHighlight.hoverDiffuse);
    updateColorButton(m_faceSelectionColorButton, m_config.faceHighlight.selectionDiffuse);
    m_faceHoverTransparencySlider->SetValue(static_cast<int>(m_config.faceHighlight.hoverTransparency * 100));
    m_faceSelectionTransparencySlider->SetValue(static_cast<int>(m_config.faceHighlight.selectionTransparency * 100));
    m_faceHoverTransparencyLabel->SetLabel(wxString::Format("%.2f", m_config.faceHighlight.hoverTransparency));
    m_faceSelectionTransparencyLabel->SetLabel(wxString::Format("%.2f", m_config.faceHighlight.selectionTransparency));
    
    updateColorButton(m_edgeHoverColorButton, m_config.edgeHighlight.hoverDiffuse);
    updateColorButton(m_edgeSelectionColorButton, m_config.edgeHighlight.selectionDiffuse);
    updateColorButton(m_edgeColorButton, m_config.edgeColor);
    m_edgeHoverLineWidthSlider->SetValue(static_cast<int>(m_config.edgeHighlight.lineWidth * 10));
    m_edgeSelectionLineWidthSlider->SetValue(static_cast<int>(m_config.edgeHighlight.selectionLineWidth * 10));
    m_edgeHoverLineWidthLabel->SetLabel(wxString::Format("%.1f", m_config.edgeHighlight.lineWidth));
    m_edgeSelectionLineWidthLabel->SetLabel(wxString::Format("%.1f", m_config.edgeHighlight.selectionLineWidth));
    
    updateColorButton(m_vertexHoverColorButton, m_config.vertexHighlight.hoverDiffuse);
    updateColorButton(m_vertexSelectionColorButton, m_config.vertexHighlight.selectionDiffuse);
    updateColorButton(m_vertexColorButton, m_config.vertexColor);
    m_vertexHoverPointSizeSlider->SetValue(static_cast<int>(m_config.vertexHighlight.pointSize * 10));
    m_vertexSelectionPointSizeSlider->SetValue(static_cast<int>(m_config.vertexHighlight.selectionPointSize * 10));
    m_vertexHoverPointSizeLabel->SetLabel(wxString::Format("%.1f", m_config.vertexHighlight.pointSize));
    m_vertexSelectionPointSizeLabel->SetLabel(wxString::Format("%.1f", m_config.vertexHighlight.selectionPointSize));
    
    updateColorButton(m_faceQueryHoverColorButton, m_config.faceQueryHighlight.hoverDiffuse);
    updateColorButton(m_faceQuerySelectionColorButton, m_config.faceQueryHighlight.selectionDiffuse);
}

