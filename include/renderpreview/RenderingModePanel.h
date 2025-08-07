#pragma once

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <functional>

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
    
    // Set callback for parameter changes
    void setParameterChangeCallback(std::function<void()> callback) { m_parameterChangeCallback = callback; }
    
    // Apply fonts to all controls
    void applyFonts();

private:
    void createUI();
    void bindEvents();
    void notifyParameterChanged();

    void onRenderingModeChanged(wxCommandEvent& event);
    void onLegacyModeChanged(wxCommandEvent& event);
    void updateLegacyChoiceFromCurrentMode();

    RenderPreviewDialog* m_parentDialog;
    RenderingManager* m_renderingManager;
    std::function<void()> m_parameterChangeCallback;

    wxChoice* m_renderingModeChoice;
    wxChoice* m_legacyChoice;

    DECLARE_EVENT_TABLE()
};
