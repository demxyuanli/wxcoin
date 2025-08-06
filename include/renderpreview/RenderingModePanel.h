#pragma once

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>

class RenderPreviewDialog;
class RenderingManager;

class RenderingModePanel : public wxPanel
{
public:
    RenderingModePanel(wxWindow* parent, RenderPreviewDialog* dialog);
    ~RenderingModePanel();

    int getRenderingMode() const;
    void setRenderingMode(int mode);

    void loadSettings();
    void saveSettings();
    void resetToDefaults();

    void setRenderingManager(RenderingManager* manager);

private:
    void createUI();
    void bindEvents();

    void onRenderingModeChanged(wxCommandEvent& event);
    void onLegacyModeChanged(wxCommandEvent& event);
    void updateLegacyChoiceFromCurrentMode();

    RenderPreviewDialog* m_parentDialog;
    RenderingManager* m_renderingManager;

    wxChoice* m_renderingModeChoice;
    wxChoice* m_legacyChoice;

    DECLARE_EVENT_TABLE()
};
