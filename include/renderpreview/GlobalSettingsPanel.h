#pragma once

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <vector>
#include "renderpreview/RenderLightSettings.h"

class RenderPreviewDialog;
class LightingPanel;
class AntiAliasingPanel;
class RenderingModePanel;
class BackgroundStylePanel;
class AntiAliasingManager;
class RenderingManager;
class BackgroundManager;

class GlobalSettingsPanel : public wxPanel
{
public:
    GlobalSettingsPanel(wxWindow* parent, RenderPreviewDialog* dialog, wxWindowID id = wxID_ANY);
    ~GlobalSettingsPanel();

    std::vector<RenderLightSettings> getLights() const;
    void setLights(const std::vector<RenderLightSettings>& lights);

    int getAntiAliasingMethod() const;
    int getMSAASamples() const;
    bool isFXAAEnabled() const;
    void setAntiAliasingMethod(int method);
    void setMSAASamples(int samples);
    void setFXAAEnabled(bool enabled);

    int getRenderingMode() const;
    void setRenderingMode(int mode);

    int getBackgroundStyle() const;
    wxColour getBackgroundColor() const;
    wxColour getGradientTopColor() const;
    wxColour getGradientBottomColor() const;
    std::string getBackgroundImagePath() const;
    bool isBackgroundImageEnabled() const;
    float getBackgroundImageOpacity() const;
    int getBackgroundImageFit() const;
    bool isBackgroundImageMaintainAspect() const;

    void setAntiAliasingManager(AntiAliasingManager* manager);
    void setRenderingManager(RenderingManager* manager);
    void setBackgroundManager(BackgroundManager* manager);

    void setAutoApply(bool enabled);
    bool isAutoApplyEnabled() const { return m_autoApply; }
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }

    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    void markAsChanged();
    void markAsSaved();

    void OnGlobalApply(wxCommandEvent& event);
    void OnMainApply(wxCommandEvent& event);
    void OnGlobalSave(wxCommandEvent& event);
    void OnGlobalReset(wxCommandEvent& event);
    void OnGlobalAutoApply(wxCommandEvent& event);

    void validatePresets();
    void testPresetFunctionality();

private:
    void createUI();
    void bindEvents();
    void applySpecificFonts();
    void applySettingsToCanvas();

    RenderPreviewDialog* m_parentDialog;
    bool m_autoApply;
    bool m_hasUnsavedChanges;

    wxNotebook* m_notebook;
    LightingPanel* m_lightingPanel;
    AntiAliasingPanel* m_antiAliasingPanel;
    RenderingModePanel* m_renderingModePanel;
    BackgroundStylePanel* m_backgroundStylePanel;

    wxButton* m_globalApplyButton;
    wxButton* m_mainApplyButton;
    wxButton* m_globalSaveButton;
    wxButton* m_globalResetButton;
    wxCheckBox* m_globalAutoApplyCheckBox;

    AntiAliasingManager* m_antiAliasingManager;
    RenderingManager* m_renderingManager;
    BackgroundManager* m_backgroundManager;

    DECLARE_EVENT_TABLE()
};