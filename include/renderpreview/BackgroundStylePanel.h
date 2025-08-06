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

private:
    void createUI();
    void bindEvents();
    void updateControlStates();

    void onBackgroundStyleChanged(wxCommandEvent& event);
    void onBackgroundColorButton(wxCommandEvent& event);
    void onGradientTopColorButton(wxCommandEvent& event);
    void onGradientBottomColorButton(wxCommandEvent& event);
    void onBackgroundImageButton(wxCommandEvent& event);
    void onBackgroundImageOpacityChanged(wxCommandEvent& event);
    void onBackgroundImageFitChanged(wxCommandEvent& event);
    void onBackgroundImageMaintainAspectChanged(wxCommandEvent& event);

    RenderPreviewDialog* m_parentDialog;
    BackgroundManager* m_backgroundManager;

    wxChoice* m_backgroundStyleChoice;
    wxButton* m_backgroundColorButton;
    wxButton* m_gradientTopColorButton;
    wxButton* m_gradientBottomColorButton;
    wxButton* m_backgroundImageButton;
    wxSlider* m_backgroundImageOpacitySlider;
    wxChoice* m_backgroundImageFitChoice;
    wxCheckBox* m_backgroundImageMaintainAspectCheckBox;
    wxStaticText* m_backgroundImagePathLabel;

    DECLARE_EVENT_TABLE()
};
