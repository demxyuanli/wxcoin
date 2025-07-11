#ifndef FLATUIFRAME_H
#define FLATUIFRAME_H

#include "flatui/BorderlessFrameLogic.h"
#include "flatui/FlatUIBar.h"
#include <wx/log.h>

// Custom events for FlatUIFrame
wxDECLARE_EVENT(wxEVT_THEME_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_PIN_STATE_CHANGED, wxCommandEvent);

class FlatUIFrame : public BorderlessFrameLogic
{
public:
    FlatUIFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style = wxBORDER_NONE);
    virtual ~FlatUIFrame();

    // Override mouse events to add specific checks (e.g., pseudo-maximization)
    void OnLeftDown(wxMouseEvent& event) override;
    void OnLeftUp(wxMouseEvent& event) override;
    void OnMotion(wxMouseEvent& event) override;

    // Methods for pseudo-maximization
    bool IsPseudoMaximized() const { return m_isPseudoMaximized; }
    void PseudoMaximize();
    void RestoreFromPseudoMaximize();

    // Generic method to log UI layout structure (remains in this class or a utility class)
    void LogUILayout(wxWindow* window = nullptr, int depth = 0);


    virtual int GetMinWidth() const override;
    virtual int GetMinHeight() const override;

    // Override SetSize methods to handle adaptive UI
    virtual void SetSize(const wxRect& rect) override;
    virtual void SetSize(const wxSize& size) override;

    virtual wxWindow* GetFunctionSpaceControl() const { return nullptr; }
    virtual wxWindow* GetProfileSpaceControl() const { return nullptr; }
    void ShowTabFunctionSpacer(bool show);
    void ShowFunctionProfileSpacer(bool show);
    
    // Global event handlers (can be overridden by derived classes)
    virtual void OnThemeChanged(wxCommandEvent& event);
    virtual void OnGlobalPinStateChanged(wxCommandEvent& event);
    
    // Theme and UI refresh functionality
    virtual void RefreshAllUI();

protected:
    // Helper methods for minimum size calculation and adaptive UI
    int CalculateMinimumWidth() const;
    int CalculateMinimumHeight() const;

    // Get FlatUIBar from derived class (FlatFrame)
    virtual FlatUIBar* GetUIBar() const { return nullptr; }

    void HandleAdaptiveUIVisibility(const wxSize& newSize);

    // Helper method for FlatUIFrame specific style initialization (e.g., background color)
    void InitFrameStyle();

    // Members for pseudo-maximization state
    bool m_isPseudoMaximized;
    wxRect m_preMaximizeRect;            // Stores frame rect before pseudo-maximization

    // Note: Dragging, resizing, rubber band members and methods are now in BorderlessFrameLogic
    // Note: m_borderThreshold is in BorderlessFrameLogic

private:
    wxDECLARE_EVENT_TABLE();
};

#endif // FLATUIFRAME_H 