#pragma once

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <string>
#include <functional>
#include "renderpreview/BackgroundManager.h"

class RenderPreviewDialog;
class BackgroundManager;

class BackgroundStylePanel : public wxPanel
{
public:
    BackgroundStylePanel(wxWindow* parent, RenderPreviewDialog* dialog);
    ~BackgroundStylePanel();

    int getBackgroundStyle() const;
    wxColour getBackgroundColor() const;
    wxColour getGradientTopColor() const;
    wxColour getGradientBottomColor() const;
    std::string getBackgroundImagePath() const;
    bool isBackgroundImageEnabled() const;
    float getBackgroundImageOpacity() const;
    int getBackgroundImageFit() const;
    bool isBackgroundImageMaintainAspect() const;

    void setBackgroundManager(BackgroundManager* manager);
    
    // Set callback for parameter changes
    void setParameterChangeCallback(std::function<void()> callback) { m_parameterChangeCallback = callback; }
    
    // Apply fonts to all controls
    void applyFonts();

private:
    void createUI();
    void bindEvents();
    void updateControlStates();
    void updateButtonColors();
    void notifyParameterChanged();

    void onBackgroundStyleChanged(wxCommandEvent& event);
    void onBackgroundColorButton(wxCommandEvent& event);
    void onGradientTopColorButton(wxCommandEvent& event);
    void onGradientBottomColorButton(wxCommandEvent& event);
    void onBackgroundImageButton(wxCommandEvent& event);
    void onBackgroundImageOpacityChanged(wxCommandEvent& event);
    void onBackgroundImageFitChanged(wxCommandEvent& event);
    void onBackgroundImageMaintainAspectChanged(wxCommandEvent& event);
    
    // Preset button event handlers
    void onEnvironmentPresetButton(wxCommandEvent& event);
    void onStudioPresetButton(wxCommandEvent& event);
    void onOutdoorPresetButton(wxCommandEvent& event);
    void onIndustrialPresetButton(wxCommandEvent& event);
    void onCadPresetButton(wxCommandEvent& event);
    void onDarkPresetButton(wxCommandEvent& event);

    RenderPreviewDialog* m_parentDialog;
    BackgroundManager* m_backgroundManager;
    std::function<void()> m_parameterChangeCallback;

    wxChoice* m_backgroundStyleChoice;
    wxButton* m_backgroundColorButton;
    wxButton* m_gradientTopColorButton;
    wxButton* m_gradientBottomColorButton;
    wxButton* m_backgroundImageButton;
    wxSlider* m_backgroundImageOpacitySlider;
    wxChoice* m_backgroundImageFitChoice;
    wxCheckBox* m_backgroundImageMaintainAspectCheckBox;
    wxStaticText* m_backgroundImagePathLabel;
    
    // Preset buttons
    wxButton* m_environmentPresetButton;
    wxButton* m_studioPresetButton;
    wxButton* m_outdoorPresetButton;
    wxButton* m_industrialPresetButton;
    wxButton* m_cadPresetButton;
    wxButton* m_darkPresetButton;

    DECLARE_EVENT_TABLE()
};
