#pragma once

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

class RenderPreviewDialog;
class AntiAliasingManager;

class AntiAliasingPanel : public wxPanel
{
public:
    AntiAliasingPanel(wxWindow* parent, RenderPreviewDialog* dialog);
    ~AntiAliasingPanel();

    int getAntiAliasingMethod() const;
    int getMSAASamples() const;
    bool isFXAAEnabled() const;
    void setAntiAliasingMethod(int method);
    void setMSAASamples(int samples);
    void setFXAAEnabled(bool enabled);

    void loadSettings();
    void saveSettings();
    void resetToDefaults();

    void setAntiAliasingManager(AntiAliasingManager* manager);

private:
    void createUI();
    void bindEvents();

    void onAntiAliasingChanged(wxCommandEvent& event);

    RenderPreviewDialog* m_parentDialog;
    AntiAliasingManager* m_antiAliasingManager;

    wxChoice* m_antiAliasingChoice;
    wxSlider* m_msaaSamplesSlider;
    wxCheckBox* m_fxaaCheckBox;

    DECLARE_EVENT_TABLE()
};
