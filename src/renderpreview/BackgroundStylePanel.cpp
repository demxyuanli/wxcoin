#include "renderpreview/BackgroundStylePanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/BackgroundManager.h"
#include "logger/Logger.h"
#include <wx/colordlg.h>
#include <wx/filedlg.h>

BEGIN_EVENT_TABLE(BackgroundStylePanel, wxPanel)
    EVT_CHOICE(wxID_ANY, BackgroundStylePanel::onBackgroundStyleChanged)
    EVT_BUTTON(wxID_ANY, BackgroundStylePanel::onBackgroundColorButton)
    EVT_BUTTON(wxID_ANY, BackgroundStylePanel::onGradientTopColorButton)
    EVT_BUTTON(wxID_ANY, BackgroundStylePanel::onGradientBottomColorButton)
    EVT_BUTTON(wxID_ANY, BackgroundStylePanel::onBackgroundImageButton)
    EVT_SLIDER(wxID_ANY, BackgroundStylePanel::onBackgroundImageOpacityChanged)
    EVT_CHOICE(wxID_ANY, BackgroundStylePanel::onBackgroundImageFitChanged)
    EVT_CHECKBOX(wxID_ANY, BackgroundStylePanel::onBackgroundImageMaintainAspectChanged)
END_EVENT_TABLE()

BackgroundStylePanel::BackgroundStylePanel(wxWindow* parent, RenderPreviewDialog* dialog)
    : wxPanel(parent, wxID_ANY), m_parentDialog(dialog), m_backgroundManager(nullptr)
{
    createUI();
    bindEvents();
    updateControlStates();
}

BackgroundStylePanel::~BackgroundStylePanel()
{
}

void BackgroundStylePanel::createUI()
{
    auto* backgroundSizer = new wxBoxSizer(wxVERTICAL);

    auto* styleBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Background Style");
    m_backgroundStyleChoice = new wxChoice(this, wxID_ANY);
    m_backgroundStyleChoice->Append("Solid Color");
    m_backgroundStyleChoice->Append("Gradient");
    m_backgroundStyleChoice->Append("Image");
    m_backgroundStyleChoice->SetSelection(0);
    styleBoxSizer->Add(m_backgroundStyleChoice, 0, wxEXPAND | wxALL, 8);
    backgroundSizer->Add(styleBoxSizer, 0, wxEXPAND | wxALL, 8);

    auto* colorBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Color Settings");
    m_backgroundColorButton = new wxButton(this, wxID_ANY, "Background Color");
    colorBoxSizer->Add(m_backgroundColorButton, 0, wxEXPAND | wxALL, 8);
    m_gradientTopColorButton = new wxButton(this, wxID_ANY, "Gradient Top Color");
    colorBoxSizer->Add(m_gradientTopColorButton, 0, wxEXPAND | wxALL, 8);
    m_gradientBottomColorButton = new wxButton(this, wxID_ANY, "Gradient Bottom Color");
    colorBoxSizer->Add(m_gradientBottomColorButton, 0, wxEXPAND | wxALL, 8);
    backgroundSizer->Add(colorBoxSizer, 0, wxEXPAND | wxALL, 8);

    auto* imageBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Image Settings");
    m_backgroundImageButton = new wxButton(this, wxID_ANY, "Choose Image");
    imageBoxSizer->Add(m_backgroundImageButton, 0, wxEXPAND | wxALL, 8);
    m_backgroundImagePathLabel = new wxStaticText(this, wxID_ANY, "No image selected");
    imageBoxSizer->Add(m_backgroundImagePathLabel, 0, wxEXPAND | wxALL, 8);
    m_backgroundImageOpacitySlider = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
    imageBoxSizer->Add(m_backgroundImageOpacitySlider, 0, wxEXPAND | wxALL, 8);
    m_backgroundImageFitChoice = new wxChoice(this, wxID_ANY);
    m_backgroundImageFitChoice->Append("Fill");
    m_backgroundImageFitChoice->Append("Fit");
    m_backgroundImageFitChoice->Append("Stretch");
    m_backgroundImageFitChoice->SetSelection(0);
    imageBoxSizer->Add(m_backgroundImageFitChoice, 0, wxEXPAND | wxALL, 8);
    m_backgroundImageMaintainAspectCheckBox = new wxCheckBox(this, wxID_ANY, "Maintain Aspect Ratio");
    imageBoxSizer->Add(m_backgroundImageMaintainAspectCheckBox, 0, wxEXPAND | wxALL, 8);
    backgroundSizer->Add(imageBoxSizer, 0, wxEXPAND | wxALL, 8);

    SetSizer(backgroundSizer);
}

void BackgroundStylePanel::bindEvents()
{
    m_backgroundStyleChoice->Bind(wxEVT_CHOICE, &BackgroundStylePanel::onBackgroundStyleChanged, this);
    m_backgroundColorButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onBackgroundColorButton, this);
    m_gradientTopColorButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onGradientTopColorButton, this);
    m_gradientBottomColorButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onGradientBottomColorButton, this);
    m_backgroundImageButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onBackgroundImageButton, this);
    m_backgroundImageOpacitySlider->Bind(wxEVT_SLIDER, &BackgroundStylePanel::onBackgroundImageOpacityChanged, this);
    m_backgroundImageFitChoice->Bind(wxEVT_CHOICE, &BackgroundStylePanel::onBackgroundImageFitChanged, this);
    m_backgroundImageMaintainAspectCheckBox->Bind(wxEVT_CHECKBOX, &BackgroundStylePanel::onBackgroundImageMaintainAspectChanged, this);
}

void BackgroundStylePanel::updateControlStates()
{
    int selection = m_backgroundStyleChoice->GetSelection();
    m_backgroundColorButton->Enable(selection == 0);
    m_gradientTopColorButton->Enable(selection == 1);
    m_gradientBottomColorButton->Enable(selection == 1);
    m_backgroundImageButton->Enable(selection == 2);
    m_backgroundImageOpacitySlider->Enable(selection == 2);
    m_backgroundImageFitChoice->Enable(selection == 2);
    m_backgroundImageMaintainAspectCheckBox->Enable(selection == 2);
}

void BackgroundStylePanel::onBackgroundStyleChanged(wxCommandEvent& event)
{
    updateControlStates();
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        int activeId = m_backgroundManager->getActiveConfigurationId();
        m_backgroundManager->setStyle(activeId, m_backgroundStyleChoice->GetSelection());
    }
    if (m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void BackgroundStylePanel::onBackgroundColorButton(wxCommandEvent& event)
{
    wxColourDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
            int activeId = m_backgroundManager->getActiveConfigurationId();
            m_backgroundManager->setBackgroundColor(activeId, dialog.GetColourData().GetColour());
        }
        if (m_parentDialog) {
            m_parentDialog->applyGlobalSettingsToCanvas();
        }
    }
}

void BackgroundStylePanel::onGradientTopColorButton(wxCommandEvent& event)
{
    wxColourDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
            int activeId = m_backgroundManager->getActiveConfigurationId();
            m_backgroundManager->setGradientTopColor(activeId, dialog.GetColourData().GetColour());
        }
        if (m_parentDialog) {
            m_parentDialog->applyGlobalSettingsToCanvas();
        }
    }
}

void BackgroundStylePanel::onGradientBottomColorButton(wxCommandEvent& event)
{
    wxColourDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
            int activeId = m_backgroundManager->getActiveConfigurationId();
            m_backgroundManager->setGradientBottomColor(activeId, dialog.GetColourData().GetColour());
        }
        if (m_parentDialog) {
            m_parentDialog->applyGlobalSettingsToCanvas();
        }
    }
}

void BackgroundStylePanel::onBackgroundImageButton(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, _("Open image file"), "", "", "Image files (*.png;*.jpg)|*.png;*.jpg", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_OK) {
        if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
            int activeId = m_backgroundManager->getActiveConfigurationId();
            m_backgroundManager->setImagePath(activeId, openFileDialog.GetPath().ToStdString());
            m_backgroundManager->setImageEnabled(activeId, true);
            m_backgroundImagePathLabel->SetLabel(openFileDialog.GetPath());
        }
        if (m_parentDialog) {
            m_parentDialog->applyGlobalSettingsToCanvas();
        }
    }
}

void BackgroundStylePanel::onBackgroundImageOpacityChanged(wxCommandEvent& event)
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        int activeId = m_backgroundManager->getActiveConfigurationId();
        m_backgroundManager->setImageOpacity(activeId, m_backgroundImageOpacitySlider->GetValue() / 100.0f);
    }
    if (m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void BackgroundStylePanel::onBackgroundImageFitChanged(wxCommandEvent& event)
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        int activeId = m_backgroundManager->getActiveConfigurationId();
        m_backgroundManager->setImageFit(activeId, m_backgroundImageFitChoice->GetSelection());
    }
    if (m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void BackgroundStylePanel::onBackgroundImageMaintainAspectChanged(wxCommandEvent& event)
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        int activeId = m_backgroundManager->getActiveConfigurationId();
        m_backgroundManager->setImageMaintainAspect(activeId, m_backgroundImageMaintainAspectCheckBox->GetValue());
    }
    if (m_parentDialog) {
        m_parentDialog->applyGlobalSettingsToCanvas();
    }
}

void BackgroundStylePanel::setBackgroundManager(BackgroundManager* manager)
{
    m_backgroundManager = manager;
}

int BackgroundStylePanel::getBackgroundStyle() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().style;
    }
    return 0;
}

wxColour BackgroundStylePanel::getBackgroundColor() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().backgroundColor;
    }
    return wxColour(0,0,0);
}

wxColour BackgroundStylePanel::getGradientTopColor() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().gradientTopColor;
    }
    return wxColour(0,0,0);
}

wxColour BackgroundStylePanel::getGradientBottomColor() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().gradientBottomColor;
    }
    return wxColour(0,0,0);
}

std::string BackgroundStylePanel::getBackgroundImagePath() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().imagePath;
    }
    return "";
}

bool BackgroundStylePanel::isBackgroundImageEnabled() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().imageEnabled;
    }
    return false;
}

float BackgroundStylePanel::getBackgroundImageOpacity() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().imageOpacity;
    }
    return 1.0f;
}

int BackgroundStylePanel::getBackgroundImageFit() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().imageFit;
    }
    return 0;
}

bool BackgroundStylePanel::isBackgroundImageMaintainAspect() const
{
    if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
        return m_backgroundManager->getActiveConfiguration().imageMaintainAspect;
    }
    return true;
}