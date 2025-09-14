#include "GeometryDecompositionDialog.h"
#include "logger/Logger.h"
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/font.h>

wxBEGIN_EVENT_TABLE(GeometryDecompositionDialog, wxDialog)
    EVT_BUTTON(wxID_OK, GeometryDecompositionDialog::onOK)
    EVT_BUTTON(wxID_CANCEL, GeometryDecompositionDialog::onCancel)
    EVT_CHOICE(wxID_ANY, GeometryDecompositionDialog::onDecompositionLevelChange)
    EVT_CHOICE(wxID_ANY, GeometryDecompositionDialog::onColorSchemeChange)
wxEND_EVENT_TABLE()

GeometryDecompositionDialog::GeometryDecompositionDialog(wxWindow* parent, GeometryReader::DecompositionOptions& options)
    : wxDialog(parent, wxID_ANY, "Geometry Decomposition Settings", wxDefaultPosition, wxSize(600, 400))
    , m_options(options)
    , m_enableDecompositionCheckBox(nullptr)
    , m_decompositionLevelChoice(nullptr)
    , m_colorSchemeChoice(nullptr)
    , m_consistentColoringCheckBox(nullptr)
    , m_previewText(nullptr)
    , m_previewPanel(nullptr)
    , m_colorPreviewPanel(nullptr)
    , m_enableDecomposition(options.enableDecomposition)
    , m_decompositionLevel(options.level)
    , m_colorScheme(options.colorScheme)
    , m_useConsistentColoring(options.useConsistentColoring)
{
    createControls();
    layoutControls();
    bindEvents();

    // Update UI to reflect current options
    updatePreview();
}

GeometryDecompositionDialog::~GeometryDecompositionDialog()
{
}

void GeometryDecompositionDialog::createControls()
{
    // Enable decomposition checkbox
    m_enableDecompositionCheckBox = new wxCheckBox(this, wxID_ANY, "Enable Geometry Decomposition");
    m_enableDecompositionCheckBox->SetValue(m_enableDecomposition);
    m_enableDecompositionCheckBox->SetToolTip("Enable automatic decomposition of complex geometries into separate components");

    // Decomposition level choice
    wxArrayString levelChoices;
    levelChoices.Add("No Decomposition");
    levelChoices.Add("Shape Level");
    levelChoices.Add("Solid Level (recommended)");
    levelChoices.Add("Shell Level");
    levelChoices.Add("Face Level (detailed)");

    m_decompositionLevelChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, levelChoices);
    m_decompositionLevelChoice->SetSelection(static_cast<int>(m_decompositionLevel));
    m_decompositionLevelChoice->SetToolTip("Choose how detailed the decomposition should be");
    m_decompositionLevelChoice->Enable(m_enableDecomposition);

    // Color scheme choice
    wxArrayString colorChoices;
    colorChoices.Add("Distinct Colors (cool tones)");
    colorChoices.Add("Warm Colors");
    colorChoices.Add("Rainbow Spectrum");
    colorChoices.Add("Monochrome Blue");
    colorChoices.Add("Monochrome Green");
    colorChoices.Add("Monochrome Gray");

    m_colorSchemeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, colorChoices);
    m_colorSchemeChoice->SetSelection(static_cast<int>(m_colorScheme));
    m_colorSchemeChoice->SetToolTip("Choose color scheme for decomposed components");
    m_colorSchemeChoice->Enable(m_enableDecomposition);

    // Consistent coloring checkbox
    m_consistentColoringCheckBox = new wxCheckBox(this, wxID_ANY, "Use Consistent Coloring");
    m_consistentColoringCheckBox->SetValue(m_useConsistentColoring);
    m_consistentColoringCheckBox->SetToolTip("Use consistent colors for similar components across imports");
    m_consistentColoringCheckBox->Enable(m_enableDecomposition);
}

void GeometryDecompositionDialog::layoutControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Configure Geometry Decomposition");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);

    mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
    mainSizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 15);

    // Main content
    wxBoxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

    // Enable decomposition
    wxStaticBox* enableBox = new wxStaticBox(this, wxID_ANY, "Decomposition Control");
    wxStaticBoxSizer* enableSizer = new wxStaticBoxSizer(enableBox, wxVERTICAL);

    enableSizer->Add(m_enableDecompositionCheckBox, 0, wxALL, 10);

    contentSizer->Add(enableSizer, 0, wxEXPAND | wxALL, 10);

    // Decomposition settings
    wxStaticBox* settingsBox = new wxStaticBox(this, wxID_ANY, "Decomposition Settings");
    wxStaticBoxSizer* settingsSizer = new wxStaticBoxSizer(settingsBox, wxVERTICAL);

    wxFlexGridSizer* settingsGrid = new wxFlexGridSizer(3, 2, 8, 15);
    settingsGrid->AddGrowableCol(1);

    // Decomposition level
    settingsGrid->Add(new wxStaticText(this, wxID_ANY, "Decomposition Level:"), 0, wxALIGN_CENTER_VERTICAL);
    settingsGrid->Add(m_decompositionLevelChoice, 1, wxEXPAND);

    // Color scheme
    settingsGrid->Add(new wxStaticText(this, wxID_ANY, "Color Scheme:"), 0, wxALIGN_CENTER_VERTICAL);
    settingsGrid->Add(m_colorSchemeChoice, 1, wxEXPAND);

    // Consistent coloring
    settingsGrid->Add(new wxStaticText(this, wxID_ANY, "Coloring Mode:"), 0, wxALIGN_CENTER_VERTICAL);
    settingsGrid->Add(m_consistentColoringCheckBox, 1, wxEXPAND);

    settingsSizer->Add(settingsGrid, 0, wxEXPAND | wxALL, 10);

    // Help text
    wxStaticText* helpText = new wxStaticText(this, wxID_ANY,
        "* Shape Level: Decomposes assemblies into individual shapes\n"
        "* Solid Level: Further decomposes shapes into individual solid bodies\n"
        "* Shell Level: Further decomposes solids into surface shells\n"
        "* Face Level: Decomposes into individual faces (most detailed)\n"
        "* Consistent coloring ensures similar components have the same color");
    helpText->SetForegroundColour(wxColour(100, 100, 100));
    wxFont helpFont = helpText->GetFont();
    helpFont.SetPointSize(helpFont.GetPointSize() - 1);
    helpText->SetFont(helpFont);
    settingsSizer->Add(helpText, 0, wxALL, 10);

    contentSizer->Add(settingsSizer, 0, wxEXPAND | wxALL, 10);

    // Color scheme preview
    wxStaticBox* colorPreviewBox = new wxStaticBox(this, wxID_ANY, "Color Scheme Preview");
    wxStaticBoxSizer* colorPreviewSizer = new wxStaticBoxSizer(colorPreviewBox, wxVERTICAL);
    
    m_colorPreviewPanel = new wxPanel(this);
    m_colorPreviewPanel->SetBackgroundColour(wxColour(255, 255, 255));
    
    wxBoxSizer* colorPreviewPanelSizer = new wxBoxSizer(wxHORIZONTAL);
    m_colorPreviewPanel->SetSizer(colorPreviewPanelSizer);
    
    colorPreviewSizer->Add(m_colorPreviewPanel, 0, wxEXPAND | wxALL, 5);
    contentSizer->Add(colorPreviewSizer, 0, wxEXPAND | wxALL, 10);

    // Preview section
    wxStaticBox* previewBox = new wxStaticBox(this, wxID_ANY, "Settings Preview");
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(previewBox, wxVERTICAL);

    m_previewPanel = new wxPanel(this);
    m_previewPanel->SetBackgroundColour(wxColour(248, 248, 248));

    m_previewText = new wxStaticText(m_previewPanel, wxID_ANY, "Preview will appear here");
    wxFont previewFont = m_previewText->GetFont();
    previewFont.SetPointSize(previewFont.GetPointSize() + 1);
    m_previewText->SetFont(previewFont);

    wxBoxSizer* previewPanelSizer = new wxBoxSizer(wxVERTICAL);
    previewPanelSizer->Add(m_previewText, 0, wxEXPAND | wxALL, 12);
    m_previewPanel->SetSizer(previewPanelSizer);

    previewSizer->Add(m_previewPanel, 1, wxEXPAND | wxALL, 5);

    contentSizer->Add(previewSizer, 1, wxEXPAND | wxALL, 10);

    mainSizer->Add(contentSizer, 1, wxEXPAND | wxALL, 5);

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okBtn = new wxButton(this, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");

    okBtn->SetDefault();
    okBtn->SetMinSize(wxSize(80, 30));
    cancelBtn->SetMinSize(wxSize(80, 30));

    buttonSizer->Add(okBtn, 0, wxALL, 5);
    buttonSizer->Add(cancelBtn, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);

    SetSizer(mainSizer);
    Fit();
}

void GeometryDecompositionDialog::bindEvents()
{
    m_enableDecompositionCheckBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        bool enabled = m_enableDecompositionCheckBox->GetValue();
        m_decompositionLevelChoice->Enable(enabled);
        m_colorSchemeChoice->Enable(enabled);
        m_consistentColoringCheckBox->Enable(enabled);
        updatePreview();
    });

    m_decompositionLevelChoice->Bind(wxEVT_CHOICE, &GeometryDecompositionDialog::onDecompositionLevelChange, this);
    m_colorSchemeChoice->Bind(wxEVT_CHOICE, &GeometryDecompositionDialog::onColorSchemeChange, this);
    m_consistentColoringCheckBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        updatePreview();
    });
}

void GeometryDecompositionDialog::updatePreview()
{
    wxString preview;
    wxColour previewColor = wxColour(0, 150, 0); // Default green

    bool enabled = m_enableDecompositionCheckBox->GetValue();

    if (!enabled) {
        preview = "Decomposition: Disabled\n"
                  "Result: Single component per file\n"
                  "Coloring: Default";
        previewColor = wxColour(150, 150, 150); // Gray
    } else {
        // Get selected level description
        wxString levelDesc;
        int levelIndex = m_decompositionLevelChoice->GetSelection();
        switch (levelIndex) {
            case 0: levelDesc = "No Decomposition"; break;
            case 1: levelDesc = "Shape Level"; break;
            case 2: levelDesc = "Solid Level"; break;
            case 3: levelDesc = "Shell Level"; break;
            case 4: levelDesc = "Face Level"; break;
            default: levelDesc = "Unknown"; break;
        }

        // Get selected color scheme description
        wxString colorDesc;
        int colorIndex = m_colorSchemeChoice->GetSelection();
        switch (colorIndex) {
            case 0: colorDesc = "Distinct Colors"; break;
            case 1: colorDesc = "Warm Colors"; break;
            case 2: colorDesc = "Rainbow"; break;
            case 3: colorDesc = "Blue Monochrome"; break;
            case 4: colorDesc = "Green Monochrome"; break;
            case 5: colorDesc = "Gray Monochrome"; break;
            default: colorDesc = "Unknown"; break;
        }

        wxString consistency = m_consistentColoringCheckBox->GetValue() ?
                              "Consistent" : "Random";

        preview = wxString::Format("Decomposition: Enabled (%s)\n"
                                  "Color Scheme: %s\n"
                                  "Coloring: %s",
                                  levelDesc.mb_str(), colorDesc.mb_str(), consistency.mb_str());

        if (levelIndex == 3) { // Face level
            preview += "\nWarning: Face level may create many small components";
            previewColor = wxColour(255, 140, 0); // Orange warning
        } else {
            previewColor = wxColour(0, 150, 0); // Green success
        }
    }

    if (m_previewText) {
        m_previewText->SetLabel(preview);
        m_previewText->SetForegroundColour(previewColor);
        m_previewPanel->Refresh();
        
        // Update color preview
        updateColorPreview();
    }
}

void GeometryDecompositionDialog::updateColorPreview()
{
    if (!m_colorPreviewPanel) return;
    
    // Clear existing preview controls
    m_colorPreviewPanel->DestroyChildren();
    wxBoxSizer* sizer = dynamic_cast<wxBoxSizer*>(m_colorPreviewPanel->GetSizer());
    if (sizer) {
        sizer->Clear();
    }
    
    // Get current color scheme
    int colorIndex = m_colorSchemeChoice->GetSelection();
    GeometryReader::ColorScheme scheme = static_cast<GeometryReader::ColorScheme>(colorIndex);
    
    // Create color palette based on scheme
    std::vector<wxColour> colors;
    switch (scheme) {
        case GeometryReader::ColorScheme::DISTINCT_COLORS:
            colors = {
                wxColour(70, 130, 180), wxColour(220, 20, 60), wxColour(34, 139, 34),
                wxColour(255, 140, 0), wxColour(128, 0, 128), wxColour(255, 20, 147),
                wxColour(0, 191, 255), wxColour(255, 69, 0), wxColour(50, 205, 50),
                wxColour(138, 43, 226), wxColour(255, 105, 180), wxColour(0, 206, 209),
                wxColour(255, 215, 0), wxColour(199, 21, 133), wxColour(72, 209, 204)
            };
            break;
        case GeometryReader::ColorScheme::WARM_COLORS:
            colors = {
                wxColour(255, 140, 0), wxColour(255, 69, 0), wxColour(255, 20, 147),
                wxColour(220, 20, 60), wxColour(255, 105, 180), wxColour(255, 215, 0),
                wxColour(255, 165, 0), wxColour(255, 99, 71), wxColour(255, 160, 122),
                wxColour(255, 192, 203), wxColour(255, 228, 225), wxColour(255, 69, 0),
                wxColour(255, 140, 0), wxColour(255, 20, 147), wxColour(255, 215, 0)
            };
            break;
        case GeometryReader::ColorScheme::RAINBOW:
            colors = {
                wxColour(255, 0, 0), wxColour(255, 127, 0), wxColour(255, 255, 0),
                wxColour(127, 255, 0), wxColour(0, 255, 0), wxColour(0, 255, 127),
                wxColour(0, 255, 255), wxColour(0, 127, 255), wxColour(0, 0, 255),
                wxColour(127, 0, 255), wxColour(255, 0, 255), wxColour(255, 0, 127),
                wxColour(255, 64, 0), wxColour(255, 191, 0), wxColour(191, 255, 0)
            };
            break;
        case GeometryReader::ColorScheme::MONOCHROME_BLUE:
            colors = {
                wxColour(25, 25, 112), wxColour(47, 79, 79), wxColour(70, 130, 180),
                wxColour(100, 149, 237), wxColour(135, 206, 235), wxColour(173, 216, 230),
                wxColour(176, 224, 230), wxColour(175, 238, 238), wxColour(95, 158, 160),
                wxColour(72, 209, 204), wxColour(64, 224, 208), wxColour(0, 206, 209),
                wxColour(0, 191, 255), wxColour(30, 144, 255), wxColour(0, 0, 255)
            };
            break;
        case GeometryReader::ColorScheme::MONOCHROME_GREEN:
            colors = {
                wxColour(0, 100, 0), wxColour(34, 139, 34), wxColour(50, 205, 50),
                wxColour(124, 252, 0), wxColour(127, 255, 0), wxColour(173, 255, 47),
                wxColour(154, 205, 50), wxColour(107, 142, 35), wxColour(85, 107, 47),
                wxColour(107, 142, 35), wxColour(154, 205, 50), wxColour(173, 255, 47),
                wxColour(127, 255, 0), wxColour(124, 252, 0), wxColour(50, 205, 50)
            };
            break;
        case GeometryReader::ColorScheme::MONOCHROME_GRAY:
            colors = {
                wxColour(64, 64, 64), wxColour(96, 96, 96), wxColour(128, 128, 128),
                wxColour(160, 160, 160), wxColour(192, 192, 192), wxColour(211, 211, 211),
                wxColour(220, 220, 220), wxColour(230, 230, 230), wxColour(105, 105, 105),
                wxColour(169, 169, 169), wxColour(200, 200, 200), wxColour(210, 210, 210),
                wxColour(220, 220, 220), wxColour(230, 230, 230), wxColour(240, 240, 240)
            };
            break;
        default:
            colors = { wxColour(128, 128, 128) };
            break;
    }
    
    // Create color swatches
    const int swatchSize = 20;
    const int swatchSpacing = 4;
    
    for (size_t i = 0; i < colors.size() && i < 12; ++i) { // Show first 12 colors
        wxPanel* swatch = new wxPanel(m_colorPreviewPanel);
        swatch->SetMinSize(wxSize(swatchSize, swatchSize));
        swatch->SetMaxSize(wxSize(swatchSize, swatchSize));
        swatch->SetBackgroundColour(colors[i]);
        
        // Add border
        swatch->SetWindowStyleFlag(wxBORDER_SIMPLE);
        
        sizer->Add(swatch, 0, wxALL, swatchSpacing);
    }
    
    // Add text label
    wxString schemeName;
    switch (scheme) {
        case GeometryReader::ColorScheme::DISTINCT_COLORS: schemeName = "Distinct Colors"; break;
        case GeometryReader::ColorScheme::WARM_COLORS: schemeName = "Warm Colors"; break;
        case GeometryReader::ColorScheme::RAINBOW: schemeName = "Rainbow"; break;
        case GeometryReader::ColorScheme::MONOCHROME_BLUE: schemeName = "Blue Monochrome"; break;
        case GeometryReader::ColorScheme::MONOCHROME_GREEN: schemeName = "Green Monochrome"; break;
        case GeometryReader::ColorScheme::MONOCHROME_GRAY: schemeName = "Gray Monochrome"; break;
        default: schemeName = "Unknown"; break;
    }
    
    wxStaticText* label = new wxStaticText(m_colorPreviewPanel, wxID_ANY, 
        wxString::Format("Sample: %s (%d colors)", schemeName, (int)colors.size()));
    wxFont labelFont = label->GetFont();
    labelFont.SetPointSize(labelFont.GetPointSize() - 1);
    label->SetFont(labelFont);
    label->SetForegroundColour(wxColour(100, 100, 100));
    
    sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    
    m_colorPreviewPanel->Layout();
}

GeometryReader::DecompositionOptions GeometryDecompositionDialog::getDecompositionOptions() const
{
    GeometryReader::DecompositionOptions options;
    options.enableDecomposition = m_enableDecompositionCheckBox->GetValue();
    options.level = static_cast<GeometryReader::DecompositionLevel>(m_decompositionLevelChoice->GetSelection());
    options.colorScheme = static_cast<GeometryReader::ColorScheme>(m_colorSchemeChoice->GetSelection());
    options.useConsistentColoring = m_consistentColoringCheckBox->GetValue();
    return options;
}

void GeometryDecompositionDialog::onOK(wxCommandEvent& event)
{
    // Save settings to the referenced options
    m_options = getDecompositionOptions();

    LOG_INF_S(wxString::Format("Geometry decomposition settings saved: Enabled=%s, Level=%d, Scheme=%d",
        m_options.enableDecomposition ? "Yes" : "No",
        static_cast<int>(m_options.level),
        static_cast<int>(m_options.colorScheme)));

    EndModal(wxID_OK);
}

void GeometryDecompositionDialog::onCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void GeometryDecompositionDialog::onDecompositionLevelChange(wxCommandEvent& event)
{
    updatePreview();
}

void GeometryDecompositionDialog::onColorSchemeChange(wxCommandEvent& event)
{
    updatePreview();
}